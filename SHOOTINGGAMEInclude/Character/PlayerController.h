#pragma once
#include <vector>

#include "Component.h"
#include "StateMachine.h"
#include "InputManager.h"
#include "SoundManager.h"

class PlayerMovement;
class PlayerAttack;
class PlayerStatus;
class CircleShape;
class Collider;
class DrawManager;

namespace InputKey
{
	constexpr int upKey = 'W';
	constexpr int downKey = 'S';
	constexpr int leftKey = 'A';
	constexpr int rightKey = 'D';
	constexpr int shotKey = VK_LBUTTON;
	constexpr int changeKey = VK_RBUTTON;
}


class PlayerController : public Component
{
public:
	std::unique_ptr<Component> Clone()const override
	{
		return std::make_unique<PlayerController>(*this);
	}
	PlayerController() :_normalState(*this), _attackState(*this) {}

	PlayerController(const PlayerController& source)
		: Component(source),_normalState(*this),_attackState(*this)
	{
	}

	void Initialize(ServiceLocator& locator)override;
	void Update(float deltaTime)override;
	void Draw()override;
	void DrawBody() const;

	//一フレームの入力を構造体に入れてStateMachineから値を取得できる状態にする
	PlayerInput ReadInput()const;

	const PlayerInput& GetCurrentInput()const { return _currentInput; }
    PlayerMovement* GetMovement()const { return _movement; }
	PlayerAttack* GetAttack()const { return _attack; }
	PlayerStatus* GetStatus()const { return _status; }
	Collider* GetCollider() const { return _collider; }
	SoundManager* GetSoundManager()const { return _soundManager; }

	void PlaySe(SoundId id);
	void MoveByInput(float deltaTime,float moveSpeed);

	//Moveを禁止
	PlayerController(PlayerController&&) = delete;
	PlayerController& operator=(PlayerController&&) = delete;
	PlayerController& operator=(const PlayerController&) = delete;
	
private:
	void SubScribeEvents();
	void InitializeStateMachine();
private:
	bool _isDead = false;

	PlayerInput _currentInput{};

	InputManager* _input = nullptr;
	PlayerMovement* _movement = nullptr;
	PlayerAttack* _attack = nullptr;
	PlayerStatus* _status = nullptr;
	CircleShape* _sizeStatus = nullptr;
	Collider* _collider = nullptr;
	
	SoundManager* _soundManager = nullptr;
	SceneManager* _sceneManager = nullptr;
	DrawManager* _drawManager = nullptr;

	StateMachine<PlayerController> _normalState;
	StateMachine<PlayerController> _attackState;
};