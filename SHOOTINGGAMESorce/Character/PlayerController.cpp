#include <typeinfo>
#include "PlayerController.h"
#include "GameObject.h"
#include "SceneManager.h"

#include "PlayerMovement.h"
#include "PlayerAttack.h"
#include "PlayerStatus.h"
#include "Collider.h"
#include "DrawManager.h"

#include "PlayerStates.h"

using namespace PlayerStates;
using namespace InputKey;

void PlayerController::Initialize(ServiceLocator& locator)
{
	_movement = GetOwner()->GetComponent<PlayerMovement>();
	_attack = GetOwner()->GetComponent<PlayerAttack>();
	_status = GetOwner()->GetComponent<PlayerStatus>();
	_sizeStatus = GetOwner()->GetComponent<CircleShape>();
	_collider = GetOwner()->GetComponent<Collider>();

	SubScribeEvents();

	InitializeStateMachine();

	_soundManager = locator.soundManager;
	_input = locator.input;
	_sceneManager = locator.sceneManager;
	_drawManager = locator.drawManager;
}
void PlayerController::SubScribeEvents()
{
	_status->OnHealthChanged.Subscribe([this](float hp, float)
		{
			if (hp > 0.0f) _normalState.Dispatch(LifeEvent::EventDamaged);
		});
	_status->OnHitSound.Subscribe([this](SoundId id) {PlaySe(id);});
}

void PlayerController::InitializeStateMachine()
{


	//各ステートマシーンの遷移先を登録
	_normalState.AddTransition<AliveState, InvincibleState>(LifeEvent::EventDamaged);
	_normalState.AddTransition<InvincibleState, AliveState>(LifeEvent::EventInvincibleEnd);


	//武器は切り替えボタンで順番に回る
    //武器を増やしたら、輪が途切れないように前後の遷移とswitchEventをつなぎ直す
    //登録が無いイベントはDispatchしても何も起きないので、切り替わらないときはここを見る
	_attackState.AddTransition<LongRangeShot, SpreadShotState>(AttackEvent::EventSpreadShot);
	_attackState.AddTransition<SpreadShotState, LongRangeShot>(AttackEvent::EventLongShot);

	//すべてのステートから遷移できるステートを登録
	_normalState.AnyAddTransition<DeadState>(LifeEvent::EventDied);
	_attackState.AnyAddTransition<AttackDisabledState>(AttackEvent::EventAttackStop);

	//最初のステートを設定
	_attackState.Start<LongRangeShot>();
	_normalState.Start<AliveState>();
}

void PlayerController::Update(float deltaTime)
{
	//死んだらすべてのステートを死亡用に変える
	if (!_status->GetIsActive() && !_isDead)
	{
		_isDead = true;
		_normalState.Dispatch(LifeEvent::EventDied);   
		_attackState.Dispatch(AttackEvent::EventAttackStop); 
	}

	_currentInput = ReadInput();
	_normalState.Update(deltaTime);
	_attackState.Update(deltaTime);
}
void PlayerController::Draw()
{
	//本体・点滅・撃破エフェクトの描き分けはステートが行う
	_drawManager->Register(DrawLayer::Player, [this] { _normalState.Draw(); });
}
void PlayerController::DrawBody() const
{
	if (!_status->GetIsActive())return;

	const auto& pos = GetOwner()->GetTransform().Position;
	//座標が画像の中心になる。画像は表示する大きさに加工済みなので、等倍で描く
	DrawRotaGraph((int)pos.x, (int)pos.y, 1.0, 0.0, _status->GetImagePath(), TRUE);
}
void PlayerController::MoveByInput(float deltaTime, float moveSpeed)
{
	_movement->Move(_currentInput.direction, moveSpeed * deltaTime);
	_movement->MovementRestrictions();
}
void PlayerController::PlaySe(SoundId id)
{
	_soundManager->PlaySe(id);
}

PlayerInput PlayerController::ReadInput()const
{
	PlayerInput input;

	if (_input->GetKey(rightKey)) input.direction.x += 1.0f;
	if (_input->GetKey(leftKey))  input.direction.x -= 1.0f;
	if (_input->GetKey(upKey))    input.direction.y -= 1.0f;   
	if (_input->GetKey(downKey))  input.direction.y += 1.0f;

	input.isShotHeld = _input->GetKey(shotKey);
	input.isChangePressed = _input->GetKeyDown(changeKey);


	return input;
}

