#include "CharacterSelectPage.h"

#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "InputSystem.h"
#include "UIRenderer.h"
#include "UiDraw.h"
#include "CharacterRoster.h"

void CharacterSelectPage::Initialize(ServiceLocator& locator)
{
	_input = locator.input;
	_uiRenderer = locator.uiRenderer;
	_characters = locator.characters;

	_list.Initialize(locator);

	MenuList::Layout layout;
	layout.top = 250.0f;
	layout.height = 60.0f;
	layout.textScale = 0.5f;
	layout.visibleRows = 4;
	_list.SetLayout(layout);
	_list.SetEmptyMessage(L"キャラクターが ありません");   //Characters.json がないとき。ステージの標準のキャラクターが出る

	//選んだ瞬間に、「選んでいる人」を変える
	_list.OnMoved.Subscribe([this](int index)
		{
			if (_characters)
				_characters->Select(index);
		});
}

void CharacterSelectPage::Open()
{
	std::vector<std::wstring> names;
	if (_characters)
	{
		for (const CharacterData& character : _characters->GetList())
			names.push_back(character.name);
	}
	_list.SetItems(std::move(names));
	_list.SetSelectedIndex(_characters ? _characters->GetSelectedIndex() : 0);
}

void CharacterSelectPage::Update()
{
	if (!_input)
		return;

	_list.Update();

	if (_input->GetKeyDown(VK_SPACE) || _input->GetKeyDown(VK_RETURN) || _input->GetKeyDown(VK_ESCAPE))
		OnClose.Invoke();
}

void CharacterSelectPage::Draw()
{
	if (!_uiRenderer)
		return;

	UiDraw::Panel(*_uiRenderer, PanelType::CharacterSelect);
	UiDraw::Text(*_uiRenderer, TextType::CharacterSelectTitle);
	_list.Draw();
	UiDraw::Text(*_uiRenderer, TextType::CharacterSelectHint);
}
