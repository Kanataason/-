#pragma once
#include "SceneManager.h"
#include "InputManager.h"
#include "GameObjectfactory.h"
#include "BulletManager.h"
#include "GameWindow.h"
#include "PlayerProvider.h"
#include "ColliderRegistry.h"
#include "ColliderManager.h"
#include "DrawManager.h"
#include "SoundManager.h"

#include "ServiceLocator.h"

//ゲーム全体で1つだけのサービスを所有し、毎フレームの処理順を決める
class GameManager
{
public:
	GameManager(HWND hwnd):_gameWindow(hwnd), _inputManager(_gameWindow)
	{
		int width = 0;
		int height = 0;
		GetDrawScreenSize(&width, &height);
		_gameWindow.OnResize((float)width, (float)height);
	}
	//初期化関数
	void Initialize();

	//更新順はInput->SceneManager->BulletManager->Resolve->Collision->描画の登録->Flush
	void Update(float deltaTime);
private:
	ServiceLocator CreateLocator();

private:
	//メンバーは宣言と逆順に破棄される
	//シーン内のコライダーが破棄時にRegistryを使うので、Registry・Providerはシーンより先に宣言する
	GameWindow _gameWindow;

	PlayerProvider _provider;
	ColliderRegistry _registry;
	ColliderManager _colliderManager;

	SceneManager _sceneManager;
	InputManager _inputManager;
	BulletManager _bulletManager;
	DrawManager _drawManager;
	SoundManager _soundManager;

	GameObjectFactory _gameObjectFactory;
};
