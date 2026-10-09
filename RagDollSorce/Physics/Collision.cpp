#include <string>
#include <DirectXMath.h>

#include "Collision.h"
#include "GameObject.h"
#include "RigidBody.h"

#include "DebugUtility.h"
#include "MathUtility.h"
#include "ConsoleUtility.h"


using namespace DirectX;

//ここから当たり判定(交差しているかどうかだけを返す)

//中心間距離が半径の合計以下かどうかで判定
bool Collision::Check(const SphereCollider& acoll, const SphereCollider& bcoll)
{
    Vector3 aCenter, bCenter;
    Vector::fromF3ToV3(acoll.GetPosition(), aCenter);
    Vector::fromF3ToV3(bcoll.GetPosition(), bCenter);

    float radiusA = acoll.GetRadius();
    float radiusB = bcoll.GetRadius();

    Vector3 diff = bCenter - aCenter;

    float distanceSq =
        diff.x * diff.x +
        diff.y * diff.y +
        diff.z * diff.z;

    return distanceSq <=
        (radiusA + radiusB) *
        (radiusA + radiusB);
}

//AABBの判定をする(各軸で範囲が重なっているか)
bool Collision::Check(const BoxCollider& acoll, const BoxCollider& bcoll)
{
    Vector3 aMax, aMin, bMax, bMin;

    Vector::fromF3ToV3(acoll.GetMax(), aMax);
    Vector::fromF3ToV3(acoll.GetMin(), aMin);
    Vector::fromF3ToV3(bcoll.GetMax(), bMax);
    Vector::fromF3ToV3(bcoll.GetMin(), bMin);

    return  aMin.x <= bMax.x && aMax.x >= bMin.x &&
        aMin.y <= bMax.y && aMax.y >= bMin.y &&
        aMin.z <= bMax.z && aMax.z >= bMin.z;
}

//Box上の最近点を求め、その点とSphere中心との距離で判定する
bool Collision::Check(const BoxCollider& box, const SphereCollider& sphere)
{
    Vector3 centerS, bMin, bMax;
    Vector3 closest;

    Vector::fromF3ToV3(sphere.GetPosition(), centerS);
    float radius = sphere.GetRadius();

    Vector::fromF3ToV3(box.GetMax(), bMax);
    Vector::fromF3ToV3(box.GetMin(), bMin);

    //各軸ごとに範囲内へクランプしてBox上の最近点を得る
    closest.x = std::max(bMin.x, std::min(centerS.x, bMax.x));
    closest.y = std::max(bMin.y, std::min(centerS.y, bMax.y));
    closest.z = std::max(bMin.z, std::min(centerS.z, bMax.z));

    Vector3 vector = closest - centerS;

    float result = vector.x * vector.x + vector.y * vector.y + vector.z * vector.z; //距離の2乗

    return result <= radius * radius;
}

