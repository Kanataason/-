#pragma once
#include <string>
#include <vector>

#include "BaseScene.h"
#include "ScoreManager.h"
#include "RoundManager.h"
#include "ResultPanel.h"
#include "PausePanel.h"
#include "GameAudio.h"
#include "GameHud.h"
#include "UIRenderer.h"
#include "UiLayout.h"
#include "UiTween.h"
#include "PowerGauge.h"

class Ragdoll;
class CameraFollow;
class TimeScale;
class InputSystem;

class GameScene : public BaseScene
{
public:
	void Initialize(ServiceLocator& locator)override;
	void Update(float deltaTime)override;
	void DrawUi()override;
	void SetStageInfo(StageData data)override;
private:
	void SetCannonInfo();
	void SubscribeEvents();

	//ポーズ。時間を止める(シーンの更新も物理も止まる)/ 戻す
	void SetPaused(bool paused);
private:
	RoundManager _roundManager;
	ScoreManager _scoreManager;
	ResultPanel _resultPanel; //玉を撃ち終えたあとに、ゲーム画面の上に重ねて出す
	PausePanel _pausePanel; //ESCで出す、ポーズのパネル
	GameAudio _audio; //出来事と音の対応(通知につなぐだけで、鳴らす音は、ここが決める)
	GameHud _hud; //スコア・残りの玉数・COMBO・「+500」などの画面表示
	InputSystem* _input = nullptr;
	PowerGauge _powerGauge; //威力ゲージ(量の計算と、描画)

	Ragdoll* _ragdoll = nullptr;
	float _launchRootX = 0.0f; //発射した瞬間の、体のX位置(飛距離の採点に使う)
	CameraFollow* _cameraFollow = nullptr;

	//時間の速さ。ポーズ(時間を止める)と、的に当たったときのスローモーション(倍率と、元の速さに戻るまでの実時間)に使う
	TimeScale* _timeScale = nullptr;
	float _hitSlowScale = 0.35f;
	float _hitSlowSeconds = 1.25f;
	float _headSlowScale = 0.2f; //頭ヒットは、もっと遅く、長く
	float _headSlowSeconds = 1.45f;
};
