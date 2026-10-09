#pragma once
#include "BaseScene.h"
#include "ServiceLocator.h"


//タイトル画面。スペースキーでゲームシーンへ進む
class Title : public BaseScene
{
public:
	~Title() override;

	void Initialize(ServiceLocator& locator)override;
	void Update(float deltaTime)override;
private:
	//SetFontSizeを毎フレーム呼ぶとフォントが作り直されて重いので、ハンドルを1回だけ作る
	float _elapsed = 0.0f;
	const float _interval = 0.5f;

	int _titleFont = -1;
	int _messageFont = -1;
};