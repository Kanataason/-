#pragma once
#undef min
#undef max

#include <algorithm>
#include "Vector2.h"

#include "CircleCollider.h"
#include "BoxCollider.h"

class Collision
{
public:
	static bool CheckCollision(const Vector2& center,float radius,float halfThickness, const Vector2& segmentStart, const Vector2& segmentEnd)
	{
        Vector2 closestPoint;
        Vector2 segment = segmentEnd - segmentStart;
        float segmentLengthSquared = segment.x * segment.x + segment.y * segment.y;
        if (segmentLengthSquared == 0.0f) 
        {
            closestPoint = segmentStart;
        }
        else
        {
            Vector2 toPoint = center - segmentStart;
            float projectionRatio = (toPoint.x * segment.x + toPoint.y * segment.y) / segmentLengthSquared;
            projectionRatio = std::clamp(projectionRatio, 0.0f, 1.0f);  //線分の範囲内に収める
            closestPoint = segmentStart + segment * projectionRatio;
        }

        Vector2 difference = center - closestPoint;
        float hitDistance = radius + halfThickness;
        return difference.x * difference.x + difference.y * difference.y <= hitDistance * hitDistance;
	}
    static bool CheckCollision(const Vector2& center,const Vector2& halfSize, float halfThickness,const Vector2& segmentStart,
        const Vector2& segmentEnd)
    {
        Vector2 xpandedHalfSize = halfSize + Vector2(halfThickness, halfThickness);
        auto boxMin = center - xpandedHalfSize;
        auto boxMax = center + xpandedHalfSize;

        Vector2 direction = segmentEnd - segmentStart;
        float enterRatio = 0.0f;  //線分が四角形に入る位置
        float exitRatio = 1.0f;  //線分が四角形から出る位置

        auto checkAxis = [&](float start, float delta, float slabMin, float slabMax) -> bool
            {
                if (std::abs(delta) < 1e-6f)
                {
                    //この軸に平行な線：始点がこの軸の範囲内にあるかだけ見る
                    return start >= slabMin && start <= slabMax;
                }
                float ratioAtMin = (slabMin - start) / delta;
                float ratioAtMax = (slabMax - start) / delta;
                if (ratioAtMin > ratioAtMax) { std::swap(ratioAtMin, ratioAtMax); }

                enterRatio = std::max(enterRatio, ratioAtMin);
                exitRatio = std::min(exitRatio, ratioAtMax);
                return enterRatio <= exitRatio;  //区間が残っていれば、まだ当たる可能性あり
            };

        return checkAxis(segmentStart.x, direction.x, boxMin.x, boxMax.x)
            && checkAxis(segmentStart.y, direction.y, boxMin.y, boxMax.y);
    }

