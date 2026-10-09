#include "EnemyController.h"
#include "EnemyStatus.h"
#include "EnemyStates.h"
#include "EnemyMovement.h" 
#include "EnemyAttack.h"
#include "GameProgress.h"

#include "GameObject.h"
#include "GameWindow.h"
#include "Collider.h"
#include "DrawManager.h"

using namespace EnemyStates;

void EnemyController::Initialize(ServiceLocator& locator)
{
	_status = GetOwner()->GetComponent<EnemyStatus>();
	_sizeStatus = GetOwner()->GetComponent<Primitive>();
	_movement = GetOwner()->GetComponent<EnemyMovement>();
	_attack = GetOwner()->GetComponent<EnemyAttack>();
	_collider = GetOwner()->GetComponent<Collider>();

	_soundManager = locator.soundManager;
	_progress = locator.progress;
	_bounds = &locator.gameWindow->GetBounds();
	_drawManager = locator.drawManager;

	//ステートのOnEnterがここで呼ばれるため、ステートが使うものは先に設定しておく
	InitializeStateMachine();
}
void EnemyController::InitializeStateMachine()
{
	//遷移先を設定
	_normalState.AddTransition<EventState, AliveState>(LifeEvent::EventMove);
	_normalState.AddTransition<EventState, MoveOnceState>(LifeEvent::EventMoveOnce);
	_normalState.AddTransition<AliveState, InvincibleState>(LifeEvent::EventSecondform);
	_normalState.AddTransition<InvincibleState, AliveState>(LifeEvent::EventInvincibleEnd);
	_normalState.AnyAddTransition<DeadState>(LifeEvent::EventDied);

	_attackState.AddTransition<LongRangeShotState, SpreadShotState>(AttackEvent::EventSpreadShot);
	_attackState.AddTransition<LongRangeShotState, ReflectShotState>(AttackEvent::EventReflectShot);
	_attackState.AnyAddTransition<LongRangeShotState>(AttackEvent::EventLongShot);
	_attackState.AnyAddTransition<AttackDisabledState>(AttackEvent::EventAttackStop);

	//JSONのAttackTypeから最初の攻撃ステートを決める
	switch (_attack->GetAttackType())
	{
	case 2001: _attackState.Start<LongRangeShotState>();break;
	case 2002: _attackState.Start<ReflectShotState>();break;
	case 2003: _attackState.Start<SpreadShotState>();break;
	}
	_normalState.Start<EventState>();
}

void EnemyController::Update(float deltaTime)
{
	//HPが0になったフレームで1回だけ死亡用のステートに切り替える
	if (!_status->GetIsActive() && !_isDead)
	{
		_isDead = true;
		_collider->SetCollisionEnabled(false);//撃破演出中は当たり判定を無効にする
		if (_progress) _progress->AddKill();//死んだ数に記録する
		_normalState.Dispatch(LifeEvent::EventDied);
		_attackState.Dispatch(AttackEvent::EventAttackStop);
	}

	//ステートマシーンのUpdate更新
	_normalState.Update(deltaTime);
	_attackState.Update(deltaTime);

	//ステートマシーンで行った移動処理を反映させる
	_sizeStatus->SetCenter(GetOwner()->GetTransform().Position);
}

void EnemyController::Draw()
{
	//本体と撃破エフェクトは、別のレイヤーに登録する
	_drawManager->Register(DrawLayer::Enemy, [this] { DrawBody(); });
	_drawManager->Register(DrawLayer::Effect, [this] { _normalState.Draw(); });
}

void EnemyController::DrawBody() const
{
	if (_status->GetIsActive())
	{
		const auto& pos = GetOwner()->GetTransform().Position;
		//座標が画像の中心になる。画像は表示する大きさと向きに加工済みなので、等倍・回転なしで描く
		DrawRotaGraph((int)pos.x, (int)pos.y, 1.0, 0.0, _status->GetImagePath(), TRUE);
	}
}