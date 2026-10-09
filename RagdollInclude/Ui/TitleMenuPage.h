#pragma once

#include "TitlePage.h"
#include "ServiceLocator.h"

class InputSystem;
class UiRenderer;

//タイトルのメニュー(セレクトステージ / キャラクターセレクト / エンド)
class TitleMenuPage : public TitlePage
{
public:
	//それぞれの項目を、決定したとき
	Event<> OnOpenStageSelect;
	Event<> OnOpenCharacterSelect;
	Event<> OnQuit;

	void Initialize(ServiceLocator& locator);

	void Update() override;
	void Draw() override;

private:
	InputSystem* _input = nullptr;
	UiRenderer* _uiRenderer = nullptr;

	int _selectedIndex = 0;   //メニューで選んでいる行(メニューを閉じても、覚えておく)
};
