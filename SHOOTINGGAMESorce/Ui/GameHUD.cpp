#include "GameHUD.h"
#include <algorithm>
#include <DxLib.h>

namespace
{
	//バーの高さと、ラベルをバーの何ピクセル上に書くか
	constexpr int BarHeight = 12;
	constexpr int LabelOffsetY = 18;

	constexpr int BarBackColor = 0x333333;
	constexpr int FrameColor = 0xFFFFFF;
	constexpr int TextColor = 0xFFFFFF;

	constexpr float PlayerDangerRatio = 0.3f;
	constexpr float HeatWarningRatio = 0.7f;
}

void GameHUD::Initialize(const ServiceLocator& locator, int bossKillCount)
{
	_progress = locator.progress;
	_bossKillCount = bossKillCount;
}

void GameHUD::Draw() const
{
	DrawPlayerHp();
	DrawHeat();
	DrawBossInfo();
}

void GameHUD::DrawPlayerHp() const
{
	float ratio = ToRatio(_playerHp, _playerMaxHp);
	int color = (ratio <= PlayerDangerRatio) ? 0xFF0000 : 0x00FF88;
	DrawBar(10, 450, 150, ratio, color, "PLAYER");
}

void GameHUD::DrawBossInfo() const
{
	if (_isBossVisible)
	{
		DrawBar(120, 20, 400, ToRatio(_bossHp, _bossMaxHp), 0xFF3333, "BOSS");
		return;
	}

	//撃破数が目標を超えても、マイナスにならないようにする
	//DxLib経由で入るWindowsのmaxマクロと衝突しないように、かっこで囲む
	int remaining = (std::max)(_bossKillCount - _progress->GetKillCount(), 0);
	DrawFormatString(470, 10, TextColor, "BOSS まで あと %d 体", remaining);
}

void GameHUD::DrawHeat() const
{
	float ratio = ToRatio(_heat, _maxHeat);
	int color = _isOverHeat ? 0xFF0000
		: (ratio >= HeatWarningRatio) ? 0xFF8800
		: 0xFFFF00;
	const char* label = _isOverHeat ? "OVER HEAT" : "HEAT";
	DrawBar(10, 400, 100, ratio, color, label);
}

void GameHUD::DrawBar(int x, int y, int width, float ratio, int color, const char* label)
{
	DrawString(x, y - LabelOffsetY, label, TextColor);
	DrawBox(x, y, x + width, y + BarHeight, BarBackColor, TRUE);                             //背景
	DrawBox(x, y, x + static_cast<int>(width * ratio), y + BarHeight, color, TRUE);         //中身
	DrawBox(x, y, x + width, y + BarHeight, FrameColor, FALSE);                             //枠
}

float GameHUD::ToRatio(float value, float max)
{
	if (max <= 0.0f) return 0.0f;
	//HPがマイナスになってもバーが逆に伸びないように、0から1に収める
	return std::clamp(value / max, 0.0f, 1.0f);
}
