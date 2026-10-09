#include <algorithm>

#include "ScoreManager.h"
#include "ObjectStatus.h"

void ScoreManager::SubscribeEvent(ObjectStatus* status)
{
	AddTarget();
	status->OnSendScore.Subscribe([this](HitInfo hit) { AddHit(hit); });
}

void ScoreManager::Update(float deltaTime)
{
	if (_comboTimer <= 0.0f)
		return;

	_comboTimer -= deltaTime;
	if (_comboTimer <= 0.0f)
		ResetCombo();
}

void ScoreManager::AddHit(const HitInfo& hit)
{
	//制限時間内に次の的に当たったらコンボ。そうでなければ、1から数え直す
	_combo = (_comboTimer > 0.0f) ? _combo + 1 : 1;
	_comboTimer = _comboWindowSeconds;

	const float multiplier = (std::min)(1.0f + _comboStep * static_cast<float>(_combo - 1), _maxMultiplier);
	float gained = hit.baseScore * multiplier;
	if (hit.isHead)
		gained += _headBonus;

	_totalScore += gained;

	ScoreGain gain;
	gain.points = gained;
	gain.combo = _combo;
	gain.isHead = hit.isHead;
	gain.position = hit.position;
	OnScoreGained.Invoke(gain);

	CheckRemainingTarget();
}
void ScoreManager::CheckRemainingTarget()
{
	AddHitTarget();

	//すべての的を倒したら一回だけ呼ぶ
	if (_hitTotalTarget >= _totalTargetCount && _totalTargetCount > 0)
		OnAllCleared.Invoke();
}

void ScoreManager::AddDistanceBonus(float meters)
{
	_totalScore += (std::max)(0.0f, meters) * _pointsPerMeter;
}

void ScoreManager::AddClearBonus(int remainingShots)
{
	const float bonus = static_cast<float>((std::max)(0, remainingShots) * _bonusPoint);
	_totalScore += bonus;
	OnBonusGained.Invoke(bonus);
}

void ScoreManager::ResetCombo()
{
	_combo = 0;
	_comboTimer = 0.0f;
}
