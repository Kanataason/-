#include <DxLib.h>

#include "EnemyStates.h"
#include "EnemyStatus.h"
#include "EnemyController.h"
#include "EnemyMovement.h"
#include "EnemyAttack.h"
#include "SoundManager.h"
#include "GameWindow.h"

#include "GameObject.h"

namespace
{
	//min〜maxの範囲でランダムな値を返す
	float RandomRange(float min, float max)
	{
		return min + static_cast<float>(GetRand(static_cast<int>(max - min)));
	}
}

namespace EnemyStates
{
	void EventState::OnEnter(State* prevState)
	{
		auto& transform = owner().GetOwner()->GetTransform();
		transform.Position.y = -100;//画面外

		constexpr float moveDistance = 100.0f;
		//画面の下からmoveDistanceだけ外に出た位置を、通り抜けるタイプの目的地にする
		if (owner().GetStatus()->GetMovePattern() == MovePattern::PassThrough)
		{
			//画面の下まで、4〜6秒かけて通り抜ける
			float bottomY = owner().GetBounds().height + moveDistance;
			float duration = 4.0f + GetRand(200) / moveDistance;
			owner().GetMovement()->SetDestination({ transform.Position.x, bottomY }, duration);
			return;
		}
		Vector2 destination = { transform.Position.x, _entryTargetY };
		owner().GetMovement()->SetDestination(destination, 4.0f);
	}
	void EventState::OnUpdate(float deltaTime)
	{
		if (owner().GetMovement()->Translate(deltaTime))
		{
			//通り抜けるタイプは、下に抜けたら消す
			if (owner().GetStatus()->GetMovePattern() == MovePattern::PassThrough)
				owner().GetOwner()->Destroy();
			else
				DispatchByState();
		}
	}
	void EventState::OnExit(State* nextState)
	{
		bool isMove = owner().GetStatus()->GetMoveFlag();
		owner().GetMovement()->SetIsMoveFlag(isMove);
	}
	void EventState::DispatchByState()
	{
		switch (owner().GetStatus()->GetCharacterType())
		{
		case CharacterType::ReflectShotEnemy:
		case CharacterType::MoveEnemy:
			stateMachine().Dispatch(LifeEvent::EventMove);
			break;
		case CharacterType::SpreadEnemy:
			stateMachine().Dispatch(LifeEvent::EventMoveOnce);
			break;
		}
	}

	void MoveOnceState::OnEnter(State* prevState)
	{
		RandomTargetPosition();
	}
	void MoveOnceState::OnUpdate(float deltaTime)
	{
		owner().GetMovement()->Translate(deltaTime);
	}
	void MoveOnceState::RandomTargetPosition()
	{
		//画面の上側に移動する
		const auto& bounds = owner().GetBounds();
		float randomX = RandomRange(ScreenMargin, bounds.width - ScreenMargin);
		float randomY = RandomRange(ScreenMargin, bounds.height - ScreenMargin - 30.0f);

		owner().GetMovement()->SetDestination({ randomX, randomY }, _moveDuration);
	}

	void DeadState::OnEnter(State* prevState)
	{
		owner().GetSoundManager()->PlaySe(SoundId::CharacterExplosion);
	}

	void DeadState::OnTimeUp()
	{
		owner().GetOwner()->Destroy();
	}
	void DeadState::OnDraw()
	{
		//死亡エフェクトを描画
		EffectUtillity::DrawEffect(GetElapsed(), _effectTime, 0xCC0000, owner().GetOwner()->GetTransform().Position);
	}
}