//2本の線分(カプセルの軸)間の最短距離で判定
bool Collision::Check(const CapsuleCollider& aCapsule, const CapsuleCollider& bCapsule)
{
    Vector3 aStart, aEnd, bStart, bEnd;

    Vector::fromF3ToV3(aCapsule.GetStart(), aStart);
    Vector::fromF3ToV3(aCapsule.GetEnd(), aEnd);

    Vector::fromF3ToV3(bCapsule.GetStart(), bStart);
    Vector::fromF3ToV3(bCapsule.GetEnd(), bEnd);

    float radiusA = aCapsule.GetRadius();
    float radiusB = bCapsule.GetRadius();

    //Capsule A の中心線
    Vector3 a1 = aEnd - aStart;

    //Capsule B の中心線
    Vector3 b1 = bEnd - bStart;

    //始点同士
    Vector3 c1 = aStart - bStart;

    float dotA = a1.x * a1.x + a1.y * a1.y + a1.z * a1.z;
    float dotB = a1.x * b1.x + a1.y * b1.y + a1.z * b1.z;
    float dotC = b1.x * b1.x + b1.y * b1.y + b1.z * b1.z;
    float dotD = a1.x * c1.x + a1.y * c1.y + a1.z * c1.z;
    float dotE = b1.x * c1.x + b1.y * c1.y + b1.z * c1.z;

    //平行判定(分母が0に近いと解が不安定になる)
    float normal = dotA * dotC - dotB * dotB;

    float s = 0.0f;
    float t = 0.0f;

    if (fabs(normal) < EPSILON)
    {
        //平行な場合はs=0固定でtだけ求める
        if (dotC > EPSILON)
        {
            t = dotE / dotC;
        }
        else
        {
            t = 0.0f;
        }
    }
    else
    {
        s = (dotB * dotE - dotC * dotD) / normal;
        t = (dotA * dotE - dotB * dotD) / normal;
    }

    //線分の範囲(0～1)に収める
    s = std::clamp(s, 0.0f, 1.0f);
    t = std::clamp(t, 0.0f, 1.0f);

    //最近接点
    Vector3 pointA = aStart + a1 * s;
    Vector3 pointB = bStart + b1 * t;

    Vector3 relPos = pointB - pointA;

    float distanceSq =
        relPos.x * relPos.x +
        relPos.y * relPos.y +
        relPos.z * relPos.z;

    float resultRadius = radiusA + radiusB;

    return distanceSq <= resultRadius * resultRadius;
}

//カプセルの軸上の最近点とSphere中心との距離で判定
bool Collision::Check(const CapsuleCollider& cap, const SphereCollider& sphere)
{
    Vector3 center, aS, aE;

    Vector::fromF3ToV3(cap.GetStart(), aS);
    Vector::fromF3ToV3(cap.GetEnd(), aE);

    auto cRadius = cap.GetRadius();
    auto sRadius = sphere.GetRadius();

    Vector::fromF3ToV3(sphere.GetPosition(), center);

    Vector3 ab = aE - aS;
    Vector3 ac = center - aS;

    float abLengthSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;

    float t = 0.0f;

    if (abLengthSq > EPSILON)
    {
        t = (ac.x * ab.x +
            ac.y * ab.y +
            ac.z * ab.z) / abLengthSq;
    }

    t = std::clamp(t, 0.0f, 1.0f);

    Vector3 closest = aS + ab * t;

    Vector3 diff = center - closest;

    float distanceSq =
        diff.x * diff.x +
        diff.y * diff.y +
        diff.z * diff.z;

    float radius = cRadius + sRadius;

    return  distanceSq <= radius * radius;
}

//カプセルの軸上の最近点とBoxの最近点との距離で判定
namespace
{
    //カプセルの軸と、箱(AABB)の、最近接点を、反復して求める。
    //Check(当たったか)と Resolve(押し出し)で、同じ計算を使うための共通の処理。
    //別々の計算だと、Checkは「当たった」と答えるのに、Resolveは「めり込みなし」と答えて、空の結果(接触点が原点)が返ってしまう
    //axisPoint: カプセルの軸上の点 / boxPoint: 箱の表面(または内部)の点
    void ClosestPointsCapsuleBox(Vector3 aS, Vector3 aE,
        Vector3 bMin, Vector3 bMax, Vector3& axisPoint, Vector3& boxPoint)
    {
        //Vector3の演算子は、constの左辺に使えないので、値で受け取って、constなしで使う
        Vector3 ab = aE - aS;
        const float sq = Vector::SqMagnitude(ab);

        //とりあえずカプセルの中点からスタート
        Vector3 p = aS + ab * 0.5f;
        Vector3 closest;

        //2～3回反復して収束させる
        for (int i = 0; i < 3; ++i)
        {
            //pに一番近い箱の表面上の点を求める
            closest.x = std::max(bMin.x, std::min(p.x, bMax.x));
            closest.y = std::max(bMin.y, std::min(p.y, bMax.y));
            closest.z = std::max(bMin.z, std::min(p.z, bMax.z));

            //その点に一番近いカプセル軸上の点を求める
            float t = 0.0f;
            if (sq >= EPSILON)
            {
                Vector3 aq = closest - aS;
                t = Vector::Dot(aq, ab) / sq;
            }
            t = std::clamp(t, 0.0f, 1.0f);
            p = aS + ab * t;
        }

        axisPoint = p;
        boxPoint = closest;
    }
}

