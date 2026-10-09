#pragma once
#include <string>
#include <vector>

#include "ScoreManager.h"
#include "RoundManager.h"
#include "UIRenderer.h"
#include "UiLayout.h"
#include "UiTween.h"

class Camera;

//ステージ中の画面表示(HUD)。スコア、残りの玉数、COMBO、「+500」、ボーナスの大きな文字、画面下のヒント
//GameSceneは、通知をつなぐ(Bind)・毎フレーム更新する(Update)・描く(Draw)だけ。何を、どこに出すかは、ここが決める
//威力ゲージは、ここには入れない(量の計算と描画を、PowerGauge が受け持つ)
class GameHud
{
public:
	void Initialize(ServiceLocator& locator);

	//各システムの通知に、表示のきっかけをつなぐ。ここで渡した score と round は、描くときに、数字を読むのにも使う
	void Bind(ScoreManager& score, RoundManager& round);

	void Update(float deltaTime);

	//UiRenderer の Begin～End の間で呼ぶ。camera は、的に当たった場所(世界座標)を、画面の位置に直すのに使う
	void Draw(Camera* camera);

private:
	//点が入ったときの演出(「+500」の浮き上がるポップアップと、COMBO表示)
	void AddScorePopup(const ScoreGain& gain);
	void StartBonusPopup(float points);

	void DrawPanelBase(const PanelData& panel);
	void DrawScorePanel();
	void DrawShotCountPanel();
	void DrawComboText();
	void DrawScorePopups(Camera* camera);
	void DrawBonusPopup();
	void DrawAimHint();

private:
	//的に当たった場所に出す、浮き上がって消える文字
	struct ScorePopup
	{
		Vector3 worldPosition{};
		std::wstring text;
		bool isHead = false;
		float age = 0.0f; //出てからの秒数
	};

	UiRenderer* _uiRenderer = nullptr; 
	ScoreManager* _score = nullptr; 
	RoundManager* _round = nullptr;

	//残りの玉数が、ポンと弾むアニメーション(ラグドールが止まったとき)
	Tween _shotCountPop;

	//クリアボーナスの、画面の中央に出る、大きな文字
	bool _bonusActive = false;
	float _bonusPoints = 0.0f;
	float _bonusAge = 0.0f; //出てからの秒数

	std::vector<ScorePopup> _scorePopups;
	Tween _comboPop; //コンボが増えたときの弾み
	int _lastCombo = 0; //前のフレームのコンボ数(増えたのを見つけるため)
};
