#include "InputManager.h"

POINT InputManager::GetScreenCenter()
{
    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    return { screenWidth / 2, screenHeight / 2 };
}

void InputManager::Update()
{
    if (GetKey(VK_ESCAPE))
        PostQuitMessage(0);

    //前のフレームを保存して、現在のフレームを更新する
    memcpy(_previous, _current, sizeof(_current));

    const bool isForeground = (GetForegroundWindow() == _gameWindow->GetHandle());

    for (int keyCode = 0; keyCode < 256; ++keyCode)
    {
        //別ウィンドウを操作中のキー入力は拾わない
        _current[keyCode] = isForeground && (GetAsyncKeyState(keyCode) & 0x8000);
    }

    UpdateMouseDelta(isForeground);
}
void InputManager::UpdateMouseDelta(bool isForeground)
{
    _mouseDelta = { 0, 0 };
    if (!isForeground) { return; }

    POINT cursorPosition{};
    GetCursorPos(&cursorPosition);//スクリーン座標

    POINT center = GetScreenCenter(); //クライアント座標
    ClientToScreen(_gameWindow->GetHandle(), &center);//スクリーン座標に揃える

    _mouseDelta.x = cursorPosition.x - center.x;
    _mouseDelta.y = center.y - cursorPosition.y;

    SetCursorPos(center.x, center.y);//カーソルを中央へ戻す
}
POINT InputManager::GetMouseDelta() const
{
    return _mouseDelta; //Update で計算した値を返すだけ
}
POINT InputManager::GetMousePosition()const
{
    POINT current;
    GetCursorPos(&current);
    return current;
}
bool InputManager::GetKey(int key)const
{
    return _current[key];
}
bool InputManager::GetKeyDown(int key)const
{
    return _current[key] && !_previous[key];
}
bool InputManager::GetKeyEnter(int key)const
{
    return !_current[key] && _previous[key];
}
bool InputManager::AnyKey()const
{
    for (int i = 0; i < 256; ++i)
    {
        if (_current[i])
        {
            return true; //1つでも押されているキーがあれば true
        }
    }
    return false; //どのキーも押されていない
}