bool Collision::Check(const CapsuleCollider& cap, const BoxCollider& box)
{
    Vector3 aS, aE, bMin, bMax;
    Vector::fromF3ToV3(cap.GetStart(), aS);
    Vector::fromF3ToV3(cap.GetEnd(), aE);
    Vector::fromF3ToV3(box.GetMin(), bMin);
    Vector::fromF3ToV3(box.GetMax(), bMax);

    const float radius = cap.GetRadius();

    Vector3 axisPoint, boxPoint;
    ClosestPointsCapsuleBox(aS, aE, bMin, bMax, axisPoint, boxPoint);

    const float distanceSq = Vector::SqMagnitude(axisPoint - boxPoint);

    //箱の内部に軸が入っている(距離がほぼ0)か、距離が半径より小さい(Resolveで、めり込み量が正になる条件と同じ)
    return distanceSq <= EPSILON || distanceSq < radius * radius;
}


//ここからは押し出し判定の処理(交差している場合の法線・めり込み量・接触点を返す)

//円コライダー同士限定の押し出し処理
CollisionResult Collision::Resolve(SphereCollider& acoll, SphereCollider& bcoll)
{
    Vector3 aCenter, bCenter;
    Vector::fromF3ToV3(acoll.GetPosition(), aCenter);
    Vector::fromF3ToV3(bcoll.GetPosition(), bCenter);

    float radiusA = acoll.GetRadius();
    float radiusB = bcoll.GetRadius();

    //2つの球の中心ベクトルと距離を取得
    Vector3 diff = aCenter - bCenter;

    float distance = std::sqrt(Vector::SqMagnitude(diff));

    float penetration =
        (radiusA + radiusB) - distance;

    //めり込んでいなければ衝突なし
    if (penetration <= 0.0f)
        return {};

    Vector3 normal;

    if (distance > EPSILON)
    {
        normal = diff;
        normal.Normalize(distance);
    }
    else
    {
        //中心が完全に一致している場合のフォールバック
        normal = { 0.0f, 1.0f, 0.0f };
    }

    Vector3 contactPoint = aCenter + normal * radiusA;

    CollisionResult result;

    result.colliderA = &acoll;
    result.colliderB = &bcoll;
    result.normal = normal;
    result.penetration = penetration;
    result.contactPoint = contactPoint;

    return result;
}

