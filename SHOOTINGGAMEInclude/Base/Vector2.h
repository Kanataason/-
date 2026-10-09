#pragma once
#include <cmath> 

struct Vector2
{
    Vector2(float xValue =0, float yValue =0) : x(xValue), y(yValue) {}
	float x = 0.0f;
	float y = 0.0f;

    Vector2 operator+(const Vector2& other) const { return { x + other.x, y + other.y }; }
    Vector2 operator-(const Vector2& other) const { return { x - other.x, y - other.y }; }
    Vector2 operator*(float scale) const { return { x * scale, y * scale }; }
    Vector2 operator/(float divisor) const { return { x / divisor, y / divisor }; }

    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
    Vector2& operator*=(float scale) { x *= scale; y *= scale; return *this; }

    float Length() const { return std::sqrt(x * x + y * y); } 
    Vector2 Normalized() const
    {
        float length = Length();
        if (length <= 0.0f)
            return { 0.0f, 0.0f };
        return { x / length, y / length };
    }

    //オーバーロードで長さがわかっているバージョンを作ることで無駄な計算を削減
    Vector2 Normalized(float length)const
    {
        if(length <= 0.0f)
            return { 0.0f, 0.0f };
        return { x / length, y / length };
    }
};