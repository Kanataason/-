#pragma once
#include <DxLib.h>
#include <string>

//縦スクロールする背景。同じ画像を2枚縦につなげてループさせる
class BackgroundScroller
{
public:
	~BackgroundScroller() { if (_image != -1) DeleteGraph(_image); }

	void Initialize(float speed)
	{
		_image = LoadGraph("Library/image/Ground.png");
		_speed = speed;
		GetDrawScreenSize(&_screenWidth, &_screenHeight);
	}

	void Update(float deltaTime)
	{
		_offsetY += _speed * deltaTime;
		if (_offsetY >= _screenHeight) _offsetY -= _screenHeight;   //1枚分ずれたら元の位置に戻す
	}

	void Draw() const
	{
		int y = static_cast<int>(_offsetY);
		//画面サイズに引き伸ばした画像を2枚、縦につなげて描画する
		DrawExtendGraph(0, y - _screenHeight, _screenWidth, y, _image, FALSE);   //上の1枚
		DrawExtendGraph(0, y, _screenWidth, y + _screenHeight, _image, FALSE);   //下の1枚
	}

private:
	int _image = -1;
	float _speed = 0.0f;
	float _offsetY = 0.0f;
	int _screenWidth = 0;
	int _screenHeight = 0;
};