//Box同士の押し出し処理(最小重なり軸を押し出し方向にする)
CollisionResult Collision::Resolve(BoxCollider& acoll, BoxCollider& bcoll)
{
    Vector3 aMax, aMin, bMax, bMin, aCenter, bCenter;
    Vector::fromF3ToV3(acoll.GetMax(), aMax);
    Vector::fromF3ToV3(acoll.GetMin(), aMin);
    Vector::fromF3ToV3(bcoll.GetMax(), bMax);
    Vector::fromF3ToV3(bcoll.GetMin(), bMin);

    //各軸の重なり量を求める
    float overlapX = std::min(aMax.x - bMin.x, bMax.x - aMin.x);
    float overlapY = std::min(aMax.y - bMin.y, bMax.y - aMin.y);
    float overlapZ = std::min(aMax.z - bMin.z, bMax.z - aMin.z);

    //一番浅くめり込んでいる軸がめり込み量になる
    float penetration =
        std::min({ overlapX, overlapY, overlapZ });
    if (penetration <= 0.0f)
    {
        return {};
    }

    Vector::fromF3ToV3(acoll.GetPosition(), aCenter);
    Vector::fromF3ToV3(bcoll.GetPosition(), bCenter);

    Vector3 normal = { 0.0f, 0.0f, 0.0f };

    //最小重なり軸に沿って押し出す(中心位置の大小で向きを決める)
    if (overlapX <= overlapY && overlapX <= overlapZ)
    {
        float move = overlapX * HALF;

        if (aCenter.x < bCenter.x)
        {
            normal = { -1.0f, 0.0f, 0.0f };

            aCenter.x -= move;
            bCenter.x += move;
        }
        else
        {
            normal = { 1.0f, 0.0f, 0.0f };

            aCenter.x += move;
            bCenter.x -= move;
        }
    }
    else if (overlapY <= overlapZ)
    {
        float move = overlapY * HALF;

        if (aCenter.y < bCenter.y)
        {
            normal = { 0.0f, -1.0f, 0.0f };

            aCenter.y -= move;
            bCenter.y += move;
        }
        else
        {
            normal = { 0.0f, 1.0f, 0.0f };

            aCenter.y += move;
            bCenter.y -= move;
        }
    }
    else
    {
        float move = overlapZ * HALF;

        if (aCenter.z < bCenter.z)
        {
            normal = { 0.0f, 0.0f, -1.0f };

            aCenter.z -= move;
            bCenter.z += move;
        }
        else
        {
            normal = { 0.0f, 0.0f, 1.0f };

            aCenter.z += move;
            bCenter.z -= move;
        }
    }

    CollisionResult result;

    result.colliderA = &acoll;
    result.colliderB = &bcoll;
    result.normal = normal;
    result.penetration = penetration;
    result.contactPoint = (aCenter + bCenter) * 0.5f;

    return result;
}

//Box-Sphereの押し出し処理
CollisionResult Collision::Resolve(BoxCollider& abox, SphereCollider& bsphere)
{
    Vector3 closest;
    Vector3 sCenter;
    Vector3 aMax;
    Vector3 aMin;

    float radius = bsphere.GetRadius();

    Vector::fromF3ToV3(bsphere.GetPosition(), sCenter);
    Vector::fromF3ToV3(abox.GetMax(), aMax);
    Vector::fromF3ToV3(abox.GetMin(), aMin);

    //Box上の最近接点
    closest.x = std::max(aMin.x, std::min(sCenter.x, aMax.x));
    closest.y = std::max(aMin.y, std::min(sCenter.y, aMax.y));
    closest.z = std::max(aMin.z, std::min(sCenter.z, aMax.z));

    Vector3 diff = sCenter - closest;

    float distance = std::sqrt(Vector::SqMagnitude(diff));

    float penetration = radius - distance;

    if (penetration <= 0.0f)
        return {};

    Vector3 normal;

    if (distance > EPSILON)
    {
        normal = diff;
        normal.Normalize(distance);
    }
    else
    {
        //Sphere中心がBox内部にある場合、一番近い面へ押し出す
        float left = sCenter.x - aMin.x;
        float right = aMax.x - sCenter.x;
        float bottom = sCenter.y - aMin.y;
        float top = aMax.y - sCenter.y;
        float back = sCenter.z - aMin.z;
        float front = aMax.z - sCenter.z;

        float minDist = left;

        normal = { -1.0f, 0.0f, 0.0f };

        if (right < minDist)
        {
            minDist = right;
            normal = { 1.0f, 0.0f, 0.0f };
        }

        if (bottom < minDist)
        {
            minDist = bottom;
            normal = { 0.0f, -1.0f, 0.0f };
        }

        if (top < minDist)
        {
            minDist = top;
            normal = { 0.0f, 1.0f, 0.0f };
        }

        if (back < minDist)
        {
            minDist = back;
            normal = { 0.0f, 0.0f, -1.0f };
        }

        if (front < minDist)
        {
            minDist = front;
            normal = { 0.0f, 0.0f, 1.0f };
        }
    }

    Vector3 spherePoint =
        sCenter - normal * radius;

    Vector3 boxPoint =
        closest;

    //接触点を2点の中間にする
    Vector3 contactPoint =
        (spherePoint + boxPoint) * 0.5f;

    CollisionResult result;

    result.colliderA = &bsphere;
    result.colliderB = &abox;
    result.normal = normal;
    result.penetration = penetration;
    result.contactPoint = contactPoint;

    return result;
}

