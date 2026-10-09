#include "Ragdoll.h"
#include <cmath>
#include <random>
#include "Skeleton.h"

#include "MatrixUtility.h"
#include "MathUtility.h"

#include "PhysicsSystem.h"
#include "GameObject.h"

#include "Transform.h"
#include "RagdollData.h"
#include "RigidBody.h"
#include "CapsuleCollider.h"
#include "Collision.h"
#include "BallJoint.h"
#include "HingeJoint.h"
using namespace RagdollData;

void Ragdoll::Initialize(ServiceLocator& locator)
{
    _physicsSystem = locator.physicsSystem;
    InitializeBoneLength();
    AddRagDoll();

}
void Ragdoll::RegisterIgnoredCollisionPairs()
{
    for (size_t boneIndexA = 0; boneIndexA < _bones.size(); ++boneIndexA)
    {
        for (size_t boneIndexB = boneIndexA + 1; boneIndexB < _bones.size(); ++boneIndexB)
        {
            const RagdollBone& boneA = _bones[boneIndexA];
            const RagdollBone& boneB = _bones[boneIndexB];

            //親子で隣り合うボーン
            const bool isAdjacent =
                boneA.parentIndex == static_cast<int>(boneIndexB) ||
                boneB.parentIndex == static_cast<int>(boneIndexA);

            const bool legsOfDifferentSides =
                RagdollTuning::kIgnoreLegLegCollision && AreLegsOfDifferentSides(boneA.name, boneB.name);

            //ペアを設定
            if (isAdjacent || legsOfDifferentSides)
                Collision::IgnoreCollision(boneA.collider->GetID(), boneB.collider->GetID());
        }
    }
}

void Ragdoll::CreateBone(int boneNameIndex)
{
    const char* boneName = boneNames[boneNameIndex];

    const int nodeIndex = _skeleton->FindNode(boneName);
    if (nodeIndex == -1)
        return; //ノードが見つからなかったので作らない

    RagdollBone bone;
    bone.nodeIndex = nodeIndex;
    bone.parentIndex = parentIndices[boneNameIndex]; //一旦boneNames上のインデックスのまま保持
    bone.name = boneName;

    bone.rigidBody = std::make_unique<RigidBody>();
    bone.collider = std::make_unique<CapsuleCollider>();
    bone.collider->SetOwner(_owner);
    bone.rigidBody->SetIsGravity(false);
    bone.rigidBody->DisableKinematic();

    if (auto radiusIt = boneRadius.find(boneName); radiusIt != boneRadius.end())
        bone.collider->SetRadius(radiusIt->second * _bodyScale * _colliderScale);

    if (auto massIt = boneMass.find(boneName); massIt != boneMass.end())
    {
        const float scaledMass = massIt->second * (float)std::pow(_bodyScale, 3);
        bone.rigidBody->SetMass(scaledMass);
        bone.rigidBody->SetInertia(scaledMass * RagdollTuning::kInertiaPerMass * _bodyScale * _bodyScale);
    }

    PhysicsBody body;
    //RagdollBoneの所有権(rigidBody/collider/joint)は持たせず、データだけコピーする
    //(bone自体の所有は、このあとmoveでRagdoll::_bonesに移る)
    body.ragdollBone.name = bone.name;
    body.ragdollBone.nodeIndex = bone.nodeIndex;
    body.ragdollBone.parentIndex = bone.parentIndex;
    body.rigidBody = bone.rigidBody.get();
    body.collider = bone.collider.get();
    body.collider->SetCollisionGroup(CollisionGroup::Ragdoll);
    body.rigidBody->SetPosition({ 0, 0, 0 });

    _physicsSystem->AddPhysicsBody(std::move(body));

    _bones.push_back(std::move(bone));
}
void Ragdoll::InitializeBoneLength()
{
    //身長を測ってそれをboneのサイズに適応
    const int head = _skeleton->FindNode("mixamorig:Head");
    const int foot = _skeleton->FindNode("mixamorig:LeftFoot");
    const int hips = _skeleton->FindNode("mixamorig:Hips");
    if (head < 0 || foot < 0 || hips < 0)
    {
        _bodyScale = 1.0f;
        _hipHeight = 0.0f; //測れないときは、足元を、そのまま砲身の位置に置く
        return;
    }
    const auto& nodes = _skeleton->GetNode();
    const float scaleY = _owner->GetTransform().Scale.y;
    const float height = nodes[head].BindGlobalTransform._42 - nodes[foot].BindGlobalTransform._42;
    const float heightMeters = height * scaleY;

    //足元(モデルの原点)から腰までの高さ(m)。Transformの大きさを掛けて、メートルにする。
    //分割されたノードのモデルでも、本体のバインドのグローバルは、位置を含んだ値になる
    _hipHeight = nodes[hips].BindGlobalTransform._42 * scaleY;

    _bodyScale = heightMeters / referenceHeight;
}

void Ragdoll::AddRagDoll()
{
    ragdollNodeIndices.clear();

    for (int boneNameIndex = 0; boneNameIndex < static_cast<int>(std::size(boneNames)); ++boneNameIndex)
        CreateBone(boneNameIndex);

    for (auto& ragdoll : _bones)
        ragdollNodeIndices.insert(ragdoll.nodeIndex);

    RegisterIgnoredCollisionPairs();
}



//-------------------Bone.Capsuleの情報をセット----------

