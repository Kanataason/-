#include <DxLib.h>

#include "BossController.h"
#include "BossStates.h"
#include "EnemyMovement.h"
#include "EnemyAttack.h"
#include "EnemyStatus.h"
#include "Collider.h"
#include "GameWindow.h"
#include "GameObject.h"
#include "DrawManager.h"

using namespace BossStates;

void BossController::Initialize(ServiceLocator& locator)
{
	_movement = GetOwner()->GetComponent<EnemyMovement>();
	_attack = GetOwner()->GetComponent<EnemyAttack>();
	_status = GetOwner()->GetComponent<EnemyStatus>();
	_shape = GetOwner()->GetComponent<Primitive>();
	_collider = GetOwner()->GetComponent<Collider>();
	_bounds = &locator.gameWindow->GetBounds();
	_drawManager = locator.drawManager;
	_soundManager = locator.soundManager;

	//第一形態の攻撃の順番
	_attackOrder = { EventLongShot, EventReflectShot, EventSpreadShot };

	SubScribeEvents();

	//ステートのOnEnterがここで呼ばれるため、ステートが使うものは先に設定しておく
	InitializeStateMachine();
}
void BossController::SubScribeEvents()
{
	_status->OnHealthChanged.Subscribe([this](float hp, float maxHp) { OnHealthChanged(hp, maxHp); });

}

void BossController::InitializeStateMachine()
{
	_normalState.AddTransition<EnterState, AliveState>(LifeEvent::EventEntered);
	_normalState.AddTransition<AliveState, InvincibleState>(LifeEvent::EventInvincible);
	_normalState.AddTransition<InvincibleState,SecondForm>(LifeEvent::EventSecondForm);
	_normalState.AnyAddTransition<DeadState>(LifeEvent::EventDied);

	//攻撃の順番を自由に入れ替えられるよう、どのステートからでも遷移できるようにする
	_attackState.AnyAddTransition<LongRangeShotState>(AttackEvent::EventLongShot);
	_attackState.AnyAddTransition<ReflectShotState>(AttackEvent::EventReflectShot);
	_attackState.AnyAddTransition<SpreadShotState>(AttackEvent::EventSpreadShot);
	_attackState.AnyAddTransition<AttackDisabledState>(AttackEvent::EventAttackStop);
	_attackState.AnyAddTransition<SwirlShotState>(AttackEvent::EventSwirlShot);

	_attackState.Start<AttackDisabledState>();
	_normalState.Start<EnterState>();
}

void BossController::StartAttack()
{
	_isAttacking = true;
	_attackOrderIndex = 0;
	_attackTimer = 0.0f;
	_attackState.Dispatch(_attackOrder[_attackOrderIndex]);
}

void BossController::Update(float deltaTime)
{
	//HPが0になったフレームで1回だけ死亡用のステートに切り替える
	if (!_status->GetIsActive() && !_isDead)
	{
		_isDead = true;
		_isAttacking = false;
		_collider->SetCollisionEnabled(false);
		_normalState.Dispatch(LifeEvent::EventDied);
		_attackState.Dispatch(AttackEvent::EventAttackStop);
	}

	UpdateAttackRotation(deltaTime);
	_normalState.Update(deltaTime);
	_attackState.Update(deltaTime);

	//ステートマシーンで行った移動処理を反映させる
	_shape->SetCenter(GetOwner()->GetTransform().Position);
}

void BossController::UpdateAttackRotation(float deltaTime)
{
	if (!_isAttacking || _attackOrder.empty()) return;

	_attackTimer += deltaTime;
	if (_attackTimer < _attackDuration) return;

	_attackTimer = 0.0f;
	if (_attackOrder.size() > 1)
	{
		int step = 1 + GetRand(static_cast<int>(_attackOrder.size()) - 2);
		_attackOrderIndex = (_attackOrderIndex + step) % _attackOrder.size();
	}
	_attackState.Dispatch(_attackOrder[_attackOrderIndex]);
}


void BossController::Draw()
{
	//本体・点滅・撃破エフェクトの描き分けはステートが行う
	_drawManager->Register(DrawLayer::Enemy, [this] { _normalState.Draw(); });
}

void BossController::DrawBody() const
{
	//座標が画像の中心になる。画像は表示する大きさと向きに加工済みなので、等倍・回転なしで描く
	const auto& pos = GetOwner()->GetTransform().Position;
	DrawRotaGraph((int)pos.x, (int)pos.y, 1.0, 0.0, _status->GetImagePath(), TRUE);
}


void BossController::OnHealthChanged(float hp, float maxHp)
{
	if (_isSecondPhase || hp <= 0.0f) return;
	if (hp > maxHp * 0.4f) return;

	_isSecondPhase = true;
	_attackOrder.push_back(EventSwirlShot);
	_normalState.Dispatch(LifeEvent::EventInvincible);   //形態移行ステートへ
}