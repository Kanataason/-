#include "GameHud.h"

#include <algorithm>
#include <cmath>

#include "BaseCamera.h"

using namespace UiInfo;

namespace
{
	//---- 点が入ったときの演出の調整値 ----
	constexpr float popupLifetime = 1.3f; //ポイントが消えるまでの秒数
	constexpr float popupRisePixels = 90.0f; //浮き上がる高さ(1280x720基準のピクセル)
	constexpr float popupBaseScale = 0.55f; //文字の大きさ
	constexpr float popupPopSeconds = 0.25f; //出た瞬間に弾む時間
	constexpr float popupFadeStart = 0.5f; //寿命のここから先で、だんだん透明になる

	//COMBO表示(スコアのパネルの下)
	constexpr float comboCenterX = 180.0f;
	constexpr float comboCenterY = 172.0f;
	constexpr float comboScale = 0.5f;
	constexpr float comboPopSeconds = 0.3f;

	//クリアボーナスの大きな文字(画面の中央)
	constexpr float bonusLifetime = 1.45f; //消えるまでの秒数(結果パネルが出る前に、消える長さ)
	constexpr float bonusPopSeconds = 0.4f; //小さい所から、ドンと出てくる時間
	constexpr float bonusFadeStart = 0.75f; //寿命のここから先で、だんだん透明になる
	constexpr float bonusCenterX = 640.0f;
	constexpr float bonusLabelY = 230.0f;
	constexpr float bonusPointsY = 330.0f;
	constexpr float bonusLabelScale = 0.6f;
	constexpr float bonusPointsScale = 1.5f;
}

void GameHud::Initialize(ServiceLocator& locator)
{
	_uiRenderer = locator.uiRenderer;
}

void GameHud::Bind(ScoreManager& score, RoundManager& round)
{
	_score = &score;
	_round = &round;

	//点が入ったら、当たった場所に500を出す
	score.OnScoreGained.Subscribe([this](ScoreGain gain) { AddScorePopup(gain); });

	//ボーナスが入ったら、画面の中央に、大きく出す
	score.OnBonusGained.Subscribe([this](float points) { StartBonusPopup(points); });

	//ラグドールが止まった直後に、残りの玉数を弾ませる
	round.OnShotSettled.Subscribe([this]() { _shotCountPop.Start(0.5f); });
}

void GameHud::Update(float deltaTime)
{
	_shotCountPop.Update(deltaTime);

	for (auto& popup : _scorePopups)
		popup.age += deltaTime;

	std::erase_if(_scorePopups, [](const ScorePopup& popup) { return popup.age >= popupLifetime; });

	//コンボが増えた瞬間に、COMBOの文字を弾ませる
	const int combo = _score ? _score->GetCombo() : 0;
	if (combo > _lastCombo)
		_comboPop.Start(comboPopSeconds);
	_lastCombo = combo;
	_comboPop.Update(deltaTime);

	if (_bonusActive)
	{
		_bonusAge += deltaTime;
		if (_bonusAge >= bonusLifetime)
			_bonusActive = false;
	}
}

void GameHud::Draw(Camera* camera)
{
	if (!_uiRenderer)
		return;

	DrawScorePanel();
	DrawShotCountPanel();
	DrawComboText();
	DrawScorePopups(camera);
	DrawBonusPopup();
	DrawAimHint();
}

void GameHud::AddScorePopup(const ScoreGain& gain)
{
	ScorePopup popup;
	popup.worldPosition = gain.position;
	popup.isHead = gain.isHead;

	const std::wstring points = L"+" + FormatWithComma(static_cast<int>(gain.points));
	popup.text = gain.isHead ? L"頭ヒット! " + points : points;

	_scorePopups.push_back(popup);
}

void GameHud::StartBonusPopup(float points)
{
	_bonusActive = true;
	_bonusPoints = points;
	_bonusAge = 0.0f;
}

//画面の下に、いま何を決めるのかを出す(角度 -> 威力の2段階)
void GameHud::DrawAimHint()
{
	if (!_round)
		return;

	std::wstring text;
	switch (_round->GetState())
	{
	case RoundState::AimAngle:
		text = L"スペースキーで かくどを きめる";
		break;
	case RoundState::AimPower:
		text = L"スペースキーで いりょくを きめる";
		break;
	default:
		return;
	}

	_uiRenderer->DrawStringCentered(text, 640.0f, 600.0f, 0.4f,
		DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
}

//「ぜんぶ たおした!」と、ボーナスの点を、画面の中央に大きく出す
void GameHud::DrawBonusPopup()
{
	if (!_bonusActive)
		return;

	const float t = _bonusAge / bonusLifetime;

	//小さい所から、大きく行き過ぎて、元の大きさに戻る(ドンと出る)。そのあとも、少し脈打つ
	const float popT = (std::min)(_bonusAge / bonusPopSeconds, 1.0f);
	const float pulse = 1.0f + 0.04f * std::sin(_bonusAge * 14.0f);
	const float scale = (0.2f + 0.8f * EaseOutBack(popT)) * pulse;

	const float alpha = t < bonusFadeStart ? 1.0f : 1.0f - (t - bonusFadeStart) / (1.0f - bonusFadeStart);

	const DirectX::XMVECTOR outline = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, alpha);

	_uiRenderer->DrawStringCentered(L"ぜんぶ たおした!", bonusCenterX, bonusLabelY, bonusLabelScale * scale,
		DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, alpha), outline, 5.0f);

	_uiRenderer->DrawStringCentered(L"+" + FormatWithComma(static_cast<int>(_bonusPoints)),
		bonusCenterX, bonusPointsY, bonusPointsScale * scale,
		DirectX::XMVectorSet(1.0f, 0.82f, 0.15f, alpha), outline, 8.0f);
}

