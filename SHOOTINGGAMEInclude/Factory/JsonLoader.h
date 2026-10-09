#pragma once
#include <string>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <vector>

#include "Transform.h"
#include "Datas.h"
#include "json.hpp"


using json = nlohmann::json;

class JsonLoader
{
public:
	//シーンデータを読み込む
	json LoadJsonData(const std::string& path)const;

	//オブジェクトデータを読み込む
	ObjectData LoadObjectData(const json& root)const;

	SceneData LoadScenes(const json& root)const;
	ObjectData LoadSceneData(const json& root)const;
	std::vector<SpawnData> LoadSpawnData(const std::string& path) const;

	//ボス1体分のデータを読み込む
	ObjectData LoadBossData(const std::string& path) const;
	std::vector<SoundData> LoadSoundData(const json& root)const;

	//当たり判定やオブジェクトのサイズを読み取る
	static SizeData LoadRadius(const json& root);
	static SizeData LoadBoxMinAndMax(const json& root);

private:
	void LoadAttackType(const json& root, ObjectData& data)const;
	StatusData LoadStatusData(const json& root)const;
	Transform LoadTransform(const json& root)const;
	//各変換をする関数
	MovePattern ToMovePattern(const std::string& name)const;
	CharacterType ToCharacterType(const std::string& name)const;
	SceneData ToSceneData(const std::string& name)const;
	ObjectType ToObjectType(const std::string& name)const;
	SoundId ToSoundId(const std::string& name)const;
};