#include "PausePanel.h"

#include <algorithm>
#include <Windows.h>

#include "InputSystem.h"
#include "UIRenderer.h"
#include "UiLayout.h"

using namespace UiInfo;

namespace
{
	constexpr float dimAlpha = 0.55f; //後ろのゲーム画面を暗くする濃さ

	//---- パネルの中の配置(パネルの上端からの距離。1280x720基準) ----
	constexpr float titleY = 60.0f;
	constexpr float buttonTop = 130.0f;
	constexpr float buttonWidth = 280.0f;
	constexpr float buttonHeight = 56.0f;
	constexpr float buttonGap = 14.0f;

	const wchar_t* const buttonLabels[] = { L"つづける", L"やりなおす", L"タイトルへ" };
	constexpr int buttonCount = static_cast<int>(sizeof(buttonLabels) / sizeof(buttonLabels[0]));
}

void PausePanel::Initialize(ServiceLocator& locator)
{
	_input = locator.input;
	_uiRenderer = locator.uiRenderer;
}

void PausePanel::Open()
{
	_visible = true;
	_selectedIndex = 0;
}

void PausePanel::Update()
{
	if (!_visible || !_input)
		return;

	if (_input->GetKeyDown('W') || _input->GetKeyDown(VK_UP))
		_selectedIndex = (_selectedIndex + buttonCount - 1) % buttonCount;
	if (_input->GetKeyDown('S') || _input->GetKeyDown(VK_DOWN))
		_selectedIndex = (_selectedIndex + 1) % buttonCount;

	//ESCは「つづける」と同じ(もう一度押すと、閉じる)
	if (_input->GetKeyDown(VK_ESCAPE))
	{
		_visible = false;
		OnResume.Invoke();
		return;
	}

	if (_input->GetKeyDown(VK_SPACE) || _input->GetKeyDown(VK_RETURN))
	{
		//1回選んだら、パネルは閉じる(シーンの切り替えは、受け取った側が予約する)
		_visible = false;
		switch (_selectedIndex)
		{
		case 0: OnResume.Invoke(); break;
		case 1: OnRetry.Invoke(); break;
		default: OnBackToTitle.Invoke(); break;
		}
	}
}

void PausePanel::Draw()
{
	if (!_visible || !_uiRenderer)
		return;

	auto it = panelList.find(PanelType::Pause);
	if (it == panelList.end())
		return;
	const PanelData& panel = it->second;

	//後ろのゲーム画面を、少し暗くする
	_uiRenderer->DrawRect(0.0f, 0.0f, UiRenderer::VirtualWidth, UiRenderer::VirtualHeight,
		DirectX::XMVectorSet(0, 0, 0, dimAlpha));

	DirectX::XMFLOAT4 fill = panel.fillColor;
	fill.w = (std::max)(fill.w, 0.8f); //後ろが透けすぎて、文字が読みにくくならないように
	_uiRenderer->DrawRect(panel.frame.left, panel.frame.top, panel.frame.width, panel.frame.height,
		XMLoadFloat4(&fill));
	_uiRenderer->DrawFrame(panel.frame.left, panel.frame.top, panel.frame.width, panel.frame.height,
		panel.frame.thickness, XMLoadFloat4(&panel.frame.frameColor));

	const float centerX = panel.frame.left + panel.frame.width * 0.5f;
	const auto white = DirectX::XMVectorSet(1, 1, 1, 1);
	const auto yellow = DirectX::XMVectorSet(1, 1, 0, 1);
	const auto black = DirectX::XMVectorSet(0, 0, 0, 1);

	_uiRenderer->DrawStringCentered(L"ポーズ", centerX, panel.frame.top + titleY, 0.7f, white, black);

	for (int i = 0; i < buttonCount; i++)
	{
		const float left = centerX - buttonWidth * 0.5f;
		const float top = panel.frame.top + buttonTop + (buttonHeight + buttonGap) * i;
		const bool selected = (i == _selectedIndex);

		_uiRenderer->DrawFrame(left, top, buttonWidth, buttonHeight, 2.0f, selected ? yellow : white);
		_uiRenderer->DrawStringCentered(buttonLabels[i], centerX, top + buttonHeight * 0.5f, 0.45f,
			selected ? yellow : white, black);
	}
}
