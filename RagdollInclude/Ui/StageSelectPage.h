#pragma once

#include "MenuList.h"
#include "TitlePage.h"
#include "ServiceLocator.h"

class InputSystem;
class UiRenderer;
class StageRoster;

//ステージ選択。ステージの一覧は、StageRoster(Stages.json)から作る
//決定(スペース)すると、選んだ番号を知らせる。ステージを始める処理は、受け取った側(Title)がやる
class StageSelectPage : public TitlePage
{
public:
	//ステージを決定したとき(引数は、一覧の中の番号)
	Event<int> OnStageChosen;

	void Initialize(ServiceLocator& locator);

	void Open() override;
	void Update() override;
	void Draw() override;

private:
	InputSystem* _input = nullptr;
	UiRenderer* _uiRenderer = nullptr;
	StageRoster* _stages = nullptr; //借りるだけ

	MenuList _list;
};
