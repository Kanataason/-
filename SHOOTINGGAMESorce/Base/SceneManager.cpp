#include "SceneManager.h"
#include "BulletManager.h"

#include "GameTitle.h"
#include "GameScene.h"

void SceneManager::Initialize(GameObjectFactory& objectfactory, ServiceLocator& locator)
{
	//シーンマネージャーにオブジェクトの生成スクリプトと、ロケーターを持たせてシーン移行時に渡せるようにする
	_factory = &objectfactory;
	_locator = locator;
	objectfactory.SetServiceLocator(locator);
	EntryScene(objectfactory);

}
void SceneManager::EntryScene(GameObjectFactory& objectfactory)
{
	const auto& json = _jsonLoader.LoadJsonData("JsonData/Scenes.json");
	for (auto& root : json.at("Scenes"))
	{
		auto data = _jsonLoader.LoadScenes(root);
		_sceneDataList[data.name] = data;
	}
	RequestChange(SceneName::EGameTitle);
}

void SceneManager::Update(float deltaTime)
{
	if (_nextScene != SceneName::None) ApplyChange();//フレームの最初に切り替える
	if (_currentScene) _currentScene->Update(deltaTime);
}


void SceneManager::ApplyChange()
{
	const SceneData& data = _sceneDataList.at(_nextScene);
	_nextScene = SceneName::None;

	_locator.bulletManager->GetPool().ReleaseAll();//前のシーンの弾を消す
	_currentScene.reset();//前のシーンを破棄。コライダーも登録解除される

	//初期化処理
	_currentScene = data.scene();
	_sceneLoader.LoadObjects(_currentScene.get(), data.path, &_jsonLoader, *_factory);
	_currentScene->Initialize(_locator);
}