#pragma once
#include <algorithm>
#include <array>
#include <DirectXMath.h>

//威力ゲージの「色の帯」と「発射の速さ」を、1か所で決める表。
//「緑のうちは弱く、黄色は中くらい、赤は強く飛ぶ」が、必ず見た目と一致する。
namespace PowerZone
{
	struct Zone
	{
		float upTo; //下から数えて、ここまで(0～1)がこの色。最後は1にすること
		float speedAtEnd; //ゲージがこの帯の上端にいるときの、発射の速さ(m/s)
		DirectX::XMFLOAT4 color;
		const wchar_t* label; //ゲージの横に出す文字
	};

	//ゲージが0のときの発射の速さ(m/s)。
	inline constexpr float minSpeed = 6.0f;

	inline const std::array<Zone, 3> zones =
	{ {
		{ 0.4f, 12.0f, { 0.2f, 0.9f, 0.2f, 1.0f }, L"よわい"  }, //緑: ほとんど飛ばない
		{ 0.9f, 24.0f, { 1.0f, 0.85f, 0.1f, 1.0f }, L"ふつう" }, //黄: ちょっと飛ぶ
		{ 1.00f, 36.0f, { 1.0f, 0.15f, 0.1f, 1.0f }, L"つよい"}, //赤: すごく飛ぶ
	} };

	//ゲージの量がいる帯
	inline const Zone& ZoneAt(float power)
	{
		for (const auto& zone : zones)
		{
			if (power <= zone.upTo)
				return zone;
		}
		return zones.back();
	}

	//ゲージの量から、発射の速さ(m/s)を求める。帯の中では直線で増え、帯の境目で途切れない
	inline float SpeedFromPower(float power)
	{
		power = (std::clamp)(power, 0.0f, 1.0f);

		float zoneStart = 0.0f;
		float speedStart = minSpeed;
		for (const auto& zone : zones)
		{
			if (power <= zone.upTo)
			{
				const float t = (power - zoneStart) / (zone.upTo - zoneStart);
				return speedStart + (zone.speedAtEnd - speedStart) * t;
			}
			zoneStart = zone.upTo;
			speedStart = zone.speedAtEnd;
		}
		return speedStart;
	}
}
