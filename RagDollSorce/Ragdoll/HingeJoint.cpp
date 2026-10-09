#include "HingeJoint.h"

HingeJoint::HingeJoint(RigidBody* bodyA,
	RigidBody* bodyB, const Vector3& hingeAxis, Quaternion initRotation) :Joint(bodyA, bodyB)
{
    //Aから見たBの相対回転
    _initialRelativeRotation = initRotation;

    //回転の軸を正規化して設定
    _hingeAxis = Vector::Normalized(hingeAxis);
}


void HingeJoint::Solve(float deltaTime)
{
    Quaternion bodyARot = _bodyA->GetRotation();
    Quaternion bodyBRot = _bodyB->GetRotation();

    //Jointの位置誤差を質量比で分配する
    Vector3 correctionA, correctionB;
    CalculateAnchorCorrection(correctionA,correctionB);

    //移動した値は疑似速度として渡すことで想定していない動きを減らす
    Vector3 pseudoVelocityA =
        correctionA * _beta / deltaTime;

    Vector3 pseudoVelocityB =
        -correctionB * _beta / deltaTime;

    _bodyA->AddPseudoVelocity(pseudoVelocityA);
    _bodyB->AddPseudoVelocity(pseudoVelocityB);

    //アンカー点の相対速度を、全部ではなく一部(この強さ)だけ消す。
    constexpr float velocityConstraintStrength = RagdollTuning::velocityConstraintStrength;
    SolveAnchorVelocity(_bodyA, _bodyB, bodyARot * _localAnchorA, bodyBRot * _localAnchorB, velocityConstraintStrength);
}


void HingeJoint::ApplyRestoringTorque()
{
    Quaternion bodyARot = _bodyA->GetRotation();
    Quaternion bodyBRot = _bodyB->GetRotation();

    //Aから見たBの現在の相対回転から
    //初期姿勢分を引いて初期姿勢からの変化だけを取り出す
    Quaternion currentRelative = bodyARot.Inverse() * bodyBRot;
    currentRelative.Normalize();

    float twistAngle = CalculateAngleError(currentRelative);

    //ワールドに変換をする
    Vector3 worldAxis = bodyARot * _initialRelativeRotation * _hingeAxis;

    //軸まわりの相対角速度
    float relativeAngVelAlongAxis =
        Vector::Dot(_bodyB->GetAngularVelocity() - _bodyA->GetAngularVelocity(), worldAxis);

    //角度のずれを比例して逆方向に戻す
    float torqueScalar =
        -twistAngle * _stiffness - relativeAngVelAlongAxis * _damping;

    Vector3 torque = worldAxis * torqueScalar; 

    //Bにかけた分の逆向きをAにもかける
    _bodyB->AddTorque(torque);
    _bodyA->AddTorque(-torque);
}


void HingeJoint::SolveAngularConstraint()
{
    Quaternion bodyARot = _bodyA->GetRotation();
    Quaternion bodyBRot = _bodyB->GetRotation();

    Quaternion currentRelative = bodyARot.Inverse() * bodyBRot;
    currentRelative.Normalize();

    float twistAngle = CalculateAngleError(currentRelative);
    bool isOutOfLimit = IsOutOfLimit(twistAngle);
    Vector3 worldHingeAxis = bodyARot * _initialRelativeRotation * _hingeAxis;

    //速度を揃える
    RemovePerpendicularAngularVelocity(worldHingeAxis);

    //回転をAだけ補正する
    CorrectHingeRotation(bodyARot, currentRelative, twistAngle, isOutOfLimit);

    //可動範囲の外へ向かう角速度を消す
    if (isOutOfLimit)
        RemoveAngularVelocityBeyondLimit(twistAngle, worldHingeAxis);
}


float HingeJoint::CalculateAngleError(const Quaternion& relativeRotation)
{
    //初期姿勢分を引いて、初期姿勢からの変化だけにする
    Quaternion delta = _initialRelativeRotation.Inverse() * relativeRotation;
    delta.Normalize();

    //変化をヒンジ軸まわりの回転(twist)とそれ以外(swing)に分ける。
    Quaternion swing, twist;
    QuaternionMath::DecomposeSwingTwist(delta, _hingeAxis, swing, twist);

    //クオータニオンはマイナスと正が同じ回転と解釈させるのでそこの対策
    //w が正になる側にそろえて角度を一意にする
    if (twist.w < 0.0f) { twist.x = -twist.x; twist.y = -twist.y; twist.z = -twist.z; twist.w = -twist.w; }

    //twistから角度(ラジアン)を取り出す
    //acosではなくatan2を使っているのは、角度が小さいときに誤差が出にくいため
    Vector3 twistVector(twist.x, twist.y, twist.z);
    float twistAngle = 2.0f * atan2f(twistVector.Length(), twist.w);

    if (twistAngle > XM_PI)
        twistAngle -= 2.0f * XM_PI;

    //軸と逆向きに回っているときは、負の角度にする
    if (Vector::Dot(twistVector, _hingeAxis) < 0.0f)
        twistAngle = -twistAngle;

    return twistAngle;

}


