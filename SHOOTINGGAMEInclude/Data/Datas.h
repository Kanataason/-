#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <variant>
#include <vector>

#include "Transform.h"

enum class SceneName
{
	EGameTitle,
	EGameScene,
	EGameOver,
	EGameClear,
	None
};
enum class ObjectType
{
	Player,
	Enemy,
	Ground,
	Boss,
	None
};
enum class CharacterType
{
	Player,
	MoveEnemy,
	ReflectShotEnemy,
	SpreadEnemy,
	Boss,
	None
};
enum class MovePattern
{
	EnterAndStay, 
	PassThrough
};

class BaseScene;

enum class SoundId
{
	PlayerShot,
	PlayerShotGun,
	PlayerHit, 
	CharacterExplosion,
	BossExplosion,
	Overheat,
	BgmGame, 
	BgmBoss,
	BgmGameClear,
	BgmGameOver,
	Count
};
//すべてJsonからの読みだした値を保存するのに使う
struct SoundData
{
	SoundId id;
	std::string path;
	int volume = 255;
	int bufferNum = 1;
};

struct SceneData
{
	SceneName name = SceneName::None;
	std::string path = "";
	std::function<std::unique_ptr<BaseScene>()> scene{};
};

struct SphereData
{
	float radius = 0.0f;
};
struct BoxData
{
	Vector2 halfSize{};
};
using SizeData = std::variant<SphereData, BoxData>;

struct StatusData
{
	CharacterType characterType = CharacterType::None;
	MovePattern movePattern = MovePattern::EnterAndStay;

	std::string imagePath = "";
	bool isMove = true;
	float health = 0;
};

//オブジェクトのデータ構造体
struct ObjectData
{
	ObjectType name = ObjectType::None;
	std::vector<int> attackType;
	Transform transform{};
	SizeData sizeData{};

	StatusData statusData{};
};
struct SpawnData
{
	ObjectData objectData;   //敵そのものの情報
	bool isRandomX = true;   //出現のルール
	int maxCount = -1;       //-1 = 無制限
};