#include <unordered_map>
#include "SceneManager.h"

#include "Title.h"
#include "GameScene.h"
#include "StageRoster.h"


//シーンの作り方をテーブルで登録
namespace
{
	const std::unordered_map<SceneName,
		std::function<std::unique_ptr<BaseScene>()>> MakeSceneFunctionTable =
	{
		{SceneName::Title,[]{return std::make_unique<Title>();}},
		{SceneName::Stage,[] {return std::make_unique<GameScene>();}},
	};
}

void SceneManager::Initialize(ServiceLocator& locator,PhysicsSystem* physicsSystem)
{
	_locator = locator;
	EntryScene();
}
void SceneManager::Update(float deltaTime)
{
	if (_nextScene != SceneName::None)
		RequestScene();

	_currentScene->Update(deltaTime);
}
void SceneManager::Draw(Renderer* renderer)
{
	_currentScene->Draw(*renderer);
	_currentScene->DrawUi();
}

void SceneManager::Release()
{
	_currentScene->Release();
}

void SceneManager::ChangeScene(SceneName name)
{
	_nextScene = name;
}
void SceneManager::RequestScene()
{
	const auto& data = _sceneList.at(_nextScene);
	_nextScene = SceneName::None;
	_currentSceneName = data.sceneName;

	//消す前にReleaseを呼ぶ(RigidBody/Colliderの登録解除は、デストラクタが自動的に行う)
	if (_currentScene)
		_currentScene->Release();

	_currentScene.reset();

	_currentScene = data.scene();

	//ステージは、選んでいるステージのJSON(Stages.json)を読む。それ以外は、Scene.jsonに書いたJSON
	std::string path = data.path;
	if (data.sceneName == SceneName::Stage && _locator.stages)
		path = _locator.stages->GetSelectedPath();

	_sceneLoader.LoadSceneGameObject(_currentScene.get(), path, _jsonLoader, *_locator.factory,
		_locator.characters);
    _currentScene->Initialize(_locator);
}

void SceneManager::EntryScene()
{
	const std::string scenesPath = "Asset/Data/Scene.json";

	const auto& root = _jsonLoader.LoadJsonData(scenesPath);

	for (auto& scene : root.at("Scenes"))
	{
		auto data = _jsonLoader.LoadSceneData(scene);
		
		const auto& it = MakeSceneFunctionTable.find(data.sceneName);
		//作り方が登録されていない名前は飛ばす(returnすると、タイトルへの切り替えまで行われず、最初のシーンが無いまま落ちる)
		if (it == MakeSceneFunctionTable.end())continue;

		data.scene = it->second;	
		_sceneList[data.sceneName] = data;
	}
	ChangeScene(SceneName::Title);
}