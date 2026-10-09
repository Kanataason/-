#pragma once

#include "Event.h"

//タイトル画面の、1つの画面(メニュー / ステージ選択 / キャラクター選択)
//Title は、いまどの画面を出しているかだけを持ち、入力と描画は、それぞれの画面が受け持つ
class TitlePage
{
public:
	virtual ~TitlePage() = default;

	//この画面を開くたびに呼ぶ(一覧の中身や、選んでいる番号を、最新に合わせる)
	virtual void Open() {}

	virtual void Update() = 0;

	//UiRenderer の Begin～End の間で呼ぶ
	virtual void Draw() = 0;

	//「もどる」を選んだとき
	Event<> OnClose;
};