void Ragdoll::InitializeRootBone(RagdollBone& bone, const WorldPose& nodePose)
{
    bone.rigidBody->SetPosition(nodePose.position);
    bone.rigidBody->SetRotation(nodePose.rotation);

    bone.collider->SetPosition(Vector::fromV3ToF3(nodePose.position));
    bone.collider->SetRotation(nodePose.rotation);
    bone.colliderLocalOffset = Vector3(0.0f, 0.0f, 0.0f);
    bone.colliderLocalRotOffset = Quaternion(0, 0, 0, 1); //Identity
}
void Ragdoll::SetupLeafCapsule(RagdollBone& bone, WorldPose& nodePose, const Vector3& axisFromParent)
{
    //足先や手先の固定の長さ
    float leafHeight = RagdollTuning::leafHeight * _bodyScale;

    //手など、短い末端ボーンは、名前ごとの長さを使う
    if (const auto heightIt = leafHeights.find(bone.name); heightIt != leafHeights.end())
        leafHeight = heightIt->second * _bodyScale;

    //ボーンのローカルY軸(伸びる方向)をワールド空間へ
    Vector3 leafAxis = nodePose.rotation * Vector3(0.0f, 1.0f, 0.0f);

    if (Vector::Dot(leafAxis, axisFromParent) < -0.5f)
        leafAxis = -leafAxis;

    //関節位置から、伸びる方向へ半分の高さぶんずらした位置を中心にする
    const Vector3 leafCenter = nodePose.position + leafAxis * (leafHeight * 0.5f);

    bone.collider->SetPosition(Vector::fromV3ToF3(leafCenter));
    bone.collider->SetHeight(leafHeight);
    bone.collider->SetRotation(nodePose.rotation);

    //中心をずらした分、オフセットもゼロではなくなる
    bone.colliderLocalOffset =
        nodePose.rotation.Inverse() * (leafCenter - nodePose.position); 
    bone.colliderLocalRotOffset = Quaternion(0, 0, 0, 1);

    //PhysicsSystem 側のコピーにも、オフセットと「重心が信頼できるか」を反映する
    //前腕・手・足は、末端でも重力トルクを使う(だらんと下げるため)
    _physicsSystem->UpdateRagdollBoneOffsets(
        bone.rigidBody.get(), bone.colliderLocalOffset, bone.colliderLocalRotOffset,
        RagdollTuning::kLeafBoneGravityTorque ||
        bone.name.find("ForeArm") != std::string::npos ||
        bone.name.find("Hand") != std::string::npos ||
        bone.name.find("Foot") != std::string::npos);
}
void Ragdoll::SetupParentCapsule(RagdollBone& parent, WorldPose& parentPose,
    const Vector3& nodePosition, float lengthFromParent, const Quaternion& capsuleRotation)
{
    Vector3 center = (parentPose.position + nodePosition) * 0.5f;
    parent.collider->SetPosition(Vector::fromV3ToF3(center));
    parent.collider->SetHeight(lengthFromParent);
    parent.collider->SetRotation(capsuleRotation);

    //RigidBody基準のローカルオフセット(capsuleRotation確定後に計算)
    parent.colliderLocalOffset =
        parentPose.rotation.Inverse() * (center - parentPose.position);
    parent.colliderLocalRotOffset =
        parentPose.rotation.Inverse() * capsuleRotation;

    //ラグドールの情報を更新する
    _physicsSystem->UpdateRagdollBoneOffsets(
        parent.rigidBody.get(), parent.colliderLocalOffset, parent.colliderLocalRotOffset, true);
}

//---------------------------- 計算---------------------
WorldPose Ragdoll::GetWorldPose(const BoneNode& node, const XMMATRIX& worldMatrix)
{
    XMFLOAT4X4 nodeWorld;
    XMStoreFloat4x4(
        &nodeWorld,
        XMMatrixMultiply(
            XMLoadFloat4x4(&node.GlobalTransform),
            worldMatrix));

    WorldPose pose;
    pose.position =
        Vector::fromF3ToV3(
            MatrixUtility::GetTransform<
            MatrixUtility::ReturnTransform::Position
            >(nodeWorld));
    pose.rotation =
        QuaternionMath::fromF4ToQ4(
            MatrixUtility::GetTransform<
            MatrixUtility::ReturnTransform::Rotation
            >(nodeWorld));
    return pose;
}
Quaternion Ragdoll::ComputeCapsuleRotation(const Vector3& axisFromParent)
{
    const Vector3 capsuleLocalAxis = { 0.0f, 1.0f, 0.0f };
    const float dot = std::clamp(Vector::Dot(capsuleLocalAxis, axisFromParent), -1.0f, 1.0f);

    //すでに同じ向き
    if (dot > 0.99999f)
        return Quaternion(0, 0, 0, 1);

    //真逆の向き: 回転軸が定まらないので、X軸まわりに180度回す
    if (dot < -0.99999f)
    {
        const Vector3 rotationAxis = { 1.0f, 0.0f, 0.0f };
        return QuaternionMath::CreateFromAxisAngle(rotationAxis, XM_PI);
    }

    //一般の場合: 2つのベクトルの外積が回転軸、内積が角度(cos)
    Vector3 rotationAxis = Vector::Cross(capsuleLocalAxis, axisFromParent);
    rotationAxis.Normalize();
    return QuaternionMath::CreateFromAxisAngle(rotationAxis, std::acos(dot));
}
Quaternion Ragdoll::ComputeBindRelativeRotation(const BoneNode& parentNode, const BoneNode& node)
{
    //Tポーズの状態の初期場所を取得する
    Quaternion bindParentRotation = QuaternionMath::fromF4ToQ4(
        MatrixUtility::GetTransform<MatrixUtility::ReturnTransform::Rotation>(parentNode.BindGlobalTransform));
    Quaternion bindNodeRotation = QuaternionMath::fromF4ToQ4(
        MatrixUtility::GetTransform<MatrixUtility::ReturnTransform::Rotation>(node.BindGlobalTransform));

    Quaternion relativeRotation = bindParentRotation.Inverse() * bindNodeRotation;
    relativeRotation.Normalize();
    return relativeRotation;
}
bool Ragdoll::CalculateBoneAxis(const Vector3& nodePosition, const Vector3& parentPosition,
    float& outLength, Vector3& outAxis)
{
    Vector3 direction = nodePosition - parentPosition;
    float length = std::sqrt(Vector::SqMagnitude(direction));

    if (length <= EPSILON) //0除算をなくすため
        return false;

    outLength = length;
    outAxis = direction / length;
    return true;
}



