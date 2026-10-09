#pragma once
#include <iostream>

#include <windows.h>
class InputSystem
{
public:
	//クライアント領域のサイズを取得する
	InputSystem(HWND hwnd) :_hwnd(hwnd) {
		GetClientRect(_hwnd, &_screenInfo);
		//クライアント領域の中央座標を計算
		POINT center =
		{
			(_screenInfo.right - _screenInfo.left) / 2,
			(_screenInfo.bottom - _screenInfo.top) / 2
		};
		ClientToScreen(_hwnd,&center);
		SetCursorPos(center.x, center.y);
	};

	void Update();

	bool GetKey(int key)const;
	bool GetKeyEnter(int key)const;
	bool GetKeyDown(int key)const;
	bool AnyKey()const;

	POINT GetMousePosition() const;
	POINT GetMouseDelta() const;

	//マウスカーソルの位置を、ウィンドウ内の座標(クライアント座標)で取得する
	//ボタンのクリック判定など、画面上の位置と比べたいときに使う
	POINT GetMouseClientPosition() const;

	//マウスの左ボタンを押した瞬間か(ボタンのクリック判定に使う)
	bool GetMouseLeftButtonDown() const { return GetKeyDown(VK_LBUTTON); }

private:
	POINT& GetScreenCenter(POINT& point)const;

	//カーソルが画面内にあるかどうか
	bool IsMouseOutsideWindow(const POINT& mousePos)const
	{
		return mousePos.x > _screenInfo.right ||
			mousePos.x < 0 ||
			mousePos.y > _screenInfo.bottom ||
			mousePos.y < 0;
	}
private:
	bool _current[256]{};
	bool _previous[256]{};



	RECT _screenInfo;
	HWND _hwnd = nullptr;
};	