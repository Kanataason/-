#pragma once
#include <memory>

#include "ModelInfo.h"
#include "ObjectType.h"
#include "GameObject.h"
#include "ModelManager.h"
#include "MaterialManager.h"
#include "ModelProvider.h"
using namespace DirectX;


//JSONのデータ(ObjectData)から、GameObjectを1つ作る
class GameObjectFactory
{
public:
	GameObjectFactory(ModelManager& modelManager,MaterialManager& materialManager):
		_modelProvider(modelManager, materialManager){}

	//GameObjectを生成する
	std::unique_ptr<GameObject> CreateObject(const ObjectData& data);

private:
	ModelProvider _modelProvider;
};