bool Ragdoll::IsPrimaryChild(size_t boneIndex) const
{
    const int parentIndex = _bones[boneIndex].parentIndex;
    for (size_t otherIndex = 0; otherIndex < boneIndex; ++otherIndex)
    {
        if (_bones[otherIndex].parentIndex == parentIndex)
            return false;
    }
    return true;
}
void Ragdoll::InitializeBonePoses()
{
    const XMMATRIX worldMatrix = _owner->GetTransform().GetWorldMatrix();

    for (size_t boneIndex = 0; boneIndex < _bones.size(); ++boneIndex)
    {
        RagdollBone& bone = _bones[boneIndex];

        //自分の関節のワールド座標
        const BoneNode& node = _skeleton->GetNode()[bone.nodeIndex];
        WorldPose nodePose = GetWorldPose(node, worldMatrix);

        //ルート(親がいない)は、置くだけ
        if (bone.parentIndex < 0)
        {
            InitializeRootBone(bone, nodePose);
            continue;
        }

        //親の関節のワールド座標
        RagdollBone& parent = _bones[bone.parentIndex];
        const BoneNode& parentNode = _skeleton->GetNode()[parent.nodeIndex];
        WorldPose parentPose = GetWorldPose(parentNode, worldMatrix);


        //親->自分の向きと長さ
        float lengthFromParent = 0.0f;
        Vector3 axisFromParent;
        if (!CalculateBoneAxis(nodePose.position, parentPose.position, lengthFromParent, axisFromParent))
            continue;

        bone.rigidBody->SetPosition(nodePose.position);
        bone.rigidBody->SetRotation(nodePose.rotation);

        SetupLeafCapsule(bone, nodePose, axisFromParent);

        const Quaternion capsuleRotation = ComputeCapsuleRotation(axisFromParent);


        //ジョイントの基準回転
        bone.initialRelativeRotation = ComputeBindRelativeRotation(parentNode, node);


        //親のcapsuleと重心オフセット(最初の子だけが設定する)
        if (!IsPrimaryChild(boneIndex))
            continue;

        SetupParentCapsule(parent, parentPose, nodePose.position, lengthFromParent, capsuleRotation);
    }
    //通常に戻るとき、動いた距離を求めるために、始めたときの Hips の位置を覚えておく
    if (!_bones.empty() && _bones[0].rigidBody)
        _ragdollStartRootPosition = _bones[0].rigidBody->GetPosition();
}


//--------------------------Jointの処理--------------
void Ragdoll::ApplyJointStiffness(RagdollBone& child)
{
    const auto stiffnessIt = stiffnessTable.find(child.name);
    if (stiffnessIt != stiffnessTable.end())
        child.joint->SetStiffness(stiffnessIt->second.first, stiffnessIt->second.second);
}
void Ragdoll::CreateHingeJoint(RagdollBone& parentBone, RagdollBone& child, const auto& hingeLimit)
{
    child.joint = std::make_unique<HingeJoint>(
        parentBone.rigidBody.get(),
        child.rigidBody.get(),
        hingeLimit.axis,
        child.initialRelativeRotation
    );
    ApplyJointStiffness(child);
    child.joint->SetRotationLimit(hingeLimit.min, hingeLimit.max);
}
void Ragdoll::CreateBallJoint(RagdollBone& parentBone, RagdollBone& child)
{
    child.joint = std::make_unique<BallJoint>(
        parentBone.rigidBody.get(),
        child.rigidBody.get(),
        child.initialRelativeRotation
    );
    ApplyJointStiffness(child);

    const auto ballLimitIt = ballLimits.find(child.name);
    if (ballLimitIt != ballLimits.end())
    {
        const auto& jointLimit = ballLimitIt->second;
        child.joint->SetTwistAndSwing(jointLimit.minTwist, jointLimit.maxTwist, jointLimit.maxSwing);
    }
}
void Ragdoll::AlignTwistWithOwnBone(size_t boneIndex)
{
    RagdollBone& bone = _bones[boneIndex];

    for (size_t childBoneIndex = boneIndex + 1; childBoneIndex < _bones.size(); ++childBoneIndex)
    {
        const RagdollBone& childBone = _bones[childBoneIndex];
        if (childBone.parentIndex == static_cast<int>(boneIndex) && childBone.rigidBody)
        {
            bone.joint->SetTwistDirectionWorld(
                childBone.rigidBody->GetPosition() - bone.rigidBody->GetPosition());
            break;
        }
    }
}
void Ragdoll::CreateJoints()
{
    for (size_t boneIndex = 0; boneIndex < _bones.size(); ++boneIndex)
    {
        RagdollBone& child = _bones[boneIndex];

        //ルートは親がいないのでジョイントなし
        if (child.parentIndex < 0)
            continue;

        //すでにジョイントがあるボーンは作らない(P を押すたびに、同じジョイントが重なって増えるのを防ぐ)
        if (child.joint)
            continue;

        RagdollBone& parentBone = _bones[child.parentIndex];

        const auto hingeLimitIt = hingeLimits.find(child.name);
        if (hingeLimitIt != hingeLimits.end())
        {
            CreateHingeJoint(parentBone, child, hingeLimitIt->second);
        }
        else
        {
            CreateBallJoint(parentBone, child);

            if (RagdollTuning::kTwistAxisAlongOwnBone)
                AlignTwistWithOwnBone(boneIndex);
        }

        _physicsSystem->AddJoint(child.joint.get());
    }
}


