#include "BallJoint.h"

//共通処理
namespace
{
    //クォータニオンの符号を反転して、w が正になる側にそろえる。
    void MakeShortestPath(Quaternion& rotation)
    {
        if (rotation.w < 0.0f)
        {
            rotation.x = -rotation.x;
            rotation.y = -rotation.y;
            rotation.z = -rotation.z;
            rotation.w = -rotation.w;
        }
    }

    //MakeShortestPath 済み(w が正)のクォータニオンの回転角(ラジアン)を返す。
    //acos ではなく atan2 を使うのは、角度が小さいときに誤差が出にくいため。
    float CalculateRotationAngle(const Quaternion& rotation)
    {
        Vector3 imaginaryPart(rotation.x, rotation.y, rotation.z);
        return 2.0f * atan2f(imaginaryPart.Length(), rotation.w);
    }

    //軸まわりの符号付き角度を返す。軸と逆向きに回っていれば負にする。
    //(CalculateRotationAngle は 0 以上しか返さないので、向きは軸との内積で決める)
    float CalculateSignedTwistAngle(const Quaternion& twist, const Vector3& twistAxis)
    {
        float angle = CalculateRotationAngle(twist);

        Vector3 imaginaryPart(twist.x, twist.y, twist.z);
        if (Vector::Dot(imaginaryPart, twistAxis) < 0.0f)
            angle = -angle;

        return angle;
    }
}

BallJoint::BallJoint(RigidBody* bodyA,
    RigidBody* bodyB, Quaternion initRotation) :Joint(bodyA,bodyB)
{
    Vector3 parentPosition = bodyA->GetPosition();
    Vector3 childPosition = bodyB->GetPosition();

    //Aの回転を基準にする。ボーンの向きを、Aから見た向きで持つため。
    Quaternion parentRotation = bodyA->GetRotation();

    Vector3 worldBoneDirection = childPosition - parentPosition;
    float boneLength = worldBoneDirection.Length();

    //twistの軸は、ボーンの向き(親->子)にする。
    if (boneLength > 0.0001f)
    {
        worldBoneDirection /= boneLength;

        //角度は初期の相対姿勢を基準にした座標系deltaで扱う。
        //そのため軸も、ワールド -> Aのローカル -> 初期姿勢基準 の順に変換して、同じ座標系にそろえる。
        _twistAxis = initRotation.Inverse() * (parentRotation.Inverse() * worldBoneDirection);
        _twistAxis.Normalize();
    }
    else
    {
        //親と子が同じ位置にあると向きが決まらないので、上向きを仮の軸にする。
        _twistAxis = Vector3(0, 1, 0);
    }

    _initialRelativeRotation = initRotation;
    _restRelativeRotation = initRotation;
}

void BallJoint::Solve(float deltaTime)
{
    Quaternion bodyARot = _bodyA->GetRotation();
    Quaternion bodyBRot = _bodyB->GetRotation();

    //ジョイントの位置のずれを、質量比で分配した補正量として求める。
    Vector3 correctionA, correctionB;
    CalculateAnchorCorrection(correctionA, correctionB);

    //位置のずれは、本物のVelocityではなく擬似速度として渡す。
    //本物のVelocityに足すと、ずれを直す動きが運動エネルギーとして残って、想定外に跳ねるため。
    Vector3 pseudoVelocityA = correctionA * _beta / deltaTime;
    Vector3 pseudoVelocityB = -correctionB * _beta / deltaTime;

    _bodyA->AddPseudoVelocity(pseudoVelocityA);
    _bodyB->AddPseudoVelocity(pseudoVelocityB);

    //アンカー点の相対速度を、全部ではなく一部(この強さ)だけ消す。
    //引数はボディの原点からアンカーまでのベクトル(ワールド座標)。
    constexpr float velocityConstraintStrength = RagdollTuning::velocityConstraintStrength;
    SolveAnchorVelocity(_bodyA, _bodyB,
        bodyARot * _localAnchorA, bodyBRot * _localAnchorB, velocityConstraintStrength);
}


