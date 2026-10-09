#pragma once
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "Octree.h"
#include "Collision.h"

class Collider;

namespace 
{
	//関数ポインタで宣言
	using CollisionCheckFunction = bool (*)(Collider*, Collider*);
	using CollisionResolveFunction = CollisionResult(*)(Collider*, Collider*);

	struct CollisionFunctionPair
	{
		CollisionCheckFunction   checkFunction;
		CollisionResolveFunction resolveFunction;
	};
	struct ColliderTypePairHash
	{
		size_t operator()(const std::pair<ColliderType, ColliderType>& typePair) const
		{
			return (static_cast<size_t>(typePair.first) << 16) ^ static_cast<size_t>(typePair.second);
		}
	};

	//コライダーに合わせた、判定と押し出しを返す
	template <typename ColliderTypeA, typename ColliderTypeB>
	CollisionFunctionPair MakeCollisionFunctionPair()
	{
		return CollisionFunctionPair{
			[](Collider* colliderA, Collider* colliderB) -> bool
			{
				return Collision::CastColliderTypeAndCheckCollision<ColliderTypeA, ColliderTypeB>(colliderA, colliderB);
			},
			[](Collider* colliderA, Collider* colliderB) -> CollisionResult
			{
				return Collision::CastColliderTypeAndResolve<ColliderTypeA, ColliderTypeB>(colliderA, colliderB);
			}
		};
	}
	//処理ペアをテーブルに登録
	static const std::unordered_map<std::pair<ColliderType, ColliderType>,
		CollisionFunctionPair,
		ColliderTypePairHash > _collisionFunctionMap =
	{
		{ {ColliderType::SphereColliderType,  ColliderType::SphereColliderType},  MakeCollisionFunctionPair<SphereCollider,  SphereCollider>()  },
		{ {ColliderType::BoxColliderType,     ColliderType::SphereColliderType},  MakeCollisionFunctionPair<BoxCollider,     SphereCollider>()  },
		{ {ColliderType::BoxColliderType,     ColliderType::BoxColliderType},     MakeCollisionFunctionPair<BoxCollider,     BoxCollider>()     },
		{ {ColliderType::CapsuleColliderType, ColliderType::SphereColliderType},  MakeCollisionFunctionPair<CapsuleCollider, SphereCollider>()  },
		{ {ColliderType::CapsuleColliderType, ColliderType::BoxColliderType},     MakeCollisionFunctionPair<CapsuleCollider, BoxCollider>()     },
		{ {ColliderType::CapsuleColliderType, ColliderType::CapsuleColliderType}, MakeCollisionFunctionPair<CapsuleCollider, CapsuleCollider>() },
	};
}


struct PairHash
{
	//ペア専用の値を作る
	std::size_t operator()(const std::pair<int, int>& p) const
	{
		return std::hash<int>{}(p.first) ^
			(std::hash<int>{}(p.second) << 1);
	}
};
class CollisionManager
{
public :

	CollisionManager(DebugDraw* debug) :_debugDraw(debug), _octree({ -100.0f, -100.0f, -100.0f }, { 100.0f,  100.0f,  100.0f }, 0)
	{
	}

	void AddCollider(Collider* collider);

	//Octreeでわけてから衝突判定
	void Intersects();

	//衝突を通知
	void ResolveEvent();

	//デバッグの線を出す
	void DebugCollider();


	bool RayCast(const XMFLOAT3& origin, const XMFLOAT3& dir, float maxDistance);
	bool RayCast(const XMFLOAT3& origin,const XMFLOAT3& direction,const float& maxDistance,XMFLOAT3& hitPosition);

	const std::unordered_map<std::pair<int, int>, CollisionResult, PairHash>& GetCollisionResults() const
	{
		return _collisionResults;
	}

	void ReleaseCollider(int id);

	//登録されているコライダーすべて(床や、眠っている箱も含む)
	const std::unordered_map<int, Collider*>& GetColliders() const { return _colliderMap; }

private:
	Collider* FindCollider(int id)
	{
		for (auto& collider : _colliderMap)
		{
			if (collider.second->GetID() == id)
			{
				return collider.second;
			}
		}

		return nullptr;
	}

	//ペアの初期化
	void InitializePair();
	//Octreで処理するコライダー設定
	void InsertOctree();

	//既に処理をしたか判定
	bool CheckShouldCollider(const Collision::Pair& pair,
		Collider* colliderA, Collider* colliderB);

	int GetColliderPriority(ColliderType type)
	{
		switch (type)
		{
		case ColliderType::CapsuleColliderType: return 0;
		case ColliderType::BoxColliderType:     return 1;
		case ColliderType::SphereColliderType:  return 2;
		}
		return 99;
	}
private:
	int _nextColliderID = 0;


	DebugDraw* _debugDraw = nullptr;

	//衝突判定するコライダー
	std::unordered_map<int, Collider*> _colliderMap;

	//処理済みペア
	std::unordered_set<std::pair<int, int>, PairHash> checkedPairs;

	//現在処理している衝突ペア
	std::set<std::pair<int, int>> _currentCollisions;

	//一フレーム前の衝突ペア
	std::set<std::pair<int, int>> _previousCollisions;
		
	//衝突結果
	std::unordered_map<std::pair<int, int>, CollisionResult, PairHash>_collisionResults;

	Octree _octree;
};