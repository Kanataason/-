#include "UiDraw.h"

#include "UIRenderer.h"

using namespace DirectX;
using namespace UiInfo;

void UiDraw::Text(UiRenderer& renderer, TextType type)
{
	auto it = textList.find(type);
	if (it == textList.end())
		return;
	const auto& textData = it->second;

	renderer.DrawStringCentered(textData.text, textData.centerX, textData.centerY, textData.scale,
		XMLoadFloat4(&textData.mainColor), XMLoadFloat4(&textData.outlineColor));
}

void UiDraw::MenuItem(UiRenderer& renderer, const MenuEntry& menu, bool selected)
{
	auto buttonIterator = buttonList.find(menu.button);
	auto textIterator = textList.find(menu.text);
	if (buttonIterator == buttonList.end() || textIterator == textList.end())
		return;

	const auto& buttonData = buttonIterator->second;
	const auto& textData = textIterator->second;

	renderer.DrawFrame(buttonData.left, buttonData.top, buttonData.width, buttonData.height,
		buttonData.thickness,
		selected ? XMVectorSet(1, 1, 0, 1) : XMLoadFloat4(&buttonData.frameColor));

	renderer.DrawStringCentered(textData.text, textData.centerX, textData.centerY, textData.scale,
		selected ? XMVectorSet(1, 1, 0, 1) : XMLoadFloat4(&textData.mainColor),
		XMLoadFloat4(&textData.outlineColor));
}

void UiDraw::Panel(UiRenderer& renderer, PanelType type, int selectedIndex)
{
	auto it = panelList.find(type);
	if (it == panelList.end())
		return;
	const PanelData& panel = it->second;

	renderer.DrawRect(panel.frame.left, panel.frame.top, panel.frame.width, panel.frame.height,
		XMLoadFloat4(&panel.fillColor));
	renderer.DrawFrame(panel.frame.left, panel.frame.top, panel.frame.width, panel.frame.height,
		panel.frame.thickness, XMLoadFloat4(&panel.frame.frameColor));

	for (int i = 0; i < static_cast<int>(panel.items.size()); i++)
		MenuItem(renderer, panel.items[i], i == selectedIndex);
}
