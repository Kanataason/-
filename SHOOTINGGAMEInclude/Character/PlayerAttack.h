#pragma once
#undef min
#undef max

#include <vector>
#include <unordered_map>

#include "Component.h"
#include "Vector2.h"
#include "GunData.h"
#include "Event.h"

class BulletManager;
class PlayerStatus;

//プレイヤーの弾の撃ち方を担当する
//いつ撃つかはPlayerStatesが決め、ここは「どう撃つか」だけを持つ
class PlayerAttack : public Component
{
public :
	Event<float, float,bool> OnHeatChanged;//ヒート値が変わったときにUIへ送るイベント

	std::unique_ptr<Component> Clone()const override
	{
		return std::make_unique<PlayerAttack>(*this);
	}
	void Initialize(ServiceLocator& locator)override;
	void Update(float deltaTime)override {};

	void InitializeShotSetting(int type);

	//真上に1発撃つ
	void LongShot();
	//真上から左右20度の範囲に、ランダムな向きで複数撃つ
	void SpreadShot();

	//Jsonで宣言した配列のindexを設定する
	void SetCurrentAttackIndex(size_t index)
	{
		if (index >= _attackTypes.size())
		{
			return; //範囲外
		}
		_currentAttackIndex = index;
	}
	void SetAttackTypes(std::vector<int> type) { _attackTypes = std::move(type); }
	//JSONにAttackTypeが無い場合でも落ちないように0を返す
	int GetAttackType()const
	{
		return (_currentAttackIndex < _attackTypes.size()) ? _attackTypes[_currentAttackIndex] : 0;
	}

	//ヒート値に関する処理
	bool CanFire()const { return !_isOverheated; }
	void AddHeat(float amount)
	{
		_heat = std::clamp(_heat + amount, 0.0f, _maxHeat);
		if (_heat >= _maxHeat) _isOverheated = true;
		OnHeatChanged.Invoke(_heat, _maxHeat,_isOverheated);
	};
	void Cool(float deltaTime)
	{
		if (_heat <= 0.0f) return;   //冷え切っていれば通知もしない
		_heat = std::max(_heat - _coolRate * deltaTime, 0.0f);
		if (_heat <= 0.0f) _isOverheated = false;   //0まで冷えたら撃てるようにする
		OnHeatChanged.Invoke(_heat, _maxHeat,_isOverheated);
	};
private:
	//メンバ変数の角度内でランダムに撃つ
	Vector2 GetRandomDirection()const;
private:
	float _heat = 0.0f;
	float _maxHeat = 100.0f;//最大ヒート
	float _coolRate = 30.0f;//1秒あたりに冷える量
	bool _isOverheated = false;

	BulletManager* _bulletManager = nullptr;

	ShotSetting _currentShotData{};

	std::vector<int> _attackTypes;
	size_t _currentAttackIndex = 0;

	int _bulletColor = 0xFFFF00;
	float _minDegrees = 70.0f;//真上から左右に20度ずつ
	float _maxDegrees = 110.0f;
};