void Ragdoll::ApplyRandomStartPush()
{
    if (RagdollTuning::kStartPushSpeed <= 0.0f)
        return;

    //ランダムな数字を作るための準備
    static std::mt19937 randomEngine{ std::random_device{}() };
    std::uniform_real_distribution<float> angleDistribution(0.0f, XM_2PI);

    //水平面上のランダムな向き
    const float angle = angleDistribution(randomEngine);
    const Vector3 pushDirection(std::cos(angle), 0.0f, std::sin(angle));

    for (auto& bone : _bones)
    {
        if (!bone.rigidBody)
            continue;

        //上半身(胸・頭)だけを押す
        const std::string& boneName = bone.name;
        if (boneName == "mixamorig:Spine2" || boneName == "mixamorig:Head")
        {
            bone.rigidBody->SetVelocity(
                bone.rigidBody->GetVelocity() + pushDirection * RagdollTuning::kStartPushSpeed);
        }
    }
}

//--------------ラグドール有効時の処理--------------------
void Ragdoll::InitializePhysicsTransform()
{
    InitializeBonePoses(); //ボーンごとの位置・回転・collider
    CreateJoints(); //ジョイント(全ボーンの位置が決まってから作る)
    ApplyRandomStartPush(); //開始時のランダムな押し
}


//--------------ラグドールの開始・終了と、吊り下げ(体全体を浮かせて移動させる)--------------------
void Ragdoll::StartFall()
{
    for (auto& bone : _bones)
    {
        if (bone.rigidBody)
        {
            SetLockZ(false);
            bone.rigidBody->ResetMaxSpeed();
        }
    }
    //ボーンを今のアニメーションのポーズに合わせて、ジョイントを作り(すでにあれば作らない)、少し押す
    InitializePhysicsTransform();

    //倒れて落ち着いたら止まる処理を有効にして、その状態をリセットする
    if (_physicsSystem)
    {
        _physicsSystem->ResetRagdollTimer();
        _physicsSystem->SetRagdollRestEnabled(true);
    }

    //重力を入れて、ラグドールを有効にする。全身が重力で地面に倒れる
    SetFlying(false);
    _isWaitingSettle = false;
    Enable();
}

void Ragdoll::StartHanging()
{
    //ラグドールがまだ動いていない(アニメーション中)ときだけ、ボーンをアニメーションのポーズに合わせる。
    //すでに動いている(倒れている)ときは、その姿勢のまま持ち上げる
    if (!_enabled)
    {
        InitializeBonePoses();

        if (!_bones.empty() && _bones[0].rigidBody)
            _ragdollStartRootPosition = _bones[0].rigidBody->GetPosition();
    }

    //ジョイントを作る(すでにあるジョイントは作り直さない)
    CreateJoints();

    //吊る骨を探して Kinematic にする。Kinematic の骨は、重力や衝突で動かず、入力でだけ動く
    _hangingBoneIndex = -1;
    for (size_t boneIndex = 0; boneIndex < _bones.size(); ++boneIndex)
    {
        if (_bones[boneIndex].name == RagdollTuning::kHangingBoneName)
        {
            _hangingBoneIndex = static_cast<int>(boneIndex);

            auto& holdBody = _bones[boneIndex].rigidBody;
            holdBody->EnableKinematic();

            //倒れていたときの勢いが残らないようにして、その場でぴたりと持ち上げる
            holdBody->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
            holdBody->SetAngularVelocity(Vector3(0.0f, 0.0f, 0.0f));
            break;
        }
    }

    _hangingInputVelocity = Vector3(0.0f, 0.0f, 0.0f);
    _hangingDriftVelocity = Vector3(0.0f, 0.0f, 0.0f);

    //吊り下げているあいだは、静止して止まる処理を使わない(止まると、ぶらぶらが止まってしまう)
    if (_physicsSystem)
    {
        _physicsSystem->ResetRagdollTimer();
        _physicsSystem->SetRagdollRestEnabled(false);
    }

    //全身のラグドールを有効にする(すでに有効なら、寝ている骨を起こすだけ)。吊る骨以外は、重力でぶら下がる
    Enable();
}

