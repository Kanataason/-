#include "Octree.h"


void Octree::Insert(Collider* collider)
{

    //すでに分割されているなら
    if (_children[0])
    {
        int index = GetChildIndex(collider->GetMin(), collider->GetMax());

        if (index != -1)
        {
            _children[index]->Insert(collider);
            return;
        }

        //跨いでいる場合はこのNodeに留める（再分割はしない）
        _colliders.push_back(collider);
        return;
    }

    //まだこのNodeに入れる
    _colliders.push_back(collider);

    //多すぎたら分割（未分割の場合のみ）
    if (_colliders.size() > MAX_COLLIDERS &&
        _depth < MAX_DEPTH)
    {
        Subdivide();
    }
}

void Octree::Query(const XMFLOAT3& min, const XMFLOAT3& max, std::vector<Collider*>& result)
{
    //範囲外なら探索打ち切り
    if (!Intersects(min, max, _min, _max))
        return;

    //このNodeのColliderを追加
    for (const auto& collider : _colliders)
    {
        result.push_back(collider);
    }

    //未分割ならここで終了
    if (!_children[0])
        return;

    //子Nodeも再帰的に探索
    for (int i = 0; i < CHILD_COUNT; ++i)
    {
        _children[i]->Query(min, max, result);
    }
}

void Octree::QueryRay(const XMFLOAT3& origin, const XMFLOAT3& direction, std::vector<Collider*>& result)
{
    //RayがこのNodeのAABBと交差していない
    if (!IntersectsRay(origin, direction, _min, _max))
    {
        return;
    }
    //このNodeに直接入っているColliderを追加
    for (auto& collider : _colliders)
    {
        result.push_back(collider);
    }
    //子Nodeがあるなら全部調べる
    for (int i = 0; i < CHILD_COUNT; ++i)
    {
        if (_children[i])
        {
            _children[i]->QueryRay(origin, direction, result);
        }
    }
}

int Octree::GetChildIndex(const XMFLOAT3& colliderMin, const XMFLOAT3& colliderMax)
{
    //このNodeの中心（子分割の基準点）
    const XMFLOAT3 center =
    {
        (_min.x + _max.x) * 0.5f,
        (_min.y + _max.y) * 0.5f,
        (_min.z + _max.z) * 0.5f
    };

    //中央面を跨ぐColliderはどの子にも収まらない
    if (colliderMin.x < center.x &&
        colliderMax.x > center.x)
        return -1;

    if (colliderMin.y < center.y &&
        colliderMax.y > center.y)
        return -1;

    if (colliderMin.z < center.z &&
        colliderMax.z > center.z)
        return -1;

    //中心とのxyz位置関係から子インデックスを決定
    int index = 0;

    if (colliderMin.x >= center.x)
        index |= 1;

    if (colliderMin.y >= center.y)
        index |= 2;

    if (colliderMin.z >= center.z)
        index |= 4;

    return index;
}
void Octree::Subdivide()
{
    //分割の基準となる中心点
    const XMFLOAT3 center =
    {
        (_min.x + _max.x) * 0.5f,
        (_min.y + _max.y) * 0.5f,
        (_min.z + _max.z) * 0.5f
    };

    //8分割した子Nodeを生成
    for (int i = 0; i < CHILD_COUNT; ++i)
    {
        XMFLOAT3 childMin;
        XMFLOAT3 childMax;

        childMin.x = (i & 1) ? center.x : _min.x;
        childMax.x = (i & 1) ? _max.x : center.x;

        childMin.y = (i & 2) ? center.y : _min.y;
        childMax.y = (i & 2) ? _max.y : center.y;

        childMin.z = (i & 4) ? center.z : _min.z;
        childMax.z = (i & 4) ? _max.z : center.z;

        _children[i] =
            std::make_unique<Octree>(
                childMin,
                childMax,
                _depth + 1);
    }

    //既存Colliderを子Nodeへ再配置
    std::vector<Collider*> remaining;

    remaining.reserve(_colliders.size());

    for (const auto& collider : _colliders)
    {
        const int index =
            GetChildIndex(
                collider->GetMin(),
                collider->GetMax());

        if (index != -1)
        {
            _children[index]->Insert(collider);
        }
        else
        {
            //跨いでいるColliderはこのNodeに残す
            remaining.push_back(collider);
        }
    }

    _colliders.swap(remaining);
}