#include "TitleMenuPage.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "InputSystem.h"
#include "UIRenderer.h"
#include "UiDraw.h"
#include "UiLayout.h"

using namespace UiInfo;

void TitleMenuPage::Initialize(ServiceLocator& locator)
{
	_input = locator.input;
	_uiRenderer = locator.uiRenderer;
}

void TitleMenuPage::Update()
{
	if (!_input)
		return;

	if (_input->GetKeyDown('W') || _input->GetKeyDown(VK_UP))
		_selectedIndex = (_selectedIndex + MenuCount - 1) % MenuCount;

	if (_input->GetKeyDown('S') || _input->GetKeyDown(VK_DOWN))
		_selectedIndex = (_selectedIndex + 1) % MenuCount;

	if (!_input->GetKeyDown(VK_SPACE))
		return;

	switch (MenuEntries[_selectedIndex].text)
	{
	case TextType::Start:
		OnOpenStageSelect.Invoke();
		break;
	case TextType::Setting:
		OnOpenCharacterSelect.Invoke();
		break;
	case TextType::End:
		OnQuit.Invoke();
		break;
	default:
		break;
	}
}

void TitleMenuPage::Draw()
{
	if (!_uiRenderer)
		return;

	for (int i = 0; i < MenuCount; i++)
		UiDraw::MenuItem(*_uiRenderer, MenuEntries[i], i == _selectedIndex);
}
