#pragma once
#include <algorithm>

#include "GunState.h"
#include "StateMachine.h"
#include "Vector2.h"
#include "EffectUtillity.h"
#include "TimedState.h"
#include "EnemyCommonStates.h"

class EnemyController;
using EnemyStateBase = StateMachine<EnemyController>::State;

namespace EnemyStates
{
	//PlayerStatesにも同名のenumがあるため、namespaceの中に置いて衝突を防ぐ
	enum LifeEvent
	{
		EventMove,
		EventMoveOnce,
		EventSecondform,
		EventInvincible,
		EventInvincibleEnd,
		EventDied,
	};
	enum AttackEvent
	{
		EventLongShot,
		EventSpreadShot,
		EventReflectShot,
		EventLaser,
		EventAttackStop
	};

	//画面の端から、この距離より内側を移動先にする
	inline constexpr float ScreenMargin = 50.0f;

	//ノーマルステート

	//画面外から登場する。MovePatternによって、留まるか通り抜けて消えるかが変わる
	class EventState :public EnemyStateBase
	{
	protected:
		void OnEnter(State* prevState)override;
		void OnUpdate(float deltaTime)override;
		void OnExit(State* nextState)override;
	private:
		void DispatchByState();
	private:
		float _entryTargetY = 100.0f;
	};

	//ランダムな位置へ一度だけ移動する
	class MoveOnceState : public EnemyStateBase
	{
	protected:
		void OnEnter(State* prevState)override;
		void OnUpdate(float deltaTime)override;
		void OnExit(State* nextState)override {};
	private:
		void RandomTargetPosition();
	private:
		float _moveDuration = 10.0f;
	};

	//同じ高さのまま左右にランダム移動し続ける
	class AliveState : public RandomMoveXState<EnemyController>
	{
	public:
		AliveState() : RandomMoveXState(2.0f, ScreenMargin) {}
	};

	class InvincibleState :public EnemyStateBase
	{
	protected:
		void OnEnter(State* prevState)override {};
		void OnUpdate(float deltaTime)override {};
		void OnExit(State* nextState)override {};
	};

	//撃破エフェクトを出し、一定時間後に削除を予約する
	class DeadState : public TimedState<EnemyController>
	{
	public:
		DeadState() : TimedState(2.0f) {}
	protected:
		void OnEnter(State* prevState)override ;
		void OnTimeUp()override;
		void OnDraw()override;
	private:
		float _effectTime = 1.0f;
	};

	//攻撃処理ステート
	//雑魚の攻撃は1種類なので、AttackTypeの0番目を使う

	class LongRangeShotState : public ShotState<EnemyController>
	{
	public:
		LongRangeShotState() : ShotState(0, 0.5f) {}
	protected:
		void Shoot(EnemyAttack& attack)override { attack.LongRangeShot(); }
	};
	class ReflectShotState : public ShotState<EnemyController>
	{
	public:
		ReflectShotState() : ShotState(0, 2.0f) {}
	protected:
		void Shoot(EnemyAttack& attack)override { attack.ReflectShot(); }
	};
	class SpreadShotState : public ShotState<EnemyController>
	{
	public:
		SpreadShotState() : ShotState(0, 2.0f) {}
	protected:
		void Shoot(EnemyAttack& attack)override { attack.SpreadShot(); }
	};
	class LaserState : public EnemyStateBase, public GunState
	{
	protected:
		void OnEnter(State* prevState)override {};
		void OnUpdate(float deltaTime)override {};
		void OnExit(State* nextState)override {};
	};
	class AttackDisabledState : public EnemyStateBase {};
}
