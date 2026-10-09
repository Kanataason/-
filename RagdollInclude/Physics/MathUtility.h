#pragma once
#include <DirectXMath.h>
#include <iostream>
#include <time.h>

using namespace DirectX;

class MathUtility
{
public:
    static int RandomRange(unsigned int min,unsigned int max)
    {
        
        static unsigned int random = static_cast<unsigned int>(time(nullptr));

        if (random == 0)
            random = 123456789;

        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;

        return min + random % (max - min + 1);
    }

    static XMVECTOR Normalize(const XMFLOAT3& v)
    {
        XMFLOAT3 unitv;

        auto sqrtv = std::sqrt(v.x* v.x+ v.y * v.y +v.z * v.z);

        //０除算なくすために確認
        if (sqrtv <= 0.000001f)
            return  XMVectorZero();

        unitv.x /= sqrtv;
        unitv.y /= sqrtv;
        unitv.z /= sqrtv;

        return XMLoadFloat3(&unitv);
    }

    static XMVECTOR Normalize(const XMVECTOR& v1, const XMVECTOR& v2)
    {
        XMFLOAT3 fv1, fv2, unitv;

        XMStoreFloat3(&fv1, v1);
        XMStoreFloat3(&fv2, v2);

        unitv = { fv2.x - fv1.x,fv2.y - fv1.y,fv2.z - fv1.z };

        auto sqrtv = std::sqrt(unitv.x * unitv.x + unitv.y * unitv.y + unitv.z * unitv.z);

        //０除算なくすために確認
        if (sqrtv <= 0.000001f)
            return  XMVectorZero();

        unitv.x /= sqrtv;
        unitv.y /= sqrtv;
        unitv.z /= sqrtv;

        return XMLoadFloat3(&unitv);
    }
    static XMVECTOR CrossVector3(const XMVECTOR& v1, const XMVECTOR& v2)
    {
        XMFLOAT3 fv1, fv2, result;

        XMStoreFloat3(&fv1, v1);
        XMStoreFloat3(&fv2, v2);

        result.x = (fv1.y * fv2.z) - (fv1.z * fv2.y);
        result.y = (fv1.z * fv2.x) - (fv1.x * fv2.z);
        result.z = (fv1.x * fv2.y) - (fv1.y * fv2.x);

        return  XMLoadFloat3(&result);
    }

};