void Ragdoll::ReleaseHanging()
{
    if (_hangingBoneIndex < 0)
        return;

    auto& holdBody = _bones[_hangingBoneIndex].rigidBody;

    //吊る骨を、ふつうの骨(Kinematic ではない)に戻す。飛ばされた勢いが残っていれば、その速度で落ち始める
    holdBody->DisableKinematic();
    holdBody->SetVelocity(_hangingInputVelocity + _hangingDriftVelocity);

    _hangingBoneIndex = -1;
    _hangingInputVelocity = Vector3(0.0f, 0.0f, 0.0f);
    _hangingDriftVelocity = Vector3(0.0f, 0.0f, 0.0f);

    //落ちて、倒れて、落ち着いたら止まる処理を、また有効にする
    if (_physicsSystem)
    {
        _physicsSystem->ResetRagdollTimer();
        _physicsSystem->SetRagdollRestEnabled(true);
    }
}

void Ragdoll::MoveHangingBone(const Vector3& delta, float deltaTime)
{
    if (_hangingBoneIndex < 0 || deltaTime <= 0.0f)
        return;

    auto& body = _bones[_hangingBoneIndex].rigidBody;

    //位置を直接動かす
    body->SetPosition(body->GetPosition() + delta);

    //動かした速度も覚えておく。ジョイントの速度の拘束が、子の骨を一緒に引っ張るために使う。
    //(速度が 0 のままだと、子は「吊る骨は止まっている」と見なして、置いていかれる)
    _hangingInputVelocity += Vector3(delta.x / deltaTime, delta.y / deltaTime, delta.z / deltaTime);
}

void Ragdoll::UpdateHanging(float deltaTime)
{
    if (_hangingBoneIndex < 0)
        return;

    auto& body = _bones[_hangingBoneIndex].rigidBody;

    //飛ばされた勢いで動く。時間とともに弱まる
    body->SetPosition(body->GetPosition() + _hangingDriftVelocity * deltaTime);
    _hangingDriftVelocity *= std::exp(-RagdollTuning::kHangingDriftDamping * deltaTime);

    //物理側からは、吊る骨は「この速度で動いている」ように見える
    body->SetVelocity(_hangingInputVelocity + _hangingDriftVelocity);

    //入力による速度は、毎フレームつくり直す
    _hangingInputVelocity = Vector3(0.0f, 0.0f, 0.0f);
}

void Ragdoll::StopRagdoll(bool includeHeight)
{
    for (auto& bone : _bones)
    {
        if (bone.rigidBody)
        {
            SetLockZ(false);
            bone.rigidBody->ResetMaxSpeed();
        }
    }
    //ラグドール中は、プレイヤーの Transform を動かしていない(ボーンは世界座標で動く)。
    //アニメーションに戻すと、体は Transform の位置に描かれるので、Hips が動いた分だけ Transform を動かして、
    //アニメーションの体を、ラグドールの体があった位置に移す
    if (!_bones.empty() && _bones[0].rigidBody)
    {
        const Vector3 movedDistance = _bones[0].rigidBody->GetPosition() - _ragdollStartRootPosition;
        auto& ownerPosition = _owner->GetTransform().Position;
        ownerPosition.x += movedDistance.x;
        if (includeHeight)
            ownerPosition.y += movedDistance.y;
        ownerPosition.z += movedDistance.z;
    }

    //吊る骨を、ふつうの骨(Kinematic ではない)に戻す
    if (_hangingBoneIndex >= 0)
    {
        _bones[_hangingBoneIndex].rigidBody->DisableKinematic();
        _hangingBoneIndex = -1;
    }
    _hangingInputVelocity = Vector3(0.0f, 0.0f, 0.0f);
    _hangingDriftVelocity = Vector3(0.0f, 0.0f, 0.0f);

    //静止して止まる処理を、また有効に戻す(次に倒れるときのため)
    if (_physicsSystem)
    {
        _physicsSystem->ResetRagdollTimer();
        _physicsSystem->SetRagdollRestEnabled(true);
    }

    //ラグドールを止める。全ボーンが寝て、速度と角速度が 0 に戻る
    SetFlying(false);
    _isWaitingSettle = false;
    Disable();
}


