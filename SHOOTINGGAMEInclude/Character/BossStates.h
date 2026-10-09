#pragma once
#include "GunState.h"
#include "StateMachine.h"
#include "Vector2.h"
#include "TimedState.h"
#include "EnemyCommonStates.h"

class BossController;
using BossStateBase = StateMachine<BossController>::State;

namespace BossStates
{
	//他のStatesにも同名のenumがあるため、namespaceの中に置いて衝突を防ぐ
	enum LifeEvent
	{
		EventEntered,
		EventSecondForm,
		EventInvincible,
		EventDied,
	};
	enum AttackEvent
	{
		EventLongShot,
		EventReflectShot,
		EventSpreadShot,
		EventSwirlShot,
		EventAttackStop,
	};

	//ボスが左右に動く範囲の、画面端からの余白
	inline constexpr float ScreenMargin = 80.0f;

	//ノーマルステート

	//画面の上から降りてくる。登場中は当たり判定をなくす
	class EnterState : public BossStateBase
	{
	protected:
		void OnEnter(State* prevState)override;
		void OnUpdate(float deltaTime)override;
		void OnExit(State* nextState)override;
		void OnDraw()override;
	private:
		float _entryTargetY = 110.0f;
		float _entryDuration = 3.0f;
	};

	//第一形態：同じ高さのまま左右に移動し続ける
	class AliveState : public RandomMoveXState<BossController>
	{
	public:
		AliveState() : RandomMoveXState(2.5f, ScreenMargin) {}
	protected:
		void OnDraw()override;
	};

	//第二形態：第一形態より速く左右に移動する
	class SecondForm : public RandomMoveXState<BossController>
	{
	public:
		SecondForm() : RandomMoveXState(1.0f, ScreenMargin) {}
	protected:
		void OnDraw()override;
	};

	//形態移行中：一定時間当たり判定をなくして点滅し、第二形態へ移る
	class InvincibleState : public TimedState<BossController>
	{
	public:
		InvincibleState() : TimedState(3.0f) {}
	protected:
		void OnEnter(State* previousState) override;
		void OnExit(State* nextState) override;
		void OnTimeUp() override;
		void OnDraw() override;
	};

	//撃破エフェクトを出し、終わったらOnDeathFinishedで知らせる
	//シーン切り替えまでHUDなどから参照されるので、削除はしない
	class DeadState : public TimedState<BossController>
	{
	public:
		DeadState() : TimedState(3.0f) {}
	protected:
		void OnEnter(State* prevState);
		void OnTimeUp()override;
		void OnDraw()override;
	private:
		float _effectTime = 1.5f;
	};

	//攻撃ステート
	//撃つ弾の種類は、JSONのAttackTypeの何番目を使うかで決める

	class LongRangeShotState : public ShotState<BossController>
	{
	public:
		LongRangeShotState() : ShotState(0, 0.4f) {}
	protected:
		void Shoot(EnemyAttack& attack)override { attack.LongRangeShot(); }
	};
	class ReflectShotState : public ShotState<BossController>
	{
	public:
		ReflectShotState() : ShotState(1, 1.2f) {}
	protected:
		void Shoot(EnemyAttack& attack)override { attack.ReflectShot(); }
	};
	class SpreadShotState : public ShotState<BossController>
	{
	public:
		SpreadShotState() : ShotState(2, 1.0f) {}
	protected:
		void Shoot(EnemyAttack& attack)override { attack.SpreadShot(); }
	};

	//撃つたびに角度を少しずつ回して、渦巻きに見せる
	class SwirlShotState : public ShotState<BossController>
	{
	public:
		SwirlShotState() : ShotState(3, 0.08f) {}
	protected:
		void OnEnter(State* prevState)override;
		void Shoot(EnemyAttack& attack)override;
	private:
		float _rotateStep = 24.0f;     //1回撃つごとに回す角度
		float _angle = 0.0f;
	};
	class AttackDisabledState : public BossStateBase {};
}