bool HingeJoint::IsOutOfLimit(float hingeAngle)const
{
    constexpr float limitTolerance = 0.02f;

    return (hingeAngle < _minAngle - limitTolerance) ||
        (hingeAngle > _maxAngle + limitTolerance);
}


void HingeJoint::RemovePerpendicularAngularVelocity(const Vector3& worldHingeAxis)
{
    Vector3 angularVelocityA = _bodyA->GetAngularVelocity();
    Vector3 angularVelocityB = _bodyB->GetAngularVelocity();
    Vector3 relativeAngularVelocity = angularVelocityB - angularVelocityA;

    //相対角速度から軸方向の成分を引くと、軸に垂直な成分だけが残る
    Vector3 perpendicularAngularVelocity =
        relativeAngularVelocity - worldHingeAxis * Vector::Dot(relativeAngularVelocity, worldHingeAxis);

    //質量ではなく慣性で分配する。角速度の変えやすさは慣性で決まるため
    float invInertiaA = _bodyA->IsKinematic() ? 0.0f : 1.0f / _bodyA->GetInertia();
    float invInertiaB = _bodyB->IsKinematic() ? 0.0f : 1.0f / _bodyB->GetInertia();
    float invInertiaSum = invInertiaA + invInertiaB;
    if (invInertiaSum <= 0.0f)
        return;

    //全部ではなく一部だけ消して、複数パスで収束させる
    constexpr float angularVelocityStrength = 0.5f;
    _bodyB->SetAngularVelocity(angularVelocityB - perpendicularAngularVelocity * (angularVelocityStrength * invInertiaB / invInertiaSum));
    _bodyA->SetAngularVelocity(angularVelocityA + perpendicularAngularVelocity * (angularVelocityStrength * invInertiaA / invInertiaSum));
}


void HingeJoint::RemoveAngularVelocityBeyondLimit(float hingeAngle, const Vector3& worldHingeAxis)
{
    Vector3 relativeAngVel = _bodyB->GetAngularVelocity() - _bodyA->GetAngularVelocity();
    float angVelAlongAxis = Vector::Dot(relativeAngVel, worldHingeAxis);

    //「さらに外へ向かっている」場合だけ消す。戻る向きの角速度は残して、自然に戻れるようにする。
    bool movingFurtherOutOfLimit =
        (hingeAngle >= _maxAngle && angVelAlongAxis > 0.0f) ||
        (hingeAngle <= _minAngle && angVelAlongAxis < 0.0f);

    if (movingFurtherOutOfLimit)
    {
        float invMassA = 1.0f / _bodyA->GetMass();
        float invMassB = 1.0f / _bodyB->GetMass();
        float sum = invMassA + invMassB;

        if (sum > 0.0f)
        {
            Vector3 velCorrection = worldHingeAxis * angVelAlongAxis;
            _bodyB->SetAngularVelocity(_bodyB->GetAngularVelocity() - velCorrection * (invMassB / sum));
            _bodyA->SetAngularVelocity(_bodyA->GetAngularVelocity() + velCorrection * (invMassA / sum));
        }
    }
}


void HingeJoint::CorrectHingeRotation(const Quaternion& bodyARot,const Quaternion& currentRelative, float hingeAngle, bool isOutOfLimit)
{
    float limitedAngle = std::clamp(hingeAngle, _minAngle, _maxAngle);
    Quaternion limitedTwist = QuaternionMath::CreateFromAxisAngle(_hingeAxis, limitedAngle);
    limitedTwist.Normalize();

    Quaternion limitedRelativeRotation = _initialRelativeRotation * limitedTwist;
    limitedRelativeRotation.Normalize();
    //範囲内は swing だけを消して弱く(0.1)、範囲外は強く(0.5)戻す
    constexpr float angularBetaInLimit = RagdollTuning::angularBetaInLimit;
    constexpr float angularBetaOutOfLimit = RagdollTuning::angularBetaOutOfLimit;

    float angularBeta = isOutOfLimit ? angularBetaOutOfLimit : angularBetaInLimit;

    //現在の姿勢から目標の姿勢へ、割合(angularBeta)だけ近づける。
    //一度に目標へ合わせず少しずつ近づけるのは、位置側の_betaと同じく、急な補正による跳ねを避けるため。
    Quaternion blendedRelativeRotation =
        QuaternionMath::Nlerp(currentRelative, limitedRelativeRotation, angularBeta);
    blendedRelativeRotation.Normalize();
    //補正するのはBだけ
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