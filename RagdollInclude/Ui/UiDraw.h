#pragma once

#include "UiLayout.h"

class UiRenderer;

//UiLayout.cpp の表(textList / buttonList / panelList)にあるものを、画面に描く、共通の関数
//UiRenderer の Begin～End の間で呼ぶ
namespace UiDraw
{
	//表にある文字を、決められた位置に描く
	void Text(UiRenderer& renderer, TextType type);

	//ボタンの枠と文字を描く。selected のときは、黄色
	void MenuItem(UiRenderer& renderer, const MenuEntry& menu, bool selected);

	//パネルの塗りと枠を描いて、中のボタンも描く。selectedIndex は、パネルの中で選んでいる行(選ぶ物がないときは -1)
	void Panel(UiRenderer& renderer, PanelType type, int selectedIndex = -1);
}
