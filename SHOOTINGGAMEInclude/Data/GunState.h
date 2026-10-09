#pragma once
#include <algorithm>

class GunState
{
public:

	void SetShotInfo(bool isShot, float coolDown) { _isShotFlag = isShot;_coolDown = coolDown; }
	//初期値設定関数
	void SetGunState(float coolDown, bool shotFlag = false, bool changeFlag = false)
	{
		_maxCoolDown = coolDown;

		_coolDown = coolDown;
		_changeCoolDown = _maxChangeTime;
		_isChangeFlag = changeFlag;
		_isShotFlag = shotFlag;
	}

	//動いてないときでもカウントは計算されるから範囲内に収めるための処理
	void Clamp()
	{
		_changeCoolDown = std::clamp(_changeCoolDown, 0.0f, _maxChangeTime);
		_coolDown = std::clamp(_coolDown, 0.0f, _maxCoolDown);
	}
	//カウントダウン用の関数
	void ElapsedTimer(float deltaTime) {

		if (_coolDown <= 0.0f)
			SetShotInfo(false, 0.0f);

		if (_changeCoolDown <= 0.0f)
			_isChangeFlag = true;

		_changeCoolDown -= deltaTime;
		_coolDown -= deltaTime;

		Clamp();

	};
protected:
	float _maxChangeTime = 1.0f;
	float _maxCoolDown = 0.2f;

	bool _isChangeFlag = false;
	bool _isShotFlag = false;
	float _coolDown = 0.0f;

	//武器切り替えクールタイム
	float _changeCoolDown = 0.0f;
};