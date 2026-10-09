#include "DxLib.h"
#include <algorithm>

#include "GameManager.h"
#include "GameTime.h"
namespace
{
	constexpr int GameWidth = 640;
	constexpr int GameHeight = 480;
	//タイトルバーや枠の分、少し余裕を残す
	constexpr double WindowFitRate = 0.90;

	//相手の画面のタスクバーを除いた広さに収まる、最大の拡大率を返す
	double CalcWindowExtendRate()
	{
		RECT workArea{};
		SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
		double workWidth = workArea.right - workArea.left;
		double workHeight = workArea.bottom - workArea.top;

		//縦横どちらかがはみ出さないよう、小さい方の倍率に合わせる
		double rate = (std::min)(workWidth / GameWidth, workHeight / GameHeight);
		return rate * WindowFitRate;
	}
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR plCmdLine, int nCmdShow)
{
	ChangeWindowMode(TRUE);
	SetGraphMode(GameWidth, GameHeight, 32);
	SetWindowSizeExtendRate(CalcWindowExtendRate());
	SetWindowSizeChangeEnableFlag(TRUE, TRUE);

	if (DxLib_Init() == -1)return -1;

	SetDrawScreen(DX_SCREEN_BACK);
	SetMouseDispFlag(FALSE);

	//JSONの読み込み失敗などの例外を捕まえて、原因を表示してから終了する
	try
	{
		auto gameManager = std::make_unique<GameManager>(GetMainWindowHandle());
		gameManager->Initialize();

		//初期化にかかった時間が最初のdeltaTimeに入らないよう、初期化の後で計測を始める
		int prevCount = GetNowCount();
		while (ProcessMessage() == 0)
		{
			float deltaTime = Time::GetDeltaTime(prevCount);
			ClearDrawScreen();

			gameManager->Update(deltaTime);

			ScreenFlip();

			if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
		}
	}
	catch (const std::exception& exception)
	{
		//ゲームのウィンドウの裏に隠れないよう最前面に出す
		MessageBoxA(GetMainWindowHandle(), exception.what(), "Error",
			MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
	}

	DxLib_End();
	return 0;
}