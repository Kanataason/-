#pragma once
#include <type_traits>
#include <algorithm>
#include <unordered_set>
#include <utility>

#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include "Collider.h"

#include "VectorUtility.h"
#include "CollisionResult.h"
class DebugDraw;

class Collision
{
public:
    //特定の当たり判定だけを除外する処理-----

    struct PairHash
    {
        //ペア専用の値を作る
        std::size_t operator()(const std::pair<int, int>& p) const
        {
            return std::hash<int>{}(p.first) ^
                (std::hash<int>{}(p.second) << 1);
        }
    };
    using Pair = std::pair<int, int>;

    inline static std::unordered_set<Pair,PairHash> ignorePairs;
    //小さい順にする
    static Pair MakePair(int idA, int idB)
    {
        if (idA < idB)
            return { idA, idB };

        return { idB, idA };
    }

    static bool ShouldCollider(
        const Collider& a,
        const Collider& b)
    {
        Pair pair = MakePair(
            a.GetID(),
            b.GetID());

        if (ignorePairs.contains(pair))
            return false;

        return true;
    }

    static void IgnoreCollision(int idA, int idB)
    {
        ignorePairs.insert(
            MakePair(idA, idB));
    }
    //---------------------------------
    //当たり判定
    static bool Check(const SphereCollider& a, const SphereCollider& b);
    static bool Check( const BoxCollider& b, const SphereCollider& a);
    static bool Check(const BoxCollider& a, const BoxCollider& b);
    static bool Check(const CapsuleCollider& a, const CapsuleCollider& b);
    static bool Check(const CapsuleCollider& a, const SphereCollider& b);
    static bool Check(const CapsuleCollider& a, const BoxCollider& b);

    //コライダーコンポーネントを継承しているものじゃないと受け付けないようにする
    template<typename T,typename To>
    requires std::is_base_of_v<Collider, T>&& std::is_base_of_v<Collider, To>
    static bool CastColliderTypeAndCheckCollision(Collider* a, Collider* b)
    {
        auto colliderA = dynamic_cast<T*>(a);
        auto colliderB = dynamic_cast<To*>(b);

        if (!colliderA || !colliderB)
            return false;

        return Collision::Check(*colliderA, *colliderB);
    }

    template<typename T, typename To>
        requires std::is_base_of_v<Collider, T>&& std::is_base_of_v<Collider, To>
    static CollisionResult CastColliderTypeAndResolve(Collider* a, Collider* b)
    {
        auto colliderA = dynamic_cast<T*>(a);
        auto colliderB = dynamic_cast<To*>(b);

        if (!colliderA || !colliderB)
            return {};

        return Collision::Resolve(*colliderA, *colliderB);
    }
    static bool RayCast(const XMFLOAT3& origin,const XMFLOAT3& direction,
        const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2, DebugDraw* debugDraw);

    //押し出し処理
    static CollisionResult Resolve(BoxCollider& a, BoxCollider& b);
    static CollisionResult Resolve(BoxCollider& a, SphereCollider& b);
    static CollisionResult Resolve(SphereCollider& a, SphereCollider& b);
    static CollisionResult Resolve(CapsuleCollider& a, CapsuleCollider& b);
    static CollisionResult Resolve(CapsuleCollider& a, SphereCollider& b);
    static CollisionResult Resolve(CapsuleCollider& a, BoxCollider& b);

};
struct Ray
{
    DirectX::XMFLOAT3 origin;
    DirectX::XMFLOAT3 direction;
};