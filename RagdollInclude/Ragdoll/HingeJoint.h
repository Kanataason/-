#pragma once
#include "Joint.h"

class HingeJoint :public Joint
{
public:
	HingeJoint(RigidBody* bodyA,
		RigidBody* bodyB, const Vector3& hingeAxis,Quaternion initRotation);
	
	//---基底クラスのコメントを参照---------

	void Solve(float deltaTime)override;
	float CalculateAngleError(const Quaternion& relativeRotation)override ;
	bool IsOutOfLimit(float hingeAngle)const override;
	void SolveAngularConstraint()override;
	void ApplyRestoringTorque()override;

	//-----------------------------


	//ここからはゲッターとセッター

    //回転できる角度設定関数
	void SetRotationLimit(float min,float max)override
	{
		//ラジアンに変換してから設定する,ラジアン前提で計算をしているため
		_minAngle = XMConvertToRadians(min);
		_maxAngle = XMConvertToRadians(max);
	}

	//もとに戻る力と減衰設定関数
	void SetStiffness(float stiffness, float damping)override
	{
		//stiffnessは脱力感を出すために胴体以外０に設定をしている
		_stiffness = stiffness;
		_damping = damping;
	}

	//回転軸設定関数
	void SetAxis(const Vector3& axis)override { _hingeAxis = axis; };
private:
	//Bの姿勢を、swing なし・角度は可動範囲内の目標へ、少しずつ近づける
    //範囲内は弱く(ヒンジの自然な動きを邪魔しない)、範囲外は強く戻す
	void CorrectHingeRotation(const Quaternion& bodyARot, const Quaternion& currentRelative, float hingeAngle, bool isOutOfLimit);

	//可動範囲の外で、さらに外へ向かっている軸まわりの相対角速度を消す
    //戻る向きの角速度は残して、自然に範囲内へ戻れるようにする
	void RemoveAngularVelocityBeyondLimit(float hingeAngle, const Vector3& worldHingeAxis);

	//軸に垂直な相対角速度を消す。ヒンジは軸まわりにしか回らないため
    //分配は質量ではなく慣性の逆数で行う(角速度の変えやすさは慣性で決まる)
	void RemovePerpendicularAngularVelocity(const Vector3& worldHingeAxis);
private:

	float _minAngle = 0.0f;
	float _maxAngle = 0.0f;

	float _stiffness = 1.0f;
	float _damping = 0.5f;

	Quaternion _initialRelativeRotation{};
	Vector3 _hingeAxis{};
};
