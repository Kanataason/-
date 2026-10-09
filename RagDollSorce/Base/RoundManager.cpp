#include "RoundManager.h"


void RoundManager::Update(float deltaTime)
{
	UpdateClearTime(deltaTime);

	if (_state != RoundState::Result)
		return;

	UpdateResultTime(deltaTime);

	//玉が残っていても、クリア済みなら、次の1発には進まない
	if (_shotCount > 0 && !_isCleared)
	{
		_state = RoundState::AimAngle;
		OnReadyNextShot.Invoke();
	}
	else
	{
		Finish();
	}
}
void RoundManager::UpdateClearTime(float deltaTime)
{
	//的を全部飛ばしたら、少し待って、玉が残っていても終わる(最後の演出を見せる時間)
    //飛んでいる最中でも、Result中でも動くように、状態の判定より先に見る
	if (_isCleared && _state != RoundState::Finished)
	{
		_clearTimer -= deltaTime;
		if (_clearTimer <= 0.0f)
		{
			Finish();
			return;
		}
	}
}

void RoundManager::UpdateResultTime(float deltaTime)
{
	_resultTimer += deltaTime;
	if (_resultTimer < _resultSeconds)
		return;
}

void RoundManager::Finish()
{
	_state = RoundState::Finished;
	_isCleared = false;
	OnFinished.Invoke(); //ResultPanel が出る
}

bool RoundManager::ConfirmAngle()
{
	if (_state != RoundState::AimAngle || _shotCount <= 0)
		return false;

	_state = RoundState::AimPower;
	return true;
}

bool RoundManager::TryUseShot()
{
	//威力のゲージを動かしている間だけ撃てる(飛んでいる最中や、結果を見せている間は撃てない)
	if (_state != RoundState::AimPower || _shotCount <= 0)
		return false;

	_state = RoundState::Flying;
	OnShotFired.Invoke();
	return true;
}

void RoundManager::NotifySettled()
{
	if (_state != RoundState::Flying)
		return;

	_state = RoundState::Result;

	--_shotCount;
	_resultTimer = 0.0f;
	OnShotSettled.Invoke();
}

void RoundManager::NotifyAllCleared()
{
	if (_state == RoundState::Finished) return;
	_isCleared = true;
	_clearTimer =_resultSeconds;
}