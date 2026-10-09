#pragma once
#include <iostream>
#include <vector>

#include <BaseScene.h>
#include "ServiceLocator.h"
#include "ObjectType.h"
#include "BaseSceneLoader.h"
#include "JsonLoader.h"

class Renderer;
class DebugDraw;
class PhysicsSystem;

class SceneManager
{
public:
	void Initialize(ServiceLocator& locator, PhysicsSystem* _physicsSystem);
	void Update(float deltaTIme);
	void Draw(Renderer* renderer);
	void Release();

	//変更を予約するだけ
	void ChangeScene(SceneName name);
	void Reload() { ChangeScene(_currentSceneName); };

	SceneName GetCurrentSceneName()const { return _currentSceneName; }
private:

	//シーンの初期化をする
	void EntryScene();

	//シーンを新しく生成
	void RequestScene();
private:
	std::unordered_map<SceneName, BaseSceneData> _sceneList;
	std::unique_ptr<BaseScene> _currentScene;

	SceneName _nextScene = SceneName::None;
	SceneName _currentSceneName = SceneName::None;

	ServiceLocator _locator{};
	JsonLoader _jsonLoader;
	SceneLoader _sceneLoader;
};