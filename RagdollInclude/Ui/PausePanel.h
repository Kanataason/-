#pragma once

#include "Event.h"
#include "ServiceLocator.h"

class InputSystem;
class UiRenderer;

//ステージ中に、ESCキーで出す、ポーズのパネル(ゲーム画面の上に重ねて出す)
class PausePanel
{
public:
	Event<> OnResume;
	Event<> OnRetry;
	Event<> OnBackToTitle;

	void Initialize(ServiceLocator& locator);

	void Open();
	bool IsVisible() const { return _visible; }

	//出ている間だけ呼ぶ。キー入力を受け付ける
	void Update();
	void Draw();
private:
	InputSystem* _input = nullptr;
	UiRenderer* _uiRenderer = nullptr;

	bool _visible = false;
	int _selectedIndex = 0; //0: つづける, 1: やりなおす, 2: タイトルへ
};
