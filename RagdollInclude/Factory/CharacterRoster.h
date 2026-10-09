#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "ObjectType.h"

//選べるキャラクター1人ぶんの情報(Asset/Data/Characters.json の1件)
struct CharacterData
{
	std::wstring name;//選ぶ画面に出す名前
	std::string modelPath;//FBXのパス(ミクサモのスケルトンで作られたもの)
	float scale = 0.01f;//モデルの大きさ(cmのモデルは0.01で、mになる)
	float colliderScale = 1.0f;//コライダーの大きさ
	float bounceRestitution = 1.0f;//地面で強く跳ねるときの反発係数。1より大きいほど、よく跳ねる
	int bounceCount = 0;//強く跳ねられる回数。0なら、ふつうの跳ね方
	std::vector<AnimationData> animations;//待機などのアニメーション

	//出来事の名前("shot" "hit" "head" "clear")→ このキャラクター専用の音の名前(Sound.jsonの"name")
	//書いていない出来事は、標準の音(出来事の名前と同じ音)を鳴らす
	std::unordered_map<std::string, std::string> sounds;
};

//遊べるキャラクターの一覧と、いま選んでいるキャラクター。
//ゲーム全体で1つだけ持つ(シーンを作り直しても、選んだキャラクターが残るように、GameManagerが持つ)
class CharacterRoster
{
public:
	//JSONから一覧を読む。ファイルがない・書き方が違うときは、空のまま(ステージのJSONのプレイヤーを、そのまま使う)
	void Load(const std::string& jsonPath);

	int GetCount() const { return static_cast<int>(_characters.size()); }
	const std::vector<CharacterData>& GetList() const { return _characters; }
	int GetSelectedIndex() const { return _selectedIndex; }

	//選んでいる人。一覧が空のときは、nullptr
	const CharacterData* GetSelected() const;

	//番号で選ぶ。範囲の外の番号は、端にそろえる
	void Select(int index);

	void SelectNext();
	void SelectPrevious();

	//ステージのJSONのプレイヤーの情報を、選んだキャラクターのモデル・大きさ・アニメーションに差し替える
	//(位置は、ステージのJSONのまま)。一覧が空のときは、何もしない
	void ApplyToPlayer(ObjectData& playerData) const;

	//出来事("shot"など)で鳴らす音の名前。選んでいるキャラクターに専用の音があれば、その名前。なければ、出来事の名前のまま
	std::string ResolveSoundName(const std::string& eventName) const;

private:
	std::vector<CharacterData> _characters;
	int _selectedIndex = 0;
};