XMMATRIX Ragdoll::WorldPoseToOwnerLocalMatrix(const Vector3& worldPosition, const Quaternion& worldRotation,
    const XMMATRIX& ownerInverseWorld, const XMVECTOR& ownerWorldRotation)
{
    const XMVECTOR worldPos = XMVectorSet(worldPosition.x, worldPosition.y, worldPosition.z, 1.0f);
    const XMVECTOR worldRot = XMVectorSet(worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w);

    const XMVECTOR localPos = XMVector3TransformCoord(worldPos, ownerInverseWorld);

    XMVECTOR localRot = XMQuaternionMultiply(worldRot, XMQuaternionInverse(ownerWorldRotation));
    localRot = XMQuaternionNormalize(localRot);

    XMMATRIX localMatrix = XMMatrixRotationQuaternion(localRot);
    localMatrix.r[3] = XMVectorSetW(localPos, 1.0f);
    return localMatrix;
}
void Ragdoll::WritePhysicsPoseToNodes()
{
    //オーナーの変換は、ループの中で変わらないので最初に1回だけ求める
    const XMMATRIX ownerWorld = _owner->GetTransform().GetWorldMatrix();
    const XMMATRIX ownerInverseWorld = XMMatrixInverse(nullptr, ownerWorld);
    //ownerWorldには拡大縮小(Scale)が入っている。Scaleが入った行列から、直接クォータニオンを取ると
    //回転が不正確になる(Rotationが0のときだけ、たまたま合う)ので、Decomposeで回転だけを取り出す
    XMVECTOR ownerWorldScale, ownerWorldRotation, ownerWorldTranslation;
    XMMatrixDecompose(&ownerWorldScale, &ownerWorldRotation, &ownerWorldTranslation, ownerWorld);

    for (auto& ragdollBone : _bones)
    {
        if (!ragdollBone.rigidBody)
            continue;

        BoneNode& node = _skeleton->GetNode()[ragdollBone.nodeIndex];

        const XMMATRIX localMatrix = WorldPoseToOwnerLocalMatrix(
            ragdollBone.rigidBody->GetPosition(),
            ragdollBone.rigidBody->GetRotation(),
            ownerInverseWorld,
            ownerWorldRotation);

        XMStoreFloat4x4(&node.GlobalTransform, localMatrix);
    }
}
void Ragdoll::UpdateNonRagdollNodes()
{
    const int nodeCount = static_cast<int>(_skeleton->GetNode().size());

    for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
    {
        BoneNode& node = _skeleton->GetNode()[nodeIndex];

        //ragdollのボーンはWritePhysicsPoseToNodes()で決まっているので触らない
        if (ragdollNodeIndices.contains(nodeIndex))
            continue;

        //親がいない(スケルトンのルート)ものは、そのまま
        if (node.ParentIndex < 0)
            continue;

        const BoneNode& parent = _skeleton->GetNode()[node.ParentIndex];

        const XMMATRIX world =
            XMLoadFloat4x4(&node.LocalTransform) *
            XMLoadFloat4x4(&parent.GlobalTransform);

        XMStoreFloat4x4(&node.GlobalTransform, world);
    }
}
void Ragdoll::CopyNodeTransformsToSkeletonBones()
{
    for (auto& bone : _skeleton->GetBones())
    {
        bone.GlobalTransform = _skeleton->GetNode()[bone.NodeIndex].GlobalTransform;
    }
}
void Ragdoll::UpdateBonesFromPhysics()
{
    WritePhysicsPoseToNodes(); //ragdollのボーン: 物理の結果を書き込む
    UpdateNonRagdollNodes(); //ragdoll以外のノード: 親に追従させる
    CopyNodeTransformsToSkeletonBones();//スキニング用のボーン行列にコピーする
}

//----------------------押し出し処理------------------

void Ragdoll::SetFlying(bool flying)
{
    _isFlying = flying;

    //自由落下中は重力がトルクを生まないので、飛んでいる間だけ止める(止めないと体がくの字に折れる)
    for (auto& bone : _bones)
        if (bone.rigidBody)
            bone.rigidBody->SetUseGravityTorque(!flying);
}

Vector3 Ragdoll::GetRootPosition() const
{
    if (_enabled && !_bones.empty() && _bones[0].rigidBody)
        return _bones[0].rigidBody->GetPosition();

    return Vector::fromF3ToV3(_owner->GetTransform().Position);
}
Vector3 Ragdoll::GetRootVelocity()const
{
    if (_enabled && !_bones.empty() && _bones[0].rigidBody)
        return _bones[0].rigidBody->GetVelocity();

    return Vector3(0.0f, 0.0f, 0.0f);
}

std::string Ragdoll::GetBoneName(const Collider* collider) const
{
    for (const auto& bone : _bones)
    {
        if (bone.collider.get() == collider)
            return bone.name;
    }
    return std::string();
}

bool Ragdoll::AllBonesSleeping() const
{
    for (const auto& bone : _bones)
    {
        if (bone.rigidBody && !bone.rigidBody->IsSleeping())
            return false;
    }
    return true;
}

void Ragdoll::Update(float deltaTime)
{
    //どれかのボーンが地面に着いているか
    bool grounded = false;
    for (const auto& bone : _bones)
    {
        if (bone.rigidBody && bone.rigidBody->IsGrounded())
        {
            grounded = true;
            break;
        }
    }

    //発射した直後は、発射の瞬間に床へ触れていた(前の計算の結果が残っている)だけなので、地面に着いたとは、みなさない
    //(すぐに飛行を終えると、重力トルクが戻って、飛んでいる間ずっと、体がくの字に折れる)
    constexpr float minFlySeconds = 0.15f;

    //地面に着いたら、飛行を終える(あとは今までどおり重力で倒れる)
    if (_isFlying && grounded && _settleElapsed >= minFlySeconds)
        SetFlying(false);

    //発射したあと、空中から地面に着くたびに知らせる(着地の音など)
    //着きっぱなしのときは出さない。着いて離れてを、細かく繰り返しても、短い間に連続では出さない
    //発射した直後に、台に触れているだけのときも、出さない
    if (_isWaitingSettle)
    {
        constexpr float groundHitIntervalSeconds = 0.3f;

        _groundHitCooldown = (std::max)(_groundHitCooldown - deltaTime, 0.0f);
        if (grounded && !_wasGrounded && _settleElapsed >= minFlySeconds && _groundHitCooldown <= 0.0f)
        {
            _groundHitCooldown = groundHitIntervalSeconds;
            OnGroundHit.Invoke();
        }
    }
    _wasGrounded = grounded;

    //発射したあと、全ボーンが静止(スリープ)したら、1回だけ通知する
    if (!_isWaitingSettle)
        return;

    _settleElapsed += deltaTime;

    //静止しないままのとき(揺れ続けるなど)に、ゲームが進まなくなるのを防ぐ保険
    constexpr float maxWaitSeconds = 20.0f;
    if (AllBonesSleeping() || _settleElapsed >= maxWaitSeconds)
    {
        _isWaitingSettle = false;
        OnSettled.Invoke();
    }
}

