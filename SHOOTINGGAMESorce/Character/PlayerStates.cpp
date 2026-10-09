#include "PlayerStates.h"
#include "PlayerController.h"

#include "SoundManager.h"
#include "PlayerAttack.h"
#include "PlayerMovement.h"
#include "PlayerStatus.h"
#include "Collider.h"

#include "EffectUtillity.h"
#include "GameObject.h"

namespace
{
	//移動速度（ピクセル/秒）。通常時と無敵中で共通
	constexpr float MoveSpeed = 350.0f;
}

namespace PlayerStates
{
	void AliveState::OnUpdate(float deltaTime)
	{
		owner().MoveByInput(deltaTime, MoveSpeed);
	}
	void AliveState::OnDraw()
	{
		owner().DrawBody();
	}

	void InvincibleState::OnEnter(State* prevState)
	{
		TimedState::OnEnter(prevState);
		owner().GetCollider()->SetCollisionEnabled(false);//無敵中は当たり判定をなくす
	}
	void InvincibleState::OnUpdate(float deltaTime)
	{
		owner().MoveByInput(deltaTime, MoveSpeed);//無敵中も動ける
		TimedState::OnUpdate(deltaTime);
	}
	void InvincibleState::OnExit(State* nextState)
	{
		owner().GetCollider()->SetCollisionEnabled(true);
	}
	void InvincibleState::OnTimeUp()
	{
		stateMachine().Dispatch(LifeEvent::EventInvincibleEnd);
	}
	void InvincibleState::OnDraw()
	{
		//残り時間から、点滅で表示するフレームかどうかを判定する
		if (EffectUtillity::IsBlinkVisible(GetRemaining()))
			owner().DrawBody();
	}

	void DeadState::OnEnter(State* prevState)
	{
		TimedState::OnEnter(prevState);
		owner().GetSoundManager()->PlaySe(SoundId::CharacterExplosion);
		owner().GetCollider()->SetCollisionEnabled(false);
	}
	void DeadState::OnTimeUp()
	{
		owner().GetStatus()->OnDeathFinished.Invoke();
	}
	void DeadState::OnDraw()
	{
		//死亡エフェクトを描写
		EffectUtillity::DrawEffect(GetElapsed(), _effectTime, 0xFF0000, owner().GetOwner()->GetTransform().Position);
	}

	//攻撃状態のステート
	void PlayerShotState::OnEnter(State* prevState)
	{
		auto* attack = owner().GetAttack();
		attack->SetCurrentAttackIndex(_attackIndex);
		attack->InitializeShotSetting(attack->GetAttackType());

		//入った直後は撃てる状態にし、切り替えには待ち時間を付ける
		SetGunState(_shotCoolTime, false, false);
	}
	void PlayerShotState::OnUpdate(float deltaTime)
	{
		const auto& controller = owner();
		const auto& attack = controller.GetAttack();

		//クールダウン処理
		owner().GetAttack()->Cool(deltaTime);
		//武器の切り替え
		if (controller.GetCurrentInput().isChangePressed && _isChangeFlag)
		{
			stateMachine().Dispatch(_switchEvent);
			return;
		}

		if (controller.GetCurrentInput().isShotHeld && !_isShotFlag && attack->CanFire())
		{
			SetShotInfo(true, _shotCoolTime);
			Shoot(*attack);
			attack->AddHeat(_heatPerShot);
		}
		ElapsedTimer(deltaTime);
	}

	void LongRangeShot::Shoot(PlayerAttack& attack)
	{
		owner().GetSoundManager()->PlaySe(SoundId::PlayerShot);
		attack.LongShot();
	}
	void SpreadShotState::Shoot(PlayerAttack& attack)
	{
		owner().GetSoundManager()->PlaySe(SoundId::PlayerShotGun);
		attack.SpreadShot();
	}
}
