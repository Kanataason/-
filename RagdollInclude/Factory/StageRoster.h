#pragma once
#include <string>
#include <vector>

//遊べるステージ1つぶんの情報
struct StageEntry
{
	std::wstring name; //選ぶ画面に出す名前
	std::string path; //ステージのJSON(的の配置や、玉数、星の点数)
};

//遊べるステージの一覧と、いま選んでいるステージ。
//ゲーム全体で1つだけ持つ(「もういちど」でシーンを作り直しても、同じステージになるように、GameManagerが持つ)
class StageRoster
{
public:
	//JSONから一覧を読む。ファイルがない・書き方が違うときは、空のまま(ステージ選択が、空になる)
	void Load(const std::string& jsonPath);

	int GetCount() const { return static_cast<int>(_stages.size()); }
	const std::vector<StageEntry>& GetList() const { return _stages; }
	int GetSelectedIndex() const { return _selectedIndex; }

	//遊ぶステージを選ぶ。範囲の外の番号は、端にそろえる
	void Select(int index);

	//選んでいるステージのJSONのパス。一覧が空のときは、空文字
	const std::string& GetSelectedPath() const;

private:
	std::vector<StageEntry> _stages;
	int _selectedIndex = 0;
};
