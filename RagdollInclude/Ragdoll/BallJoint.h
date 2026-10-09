#pragma once
#include "Joint.h"
#include "DirectXMath.h"
#include "Transform.h"

class BallJoint :public Joint
{
public:
    BallJoint(
        RigidBody* bodyA,
        RigidBody* bodyB,Quaternion InitRotation);

    //基底クラスのコメントを参照----
    void Solve(float deltaTime) override;
    void SolveAngularConstraint()override;
    void ApplyRestoringTorque()override;

    //-------------------

    //この関数はこのクラスでは使わない
    bool IsOutOfLimit(float hingeAngle)const override { return false; }

    //Ball ジョイントでは、ヒンジ角度の代わりに「swing の大きさ(基準姿勢からの首振りの角度)」を返す。
    float CalculateAngleError(const Quaternion& relativeRotation)override;


    //ここからはゲッターとセッター

    //ねじれと首振りをラジアンに変更して設定関数
    void SetTwistAndSwing(float minT, float maxT, float maxS)override 
    {
        _minTwist = XMConvertToRadians(minT);
        _maxTwist = XMConvertToRadians(maxT);
        _maxSwing = XMConvertToRadians(maxS);
    };

    //戻りの強さと減衰設定関数
    void SetStiffness(float stiffness, float damping)override
    {
        _stiffness = stiffness;
        _damping = damping;
    }

    //復元トルクで戻る先の姿勢角度設定関数
    void SetRestRotation(const Quaternion& restRot)override { _restRelativeRotation = restRot; }

    //ボーンの軸(twist の軸)を、ワールド座標の向きで指定する
    void SetTwistDirectionWorld(const Vector3& worldDirection) override
    {
        Vector3 direction = worldDirection;

        //向きがほぼ0だと軸が決まらないので、何もしない
        if (direction.Length() < EPSILON)
            return;
        direction.Normalize();

        //ワールドの向きを、Bのローカル座標へ変換して持つ
        Quaternion bodyBRotation = _bodyB->GetRotation();
        _twistAxis = bodyBRotation.Inverse() * direction;
        _twistAxis.Normalize();
    }

    //回転軸を設定関数
    Vector3 GetTwistAxis() const { return _twistAxis; }

    //親の情報を設定関数
    void SetTransform(Transform& transform) { _owner = &transform; };
    XMMATRIX GetOwnerWorldPos() { return _owner->GetWorldMatrix(); }

private:
    //Bの姿勢を、可動範囲内の目標へ少しずつ近づける
    //一度に目標へ合わせないのは、急な補正による跳ねを避けるようにする
    //rejectedAngle: 範囲から、どれだけ外れているか(rad)。大きく外れているときは、一気に戻す
    void CorrectRotationToLimit(const Quaternion& bodyARot, const Quaternion& currentRelative, const Quaternion& limitedDelta, float rejectedAngle);

    //範囲外へ向かう角速度を消す(速度レベル)。
    //姿勢だけ戻しても、外へ向かう角速度が残っていると、すぐまた外へ出てしまうため。
    void RemoveAngularVelocityBeyondLimit(const Vector3& worldRejAxis);

    //swing と twist を、それぞれ可動範囲に収めて合成した delta を返す(計算だけで、ボディは触らない)。
    Quaternion CalculateLimitedDelta(const Quaternion& swing, const Quaternion& twist)const;
private:
    Quaternion _restRelativeRotation{};
    Quaternion _initialRelativeRotation{};
    Transform* _owner = nullptr;

    //それぞれのRigidBodyから見た関節位置

    Vector3  _twistAxis;

    float _stiffness = 9.0f;
    float _damping = 0.5f;

    float _minTwist = 0.0f;
    float _maxTwist = 0.0f;
    float _maxSwing = 0.0f;
};