void BallJoint::ApplyRestoringTorque()
{
    Quaternion bodyARot = _bodyA->GetRotation();
    Quaternion bodyBRot = _bodyB->GetRotation();

    Quaternion currentRelative = bodyARot.Inverse() * bodyBRot;
    currentRelative.Normalize();

    float swingAngle = CalculateAngleError(currentRelative);

    Vector3 worldSwingAxis = bodyARot * _restRelativeRotation * swingAngle;
    //ここが重要:「delta方向」に戻すので符号はマイナス
    //(swingは rest->current の回転を表すので、restへ戻すには逆方向)

    Vector3 relativeAngVel = _bodyB->GetAngularVelocity() - _bodyA->GetAngularVelocity();

    //ばね: swing の角度に比例して、戻す向きにかける。
    Vector3 torque = -worldSwingAxis * (swingAngle * _stiffness) - relativeAngVel * _damping;

    //Bにかけた分の逆向きをAにもかける 
    _bodyB->AddTorque(torque);
    _bodyA->AddTorque(-torque);
}


void BallJoint::SolveAngularConstraint()
{
    Quaternion bodyARot = _bodyA->GetRotation();
    Quaternion bodyBRot = _bodyB->GetRotation();

    //Aから見たBの現在の相対回転から、初期姿勢分を引いて初期姿勢からの変化(delta)だけを取り出す
    Quaternion currentRelative = bodyARot.Inverse() * bodyBRot;
    currentRelative.Normalize();

    Quaternion delta = _initialRelativeRotation.Inverse() * currentRelative;
    delta.Normalize();

    //変化を、首振りとボーン軸まわりのねじれに分ける
    Quaternion swing, twist;
    QuaternionMath::DecomposeSwingTwist(delta, _twistAxis, swing, twist);

    //符号を合わせる
    MakeShortestPath(swing);
    MakeShortestPath(twist);

    //可動範囲に収めた delta。範囲内なら delta と同じになる。
    Quaternion limitedDelta = CalculateLimitedDelta(swing, twist);


    //範囲内なら単位回転(角度0)になる。
    Quaternion rejectedRotation = delta.Inverse() * limitedDelta;
    rejectedRotation.Normalize();
    MakeShortestPath(rejectedRotation);
    float rejectedAngle = CalculateRotationAngle(rejectedRotation);

    Vector3 rejectedAxis(rejectedRotation.x, rejectedRotation.y, rejectedRotation.z);
    float rejectedAxisLength = rejectedAxis.Length();

    //境界ぎりぎりで補正が入ったり入らなかったりして、震えるのを防ぐ。
    constexpr float rejectedAngleDeadband = RagdollTuning::rejectedAngleDeadband;

    //範囲内なら、速度も姿勢も触らない。
    if (rejectedAxisLength <= EPSILON || rejectedAngle <= rejectedAngleDeadband)
        return;

    rejectedAxis /= rejectedAxisLength;

    Vector3 worldRejectedAxis = bodyBRot * rejectedAxis;

    //速度を先に、姿勢を後に計算
    RemoveAngularVelocityBeyondLimit(worldRejectedAxis);
    CorrectRotationToLimit(bodyARot, currentRelative, limitedDelta, rejectedAngle);
}


void BallJoint::RemoveAngularVelocityBeyondLimit(const Vector3& worldRejectedAxis)
{
    //範囲に当たるとき両方が0でないことで軸がずれるのを防ぐ
    Vector3 angularVelocityA = _bodyA->GetAngularVelocity();
    Vector3 angularVelocityB = _bodyB->GetAngularVelocity();

    //制限は「AとBの相対的な回転」に対するものなので、相対角速度で判定する
    Vector3 relativeAngularVelocity = angularVelocityB - angularVelocityA;
    float relativeSpeedAlongRejectedAxis = Vector::Dot(relativeAngularVelocity, worldRejectedAxis);

    //戻す向きとは逆(さらに外)へ回っているときだけ消す
    if (relativeSpeedAlongRejectedAxis >= 0.0f)
        return;

    //角速度の変えやすさは慣性で決まる。動かせないボディは受け取らない
    float invInertiaA = _bodyA->IsKinematic() ? 0.0f : 1.0f / _bodyA->GetInertia();
    float invInertiaB = _bodyB->IsKinematic() ? 0.0f : 1.0f / _bodyB->GetInertia();
    float invInertiaSum = invInertiaA + invInertiaB;
    if (invInertiaSum <= 0.0f)
        return;

    constexpr float velocityRemovalRatio = RagdollTuning::velocityRemovalRatio;
    Vector3 velocityToRemove = worldRejectedAxis * (relativeSpeedAlongRejectedAxis * velocityRemovalRatio);

    //相対角速度が消える向きに、慣性の逆数で分配する
    _bodyB->SetAngularVelocity(angularVelocityB - velocityToRemove * (invInertiaB / invInertiaSum));
    _bodyA->SetAngularVelocity(angularVelocityA + velocityToRemove * (invInertiaA / invInertiaSum));
}


