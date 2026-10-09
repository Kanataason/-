#pragma once
#include <iostream>
#include <unordered_map>
#include "Datas.h"
#include "GameObject.h"
#include "ServiceLocator.h"
#include "ColliderRegistry.h"

//ObjectDataから、コンポーネントを組み立てたGameObjectを作る
class GameObjectFactory
{
public:
	GameObjectFactory() = default;

	void SetServiceLocator(const ServiceLocator& locator) { _locator = locator; };
	//sceneに存在しているオブジェクトを生成する
	std::unique_ptr<GameObject> CreateObject(ObjectData data);

private:
	//ObjectTypeごとの組み立て処理。_objectBuilderTableから呼ばれる
	void BuildPlayer(GameObject& object, const ObjectData& data);
	void BuildEnemy(GameObject& object, const ObjectData& data);
	//移動・攻撃・HPは雑魚と同じコンポーネントを使い、行動だけBossControllerで変える
	void BuildBoss(GameObject& object, const ObjectData& data);

	//サイズをオブジェクトごとに設定、bulletで誰が撃ったかの判断するteamを渡す
	void SetShapeData(GameObject& gameObject, const ObjectData& objectData,Team team);

	//メンバー関数ポインタ
	using ObjectBuilder = void (GameObjectFactory::*)(GameObject&, const ObjectData&);
	inline static const std::unordered_map<ObjectType, ObjectBuilder> _objectBuilderTable =
	{
		{ ObjectType::Player, &GameObjectFactory::BuildPlayer },
		{ ObjectType::Enemy,  &GameObjectFactory::BuildEnemy  },
		{ ObjectType::Boss,&GameObjectFactory::BuildBoss}
	};

	//PlayerをEnemyに知らせるため
	ServiceLocator _locator{};

};