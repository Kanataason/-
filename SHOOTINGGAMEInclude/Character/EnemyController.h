#pragma once
#include "Component.h"
#include "StateMachine.h"

class Primitive;
class EnemyStatus;
class EnemyMovement;
class EnemyAttack;
class Collider;
class GameProgress;
class DrawManager;
class SoundManager;

struct ScreenBounds;

//敵の司令塔
//各コンポーネントを束ね、移動用と攻撃用の2つのステートマシンを動かす
class EnemyController : public Component
{
public:
	std::unique_ptr<Component> Clone()const override
	{
		return std::make_unique<EnemyController>(*this);
	}
	EnemyController() :_normalState(*this),_attackState(*this) {}
	EnemyController(const EnemyController& source)
		: Component(source), _normalState(*this), _attackState(*this)
	{
	}

	void Initialize(ServiceLocator& locator)override;
	void Update(float deltaTime)override;
	void Draw()override;

	//ステートから各コンポーネントを使うためのアクセサ
	EnemyMovement* GetMovement()const { return _movement; }
	EnemyAttack* GetAttack()const { return _attack; }
	EnemyStatus* GetStatus()const { return _status; }
	SoundManager* GetSoundManager()const { return _soundManager; }
	//移動先を決めるための画面サイズ
	//ステートは他のコンポーネントの初期化前に開始されるので、ここで持つ
	const ScreenBounds& GetBounds()const { return *_bounds; }

	//Moveを禁止
	EnemyController(EnemyController&&) = delete;
	EnemyController& operator=(EnemyController&&) = delete;
	EnemyController& operator=(const EnemyController&) = delete;
private:
	void InitializeStateMachine();
	//敵の種類ごとの形で本体を描く
	void DrawBody() const;
private:
	bool _isDead = false;
	DrawManager* _drawManager = nullptr;

	Primitive* _sizeStatus = nullptr;
	EnemyStatus* _status = nullptr;
	EnemyMovement* _movement = nullptr;
	EnemyAttack* _attack = nullptr;
	Collider* _collider = nullptr;
	GameProgress* _progress = nullptr;
	SoundManager* _soundManager = nullptr;

	const ScreenBounds* _bounds = nullptr;

	StateMachine<EnemyController> _normalState;
	StateMachine<EnemyController> _attackState;
};