#include "StageSelectPage.h"

#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "InputSystem.h"
#include "UIRenderer.h"
#include "UiDraw.h"
#include "StageRoster.h"

void StageSelectPage::Initialize(ServiceLocator& locator)
{
	_input = locator.input;
	_uiRenderer = locator.uiRenderer;
	_stages = locator.stages;

	_list.Initialize(locator);

	MenuList::Layout layout;
	layout.top = 225.0f;
	layout.height = 56.0f;
	layout.textScale = 0.55f;
	layout.visibleRows = 4;
	_list.SetLayout(layout);
	_list.SetEmptyMessage(L"ステージが ありません");   //Stages.json がないとき
}

void StageSelectPage::Open()
{
	std::vector<std::wstring> names;
	if (_stages)
	{
		for (const StageEntry& stage : _stages->GetList())
			names.push_back(stage.name);
	}
	_list.SetItems(std::move(names));

	//前に遊んだステージを、選んだままにしておく
	_list.SetSelectedIndex(_stages ? _stages->GetSelectedIndex() : 0);
}

void StageSelectPage::Update()
{
	if (!_input)
		return;

	_list.Update();

	//決定: 選んだステージを、遊び始める(一覧が空のときは、選ぶものがないので、何も起きない)
	if (_input->GetKeyDown(VK_SPACE) && _list.GetCount() > 0)
	{
		OnStageChosen.Invoke(_list.GetSelectedIndex());
		return;
	}

	//もどる
	if (_input->GetKeyDown(VK_ESCAPE) || _input->GetKeyDown(VK_RETURN))
		OnClose.Invoke();
}

void StageSelectPage::Draw()
{
	if (!_uiRenderer)
		return;

	UiDraw::Panel(*_uiRenderer, PanelType::StageSelect);
	UiDraw::Text(*_uiRenderer, TextType::StageSelectTitle);
	_list.Draw();
	UiDraw::Text(*_uiRenderer, TextType::StageSelectHint);
}
