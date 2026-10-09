#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <array>

#include "Event.h"
#include "ServiceLocator.h"

class InputSystem;
class UiRenderer;
class SoundManager;


//ステージが終わったときに、ゲーム画面の上に重ねて出す結果パネル
class ResultPanel
{
public:
	//「もういちど」を選んだとき / 「タイトルへ」を選んだとき
	Event<> OnRetry;
	Event<> OnBackToTitle;

	void Initialize(ServiceLocator& locator);

	//結果を出し始める(星の数もここで決める)。毎フレーム呼んでも、2回目以降は無視する
	void Show(int finalScore);

	void Update(float deltaTime);
	void Draw();

	void SetStarThresholds(const std::array<int, 3>& starThresholds) { _starThresholds = starThresholds; }

	bool IsVisible() const { return _visible; }
private:
	//星の数 スコアが、何点以上で何個か
	int CalculateStars(int score);

	//開始から start 秒後に始まる、duration 秒のアニメーションの進み具合
	float Progress(float start, float duration) const;

	void DrawStars(float centerX, float centerY);
	void DrawButtons(float panelTop);
private:
	InputSystem* _input = nullptr;
	UiRenderer* _uiRenderer = nullptr;
	SoundManager* _soundManager = nullptr;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _starOn;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _starOff;

	std::array<int, 3> _starThresholds = { 1500, 3500, 6000 }; //何も渡されなかったとき用

	bool _oneShotClipFlag = false;
	bool _visible = false;
	float _elapsed = 0.0f; //Show してからの秒数。アニメーションは、全部これから計算する
	int _score = 0;
	int _starCount = 0;
	int _selectedIndex = 0; //0: もういちど, 1: タイトルへ
};
