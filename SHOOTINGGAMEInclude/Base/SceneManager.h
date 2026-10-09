#pragma once
#include <unordered_map>
#include <iostream>
#include "BaseScene.h"
#include "GameObjectfactory.h"
#include "JsonLoader.h"
#include "SceneLoader.h"
#include "ServiceLocator.h"

#include "Datas.h"



//シーンの切り替えを担当する
//今のシーンだけを実体として持ち、切り替えのたびにJSONから作り直す。リトライで最初の状態に戻る
class SceneManager
{
public:
	SceneManager() = default;
	~SceneManager() = default;

	void Initialize(GameObjectFactory& objectfactory, ServiceLocator& locator);

	void Update(float deltaTime);
	void RequestChange(SceneName name) { _nextScene = name; }   //予約するだけ
	void DrawUI() { if (_currentScene) _currentScene->DrawUI(); }
private:
	void ApplyChange();

	//シーンの情報を設定
	void EntryScene(GameObjectFactory& objectfactory);
private:
	std::unordered_map<SceneName, SceneData> _sceneDataList;    //シーンの作り方だけ保存しておく
	std::unique_ptr<BaseScene> _currentScene;
	SceneName _nextScene = SceneName::None;

	GameObjectFactory* _factory = nullptr;
	ServiceLocator _locator{};
	JsonLoader _jsonLoader;
	SceneLoader _sceneLoader;

};