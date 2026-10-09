#pragma once
#include "ServiceLocator.h"
#include "VectorUtility.h"
#include "Event.h"

class ObjectStatus;
struct HitInfo;

//的に当たって、点が入ったときの情報(「+500」のようなポップアップを出すのに使う)
struct ScoreGain
{
	float points = 0.0f; //今回入った点(コンボ倍率・頭ボーナスを含む)
	int combo = 0; //今のコンボ数
	bool isHead = false; //頭に当たったか
	Vector3 position{}; //当たった場所(世界座標)
};

//スコアの採点
class ScoreManager
{
public:
	//すべての的を,壊した瞬間
	Event<>OnAllCleared;

	//的に当たって、点が入った瞬間
	Event<ScoreGain> OnScoreGained;

	//クリアボーナスが入った瞬間(引数は、入った点)。画面に大きく出すのに使う
	Event<float> OnBonusGained;

	void Initialize(ServiceLocator& locator) {};

	void SubscribeEvent(ObjectStatus* object);

	//コンボの制限時間を進める
	void Update(float deltaTime);

	//1発ぶんの飛距離を点にして足す
	void AddDistanceBonus(float meters);
	//残りの玉ぶんのボーナスを足して、OnBonusGainedで知らせる
	void AddClearBonus(int remainingShots);

	//コンボを途切れさせる(1発が終わったときに呼ぶ)
	void ResetCombo();

	float GetScore()const { return _totalScore; }
	int GetCombo()const { return _combo; }
private:
	void CheckRemainingTarget();

	//スコアオブジェクトに当たった時のカウントと処理
	void AddHitTarget() { ++_hitTotalTarget; }
	void AddTarget() { ++_totalTargetCount; }
	void AddHit(const HitInfo& hit);
private:
	const int _bonusPoint = 1000;

	int _totalTargetCount = 0;
	int _hitTotalTarget = 0;

	//---- 採点の調整値 ----
	float _comboWindowSeconds = 3.0f; //前の的から、この秒数以内に当てるとコンボ
	float _comboStep = 0.25f; //コンボ1つにつき、倍率が増える量
	float _maxMultiplier = 3.0f; //倍率の上限
	float _headBonus = 500.0f; //頭に当たったときの追加点(倍率は掛けない)
	float _pointsPerMeter = 10.0f; //飛距離1mあたりの点

	float _totalScore = 0.0f;
	int _combo = 0;
	float _comboTimer = 0.0f; //0より大きい間は、コンボが続いている
};