    static bool Resolve(CircleCollider* colliderA, CircleCollider* colliderB)
    {
        float radiusSum = colliderA->GetRadius() + colliderB->GetRadius();

        Vector2& centerA = colliderA->GetOwner()->GetTransform().Position;
        Vector2& centerB = colliderB->GetOwner()->GetTransform().Position;

        Vector2 direction = centerB - centerA;
        float distanceSquared = direction.x * direction.x + direction.y * direction.y;

        //まず2乗同士で比較して、当たっていなければsqrtを使わずに終わる
        if (distanceSquared >= radiusSum * radiusSum) { return false; }

        float distance = std::sqrt(distanceSquared);
        Vector2 pushNormal = (distance > 0.0001f)
            ? direction / distance   //A→B方向の単位ベクトル
            : Vector2(1.0f, 0.0f);   //中心が重なっているときは、とりあえず右に押す

        float penetrationDepth = radiusSum - distance;  //めり込み量

        //半分ずつ押し戻す
        centerA -= pushNormal * (penetrationDepth * 0.5f);
        centerB += pushNormal * (penetrationDepth * 0.5f);
        return true;
    }
    static bool Resolve(BoxCollider* boxCollider, CircleCollider* circleCollider)
    {
        const Vector2& boxCenter = boxCollider->GetOwner()->GetTransform().Position;
        Vector2& circleCenter = circleCollider->GetOwner()->GetTransform().Position;  //押し戻すので参照
        float radius = circleCollider->GetRadius();

        Vector2 boxMin = boxCenter - boxCollider->GetHalfSize();
        Vector2 boxMax = boxCenter + boxCollider->GetHalfSize();

        Vector2 closestPoint(std::clamp(circleCenter.x, boxMin.x, boxMax.x),
            std::clamp(circleCenter.y, boxMin.y, boxMax.y));

        Vector2 difference = circleCenter - closestPoint;
        float distanceSquared = difference.x * difference.x + difference.y * difference.y;
        if (distanceSquared >= radius * radius) { return false; }  //当たっていない

        Vector2 pushNormal;
        float penetrationDepth;

        if (distanceSquared > 0.0001f)
        {
            //円の中心が四角形の外：最近点から中心への向きに押し出す
            float distance = std::sqrt(distanceSquared);
            pushNormal = difference / distance;
            penetrationDepth = radius - distance;
        }
        else
        {
            //円の中心が四角形の中：一番近い辺の方向に押し出す
            float distanceToLeft = circleCenter.x - boxMin.x;
            float distanceToRight = boxMax.x - circleCenter.x;
            float distanceToTop = circleCenter.y - boxMin.y;
            float distanceToBottom = boxMax.y - circleCenter.y;

            float nearestEdgeDistance = std::min({ distanceToLeft, distanceToRight,
                                                   distanceToTop, distanceToBottom });

            if (nearestEdgeDistance == distanceToLeft) { pushNormal = Vector2(-1.0f, 0.0f); }
            else if (nearestEdgeDistance == distanceToRight) { pushNormal = Vector2(1.0f, 0.0f); }
            else if (nearestEdgeDistance == distanceToTop) { pushNormal = Vector2(0.0f, -1.0f); }
            else { pushNormal = Vector2(0.0f, 1.0f); }

            penetrationDepth = nearestEdgeDistance + radius;  //辺まで出て、さらに半径分
        }

        circleCenter += pushNormal * penetrationDepth;
        return true;

    }
    static bool Resolve(BoxCollider* boxColliderA, BoxCollider* boxColliderB)
    {
        Vector2& centerA = boxColliderA->GetOwner()->GetTransform().Position;
        Vector2& centerB = boxColliderB->GetOwner()->GetTransform().Position;

        Vector2 minA = centerA - boxColliderA->GetHalfSize();
        Vector2 maxA = centerA + boxColliderA->GetHalfSize();
        Vector2 minB = centerB - boxColliderB->GetHalfSize();
        Vector2 maxB = centerB + boxColliderB->GetHalfSize();

        float overlapX = std::min(maxA.x - minB.x, maxB.x - minA.x);
        float overlapY = std::min(maxA.y - minB.y, maxB.y - minA.y);

        if (overlapX <= 0.0f || overlapY <= 0.0f) { return false; }  //どちらかの軸で離れている

        Vector2 pushNormal;  //AからBへ押す向き
        float penetrationDepth;

        if (overlapX < overlapY)
        {
            //横の方がめり込みが浅い → 横に押し戻す
            pushNormal = Vector2(centerB.x >= centerA.x ? 1.0f : -1.0f, 0.0f);
            penetrationDepth = overlapX;
        }
        else
        {
            //縦の方がめり込みが浅い → 縦に押し戻す
            pushNormal = Vector2(0.0f, centerB.y >= centerA.y ? 1.0f : -1.0f);
            penetrationDepth = overlapY;
        }
        //半分ずつ押し戻す
        centerA -= pushNormal * (penetrationDepth * 0.5f);
        centerB += pushNormal * (penetrationDepth * 0.5f);
        return true;
    }
};