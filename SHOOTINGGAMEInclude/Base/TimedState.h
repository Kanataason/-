#pragma once
#include "StateMachine.h"

//一定時間たったらOnTimeUpを1回だけ呼ぶステートの基底クラス
//無敵時間や、撃破演出の待ち時間などに使う
//一定時間だけ何かして次へ移る行動は、これを継承してOnTimeUpでDispatchする
template<class TOwner>
class TimedState : public StateMachine<TOwner>::State
{
protected:
	using StateBase = typename StateMachine<TOwner>::State;

	explicit TimedState(float duration) : _duration(duration) {}

	//継承先でOnEnter・OnUpdateをoverrideするときは、この処理も呼ぶこと
	void OnEnter(StateBase* previousState) override
	{
		_elapsed = 0.0f;
		_isTimeUp = false;
	}
	void OnUpdate(float deltaTime) override
	{
		_elapsed += deltaTime;
		if (!_isTimeUp && _elapsed >= _duration)
		{
			_isTimeUp = true;
			OnTimeUp();
		}
	}

	//時間になったときの処理
	virtual void OnTimeUp() {}

	float GetElapsed() const { return _elapsed; }
	float GetRemaining() const { return _duration - _elapsed; }
private:
	float _duration;
	float _elapsed = 0.0f;
	bool _isTimeUp = false;
};