//Capsule同士の押し出し処理(2線分間の最近接点を求めてから押し出す)
CollisionResult Collision::Resolve(CapsuleCollider& a, CapsuleCollider& b)
{
    Vector3 a1, b1, c1, aS, aE, bS, bE;

    auto aRadius = a.GetRadius();
    auto bRadius = b.GetRadius();

    Vector::fromF3ToV3(a.GetStart(), aS);
    Vector::fromF3ToV3(a.GetEnd(), aE);
    Vector::fromF3ToV3(b.GetStart(), bS);
    Vector::fromF3ToV3(b.GetEnd(), bE);

    a1 = aE - aS;
    b1 = bE - bS;
    c1 = aS - bS;

    auto dotA = Vector::Dot(a1, a1);
    auto dotB = Vector::Dot(a1, b1);
    auto dotC = Vector::Dot(b1, b1);
    auto dotD = Vector::Dot(a1, c1);
    auto dotE = Vector::Dot(b1, c1);

    auto denom = dotA * dotC - dotB * dotB;
    float s = 0.0f, t = 0.0f;

    //---- s(カプセルA上の位置)を先に決める ----
    if (dotA <= EPSILON && dotC <= EPSILON)
    {
        //両方とも点(長さ0)
        s = 0.0f; t = 0.0f;
    }
    else if (dotA <= EPSILON)
    {
        s = 0.0f;
        t = std::clamp(dotE / dotC, 0.0f, 1.0f);
    }
    else if (dotC <= EPSILON)
    {
        t = 0.0f;
        s = std::clamp(-dotD / dotA, 0.0f, 1.0f);
    }
    else
    {
        if (fabs(denom) > EPSILON)
        {
            s = std::clamp((dotB * dotE - dotC * dotD) / denom, 0.0f, 1.0f);
        }
        else
        {
            s = 0.0f; //平行な場合
        }

        //sを確定させてからtを計算する
        t = (dotB * s + dotE) / dotC;

        //tが範囲外ならクランプして、sを再計算する
        if (t < 0.0f)
        {
            t = 0.0f;
            s = std::clamp(-dotD / dotA, 0.0f, 1.0f);
        }
        else if (t > 1.0f)
        {
            t = 1.0f;
            s = std::clamp((dotB - dotD) / dotA, 0.0f, 1.0f);
        }
    }

    Vector3 pointA = aS + a1 * s;
    Vector3 pointB = bS + b1 * t;

    Vector3 relPos = pointB - pointA;
    float length = std::sqrt(Vector::Dot(relPos, relPos));
    float penetration = aRadius + bRadius - length;

    if (penetration <= 0.0f)
        return {};

    if (length > EPSILON)
    {
        Vector3 normal = relPos / length;
        Vector3 contactPoint = (pointA + pointB) * 0.5f;

        CollisionResult result;
        //TODO: colliderA/Bをb,aの順で入れているが、normalはa->b向きのまま。
        //CollisionManager側の解釈(colliderAから見た押し出し方向か)に合わせて統一する
        result.colliderA = &b;
        result.colliderB = &a;
        result.normal = normal;
        result.penetration = penetration;
        result.contactPoint = contactPoint;

        return result;
    }
    return {};
}

