#pragma once
#include <DxLib.h>
#include <algorithm>
#include "Vector2.h"

struct ScreenBounds
{
    float width;
    float height;

    //画面内かどうか確認関数
    bool Contains(const Vector2& position, float margin) const
    {
        return position.x >= -margin && position.x <= width + margin &&
            position.y >= -margin && position.y <= height + margin;
    }

    //Y軸上で画面外に出ないようにする
    float ClampY(float wrappedPosition, float min, float max,float offset)const
    {
        //プレイヤーの半径を追加してめり込まないようにする
        return std::clamp(wrappedPosition, min + offset, max - offset);
    }

    //左から出たら右、右から出たら左へ
    Vector2 Wrap(const Vector2& position) const
    {
        Vector2 wrappedPosition = position;
        if (wrappedPosition.x < 0.0f) { wrappedPosition.x += width; }
        if (wrappedPosition.x > width) { wrappedPosition.x -= width; }

        return wrappedPosition;
    }
};

class GameWindow
{
public:
    GameWindow(HWND hwnd) :_handle(hwnd) {};
    HWND GetHandle() const { return _handle; }
    const ScreenBounds& GetBounds() const { return _bounds; }
   
    void OnResize(float newWidth, float newHeight) { _bounds = { newWidth, newHeight }; } 

private:
    HWND _handle = nullptr;
    ScreenBounds _bounds{};
};
