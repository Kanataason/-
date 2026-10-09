#pragma once
#include "Collider.h"
#include "Transform.h"
#include "DebugUtility.h"

//球形のCollider
class SphereCollider : public Collider
{
public:
    SphereCollider() = default;
    ~SphereCollider() override = default;

    //Colliderの種類を取得
    ColliderType GetType() const override
    {
        return ColliderType::SphereColliderType;
    }

    //Sphereを囲むAABBの最小座標を取得
    XMFLOAT3 GetMin() const override
    {
        auto pos = GetCenter();

        //中心座標から半径分だけ各方向へ広げる
        return
        {
            pos.x - _radius,
            pos.y - _radius,
            pos.z - _radius
        };
    }

    //Sphereを囲むAABBの最大座標を取得
    XMFLOAT3 GetMax() const override
    {
        auto pos = GetCenter();

        //中心座標から半径分だけ各方向へ広げる
        return
        {
            pos.x + _radius,
            pos.y + _radius,
            pos.z + _radius
        };
    }

    //レイとSphereの交差判定
    //※現在は未実装
    bool RayCast(
        const XMFLOAT3& origin,
        const XMFLOAT3& direction,
        float& distance,
        XMFLOAT3& hitPosition) override
    {
        return false;
    }

    //デバッグ用にSphereを描画
    void DrawDebug(DebugDraw* debugDraw) const override
    {
        debugDraw->DrawSphere(
            GetPosition(),
            GetRadius(),
            { 0, 1, 0, 1 });
    }

    //Sphereの半径を設定
    void SetRadius(float radius)
    {
        _radius = radius;
    }

    //Sphereの半径を取得
    float GetRadius() const
    {
        return _radius;
    }

private:
    //Sphereの半径
    float _radius = 0.5f;
};