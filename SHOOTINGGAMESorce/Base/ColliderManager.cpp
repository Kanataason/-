#include <map>
#include "ColliderManager.h"
#include "Collider.h"
#include "ColliderRegistry.h"
#include "Collision.h"

namespace
{
    //コライダーの種類の組み合わせごとに、押し戻し処理を呼び分けるための表
    using CollisionCheck = bool (*)(Collider& colliderA, Collider& colliderB);

    template<typename ColliderA, typename ColliderB>
    CollisionCheck MakeCollisionResolve()
    {
        return [](Collider& colliderA, Collider& colliderB) -> bool
            {
                return Collision::Resolve(static_cast<ColliderA*>(&colliderA),
                    static_cast<ColliderB*>(&colliderB));
            };
    }

    const std::map<std::pair<ColliderType, ColliderType>, CollisionCheck> collisionTable =
    {
        { { ColliderType::Circle, ColliderType::Circle }, MakeCollisionResolve<CircleCollider, CircleCollider>() },
        { { ColliderType::Box,    ColliderType::Circle }, MakeCollisionResolve<BoxCollider,    CircleCollider>() },
        { { ColliderType::Box, ColliderType::Box }, MakeCollisionResolve<BoxCollider, BoxCollider>() }
    };

    //逆向きも吸収して判定する
    bool ResolveCollision(Collider& colliderA, Collider& colliderB)
    {
        if (auto it = collisionTable.find({ colliderA.GetColliderType(), colliderB.GetColliderType() }); it != collisionTable.end())
            return it->second(colliderA, colliderB);

        if (auto it = collisionTable.find({ colliderB.GetColliderType(), colliderA.GetColliderType() }); it != collisionTable.end())
            return it->second(colliderB, colliderA);

        return false;  //未登録の組み合わせ
    }
}

void ColliderManager::Initialize(ServiceLocator& locator)
{
	_colliderRegistry = locator.colliderRegistry;
}

void ColliderManager::CheckCollisions(BulletPool& pool)
{
    const auto& colliders = _colliderRegistry->GetColliders();
    if (colliders.empty()) { return; }

    //弾ごとに当たり判定をチェック
    pool.ForEachActive([&](Bullet& bullet) {
        const Vector2& bulletHead = bullet.position;
        const Vector2 bulletTail = bullet.GetTail();
        float halfThickness = bullet.thickness * 0.5f;

        for (Collider* targetCollider : colliders)
        {
            if (!targetCollider->IsCollisionEnabled()) { continue; }
            if (targetCollider->GetTeam() == bullet.ownerTeam) { continue; }

            const Vector2& targetPosition = targetCollider->GetOwner()->GetTransform().Position;
            bool isHit = false;

            //これ以上当たり判定の形は増える予定がないのでswitchで設定
            switch (targetCollider->GetColliderType())
            {
            case ColliderType::Circle:
            {
                auto* circleCollider = static_cast<CircleCollider*>(targetCollider);
                isHit = Collision::CheckCollision(targetPosition, circleCollider->GetRadius(),
                    halfThickness, bulletTail, bulletHead);
                break;
            }
            case ColliderType::Box:
            {
                auto* boxCollider = static_cast<BoxCollider*>(targetCollider);
                isHit = Collision::CheckCollision(targetPosition, boxCollider->GetHalfSize(),
                    halfThickness, bulletTail, bulletHead);
                break;
            }
            default:break;
            }

            if (isHit)
            {
                targetCollider->GetOwner()->OnColliderEnter(bullet.damage);
                pool.Release(&bullet);
                break;
            }
        }
        });
}

void ColliderManager::ResolveCollider()
{
    const auto& colliders = _colliderRegistry->GetColliders();

    //全ペアを1回ずつ調べる
    for (std::size_t indexA = 0; indexA < colliders.size(); ++indexA)
    {
        for (std::size_t indexB = indexA + 1; indexB < colliders.size(); ++indexB)
        {
            Collider* colliderA = colliders[indexA];
            Collider* colliderB = colliders[indexB];

            //当たり判定がないもの、同じチームには当たらないようにする
            if (!colliderA->IsCollisionEnabled() || !colliderB->IsCollisionEnabled()) { continue; }
            if (colliderA->GetTeam() == colliderB->GetTeam()) { continue; }
          
            if (ResolveCollision(*colliderA, *colliderB))
            {
                //Playerなら1ダメージを与える
                Collider* damaged = (colliderA->GetTeam() == Team::Player) ? colliderA : colliderB;
                damaged->GetOwner()->OnColliderEnter(1);
            }
        }
    }
}