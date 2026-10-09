#pragma once
#include <vector>
#include "Component.h"
#include "StateMachine.h"

class EnemyMovement;
class EnemyAttack;
class EnemyStatus;
class Primitive;
class Collider;
class DrawManager;
class SoundManager;
struct ScreenBounds;


//ボスの司令塔
//移動用と攻撃用の2つのステートマシンを動かし、攻撃の種類を一定時間ごとに切り替える
class BossController : public Component
{
public:
	BossController() :_normalState(*this), _attackState(*this) {}
	BossController(const BossController& source)
		: Component(source), _normalState(*this), _attackState(*this)
	{
	}
	std::unique_ptr<Component> Clone()const override
	{
		return std::make_unique<BossController>(*this);
	}

	void Initialize(ServiceLocator& locator)override;
	void Update(float deltaTime)override;
	void Draw()override;

	//ボス本体を描く。描くかどうかはステートが決める
	void DrawBody() const;
	//登場が終わったら攻撃の切り替えを始める
	void StartAttack();

	//ステートから各コンポーネントを使うためのアクセサ
	EnemyMovement* GetMovement()const { return _movement; }
	EnemyAttack* GetAttack()const { return _attack; }
	EnemyStatus* GetStatus()const { return _status; }
	Collider* GetCollider()const { return _collider; }
	SoundManager* GetSoundManager()const { return _soundManager; }
	const ScreenBounds& GetBounds()const { return *_bounds; }

	//Moveを禁止
	BossController(BossController&&) = delete;
	BossController& operator=(BossController&&) = delete;
	BossController& operator=(const BossController&) = delete;
private:
	//HPが4割を切った最初の1回だけ、無敵を挟んで第二形態へ移る。HPが0のときは死亡処理に任せる
	void OnHealthChanged(float hp, float maxHp);
	void SubScribeEvents();
	void InitializeStateMachine();
	//一定時間ごとに、今とは違う攻撃へランダムに切り替える
	void UpdateAttackRotation(float deltaTime);
private:
	bool _isSecondPhase = false;
	bool _isDead = false;
	bool _isAttacking = false;

	//攻撃の順番と、1種類あたりの時間
	std::vector<int> _attackOrder;
	size_t _attackOrderIndex = 0;
	float _attackTimer = 0.0f;
	float _attackDuration = 5.0f;

	EnemyMovement* _movement = nullptr;
	EnemyAttack* _attack = nullptr;
	EnemyStatus* _status = nullptr;
	Primitive* _shape = nullptr;
	Collider* _collider = nullptr;
	const ScreenBounds* _bounds = nullptr;
	DrawManager* _drawManager = nullptr;
	SoundManager* _soundManager = nullptr;

	StateMachine<BossController> _normalState;
	StateMachine<BossController> _attackState;
};
