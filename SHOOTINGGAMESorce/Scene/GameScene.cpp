#include "GameScene.h"
#include "GameObject.h"
#include "Transform.h"

#include "SoundManager.h"
#include "JsonLoader.h"

#include "PlayerStatus.h"
#include "PlayerAttack.h"
#include "EnemyStatus.h"
#include "SceneManager.h"
#include "DrawManager.h"

void GameScene::Initialize(ServiceLocator& locator)
{
	//進行状況はこのシーンだけのものなので、コピーしたlocatorに追加して渡す
	//SceneManagerが持っている元のlocatorは書き換えない
	ServiceLocator sceneLocator = locator;
	sceneLocator.progress = &_progress;
	BaseScene::Initialize(sceneLocator);

	_background.Initialize(_scrollSpeed);
	JsonLoader loader;
	_bossData = loader.LoadBossData("JsonData/Boss.json");
	_spawner.Initialize(sceneLocator, loader.LoadSpawnData("JsonData/Spawn.json"));
	_hud.Initialize(sceneLocator, BossKillCount);

	locator.soundManager->PlayBgm(SoundId::BgmGame);

	SubScribeEvents();
}
void GameScene::SubScribeEvents()
{
	//プレイヤー：シーンの初期化時に1回だけ
	GameObject* player = _locator.playerProvider->GetPlayer();
	if (!player) return;
	if (auto* playerStatus = player->GetComponent<PlayerStatus>())
	{
		_hud.SetPlayerHp(playerStatus->GetHealth(), playerStatus->GetMaxHealth());   //初期値
		playerStatus->OnHealthChanged.Subscribe([this](float hp, float maxHp) { _hud.SetPlayerHp(hp, maxHp); });

		playerStatus->OnDeathFinished.Subscribe([this]
			{
				_locator.sceneManager->RequestChange(SceneName::EGameOver);
			});
	}
	if (auto* playerAttack = player->GetComponent<PlayerAttack>())
	{
		playerAttack->OnHeatChanged.Subscribe([this](float heat, float max,bool isOver) 
			{ _hud.SetHeat(heat, max,isOver); });
	}
}

void GameScene::SpawnBoss()
{
	auto boss = _locator.factory->CreateObject(_bossData);

	_locator.soundManager->PlayBgm(SoundId::BgmBoss);
	//ボス：生成したときに1回だけ
	if (auto* bossStatus = boss->GetComponent<EnemyStatus>())
	{
		_hud.SetBossHp(bossStatus->GetHealth(), bossStatus->GetMaxHealth());   //初期値
		bossStatus->OnHealthChanged.Subscribe([this](float hp, float maxHp) { _hud.SetBossHp(hp, maxHp); });

		bossStatus->OnDeathFinished.Subscribe([this]
			{
				_locator.sceneManager->RequestChange(SceneName::EGameClear);
			});
	}
	Spawn(std::move(boss));
}

void GameScene::DrawUI()
{
	_locator.drawManager->Register(DrawLayer::UI, [this] { _hud.Draw(); });
}

void GameScene::Update(float deltaTime)
{
	_background.Update(deltaTime);
	_locator.drawManager->Register(DrawLayer::Background, [this] { _background.Draw(); });
	_spawner.Update(deltaTime, *this);

	if (!_isBossSpawned && _progress.GetKillCount() >= BossKillCount)
	{
		_isBossSpawned = true;
		_spawner.SetActive(false);//雑魚の出現を止める
		SpawnBoss();
	}

	BaseScene::Update(deltaTime);
}