void Ragdoll::Launch(const Vector3& direction,float speed)
{
    SetFlying(true);

    _isWaitingSettle = true;
    _settleElapsed = 0.0f;
    _groundHitCooldown = 0.0f;

    //寝ているBoneを起こす。飛んでいる間は、速度の上限を上げる(通常の上限だと、初速が頭打ちになる)
    for (auto& bone : _bones)
        if (bone.rigidBody)
        {
            bone.rigidBody->SetSleeping(false);
            bone.rigidBody->SetMaxSpeed(RagdollTuning::kLaunchMaxVelocitySpeed);
        }

    //体ぜんたいで「ぴょん」と跳ねる回数を、発射のたびに戻す(回数0のキャラクターは、ふつうのバウンドのまま)
    if (_physicsSystem)
        _physicsSystem->SetRagdollHop(_bounceRestitution, _bounceCount);

    //各ボーンを、今のZで固定する(2D)。全ボーン分を1回で設定する
    SetLockZ(true);

    //スリープタイマーをリセット
    if (_physicsSystem)
        _physicsSystem->ResetRagdollTimer();

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f); //-1～1
    std::uniform_real_distribution<float> chance(0.0f, 1.0f); //0～1

    //---- 発射の向きと速さを、少しずらす(XY平面の中で) ----
    const float angleJitter = DirectX::XMConvertToRadians(RagdollTuning::kLaunchAngleJitterDeg) * unit(rng);
    const float cosJitter = std::cos(angleJitter);
    const float sinJitter = std::sin(angleJitter);
    const Vector3 launchDirection(
        direction.x * cosJitter - direction.y * sinJitter,
        direction.x * sinJitter + direction.y * cosJitter,
        direction.z);
    const float launchSpeed = speed * (1.0f + RagdollTuning::kLaunchSpeedJitter * unit(rng));
    Vector3 launchVelocity = launchDirection * launchSpeed;

    Vector3 rootPosition(0.0f, 0.0f, 0.0f);
    if (!_bones.empty() && _bones[0].rigidBody)
        rootPosition = _bones[0].rigidBody->GetPosition();

    //---- 全身の回転: 飛ぶ向き(右なら前転)に合わせるが、たまに逆回り。速さも毎回変わる ----
    float spinDirection = (launchDirection.x >= 0.0f) ? -1.0f : 1.0f;
    if (chance(rng) < RagdollTuning::kLaunchSpinReverseChance)
        spinDirection = -spinDirection;
    const float tumbleSpeed = RagdollTuning::kLaunchTumbleSpeed * (1.0f + RagdollTuning::kLaunchTumbleJitter * unit(rng));
    Vector3 tumble(0.0f, 0.0f, spinDirection * tumbleSpeed);

    for (auto& bone : _bones)
    {
        if (!bone.rigidBody)
            continue;

        if (_hangingBoneIndex >= 0 && &bone == &_bones[_hangingBoneIndex])
        {
            _hangingDriftVelocity = launchVelocity;
            continue;
        }

        //全身の回転ぶんの速度(腰から離れているほど、速く動く)
        const Vector3 relative = bone.rigidBody->GetPosition() - rootPosition;
        Vector3 velocity = launchVelocity + Vector::Cross(tumble, relative);

        //ボーンごとのランダムな速度。腰(ルート)と胴体(背骨)は動かさず、手足や頭は大きく散らす
        //胴体の関節に、ばらばらの速度や回転を足すと、背骨が前かがみの向きに、端まで曲がったまま、飛行中ずっと固まる
        const bool isRoot = bone.parentIndex < 0;
        const bool isTrunk = bone.name.find("Spine") != std::string::npos;
        if (!isRoot && !isTrunk)
        {
            const bool isExtremity =
                bone.name.find("Hand") != std::string::npos ||
                bone.name.find("Foot") != std::string::npos ||
                bone.name.find("Head") != std::string::npos ||
                bone.name.find("ForeArm") != std::string::npos;
            const float scatterScale = RagdollTuning::kLaunchScatterSpeed *
                (isExtremity ? RagdollTuning::kLaunchExtremityScatterScale : 1.0f);

            velocity = velocity + Vector3(unit(rng), unit(rng), 0.0f) * scatterScale;
        }
        bone.rigidBody->SetVelocity(velocity);

        //ボーンごとのランダムな回転(Zまわり)。胴体は、全身の回転だけにする(ばたつかせない)
        const Vector3 flail = isTrunk ? Vector3(0.0f, 0.0f, 0.0f) : Vector3(0.0f, 0.0f, unit(rng));
        bone.rigidBody->SetAngularVelocity(tumble + flail * RagdollTuning::kLaunchFlailSpeed);
    }
}
void Ragdoll::SetLockZ(bool lock)
{
    for (auto& bone : _bones)
    {
        if (!bone.rigidBody) continue;
        if (lock) bone.rigidBody->SetLockStatus(Axis::Z, bone.rigidBody->GetPosition().z);
        else      bone.rigidBody->ClearAxisLock(Axis::Z);
    }
}

