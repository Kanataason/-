#pragma once
#include "ServiceLocator.h"
#include "GameProgress.h"

//ゲーム中のUIを描画する
//プレイヤーのHP・ヒートゲージ・ボスのHPか、ボスまでの残り撃破数
//値はGameSceneがEventで受け取ってSetで入れる
class GameHUD
{
public:
	void Initialize(const ServiceLocator& locator, int bossKillCount);
	void Draw() const;

	void SetPlayerHp(float hp, float maxHp) { _playerHp = hp; _playerMaxHp = maxHp; }
	//呼ばれた時点でボスが出たとみなし、残り撃破数の代わりにHPバーを出す
	void SetBossHp(float hp, float maxHp) { _bossHp = hp; _bossMaxHp = maxHp; _isBossVisible = true; }
	void SetHeat(float heat, float maxHeat, bool isOverHeat) { _heat = heat; _maxHeat = maxHeat; _isOverHeat = isOverHeat; }

private:
	//残り3割以下で赤くする
	void DrawPlayerHp() const;
	//ボスが出ていればHPバー、出ていなければ残り撃破数
	void DrawBossInfo() const;
	//オーバーヒート中は赤、7割以上はオレンジ、通常は黄
	void DrawHeat() const;

	//ラベル付きのバーを描く。ratioは0から1
	static void DrawBar(int x, int y, int width, float ratio, int color, const char* label);
	//valueがmaxの何割かを0から1で返す。maxが0以下なら0
	static float ToRatio(float value, float max);

private:
	GameProgress* _progress = nullptr;
	int _bossKillCount = 0;

	float _playerHp = 0.0f;
	float _playerMaxHp = 0.0f;

	float _heat = 0.0f;
	float _maxHeat = 0.0f;
	bool _isOverHeat = false;

	float _bossHp = 0.0f;
	float _bossMaxHp = 0.0f;
	bool _isBossVisible = false;
};
