#pragma once
#include <string>
#include <vector>

#include "JsonLoader.h"

class BaseScene;
class GameObjectFactory;
class CharacterRoster;

class SceneLoader
{
public :
	//characters を渡すと、JSONのプレイヤーを、選んだキャラクターのモデルに差し替えて作る(nullptrなら、JSONのまま)
	void LoadSceneGameObject(BaseScene* scene,const std::string& loadPath,
		JsonLoader& loader,GameObjectFactory& factory,const CharacterRoster* characters = nullptr);
private:
	StageData LoadSceneStageInfo(const json& root);
private :
	JsonLoader _jsonLoader;
};