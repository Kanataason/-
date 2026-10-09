#pragma once
#include <DxLib.h>
#include "StateMachine.h"
#include "GunState.h"
#include "Vector2.h"
#include "GameObject.h"
#include "GameWindow.h"
#include "EnemyMovement.h"
#include "EnemyAttack.h"

//雑魚とボスで共通して使うステートの基底クラス
//TOwnerはステートの持ち主（EnemyController / BossController）

//同じ高さのまま、左右のランダムな位置へ移動し続ける
//TOwnerに必要なもの：GetMovement()、GetBounds()
template<class TOwner>
class RandomMoveXState : public StateMachine<TOwner>::State
{
protected:
	using StateBase = typename StateMachine<TOwner>::State;

	RandomMoveXState(float moveDuration, float screenMargin)
		: _moveDuration(moveDuration), _screenMargin(screenMargin) {
	}

	void OnEnter(StateBase* previousState) override
	{
		SetRandomTargetX();
	}
	void OnUpdate(float deltaTime) override
	{
		//目的地に着いたら次の目的地を決める
		if (this->owner().GetMovement()->Translate(deltaTime))
			SetRandomTargetX();
	}
private:
	void SetRandomTargetX()
	{
		auto& controller = this->owner();
		float range = controller.GetBounds().width - _screenMargin * 2.0f;
		Vector2 target = controller.GetOwner()->GetTransform().Position;
		target.x = _screenMargin + static_cast<float>(GetRand(static_cast<int>(range)));
		controller.GetMovement()->SetDestination(target, _moveDuration);
	}
private:
	float _moveDuration;
	float _screenMargin;   //画面の端から、この距離より内側を移動先にする
};

//JSONのAttackTypeのattackIndex番目の弾を、一定間隔で撃ち続ける
//どう撃つかだけを継承先のShootで決める
//TOwnerに必要なもの：GetAttack()
template<class TOwner>
class ShotState : public StateMachine<TOwner>::State, public GunState
{
protected:
	using StateBase = typename StateMachine<TOwner>::State;

	ShotState(size_t attackIndex, float shotCoolDown)
		: _attackIndex(attackIndex), _shotCoolDown(shotCoolDown) {
	}

	void OnEnter(StateBase* previousState) override
	{
		EnemyAttack* attack = this->owner().GetAttack();
		attack->SetCurrentAttackIndex(_attackIndex);
		attack->InitializeShotSetting(attack->GetAttackType());
		SetGunState(_shotCoolDown);
	}
	void OnUpdate(float deltaTime) override
	{
		if (_coolDown <= 0.0f)
		{
			SetShotInfo(false, _shotCoolDown);
			Shoot(*this->owner().GetAttack());
		}
		ElapsedTimer(deltaTime);
	}

	//1回分の撃ち方
	virtual void Shoot(EnemyAttack& attack) = 0;
private:
	size_t _attackIndex;
	float _shotCoolDown;
};
