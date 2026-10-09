#include <DxLib.h>

#include "BossStates.h"
#include "BossController.h"
#include "EnemyMovement.h"
#include "EnemyAttack.h"
#include "EnemyStatus.h"
#include "SoundManager.h"

#include "Collider.h"
#include "GameWindow.h"
#include "EffectUtillity.h"
#include "GameObject.h"


namespace BossStates
{
	void EnterState::OnEnter(State* prevState)
	{
		owner().GetCollider()->SetCollisionEnabled(false);   //登場中は当たり判定をなくす

		auto& position = owner().GetOwner()->GetTransform().Position;
		owner().GetMovement()->SetDestination({ position.x, _entryTargetY }, _entryDuration);
	}
	void EnterState::OnUpdate(float deltaTime)
	{
		if (owner().GetMovement()->Translate(deltaTime))
		{
			stateMachine().Dispatch(LifeEvent::EventEntered);
		}
	}
	void EnterState::OnExit(State* nextState)
	{
		owner().GetCollider()->SetCollisionEnabled(true);
		owner().StartAttack();
	}
	void EnterState::OnDraw()
	{
		owner().DrawBody();
	}

	void AliveState::OnDraw()
	{
		owner().DrawBody();
	}

	void SecondForm::OnDraw()
	{
		owner().DrawBody();
	}

	void InvincibleState::OnEnter(State* prevState)
	{
		TimedState::OnEnter(prevState);
		owner().GetCollider()->SetCollisionEnabled(false);   //無敵中は当たり判定をなくす
	}
	void InvincibleState::OnExit(State* nextState)
	{
		owner().GetCollider()->SetCollisionEnabled(true);
	}
	void InvincibleState::OnTimeUp()
	{
		stateMachine().Dispatch(LifeEvent::EventSecondForm);
	}
	void InvincibleState::OnDraw()
	{
		//残り時間から、点滅で表示するフレームかどうかを判定する
		if (EffectUtillity::IsBlinkVisible(GetRemaining()))
			owner().DrawBody();
	}
	void DeadState::OnEnter(State* prevState)
	{
		owner().GetSoundManager()->PlaySe(SoundId::BossExplosion);
	}

	void DeadState::OnTimeUp()
	{
		owner().GetSoundManager()->PlaySe(SoundId::BgmGameClear);
		owner().GetStatus()->OnDeathFinished.Invoke();
	}
	void DeadState::OnDraw()
	{
		//雑魚より大きく見せるため、位置と時間をずらしてエフェクトを重ねる
		const float elapsed = GetElapsed();
		const Vector2& position = owner().GetOwner()->GetTransform().Position;
		EffectUtillity::DrawEffect(elapsed, _effectTime, 0xFF00FF, position);
		EffectUtillity::DrawEffect(elapsed - 0.3f, _effectTime, 0xFFAA00, position + Vector2(-30.0f, 20.0f));
		EffectUtillity::DrawEffect(elapsed - 0.6f, _effectTime, 0xFFFFFF, position + Vector2(30.0f, -20.0f));
		EffectUtillity::DrawEffect(elapsed - 0.9f, _effectTime, 0xAAFFFF, position + Vector2(30.0f, -25.0f));
		EffectUtillity::DrawEffect(elapsed - 1.2f, _effectTime, 0x00FFFF, position + Vector2(35.0f, -35.0f));
		EffectUtillity::DrawEffect(elapsed - 1.7f, _effectTime, 0xAAFF00, position + Vector2(25.0f, -30.0f));
	}

	void SwirlShotState::OnEnter(State* prevState)
	{
		ShotState::OnEnter(prevState);
		_angle = 0.0f;
	}
	void SwirlShotState::Shoot(EnemyAttack& attack)
	{
		attack.SwirlShot(_angle);
		_angle += _rotateStep;
		if (_angle >= 360.0f) _angle -= 360.0f;
	}
}