void Ragdoll::ApplyPushImpulse(const Vector3& pushDirIn, float speed, RagdollBone& hitBone, const Vector3& contactPoint)
{
    //方向を正規化する
    Vector3 pushDir = pushDirIn;
    float len = pushDir.Length();

    //０なら何もしない
    if (len < 0.0001f) return;
    pushDir /= len;

    if (!hitBone.rigidBody) return;

    //寝ているBoneを起こす
    for (auto& bone : _bones)
    {
        if (bone.rigidBody)
            bone.rigidBody->SetSleeping(false);
    }

    if (_physicsSystem)
        _physicsSystem->ResetRagdollTimer();

    auto& body = hitBone.rigidBody;
    Vector3 impulseVelocity = pushDir * speed;

    body->SetVelocity(body->GetVelocity() + impulseVelocity);

    //角速度を加える(接触点が重心からずれているぶん回る)
    Vector3 com = body->GetPosition() + body->GetRotation() * hitBone.colliderLocalOffset;
    Vector3 r = contactPoint - com;

    constexpr float pushAngularScale = 0.3f;
    body->SetAngularVelocity(
        body->GetAngularVelocity() + Vector::Cross(r, impulseVelocity) * (pushAngularScale / body->GetInertia() * body->GetMass()));
}
void Ragdoll::ApplyLaunchImpulse(const Vector3& pushDirIn, RagdollBone& hitBone, const Vector3& contactPoint)
{
    //水平方向に飛ばす
    Vector3 dir = pushDirIn;
    dir.y = 0.0f;
    if (dir.Length() < EPSILON)
        dir = pushDirIn;
    if (dir.Length() < EPSILON)
        return;
    dir.Normalize();

    //寝ているBoneを起こす
    for (auto& bone : _bones)
    {
        if (bone.rigidBody)
            bone.rigidBody->SetSleeping(false);
    }

    if (_physicsSystem)
        _physicsSystem->ResetRagdollTimer();

    //吹っ飛ばす水平と上向きに飛ばす
    Vector3 launchVelocity =
        dir * RagdollTuning::kLaunchSpeed + Vector3(0.0f, RagdollTuning::kLaunchUpSpeed, 0.0f);


    //ルートだけに力を与える
    Vector3 rootPosition(0.0f, 0.0f, 0.0f);
    if (!_bones.empty() && _bones[0].rigidBody)
        rootPosition = _bones[0].rigidBody->GetPosition();
    Vector3 tumble = Vector::Cross(Vector3(0.0f, 1.0f, 0.0f), dir) * RagdollTuning::kLaunchTumbleSpeed;

    //ランダムに範囲から数字を出してばたつきをつけている
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> flailDist(-1.0f, 1.0f);

    for (auto& bone : _bones)
    {
        if (!bone.rigidBody)
            continue;


        //吊る骨(Kinematic)は物理で動かせないので、飛ばされた勢いは別に持たせて、UpdateHanging で動かす
        if (_hangingBoneIndex >= 0 && &bone == &_bones[_hangingBoneIndex])
        {
            _hangingDriftVelocity = launchVelocity;
            continue;
        }
        Vector3 relative = bone.rigidBody->GetPosition() - rootPosition;
        bone.rigidBody->SetVelocity(launchVelocity + Vector::Cross(tumble, relative));

        Vector3 flail(flailDist(rng), flailDist(rng), flailDist(rng));
        bone.rigidBody->SetAngularVelocity(tumble + flail * RagdollTuning::kLaunchFlailSpeed);
    }

    //当たったボーンには、さらに接触点からの回転を加える 
    if (hitBone.rigidBody)
    {
        auto& body = hitBone.rigidBody;
        Vector3 com = body->GetPosition() + body->GetRotation() * hitBone.colliderLocalOffset;
        Vector3 leverArm = contactPoint - com;

        //当たった箇所が重心から離れている程回転をする
        body->SetAngularVelocity(
            body->GetAngularVelocity() +
            Vector::Cross(leverArm, launchVelocity) * (RagdollTuning::kLaunchSpinScale / body->GetInertia() * body->GetMass()));
    }
}
bool Ragdoll::IsPushCollider(const Collider* other) const
{
    if (!other) return false;

    //地面と自分以外
    return other->GetCollisionGroup() == CollisionGroup::Camera;
}
void Ragdoll::OnCollisionEnter(Collider* other, const CollisionResult& result)
{
    if (!IsPushCollider(other))
        return;

    //ラグドールが無効のとき(アニメーション再生中)は、飛ばさない
    if (!_enabled)
        return;

    RagdollBone* hitBone = nullptr;
    for (auto& bone : _bones)
    {
        if (bone.collider.get() == result.colliderA ||
            bone.collider.get() == result.colliderB)
        {
            hitBone = &bone;
            break;
        }
    }
    if (!hitBone) return;

    //当たったボーンによって方向を変える
    const Vector3 pushDir = (hitBone->collider.get() == result.colliderA) ? result.normal : -result.normal;
    if (RagdollTuning::kLaunchOnHit)
    {
        const auto now = std::chrono::steady_clock::now();
        if (_hasLaunched &&
            std::chrono::duration<float>(now - _lastLaunchTime).count() < RagdollTuning::kLaunchCooldown)
        {
            return;
        }
        _lastLaunchTime = now;
        _hasLaunched = true;

        ApplyLaunchImpulse(pushDir, *hitBone, result.contactPoint);
    }
    else
        ApplyPushImpulse(pushDir, result.penetration * 0.9f, *hitBone, result.contactPoint);
}
