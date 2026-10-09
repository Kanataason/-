#pragma once
#include <algorithm>

//UIのアニメーション用。時間を0～1に変えるだけの、小さな部品
struct Tween
{
	float time = 0.0f;
	float duration = 0.0f; //0のときは止まっている(T()は1を返す)

	void Start(float seconds)
	{
		time = 0.0f;
		duration = seconds;
	}

	void Update(float deltaTime)
	{
		if (duration > 0.0f)
			time = (std::min)(time + deltaTime, duration);
	}

	bool IsPlaying() const { return duration > 0.0f && time < duration; }

	//進み具合(0～1)
	float T() const { return duration > 0.0f ? time / duration : 1.0f; }
};

//最初が速く、だんだん遅くなる動き。t=0で0、t=1で1
inline float EaseOutCubic(float t)
{
	const float u = 1.0f - t;
	return 1.0f - u * u * u;
}

//少し行き過ぎてから戻る動き(ポンッとした弾み)。t=0で0、t=1で1
inline float EaseOutBack(float t)
{
	constexpr float c1 = 1.70158f;
	constexpr float c3 = c1 + 1.0f;
	const float u = t - 1.0f;
	return 1.0f + c3 * u * u * u + c1 * u * u;
}
