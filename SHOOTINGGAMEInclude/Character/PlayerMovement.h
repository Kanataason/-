#pragma once
#include "Component.h"
#include "Vector2.h"
#include "GameWindow.h"

class CircleShape;

class PlayerMovement : public Component
{
public:
	std::unique_ptr<Component> Clone()const override
	{
		return std::make_unique<PlayerMovement>(*this);
	}
	void Initialize(ServiceLocator& locator)override;
	void Update(float deltaTime)override {};

	//directionの向きにspeedだけ進む。後ろ方向への移動は遅くなる
	void Move(Vector2 direction,float speed);
	//左右は反対側の端へワープし、上下は画面外に出ないように位置を補正する
	void MovementRestrictions();
private:
 const ScreenBounds* _gameWindow = nullptr;

 float _damping = 0.6f;

 CircleShape* _sizeStatus = nullptr;
};