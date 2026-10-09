#pragma once
#include <memory>

#include "ObjectType.h"
#include "ModelInfo.h"
#include "ModelManager.h"
#include "MaterialManager.h"

//ObjectDataから、描画できる状態のモデルを用意する
//(モデルの読み込み + 各メッシュへのマテリアル/テクスチャの割り当て)
class ModelProvider
{
public:
	ModelProvider(ModelManager& modelManager, MaterialManager& materialManager) :
		_modelManager(modelManager), _materialManager(materialManager) {}

	//失敗(未対応の形・FBXの読み込みエラーなど)したらnullptrを返す
	std::shared_ptr<ModelDatas> Prepare(const ObjectData& data);

private:
	ModelManager& _modelManager;
	MaterialManager& _materialManager;
};
