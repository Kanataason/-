#pragma once
#include <DirectXMath.h>

#include "ServiceLocator.h"

class UiRenderer;
class RoundManager;

//威力ゲージ。スペースキーで威力を決める間、量を往復させて、いまの量を RoundManager に渡す。画面左のゲージを描く
class PowerGauge
{
public:
	void Initialize(ServiceLocator& locator);
	void Bind(RoundManager& roundManager);

	//量の計算を進めて、RoundManager に、いまの量を渡す
	void Update(float deltaTime);

	//UiRenderer の Begin～End の間で呼ぶ
	void Draw();
private:
	RoundManager* _roundManager = nullptr;
	UiRenderer* _uiRenderer = nullptr;

	float _gaugeSpeed = 2.0f;
	float _gaugeFrame = 0.0f;
	float _gaugeAmount = 0.0f;
};