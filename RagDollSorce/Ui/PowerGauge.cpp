#include "PowerGauge.h"

#include <algorithm>
#include <cmath>

#include "RoundManager.h"
#include "UIRenderer.h"
#include "UiLayout.h"
#include "PowerZone.h"

using namespace UiInfo;

namespace
{
	constexpr float dimZoneAlpha = 0.28f; //まだ届いていない色の帯の、薄さ
	constexpr float gaugeLabelOffsetY = 22.0f; //ゲージの下端から、帯の名前までの距離
	constexpr float gaugeLabelScale = 0.4f;
}

void PowerGauge::Initialize(ServiceLocator& locator)
{
	_uiRenderer = locator.uiRenderer;
}

void PowerGauge::Bind(RoundManager& roundManager)
{
	_roundManager = &roundManager;
}

void PowerGauge::Draw()
{
	auto it = panelList.find(PanelType::CannonGauge);
	if (it == panelList.end() || !_uiRenderer || !_roundManager)
		return;

	auto& panelData = it->second.frame;

	//背景
	_uiRenderer->DrawRect(panelData.left, panelData.top, panelData.width,
		panelData.height, DirectX::XMVectorSet(0, 0, 0, 0.6f));

	//中身: 量に応じて、下端から上へ伸びる。色の帯ごとに区切って塗る
	//帯の位置は固定。まだ届いていない帯も薄く見せて、「上へ行くほど強い」が先にわかるようにする
	float zoneStart = 0.0f;
	for (const auto& zone : PowerZone::zones)
	{
		const float zoneHeight = panelData.height * (zone.upTo - zoneStart);
		const float zoneTop = panelData.top + panelData.height * (1.0f - zone.upTo);

		DirectX::XMFLOAT4 dimColor = zone.color;
		dimColor.w = dimZoneAlpha;
		_uiRenderer->DrawRect(panelData.left, zoneTop, panelData.width, zoneHeight, XMLoadFloat4(&dimColor));

		const float filledEnd = (std::min)(_gaugeAmount, zone.upTo);
		if (filledEnd > zoneStart)
		{
			const float segmentHeight = panelData.height * (filledEnd - zoneStart);
			const float segmentTop = panelData.top + panelData.height * (1.0f - filledEnd);
			_uiRenderer->DrawRect(panelData.left, segmentTop, panelData.width, segmentHeight,
				XMLoadFloat4(&zone.color));
		}

		//帯の境目に、白い線を引く(一番上の境目は枠と重なるので、引かない)
		if (zone.upTo < 1.0f)
			_uiRenderer->DrawRect(panelData.left, zoneTop - 1.0f, panelData.width, 2.0f,
				DirectX::XMVectorSet(1, 1, 1, 0.8f));

		zoneStart = zone.upTo;
	}
	_uiRenderer->DrawFrame(panelData.left, panelData.top, panelData.width,
		panelData.height, 2.0f, DirectX::XMVectorSet(1, 1, 1, 1));

	//威力を決めている間と、発射したあとは、いまの帯の名前を、その色でゲージの下に出す
	if (_roundManager->GetState() != RoundState::AimAngle)
	{
		const PowerZone::Zone& zone = PowerZone::ZoneAt(_gaugeAmount);
		_uiRenderer->DrawStringCentered(zone.label,
			panelData.left + panelData.width * 0.5f, panelData.top + panelData.height + gaugeLabelOffsetY,
			gaugeLabelScale, XMLoadFloat4(&zone.color), DirectX::XMVectorSet(0, 0, 0, 1));
	}
}

void PowerGauge::Update(float deltaTime)
{
	if (!_roundManager)
		return;

	switch (_roundManager->GetState())
	{
	case RoundState::AimAngle:
		//角度を決めている間は、ゲージを空にして待つ(次の1発も、毎回0から動き出す)
		_gaugeFrame = 0.0f;
		_gaugeAmount = 0.0f;
		break;

	case RoundState::AimPower:
		_gaugeFrame += _gaugeSpeed * deltaTime;
		_gaugeAmount = std::abs(std::sin(_gaugeFrame));
		break;

	default:
		//発射したあとは、止めた量のまま表示しておく
		break;
	}

	//大砲が撃つときに読めるように、今の量を渡しておく
	_roundManager->SetPowerAmount(_gaugeAmount);
}