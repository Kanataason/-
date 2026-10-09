#include "GameScene.h"

#include <cmath>

#include "CannonController.h"
#include "Player.h"
#include "Ragdoll.h"
#include "CameraFollow.h"
#include "BaseCamera.h"
#include "ObjectStatus.h"
#include "TimeScale.h"
#include "InputSystem.h"
#include "SceneManager.h"

void GameScene::Initialize(ServiceLocator& locator)
{
	BaseScene::Initialize(locator);

	_timeScale = locator.timeScale;
	_input = locator.input;

	SetPaused(false); //ポーズのまま、シーンが切り替わっても、止まったままにならないように

	_resultPanel.Initialize(locator);
	_pausePanel.Initialize(locator);
	_audio.Initialize(locator);
	_hud.Initialize(locator);
	_powerGauge.Initialize(locator);

	SetCannonInfo();
	SubscribeEvents();
}
void GameScene::SetCannonInfo()
{
	auto* cannon = FindGameObject<CannonController>();
	auto* player = FindGameObject<Player>();

	if (cannon)
	{
		cannon->SetRoundManager(&_roundManager);
		cannon->SetInitializePlayer(player);
	}

	for (const auto& obj : _objects)
	{
		if (auto* component = obj->GetComponent<ObjectStatus>())
			_scoreManager.SubscribeEvent(component);
	}
}

void GameScene::SubscribeEvents()
{
	//ラグドールが止まったら、1発ぶんの流れを進める(RoundManager)
	//止まった瞬間 -> 飛距離ボーナスを足して、コンボを区切る(残りの玉数の表示は、GameHudが弾ませる)
	//少し待つ     -> ラグドールを大砲に戻して、次の1発へ
	_ragdoll = FindGameObject<Ragdoll>();
	if (_ragdoll)
	{
		_ragdoll->OnSettled.Subscribe([this]() { _roundManager.NotifySettled(); });
	}

	//出来事に、音をつなぐ(どの音を鳴らすかは、GameAudioが決める)
	_audio.Bind(_ragdoll, _scoreManager, _roundManager);
	_powerGauge.Bind(_roundManager);

	//カメラ: 発射したらラグドールを追いかけ、次の1発の準備ができたら最初の位置へ戻る
	_cameraFollow = FindGameObject<CameraFollow>();
	if (_cameraFollow)
		_cameraFollow->SetTarget(_ragdoll);

	//画面表示(スコア、残りの玉数、「+500」など)を、通知につなぐ
	_hud.Bind(_scoreManager, _roundManager);

	//点が入ったら
	_scoreManager.OnScoreGained.Subscribe([this](ScoreGain gain)
		{
			//当たった瞬間に、少し遅くする(頭ヒットは、もっと遅く)
			if (_timeScale)
			{
				if (gain.isHead)
					_timeScale->Request(_headSlowScale, _headSlowSeconds);
				else
					_timeScale->Request(_hitSlowScale, _hitSlowSeconds);
			}
		});

	_roundManager.OnShotFired.Subscribe([this]()
		{
			//発射した位置を覚えておく(止まったときの飛距離の採点に使う)
			if (_ragdoll)
				_launchRootX = _ragdoll->GetRootPosition().x;
			if (_cameraFollow)
				_cameraFollow->StartFollow();
		});
	_roundManager.OnShotSettled.Subscribe([this]()
		{
			//1発が終わったら、飛んだ距離を点にして、コンボを途切れさせる
			if (_ragdoll)
				_scoreManager.AddDistanceBonus(std::abs(_ragdoll->GetRootPosition().x - _launchRootX));
			_scoreManager.ResetCombo();
		});
	_roundManager.OnReadyNextShot.Subscribe([this]()
		{
			if (_ragdoll)
				_ragdoll->StopRagdoll(false);
			if (_cameraFollow)
				_cameraFollow->ReturnToStart();
		});

	_scoreManager.OnAllCleared.Subscribe([this]()
		{
			_scoreManager.AddClearBonus(_roundManager.GetShotCount());
			_roundManager.NotifyAllCleared();
		});

	//玉を撃ち終えたら、いまのステージの結果を、ゲーム画面の上に出す
	_roundManager.OnFinished.Subscribe([this]()
		{
			_resultPanel.Show(static_cast<int>(_scoreManager.GetScore()));
		});

	//結果パネルの選択。シーンの切り替えは「予約」だけで、実際の入れ替えは次のフレームの頭に行われる
	//同じステージ(SceneName::Stage)を指定すると、選んでいるステージのJSONから作り直されるので、スコアも玉数も、的の配置も、最初に戻る
	_resultPanel.OnRetry.Subscribe([this]() { _sceneManager->Reload(); });
	_resultPanel.OnBackToTitle.Subscribe([this]() { _sceneManager->ChangeScene(SceneName::Title); });

	//ポーズのパネルの選択。シーンを切り替えるときは、先に時間を戻しておく
	_pausePanel.OnResume.Subscribe([this]() { SetPaused(false); });
	_pausePanel.OnRetry.Subscribe([this]()
		{
			SetPaused(false);
			_sceneManager->Reload();
		});
	_pausePanel.OnBackToTitle.Subscribe([this]()
		{
			SetPaused(false);
			_sceneManager->ChangeScene(SceneName::Title);
		});
}

void GameScene::SetPaused(bool paused)
{
	if (_timeScale)
		_timeScale->SetPaused(paused);
}

void GameScene::SetStageInfo(StageData data)
{
	_resultPanel.SetStarThresholds(data.starThresholds);
	_roundManager.SetShotCount(data.shotCount);
}

void GameScene::Update(float deltaTime)
{
	//ポーズ中は、パネルの操作だけ受け付ける(ゲームの入力・更新は、全部止める)
	if (_pausePanel.IsVisible())
	{
		_pausePanel.Update();
		return;
	}

	//ESCでポーズ。結果パネルが出ているときは、出さない
	if (_input && !_resultPanel.IsVisible() && _input->GetKeyDown(VK_ESCAPE))
	{
		_pausePanel.Open();
		SetPaused(true);
		return;
	}

	BaseScene::Update(deltaTime);

	_audio.Update(deltaTime);
	_powerGauge.Update(deltaTime);
	_roundManager.Update(deltaTime);
	_scoreManager.Update(deltaTime);
	_hud.Update(deltaTime);
	_resultPanel.Update(deltaTime);
}

void GameScene::DrawUi()
{
	_uiRenderer->Begin();

	_hud.Draw(_camera);
	_powerGauge.Draw();
	_resultPanel.Draw(); //(出ていないときは、何もしない)
	_pausePanel.Draw(); //一番手前

	_uiRenderer->End();
}

