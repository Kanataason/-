#pragma once
#include "ServiceLocator.h"
#include "Event.h"

//1発ごとの流れ
enum class RoundState
{
	AimAngle,
	AimPower,
	Flying,
	Result,
	Finished
};

class RoundManager
{
public:
	//発射した瞬間(カメラが追いかけ始めるのに使う)
	Event<> OnShotFired;
	//ラグドールが止まった瞬間(UIのアニメーションを始めるのに使う)
	Event<> OnShotSettled;
	//結果を見せ終わって、次の1発に進めるとき(ラグドールを大砲に戻すのに使う)
	Event<> OnReadyNextShot;
	//玉を全部撃ち終えて、止まったあと
	Event<> OnFinished;

	void Initialize(ServiceLocator& locator) {};
	void Update(float deltaTime);

	//角度を決める。角度を狙っているときだけ受け付けて、威力のゲージに進む。進めたらtrue
	bool ConfirmAngle();

	//威力を決めて撃つ。威力のゲージを動かしているときだけ撃てる。撃てたらtrue
	bool TryUseShot();

	//威力ゲージの今の量(0～1)。ゲージを動かす側(GameScene)が毎フレーム書き、大砲が撃つときに読む
	void SetPowerAmount(float amount) { _powerAmount = amount; }
	float GetPowerAmount()const { return _powerAmount; }

	//ラグドールが止まったことを伝える(飛んでいるときだけ受け付ける)
	void NotifySettled();

	//すべての的を壊したら呼ばれる
	void NotifyAllCleared();

	void SetShotCount(int shotCount) { _shotCount = shotCount; }

	int GetShotCount()const { return _shotCount; }
	RoundState GetState()const { return _state; }
private:
	void UpdateResultTime(float deltaTime);
	void UpdateClearTime(float deltaTime);

	//終わりにして、結果を出す(OnFinished を1回だけ呼ぶ)
	void Finish();
private:
	//止まってから、次に進むまでの間(秒)
	const float _resultSeconds = 1.5f;
	float _clearTimer = 0.0f;
	bool _isCleared = false; //的を全部飛ばした(終わるまで待っている)

	int _shotCount = 3;
	RoundState _state = RoundState::AimAngle;
	float _resultTimer = 0.0f;
	float _powerAmount = 0.0f;
};
