#include "InputSystem.h"

POINT& InputSystem::GetScreenCenter(POINT& point)const
{
    RECT rect;
    GetClientRect(_hwnd, &rect);

    point.x = (rect.right - rect.left) / 2;
    point.y = (rect.bottom - rect.top) / 2;

    ClientToScreen(_hwnd, &point);
    return point;
}

void InputSystem::Update()
{

    //前のフレームを保存して、現在のフレームを更新する
    memcpy(_previous, _current, sizeof(_current));

    for (int i = 0; i < 256; i++)
    {
        _current[i] = GetAsyncKeyState(i) & 0x8000;
    }
}
POINT InputSystem::GetMouseDelta()const
{
    POINT current;
    GetCursorPos(&current);

    //別ウィンドウなら動かさない
    if (GetForegroundWindow() != _hwnd){ return{ 0,0 }; }

    //マウスのポジションが画面外に出たら戻す
    if (IsMouseOutsideWindow(current)) { return { 0,0 }; }
    POINT center;
    GetScreenCenter(center);

    POINT delta;
    delta.x = current.x - center.x;
    delta.y = center.y - current.y;

    //カーソルを中央へ戻す
   //SetCursorPos(center.x, center.y);

    return delta;
}
POINT InputSystem::GetMousePosition()const
{
    POINT current;
    GetCursorPos(&current);
    return current;
}
POINT InputSystem::GetMouseClientPosition()const
{
    POINT current;
    GetCursorPos(&current);

    //スクリーン座標(画面全体)から、ウィンドウ内の座標に変換する
    ScreenToClient(_hwnd, &current);
    return current;
}
bool InputSystem::GetKey(int key)const
{
    return _current[key];
}
bool InputSystem::GetKeyDown(int key)const
{
	return _current[key] && !_previous[key];
}
bool InputSystem::GetKeyEnter(int key)const
{
    return !_current[key] && _previous[key];
}
bool InputSystem::AnyKey()const
{
    for (int i = 0; i < 256; ++i)
    {
        if (_current[i])
        {
            return true;
        }
    }

    return false;
}