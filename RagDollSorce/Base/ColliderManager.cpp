#include "CollisionManager.h"
#include "Collider.h"
#include "GameObject.h"
#include "DebugUtility.h"

void CollisionManager::AddCollider(Collider* collider)
{
    //Idでだけで探せるようにMapに登録
    collider->SetID(_nextColliderID++);
    _colliderMap[collider->GetID()] = collider;
}

void CollisionManager::Intersects()
{
    InitializePair();

    InsertOctree();

    for (auto& it : _colliderMap)
    {
        auto& collider = it.second;

        //動いていない物(床・眠っている箱など)は、自分から相手を探さない。
        //Octreeには入っているので、動いている相手が見つけてくれる(動かない物どうしは、調べなくて済む)
        if (!collider->IsMoving())
            continue;

        std::vector<Collider*> candidates;

        _octree.Query(collider->GetMin(),collider->GetMax(),candidates);


        for (auto& other : candidates)
        {
            auto colliderA = collider;
            auto colliderB = other;

            //必ず小さい->大きいの順番にする
            if (GetColliderPriority(colliderA->GetType()) > GetColliderPriority(colliderB->GetType()))
            {
                std::swap(colliderA, colliderB);
            }
            auto pair = Collision::MakePair(colliderA->GetID(), colliderB->GetID());

            //同一ID・重複ペア・隣接ボーン等、判定不要なペアを除外
            if (!CheckShouldCollider(pair, colliderA, colliderB))
                continue;

            bool hit = false;
            CollisionResult result;

            auto typePairKey = std::make_pair(colliderA->GetType(), colliderB->GetType());
            auto foundFunctionPair = _collisionFunctionMap.find(typePairKey);

            //この型の組み合わせに対応する判定関数が登録されているか確認
            if (foundFunctionPair != _collisionFunctionMap.end())
            {
                hit = foundFunctionPair->second.checkFunction(colliderA, colliderB);

                if (hit)
                {
                    result = foundFunctionPair->second.resolveFunction(colliderA, colliderB);
                }
            }

            if (hit)
            {
                _currentCollisions.insert(pair);
                _collisionResults[pair] = result;
            }
        }

    }


}
bool CollisionManager::CheckShouldCollider(
    const Collision::Pair& pair,Collider* colliderA, Collider* colliderB)
{
    //同じコライダーじゃないか
    if (colliderA->GetID() == colliderB->GetID())
        return false;

    //すでに衝突処理をしたペアか
    if (_currentCollisions.contains(pair))
        return false;
    
    //すでに判定したペアか
    if (checkedPairs.contains(pair))
        return false;

    checkedPairs.insert(pair);

    //ラグドールの隣接ボーンなど、衝突させるべきでない組み合わせは除外
    if (!Collision::ShouldCollider(*colliderA, *colliderB))
        return false;

    return true;
}

void CollisionManager::ResolveEvent()
{
    //Enter / Stay 通知
    for (const auto& pair : _currentCollisions)
    {
        bool wasColliding = _previousCollisions.contains(pair);

        auto colliderA = FindCollider(pair.first);
        auto colliderB = FindCollider(pair.second);
        if (!colliderA || !colliderB)
            continue;

        CollisionResult result;
        auto it = _collisionResults.find(pair);
        if (it != _collisionResults.end())
            result = it->second;

        auto ownerA = colliderA->GetOwner();
        auto ownerB = colliderB->GetOwner();

        if (wasColliding)
        {
            if (ownerA) ownerA->OnCollisionStay(colliderB, result);
            if (ownerB) ownerB->OnCollisionStay(colliderA, result);
        }
        else
        {
            if (ownerA) ownerA->OnCollisionEnter(colliderB, result);
            if (ownerB) ownerB->OnCollisionEnter(colliderA, result);
        }
    }
    //Exit
    //---------------------

    for (const auto& pair : _previousCollisions)
    {
        if (_currentCollisions.contains(pair))
            continue;

        auto colliderA = FindCollider(pair.first);
        auto colliderB = FindCollider(pair.second);

        if (!colliderA || !colliderB)
            continue;

        CollisionResult result;
        auto it = _collisionResults.find(pair);
        if (it != _collisionResults.end())
            result = it->second;

        auto ownerA = colliderA->GetOwner();
        auto ownerB = colliderB->GetOwner();

        if (ownerA)
        colliderA->GetOwner()->OnCollisionExit(colliderB, result);

        if (ownerB)
        colliderB->GetOwner()->OnCollisionExit(colliderA, result);
    }
}
void CollisionManager::DebugCollider()
{
    _debugDraw->ClearList();
    for (const auto& it : _colliderMap)
    {
        it.second->DrawDebug(_debugDraw);
    }
}
void CollisionManager::ReleaseCollider(int id)
{
    _colliderMap.erase(id);

    //Octreeは次のIntersects()まで前回の生ポインタを持ち続けるので、
    //破棄済みのColliderをRayCastが触らないように空にしておく(次のInsertOctreeで再構築される)
    _octree.Clear();
}

void CollisionManager::InsertOctree()
{
    _octree.Clear();

    //ColliderをOctreeに登録
    for (auto& collider : _colliderMap)
    {
        _octree.Insert(collider.second);
    }

}
void CollisionManager::InitializePair()
{
    //通知用に取っておく
    _previousCollisions = _currentCollisions;

    //ペアの初期化
    _currentCollisions.clear();
    _collisionResults.clear();
    checkedPairs.clear();
}

bool CollisionManager::RayCast(const XMFLOAT3& origin, const XMFLOAT3& dir, float maxDistance)
{
    std::vector<Collider*> candidates;

    //Rayに対してOctreeから候補を取得
    _octree.QueryRay(origin, dir, candidates);

    float closestDistance = maxDistance;
    bool hit = false;
    for (auto& coll : candidates)
    {
        float distance;
        XMFLOAT3 tempHitPosition;

        if (coll->RayCast(origin, dir, distance, tempHitPosition))
        {
            //一番近いコライダーを返す
            if (distance < closestDistance)
            {
                closestDistance = distance;
                hit = true;
            }
        }
    }
    return hit;
}

bool CollisionManager::RayCast(const XMFLOAT3& origin,
    const XMFLOAT3& direction,const float& maxDistance,XMFLOAT3& hitPosition)
{
    std::vector<Collider*> candidates;

    //Rayに対してOctreeから候補を取得
    _octree.QueryRay(origin, direction, candidates);

    float closestDistance = maxDistance;
    bool hit = false;
    for (auto& coll : candidates)
    {
        float distance;
        XMFLOAT3 tempHitPosition;

        if (coll->RayCast( origin, direction, distance, tempHitPosition))
        {
            //一番近いコライダーを返す
            if (distance < closestDistance)
            {
                closestDistance = distance;
                hitPosition = tempHitPosition;
                hit = true;
            }
        }
    }
    return hit;
}