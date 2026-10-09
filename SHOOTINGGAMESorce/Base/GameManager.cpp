#include "GameManager.h"
#include "JsonLoader.h"

void GameManager::Initialize()
{
	JsonLoader loader;
	const auto& soundJson = loader.LoadJsonData("JsonData/Sound.json");
	_soundManager.Initialize(loader.LoadSoundData(soundJson));

	auto context = CreateLocator();
	_sceneManager.Initialize(_gameObjectFactory,context);
	_bulletManager.Initialize(context);
	_colliderManager.Initialize(context);
}
void GameManager::Update(float deltaTime)
{
	//描画の順番は呼ぶ順番ではなく、DrawLayerで決まる
	_inputManager.Update();
	_sceneManager.Update(deltaTime);
	_bulletManager.Update(deltaTime);

	//移動が終わった後に当たり判定を行う
	_colliderManager.ResolveCollider();
	_colliderManager.CheckCollisions(_bulletManager.GetPool());

	//描画は登録だけ行い、最後にレイヤーの順でまとめて描く
	_drawManager.Register(DrawLayer::Bullet, [this] { _bulletManager.Draw(); });
	_sceneManager.DrawUI();
	_drawManager.Flush();
}

ServiceLocator GameManager::CreateLocator()
{
	ServiceLocator locator{};
	locator.input = &_inputManager;
	locator.bulletManager = &_bulletManager;
	locator.drawManager = &_drawManager;
	locator.gameWindow = &_gameWindow;
	locator.playerProvider = &_provider;
	locator.colliderRegistry = &_registry;
	locator.sceneManager = &_sceneManager;
	locator.factory = &_gameObjectFactory;
	locator.soundManager = &_soundManager;

	return locator;
}