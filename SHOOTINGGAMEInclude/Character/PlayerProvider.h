#pragma once

class GameObject;

//敵などがプレイヤーを参照するための窓口
//プレイヤーは死亡しても削除しないので、シーンが作り直されるまでポインタは有効
class PlayerProvider
{
public:
	//リトライ対策として、プレイヤーを設定し直すときに生存状態もリセットする
	void SetPlayer(GameObject* player) { _player = player; _active = true; }
	GameObject* GetPlayer() const { return _player; }

	void SetPlayerActive(bool active) { _active = active; }
	bool GetActive()const { return _active; }
private:
	GameObject* _player = nullptr;
	bool _active = true;
};