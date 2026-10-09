#include <cmath>
#include <algorithm>
#include "RagdollSystem.h"
#include "RagdollData.h"
#include "RigidBody.h"


bool RagdollSystem::IsRagdollBody(const PhysicsBody& body)
{
    return body.rigidBody && body.ragdollBone.nodeIndex >= 0 && body.rigidBody->GetUseGravity();
}


bool RagdollSystem::IsUpperBodyBone(const std::string& boneName)
{
    return boneName == "mixamorig:Spine1" ||
        boneName == "mixamorig:Spine2" ||
        boneName == "mixamorig:Head";
}


float RagdollSystem::ComputeAngularSpeed(const Quaternion& current, const Quaternion& previous, float deltaTime)
{
    //qと -q は同じ回転なので、内積は絶対値をとる
    //acosの範囲外1超えにならないよう1で切る
    const float quaternionDot =
        current.x * previous.x + current.y * previous.y +
        current.z * previous.z + current.w * previous.w;
    const float clampedDot = std::min(1.0f, std::fabs(quaternionDot));
    return 2.0f * std::acos(clampedDot) / deltaTime;
}


void RagdollSystem::ApplyRootGravityTorque(std::vector<PhysicsBody>& physicsBodies, float subDeltaTime)
{
    if (!RagdollTuning::kRootGravityTorque)
        return;

    PhysicsBody* rootBody = nullptr;
    Vector3 weightedCom(0.0f, 0.0f, 0.0f);
    float totalMass = 0.0f;
    for (auto& body : physicsBodies)
    {
        //ragdollのボーンだけが対象
        if (!body.rigidBody || body.ragdollBone.nodeIndex < 0)
            continue;

        Vector3 position = body.rigidBody->GetPosition();
        if (body.ragdollBone.parentIndex < 0)
        {
            //ルートは重心の計算には入れず、トルクを加える相手として覚えておく
            rootBody = &body;
        }
        else
        {
            //ボーン原点ではなく、ボーンの重心で数える
            position = position + body.rigidBody->GetRotation() * body.ragdollBone.colliderLocalOffset;
            weightedCom = weightedCom + position * body.rigidBody->GetMass();
            totalMass += body.rigidBody->GetMass();
        }
    }

    if (rootBody && totalMass > 0.0f &&
        !rootBody->rigidBody->IsKinematic() && !rootBody->rigidBody->IsSleeping())
    {
        Vector3 childrenCom = weightedCom / totalMass;
        //ルート -> 子全体の重心のベクトルと、子全体の質量から、ワールド空間の重力トルクを加える
        rootBody->rigidBody->ApplyGravityTorqueWorld(
            childrenCom - rootBody->rigidBody->GetPosition(), totalMass, subDeltaTime);
    }
}



void RagdollSystem::UpdateRest(std::vector<PhysicsBody>& physicsBodies, float deltaTime)
{
    if (!RagdollTuning::kEnableRest || !_restEnabled)
        return;

    const MotionSummary motion = MeasureMotion(physicsBodies, deltaTime);
    const bool isDown = UpdateDownState(motion);

    //「ほぼ静止」: 静止の基準に kSettleSpeedFactor 倍の余裕を持たせたもの
    const bool nearlyStill = motion.hasRagdoll && isDown &&
        motion.maxLinearSpeed < RagdollTuning::kSettleSpeedFactor * RagdollTuning::kRestLinearSpeed &&
        motion.maxAngularSpeed < RagdollTuning::kSettleSpeedFactor * RagdollTuning::kRestAngularSpeed;

    //ほぼ静止している時間(条件を外れたら 0 に戻る)
    _settleTimer = nearlyStill ? _settleTimer + deltaTime : 0.0f;
    if (nearlyStill)
        ApplySettleDamping(physicsBodies);

    //「完全静止」: 全ボーンが静止の基準を下回っている。その継続時間を測る
    const bool fullyStill = motion.hasRagdoll && isDown && motion.allStill;
    _restTimer = fullyStill ? _restTimer + deltaTime : 0.0f;

    const bool restTimeReached = _restTimer >= RagdollTuning::kRestTime;
    const bool forceRestTimeReached = _settleTimer >= RagdollTuning::kForceRestTime;
    if (RagdollTuning::kFreezeAtEnd && motion.hasRagdoll && (restTimeReached || forceRestTimeReached))
        FreezeAllBones(physicsBodies);
}