Quaternion BallJoint::CalculateLimitedDelta(const Quaternion& swing, const Quaternion& twist)const
{
    //twist角度を範囲
    float twistAngle = CalculateSignedTwistAngle(twist, _twistAxis);

    float limitedTwistAngle = std::clamp(twistAngle, _minTwist, _maxTwist);
    Quaternion limitedTwist = QuaternionMath::CreateFromAxisAngle(_twistAxis, limitedTwistAngle);
    limitedTwist.Normalize();

    //swingコーンの内側に収める。向きは保ったまま、角度だけを詰める
    float swingAngle = CalculateRotationAngle(swing);

    Quaternion limitedSwing = swing;
    if (swingAngle > _maxSwing)
    {
        Vector3 swingAxis(swing.x, swing.y, swing.z);
        float swingAxisLength = swingAxis.Length();

        //軸がほぼ0なら向きが決まらない
        if (swingAxisLength > EPSILON)
        {
            swingAxis /= swingAxisLength;
            limitedSwing = QuaternionMath::CreateFromAxisAngle(swingAxis, _maxSwing);
            limitedSwing.Normalize();
        }
    }

    //合成の順序は DecomposeSwingTwist の分解(swing * twist)に合わせる
    Quaternion limitedDelta = limitedSwing * limitedTwist;
    limitedDelta.Normalize();

    return limitedDelta;
}


float BallJoint::CalculateAngleError(const Quaternion& relativeRotation)
{
    //基準姿勢分を引いて、基準姿勢からの変化だけにする
    Quaternion delta = _restRelativeRotation.Inverse() * relativeRotation;
    delta.Normalize();

    //復元の対象は swing だけ。twist は使わない
    Quaternion swing, twist;
    QuaternionMath::DecomposeSwingTwist(delta, _twistAxis, swing, twist);
    MakeShortestPath(swing);

    Vector3 swingAxis(swing.x, swing.y, swing.z);
    float swingAxisLength = swingAxis.Length();

    //swing がほぼ0なら、軸が決まらないので、ずれ0として返す
    if (swingAxisLength <= EPSILON)
        return 0.0f;

    return 2.0f * atan2f(swingAxisLength, swing.w);
}


void BallJoint::CorrectRotationToLimit(const Quaternion& bodyARot, const Quaternion& currentRelative, const Quaternion& limitedDelta, float rejectedAngle)
{
    //目標の相対回転(Aから見たBの、可動範囲内の姿勢)
    Quaternion limitedRelativeRotation = _initialRelativeRotation * limitedDelta;
    limitedRelativeRotation.Normalize();

    //目標へ近づける割合。範囲を大きく超えているときは、1回で戻りきれず、折れ曲がったまま固まるので、一気に戻す(1.0)
    const float angularBeta = (rejectedAngle > RagdollTuning::angularSnapAngle) ? 1.0f : RagdollTuning::angularBeta;
    Quaternion blendedRelativeRotation = QuaternionMath::Nlerp(currentRelative, limitedRelativeRotation, angularBeta);
    blendedRelativeRotation.Normalize();

    //Aを基準にBの姿勢を決め直す。SetRotation は角速度を変えないので、速度側を先に処理している
    Quaternion newBodyBRot = bodyARot * blendedRelativeRotation;
    newBodyBRot.Normalize();
    //B が Kinematic(吊られている骨)のときは、B は動かさず、A を回して相対回転を合わせる
    if (_bodyB->IsKinematic())
    {
        if (!_bodyA->IsKinematic())
        {
            Quaternion inverseRelativeRotation = blendedRelativeRotation.Inverse();
            Quaternion newBodyARot = _bodyB->GetRotation() * inverseRelativeRotation;
            newBodyARot.Normalize();
            _bodyA->SetRotation(newBodyARot);
        }
        return;
    }
    _bodyB->SetRotation(newBodyBRot);
}