//当たった場所から浮き上がって、消える文字。当たった場所は世界座標なので、カメラが動いても場所についていく
void GameHud::DrawScorePopups(Camera* camera)
{
	if (!camera)
		return;

	const DirectX::XMMATRIX view = camera->GetViewMatrix();
	const DirectX::XMMATRIX projection = camera->GetProjectionMatrix();

	for (const auto& popup : _scorePopups)
	{
		const float t = popup.age / popupLifetime;

		const DirectX::XMVECTOR world = DirectX::XMVectorSet(
			popup.worldPosition.x, popup.worldPosition.y, popup.worldPosition.z, 1.0f);
		const DirectX::XMFLOAT2 screen = _uiRenderer->WorldToScreen(world, view, projection);

		const float rise = popupRisePixels * EaseOutCubic(t);
		const float alpha = t < popupFadeStart ? 1.0f : 1.0f - (t - popupFadeStart) / (1.0f - popupFadeStart);

		//出た瞬間に大きく弾んで、元の大きさに戻る。頭ヒットは、さらに大きくて金色
		const float popT = (std::min)(popup.age / popupPopSeconds, 1.0f);
		const float scale = popupBaseScale * (popup.isHead ? 1.3f : 1.0f) * (1.0f + 0.5f * (1.0f - EaseOutBack(popT)));

		const DirectX::XMVECTOR color = popup.isHead
			? DirectX::XMVectorSet(1.0f, 0.8f, 0.2f, alpha)
			: DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, alpha);

		_uiRenderer->DrawStringCentered(popup.text, screen.x, screen.y - rise, scale,
			color, DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, alpha));
	}
}

//「COMBO x3」(2コンボ以上のときだけ)
void GameHud::DrawComboText()
{
	const int combo = _score ? _score->GetCombo() : 0;
	if (combo < 2)
		return;

	const float pop = 1.0f + 0.5f * (1.0f - EaseOutBack(_comboPop.T()));
	_uiRenderer->DrawStringCentered(L"COMBO x" + std::to_wstring(combo), comboCenterX, comboCenterY,
		comboScale * pop, DirectX::XMVectorSet(1.0f, 0.55f, 0.2f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
}

//パネルの塗り・枠・見出しの文字を描く(スコアと残りの数で共通)
void GameHud::DrawPanelBase(const PanelData& panel)
{
	_uiRenderer->DrawRect(panel.frame.left, panel.frame.top, panel.frame.width, panel.frame.height,
		XMLoadFloat4(&panel.fillColor));
	_uiRenderer->DrawFrame(panel.frame.left, panel.frame.top, panel.frame.width, panel.frame.height,
		panel.frame.thickness, XMLoadFloat4(&panel.frame.frameColor));

	for (const auto& item : panel.items)
	{
		auto textIterator = textList.find(item.text);
		if (textIterator == textList.end())
			continue;

		const auto& textData = textIterator->second;
		_uiRenderer->DrawStringCentered(textData.text, textData.centerX, textData.centerY, textData.scale,
			XMLoadFloat4(&textData.mainColor), XMLoadFloat4(&textData.outlineColor));
	}
}

//画面の上の中央に、残りの発数を出す
void GameHud::DrawShotCountPanel()
{
	auto it = panelList.find(PanelType::ShotCount);
	if (it == panelList.end() || !_round)
		return;
	const PanelData& panel = it->second;

	DrawPanelBase(panel);

	constexpr float valueScale = 0.55f;
	constexpr float valueTop = 58.0f;
	constexpr float fontLineHeight = 96.0f; //UIFont.spritefont の行の高さ(scale 1.0のとき)

	//ラグドールが止まった直後は、1.6倍から弾んで元の大きさに戻る
	const float pop = 1.0f + 0.6f * (1.0f - EaseOutBack(_shotCountPop.T()));

	//拡大しても数字の中心がずれないように、中心を基準に描く
	const float centerX = panel.frame.left + panel.frame.width * 0.5f;
	const float centerY = valueTop + fontLineHeight * valueScale * 0.5f;
	_uiRenderer->DrawStringCentered(std::to_wstring(_round->GetShotCount()), centerX, centerY,
		valueScale * pop, DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMVectorSet(0, 0, 0, 1), 0.0f);
}

//画面の左上に、いまのスコアを出す
void GameHud::DrawScorePanel()
{
	auto it = panelList.find(PanelType::Score);
	if (it == panelList.end() || !_score)
		return;
	const PanelData& panel = it->second;

	DrawPanelBase(panel);

	//位置は固定なのでここで設定,
	//現在のスコア
	constexpr float valueScale = 0.55f;
	constexpr float valueTop = 58.0f;
	constexpr float rightPadding = 20.0f;
	const std::wstring scoreText = FormatWithComma(static_cast<int>(_score->GetScore()));
	_uiRenderer->DrawString(scoreText, panel.frame.left + panel.frame.width - rightPadding, valueTop, valueScale,
		DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), TextAlign::Right);
}