RagdollSystem::MotionSummary RagdollSystem::MeasureMotion(std::vector<PhysicsBody>& physicsBodies, float deltaTime)
{
    MotionSummary motion;

    for (auto& body : physicsBodies)
    {
        if (!IsRagdollBody(body))
            continue;

        motion.hasRagdoll = true;
        if (body.ragdollBone.parentIndex < 0)
        {
            //親がいないボーン = ルート腰
            motion.hasRoot = true;
            motion.rootY = body.rigidBody->GetPosition().y;
        }
        //動けるボーンか確認して処理をする
        if (body.rigidBody->IsSleeping())
            continue;

        if (body.rigidBody->IsGrounded())
        {
            motion.anyBoneGrounded = true;
            
            //上半身が地面に触れていたら倒れているものとする
            if (IsUpperBodyBone(body.ragdollBone.name))
                motion.upperBodyGrounded = true;
        }

        const Vector3 position = body.rigidBody->GetPosition();
        const Quaternion rotation = body.rigidBody->GetRotation();

        if (!body.restHasLast)
        {
            //前回の値がまだ無いので、速度は測れない
            //静止とはみなさない
            motion.allStill = false;
        }
        else if (deltaTime > 0.0f)
        {
            //前回からの移動量・回転量を速度に直す
            const float linearSpeed = (position - body.restLastPos).Length() / deltaTime;
            const float angularSpeed = ComputeAngularSpeed(rotation, body.restLastRot, deltaTime);

            if (linearSpeed > motion.maxLinearSpeed)
                motion.maxLinearSpeed = linearSpeed;
            if (angularSpeed > motion.maxAngularSpeed)
                motion.maxAngularSpeed = angularSpeed;

            //1つでも基準より速いボーンがあれば、静止していない
            if (linearSpeed > RagdollTuning::kRestLinearSpeed ||
                angularSpeed > RagdollTuning::kRestAngularSpeed)
            {
                motion.allStill = false;
            }
        }

        //次回の比較用に、今回の位置・回転を保存する
        body.restLastPos = position;
        body.restLastRot = rotation;
        body.restHasLast = true;
    }

    return motion;
}

bool RagdollSystem::UpdateDownState(const MotionSummary& motion)
{
    bool isDown = motion.upperBodyGrounded;

    if (!motion.hasRagdoll)
    {
        //ragdollが居なくなったので、記録をリセットする
        _hasInitialRootY = false;
        _downLatched = false;
    }
    else if (!_hasInitialRootY && motion.anyBoneGrounded && motion.hasRoot)
    {
        //最初にどれかのボーンが地面に触れたときのルートの高さを、基準として記録する
        _initialRootY = motion.rootY;
        _hasInitialRootY = true;
    }

    //膝立ち・座りのように足だけが地面に触れている場合も倒れたとみなす
    if (RagdollTuning::kRestUseRootDrop && motion.hasRagdoll && motion.hasRoot &&
        _hasInitialRootY &&
        motion.rootY < _initialRootY - RagdollTuning::kRestRootDrop)
    {
        isDown = true;
    }

    //一度倒れたら保持する。速く動いたときだけ解除する 押したときだけ
    if (isDown)
        _downLatched = true;
    else if (motion.maxLinearSpeed > RagdollTuning::kRestUnlatchSpeed)
        _downLatched = false;

    return _downLatched;
}



void RagdollSystem::ApplySettleDamping(std::vector<PhysicsBody>& physicsBodies)
{
    //ramp:0(始めたばかり)～1(kSettleRampTime 経過)
    const float ramp = std::min(1.0f, _settleTimer / RagdollTuning::kSettleRampTime);
    //settleFactor:1(減衰なし)～kSettleDamping(最大の減衰)
    const float settleFactor = 1.0f - (1.0f - RagdollTuning::kSettleDamping) * ramp;

    for (auto& body : physicsBodies)
    {
        if (!IsRagdollBody(body) || body.rigidBody->IsSleeping())
            continue;

        body.rigidBody->SetVelocity(body.rigidBody->GetVelocity() * settleFactor);
        body.rigidBody->SetAngularVelocity(body.rigidBody->GetAngularVelocity() * settleFactor);
    }
}



void RagdollSystem::FreezeAllBones(std::vector<PhysicsBody>& physicsBodies)
{
    for (auto& body : physicsBodies)
    {
        if (!IsRagdollBody(body))
            continue;

        body.rigidBody->SetSleeping(true);
        body.restHasLast = false;
    }
}