#pragma once
#include <ThirdParty/nlohmann/json.hpp>
#include <string>
#include <vector>
#include <DirectXMath.h>
#include <unordered_map>

using json = nlohmann::json;
#include "ObjectType.h"
#include "Transform.h"

using namespace DirectX;

class JsonLoader
{
public :
	json LoadJsonData(const std::string& assetPath)const;

	ObjectData LoadGameObjectData(const json& root);

	//"count"で並べる指定があるとき、並べたぶんのObjectDataに増やして返す(指定がなければ、1個だけ)
	std::vector<ObjectData> ExpandRepeat(const ObjectData& base)const;

	BaseSceneData LoadSceneData(const json& root)const;
private:

	//各Jsonから値を取り出す
	ObjectType LoadObjectType(const json& root)const;
	PrimitiveShape LoadPrimitiveShape(const json& root)const;
	Attribute LoadObjectAttribute(const json& root)const;
	std::string LoadPath(const json& root)const;
	Transform LoadTransform(const json& root);
	std::string LoadMaterial(const json& root)const;
	void LoadAnimationPath(const json& root, std::vector<AnimationData>& animationPaths);
	void LoadRepeat(const json& root, ObjectData& data)const;

	//取り出した値を変換する
	ObjectType ToObjectType(const std::string& type)const;
	PrimitiveShape ToPrimitiveShape(const std::string& shape)const;
	Attribute ToObjectAttribute(const std::string& attribute)const;
	SceneName ToSceneName(const std::string& name)const;
};