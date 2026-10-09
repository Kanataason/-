#pragma once
#include <DirectXMath.h>
#include <algorithm>

#include "Component.h"

#include "VectorUtility.h"
using namespace DirectX;

class PhysicsSystem;

//Colliderの形状種別
enum ColliderType
{
    CapsuleColliderType,
    BoxColliderType,
    SphereColliderType
};
enum class CollisionGroup
{
    None = 0,
    Ragdoll = 1,
    Camera = 2,
    Ground = 3,
    Target = 4
};


class Collider : public Component
{
public:
    //CollisionManagerが割り振る一意なID(未登録時は-1)
    void SetID(int id)
    {
        _id = id;
    }

    int GetID() const
    {
        return _id;
    }

    //登録先のPhysicsSystemを覚えておく(デストラクタで、自動的に登録解除するため)
    void SetRegistry(PhysicsSystem* registry)
    {
        _registry = registry;
    }

    //同じグループ同士の衝突可否などに使う分類ID
    void SetCollisionGroup(CollisionGroup group)
    {
        _collisionGroup = group;
    }

    CollisionGroup GetCollisionGroup() const
    {
        return _collisionGroup;
    }

    //今、動いているか(動く物で、眠っていない)。動いていない物は、自分から相手を探しに行かない
    //(動いている相手が、動かない物を見つけてくれるので、組み合わせは漏れない)
    //PhysicsSystemが、毎サブステップ更新する。RigidBodyのない物(床など)は、いつもfalse
    void SetMoving(bool moving) { _isMoving = moving; }
    bool IsMoving() const { return _isMoving; }

    //トリガー化する(当たり判定はするが物理的な押し出しをしない)
    void OnTrigger() { isTrigger = true; }
    void OffTrigger() { isTrigger = false; }
    bool GetTrigger() const { return isTrigger; }

    void SetPosition(const XMFLOAT3& position) { Position = position; }
    virtual const XMFLOAT3& GetPosition()const { return Position; }

    void SetRotation(const Quaternion& rotation) { Rotation = rotation; }
    virtual const Quaternion& GetRotation() const { return Rotation; }

    //デバッグ用のワイヤーフレーム描画(形状ごとに実装)
    virtual void DrawDebug(DebugDraw* debugDraw) const = 0;

    //Positionからのオフセット(中心をずらしたい場合に使う)をセットする
    void SetCenter(const XMFLOAT3& offset) { _center = offset; }

    //ローカルの基準軸(カプセルの伸びる方向など)をセットする
    void SetLocalAxis(const Vector3& axis)
    {
        _localAxis = axis;
        _localAxis.Normalize();
    }

    //Position + _center = 実際の中心座標
    virtual XMFLOAT3 GetCenter()const
    {
        return
        {
            Position.x + _center.x,
            Position.y + _center.y,
            Position.z + _center.z
        };
    }

    //AABBの最小/最大座標(形状ごとに実装)
    virtual XMFLOAT3 GetMin()const = 0;
    virtual XMFLOAT3 GetMax()const = 0;

    //登録済みのCollisionManagerがあれば、自動的に登録を解除する(定義はCollider.cpp)
    virtual ~Collider();

    //形状種別を返す(衝突判定の関数テーブルのキーとして使われる)
    virtual ColliderType GetType() const = 0;

    //このColliderとRayとの交差判定(形状ごとに実装)
    virtual bool RayCast(
        const XMFLOAT3& origin,
        const XMFLOAT3& direction,
        float& distance,
        XMFLOAT3& hitPosition) = 0;
private:
    int _id = -1;
    CollisionGroup _collisionGroup = CollisionGroup::None;
    PhysicsSystem* _registry = nullptr;
    bool _isMoving = false;
protected:
    bool isTrigger = false;

    Vector3 _localAxis = { 0, 1, 0 };
    Vector3 _localOffset = { 0, 0, 0 };

    Quaternion Rotation{};
    XMFLOAT3 Position{};
    XMFLOAT3 _center{};
};

