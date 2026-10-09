#include "GameTitle.h"
#include "GameObject.h"
#include "Transform.h"

#include "InputManager.h"
#include "SceneManager.h"
#include "EffectUtillity.h"
#include "DrawManager.h"

Title::~Title()
{
	//シーン削除時フォント解放
	if (_titleFont != -1) DeleteFontToHandle(_titleFont);
	if (_messageFont != -1) DeleteFontToHandle(_messageFont);
}

void Title::Initialize(ServiceLocator& locator)
{
	_elapsed = 0.0f;
	BaseScene::Initialize(locator);
	_titleFont = CreateFontToHandle(nullptr, 50, 3);
	_messageFont = CreateFontToHandle(nullptr, 20, 3);
}
void Title::Update(float deltaTime)
{
	BaseScene::Update(deltaTime);

	_elapsed += deltaTime;
	_locator.drawManager->Register(DrawLayer::UI, [this]
		{
			DrawStringToHandle(150, 150, "SHOOTING GAME", 0xFFFFFF, _titleFont);
			if (EffectUtillity::IsBlinkVisible(_elapsed, _interval))
				DrawStringToHandle(200, 300, "PRESS SPACE TO START", 0xFFFFFF, _messageFont);
		});

	if (_locator.input->GetKeyDown(VK_SPACE))
		_locator.sceneManager->RequestChange(SceneName::EGameScene);
}
