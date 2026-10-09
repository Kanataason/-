#pragma once
#undef min
#undef max

#include <algorithm>
#include <DirectXMath.h>
#include "Collider.h"
#include "DebugUtility.h"


using namespace DirectX;

class CapsuleCollider : public Collider
{
public:
    CapsuleCollider() = default;
    ~CapsuleCollider() override = default;

    //Colliderの種類を取得
    ColliderType GetType() const override
    {
        return ColliderType::CapsuleColliderType;
    }

    //カプセルの半径を設定
    void SetRadius(float radius)
    {
        _radius = radius;
    }

    //カプセルの全体の高さを設定
    void SetHeight(float height)
    {
        _height = height;
    }

    //カプセルの半径を取得
    float GetRadius() const
    {
        return _radius;
    }

    //カプセルの全体の高さを取得
    float GetHeight() const
    {
        return _height;
    }

    //カプセル中央の円柱部分の半分の長さを取得
    //カプセル全体の高さから上下の半球部分の直径を除く
    float GetHelfLength() const
    {
        return _height * 0.5f - _radius;
    }

    //カプセルの中心軸方向をワールド空間で取得
    Vector3 GetAxis() const
    {
        //ローカル軸をColliderの回転でワールド空間へ変換
        Vector3 axis = Rotation * _localAxis;

        //軸方向を正規化
        axis.Normalize();

        return axis;
    }

    //カプセルの中心軸の始点を取得
    DirectX::XMFLOAT3 GetStart() const
    {
        //Colliderの中心位置を取得
        auto pos = Vector::fromF3ToV3(GetCenter());

        //ローカルオフセットを回転させてワールド座標へ変換
        pos += Rotation * _localOffset;

        //カプセルの中心軸方向を取得
        Vector3 axis = GetAxis();

        //中心から端点までの距離
        float halfLength = GetHelfLength();

        //中心から軸方向の反対側へ移動して始点を求める
        Vector3 start = pos - axis * halfLength;

        return
        {
            start.x,
            start.y,
            start.z
        };
    }

    //カプセルの中心軸の終点を取得
    DirectX::XMFLOAT3 GetEnd() const
    {
        //Colliderの中心位置を取得
        Vector3 pos = Vector::fromF3ToV3(GetCenter());

        //ローカルオフセットを回転させてワールド座標へ変換
        pos += Rotation * _localOffset;

        //カプセルの中心軸方向を取得
        Vector3 axis = GetAxis();

        //中心から端点までの距離
        float halfLength = GetHelfLength();

        //中心から軸方向へ移動して終点を求める
        Vector3 end = pos + axis * halfLength;

        return
        {
            end.x,
            end.y,
            end.z
        };
    }

    //カプセルを囲むAABBの最小座標を取得
    XMFLOAT3 GetMin() const override
    {
        XMFLOAT3 start = GetStart();
        XMFLOAT3 end = GetEnd();
        float radius = GetRadius();

        //始点と終点の各軸の最小値から半径分だけ外側へ広げる
        return
        {
            std::min(start.x, end.x) - radius,
            std::min(start.y, end.y) - radius,
            std::min(start.z, end.z) - radius
        };
    }

    //カプセルを囲むAABBの最大座標を取得
    XMFLOAT3 GetMax() const override
    {
        XMFLOAT3 start = GetStart();
        XMFLOAT3 end = GetEnd();
        float radius = GetRadius();

        //始点と終点の各軸の最大値から半径分だけ外側へ広げる
        return
        {
            std::max(start.x, end.x) + radius,
            std::max(start.y, end.y) + radius,
            std::max(start.z, end.z) + radius
        };
    }

    //デバッグ用にカプセルを描画
    void DrawDebug(DebugDraw* debugDraw) const override
    {
        debugDraw->DrawCapsule(
            GetStart(),
            GetEnd(),
            GetRadius(),
            { 0, 1, 0, 1 });
    }

    //レイとカプセルの交差判定
    //※現在は未実装
    bool RayCast(
        const XMFLOAT3& origin,
        const XMFLOAT3& direction,
        float& distance,
        XMFLOAT3& hitPosition) override
    {
        return false;
    }

private:
    //カプセルの半径
    float _radius = 0.16f;

    //カプセル全体の高さ
    float _height = 1.0f;
};