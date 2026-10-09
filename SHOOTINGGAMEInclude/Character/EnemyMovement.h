#pragma once
#include "Component.h"
#include "Vector2.h"

//敵の移動を担当する
//開始位置から目標位置まで、指定した時間で補間して移動する
class EnemyMovement : public Component
{
public:
	std::unique_ptr<Component> Clone()const override
	{
		return std::make_unique<EnemyMovement>(*this);
	}
	void Initialize(ServiceLocator& locator)override {};
	void Update(float deltaTime)override {};

	//目標位置に着いたらtrueを返す
	bool Translate(float deltaTime);
	//目的地までLerpを使って滑らかに動かしていく
	void SetDestination(const Vector2& targetPos, float duration);

	void SetIsMoveFlag(bool isMove) { _isMove = isMove; }
private:
	Vector2 _startPos{};
	Vector2 _targetPos{};

	bool _isMove = true;

	float _duration = 12.0f;
	float _elapsedTime = 0.0f;
};