//Capsule-Sphereの押し出し処理
CollisionResult Collision::Resolve(CapsuleCollider& acap, SphereCollider& asphere)
{
    Vector3 center, aS, aE;

    Vector::fromF3ToV3(acap.GetStart(), aS);
    Vector::fromF3ToV3(acap.GetEnd(), aE);

    auto cRadius = acap.GetRadius();
    auto sRadius = asphere.GetRadius();

    Vector::fromF3ToV3(asphere.GetPosition(), center);

    Vector3 ab = aE - aS;
    Vector3 ac = center - aS;

    float abLengthSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;

    float t = 0.0f;

    if (abLengthSq > EPSILON)
    {
        t = (ac.x * ab.x +
            ac.y * ab.y +
            ac.z * ab.z) / abLengthSq;
    }

    t = std::clamp(t, 0.0f, 1.0f);

    Vector3 closest = aS + ab * t;

    Vector3 diff = center - closest;

    float distanceSq =
        diff.x * diff.x +
        diff.y * diff.y +
        diff.z * diff.z;

    float distance = std::sqrt(distanceSq);

    float penetration =
        cRadius + sRadius - distance;

    if (penetration <= 0.0f)
        return {};

    Vector3 normal;

    if (distance > EPSILON)
    {
        normal = diff / distance;
    }
    else
    {
        normal = Vector3(0.0f, 1.0f, 0.0f);
    }

    Vector3 sphereSurfacePoint = center - normal * sRadius;
    Vector3 contactPoint = (closest + sphereSurfacePoint) * 0.5f;

    CollisionResult result;

    result.colliderA = &asphere;
    result.colliderB = &acap;
    result.normal = normal;
    result.penetration = penetration;
    result.contactPoint = contactPoint;

    return result;
}

//Capsule-Boxの押し出し処理(反復法でカプセル軸とBoxの最近接点を収束させる)
CollisionResult Collision::Resolve(CapsuleCollider& cap, BoxCollider& box)
{
    Vector3 aS, aE, bMin, bMax;
    Vector::fromF3ToV3(cap.GetStart(), aS);
    Vector::fromF3ToV3(cap.GetEnd(), aE);
    Vector::fromF3ToV3(box.GetMin(), bMin);
    Vector::fromF3ToV3(box.GetMax(), bMax);

    float radius = cap.GetRadius();

    //Checkと同じ計算で、最近接点を求める(違う計算だと、当たったのに、結果が空になる)
    Vector3 p, closest;
    ClosestPointsCapsuleBox(aS, aE, bMin, bMax, p, closest);

    Vector3 diff = closest - p;
    float distanceSq = Vector::SqMagnitude(diff);

    if (distanceSq > EPSILON)
    {
        float distance = std::sqrt(distanceSq);
        float penetration = radius - distance;

        if (penetration > 0.0f)
        {
            Vector3 normal = -diff / distance;
            Vector3 contactPoint = (p + closest) * 0.5f;

            CollisionResult result;
            result.colliderA = &cap;
            result.colliderB = &box;
            result.normal = normal;
            result.penetration = penetration;
            result.contactPoint = contactPoint;

            return result;
        }
    }
    else
    {
        //カプセル軸上の点pがすでに箱の内部にある場合、一番近い面へ押し出す
        float dx1 = p.x - bMin.x, dx2 = bMax.x - p.x;
        float dy1 = p.y - bMin.y, dy2 = bMax.y - p.y;
        float dz1 = p.z - bMin.z, dz2 = bMax.z - p.z;

        float minDist = dx1;
        Vector3 normal(-1, 0, 0);

        if (dx2 < minDist) { minDist = dx2; normal = Vector3(1, 0, 0); }
        if (dy1 < minDist) { minDist = dy1; normal = Vector3(0, -1, 0); }
        if (dy2 < minDist) { minDist = dy2; normal = Vector3(0, 1, 0); }
        if (dz1 < minDist) { minDist = dz1; normal = Vector3(0, 0, -1); }
        if (dz2 < minDist) { minDist = dz2; normal = Vector3(0, 0, 1); }

        CollisionResult result;
        result.colliderA = &cap;
        result.colliderB = &box;
        result.normal = normal;
        result.penetration = minDist + radius; //内部に入り込んだ分 + 半径ぶん
        result.contactPoint = p + normal * minDist;
        return result;
    }
    return {};
}

bool Collision::RayCast(const XMFLOAT3& origin, const XMFLOAT3& direction, 
    const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2, DebugDraw* debugDraw)
{
    debugDraw->DrawRay(origin, direction,1.4f, { 1,0,0,1 });
    return false;
}