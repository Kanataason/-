#pragma once
#include "BaseScene.h"
#include "BackgroundScroller.h"
#include "EnemySpawner.h"
#include "GameHUD.h"

//ゲーム本編
//背景スクロール・雑魚の出現・ボス出現の判定・UIを持つ
class GameScene : public BaseScene
{
public:
	void Initialize(ServiceLocator& locator)override;

	//更新順->バックグラウンド->バックグラウンドDraw->ランダムスポナー->Update
	void Update(float deltaTime)override;

	void DrawUI()override;
private:
	void SubScribeEvents();
	//ボスを生成して、HPバーとGameClearへの遷移をつなぐ
	void SpawnBoss();
private:
	ObjectData _bossData;   //Initializeで読み込んでおき、出現時に使う

	float _scrollSpeed = 120.0f;   //背景のスクロール速度

	//シーンが持つので、リトライで作り直されると自動で0に戻る
	GameProgress _progress;
	GameHUD _hud;
	bool _isBossSpawned = false;
	static constexpr int BossKillCount = 5;   //何体倒したらボスを出すか

	EnemySpawner _spawner;
	BackgroundScroller _background;
};