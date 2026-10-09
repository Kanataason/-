#pragma once
#include "Collider.h"

class CircleCollider : public Collider
{
public:
    CircleCollider(float radius, Team team) : Collider(team), _radius(radius) 
    {
        SetColliderType(ColliderType::Circle);
    }

    std::unique_ptr<Component> Clone() const override
    {
        return std::make_unique<CircleCollider>(*this);
    }
    void Update(float deltaTime)override {};

    float GetRadius() const { return _radius; }

private:
    float _radius;
};
