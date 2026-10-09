#pragma once
#include "Component.h"
#include "Vector2.h"
#include "Datas.h"
#include "CharacterStatus.h"
#include "Primitive.h"

//四角形の中心と半分の大きさを持つ。描画に使う
class BoxShape : public Primitive
{
public:
	void SetHalfSize(Vector2 half) {_halfSize = half;}
	void SetCenter(Vector2 center)override { _center = center; }

	Vector2 GetCenter() const override{ return _center; }
	Vector2 GetHalfSize() const override{ return _halfSize; }
	Vector2 GetMin() const override { return _center - _halfSize; }
	Vector2 GetMax() const override { return _center + _halfSize; }
private:
	Vector2 _center;
	Vector2 _halfSize;
};

//敵のHP・種類・動き方を持つ
class EnemyStatus : public Component,public CharacterStatus
{
public:
	std::unique_ptr<Component> Clone()const override
	{
		return std::make_unique<EnemyStatus>(*this);
	}
	void Initialize(ServiceLocator& locator)override {};
	void Update(float deltaTime)override {};

	void SetMoveFlag(bool isMove) { _isMove = isMove; }
	bool GetMoveFlag()const { return _isMove; }

	//HPが0になったら非アクティブにするだけ。死亡時の処理はEnemyControllerが行う
	void OnColliderEnter(float damage)override
	{
		if (!GetIsActive()) return;//死亡演出中はダメージを受けない
		ApplyDamage(damage);
	}
	//攻撃のパターンを設定 パターンはGunDataを参照
	void SetMovePattern(MovePattern pattern) { _pattern = pattern; }
	MovePattern GetMovePattern() const { return _pattern; }
	void OnDead()override
	{
		SetIsActive(false);
	}

private:
	MovePattern _pattern = MovePattern::EnterAndStay;
	bool _isMove = true;
};