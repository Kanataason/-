#pragma once
#include <memory>

#include "ObjectType.h"
#include "ModelInfo.h"

class GameObject;

//役割(ObjectType)ごとの、コンポーネントの組み立て関数。
//BaseGameObjectFactory.cppの表に登録するだけでよい。
namespace ObjectBuilders
{
	void BuildProp(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model);
	void BuildPlayer(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model);
	void BuildCamera(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model);
	void BuildLight(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model);
	void BuildCannon(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model);
	void BuildScoreObject(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model);
}
