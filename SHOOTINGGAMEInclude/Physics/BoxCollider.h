#pragma once
#include "Collider.h"

class BoxCollider : public Collider
{
public :
	BoxCollider(Vector2 halfSize,Team team):_halfSize(halfSize),Collider(team)
	{
		SetColliderType(ColliderType::Box);
	}

	std::unique_ptr<Component> Clone() const override
	{
		return std::make_unique<BoxCollider>(*this);
	}
	void Update(float deltaTime)override {};

	Vector2 GetHalfSize()const { return _halfSize; }
private:
	Vector2 _halfSize{};
};