#pragma once
#undef min
#undef max
#define NOMINMAX
#include <Windows.h>

#include "Collider.h"
#include "DebugUtility.h"
#include  <algorithm>
constexpr float HALF = 0.5f;


class BoxCollider : public Collider
{
public:
	BoxCollider() = default;
	~BoxCollider()override = default;

	ColliderType GetType() const override
	{
		return ColliderType::BoxColliderType;
	}

	void SetSize(const DirectX::XMFLOAT3& size) { _size = size; }
	const DirectX::XMFLOAT3& GetSize() const { return _size; }

	//各軸の半分のサイズ(中心からの距離)
	DirectX::XMFLOAT3 GetHalfSize()const { return { _size.x * HALF,_size.y * HALF,_size.z * HALF }; }

	//AABBの最小座標(中心 - 半サイズ)
	DirectX::XMFLOAT3 GetMin() const override
	{
		auto center = GetCenter();
		DirectX::XMFLOAT3 halfSize = GetHalfSize();

		return
		{
			center.x - halfSize.x,
			center.y - halfSize.y,
			center.z - halfSize.z
		};
	}

	//Rayとこのボックス(AABB)との交差判定
	//origin: レイの始点
	//direction: レイの方向
	//distance: ヒットした場合、始点からの距離が入る
	//hitPosition: ヒットした場合、ワールド座標が入る
	//戻り値: 交差していればtrue
	bool RayCast(const XMFLOAT3& origin, const XMFLOAT3& direction, float& distance, XMFLOAT3& hitPosition) override
	{
		constexpr float EPSILON = 0.00001f;

		float tMax = FLT_MAX;
		float tMin = 0.0f;

		XMFLOAT3 max = GetMax();
		XMFLOAT3 min = GetMin();

		//X軸方向のスラブ判定
		if (std::abs(direction.x) < EPSILON)
		{
			//レイがX軸に対して平行な場合、始点がX範囲内かだけ確認
			if (origin.x < min.x || origin.x > max.x)
			{
				return false;
			}
		}
		else
		{
			float t1X = (min.x - origin.x) / direction.x;
			float t2X = (max.x - origin.x) / direction.x;

			float tMinX = std::min(t1X, t2X);
			float tMaxX = std::max(t1X, t2X);

			tMin = std::max(tMinX, tMin);
			tMax = std::min(tMaxX, tMax);

			if (tMin > tMax)
				return false;
		}

		//Y軸方向のスラブ判定
		if (std::abs(direction.y) < EPSILON)
		{
			if (origin.y < min.y || origin.y > max.y)
			{
				return false;
			}
		}
		else
		{
			float t1 = (min.y - origin.y) / direction.y;
			float t2 = (max.y - origin.y) / direction.y;

			float tMinY = std::min(t1, t2);
			float tMaxY = std::max(t1, t2);

			tMin = std::max(tMin, tMinY);
			tMax = std::min(tMax, tMaxY);

			if (tMin > tMax)
				return false;
		}

		//Z軸方向のスラブ判定
		if (std::abs(direction.z) < EPSILON)
		{
			if (origin.z < min.z || origin.z > max.z)
			{
				return false;
			}
		}
		else
		{
			float t1 = (min.z - origin.z) / direction.z;
			float t2 = (max.z - origin.z) / direction.z;

			float tMinZ = std::min(t1, t2);
			float tMaxZ = std::max(t1, t2);

			tMin = std::max(tMin, tMinZ);
			tMax = std::min(tMax, tMaxZ);

			if (tMin > tMax)
				return false;
		}

		//交差区間の手前側(tMin)が最初にヒットする位置
		distance = tMin;

		XMVECTOR o = XMLoadFloat3(&origin);
		XMVECTOR d = XMLoadFloat3(&direction);

		XMStoreFloat3(
			&hitPosition,
			o + d * distance
		);

		return true;
	};

	//AABBの最大座標(中心 + 半サイズ)
	DirectX::XMFLOAT3 GetMax()const override
	{
		auto center = GetCenter();
		DirectX::XMFLOAT3 halfSize = GetHalfSize();

		return
		{
			center.x + halfSize.x,
			center.y + halfSize.y,
			center.z + halfSize.z
		};
	}

	//デバッグ用にAABBをワイヤーフレームで描画(緑色固定)
	void DrawDebug(DebugDraw* debugDraw) const override
	{
		debugDraw->DrawBox(GetMin(), GetMax(), { 0,1,0,1 });
	}

private:
	DirectX::XMFLOAT3 _size = { 1.0,1.0,1.0 };//フルスケール
};