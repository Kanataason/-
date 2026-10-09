#include "StageRoster.h"

#include <algorithm>
#include <fstream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <ThirdParty/nlohmann/json.hpp>

#include "StringUtility.h"

using json = nlohmann::json;

void StageRoster::Load(const std::string& jsonPath)
{
	_stages.clear();
	_selectedIndex = 0;

	std::ifstream file(jsonPath);
	if (!file.is_open())
	{
		OutputDebugStringA("StageRoster: Stages.json not found\n");
		return;
	}

	//書き方を間違えていても、例外を投げずに、空で返す
	const json root = json::parse(file, nullptr, false);
	if (root.is_discarded() || !root.is_object() || !root.contains("Stages") || !root["Stages"].is_array())
	{
		OutputDebugStringA("StageRoster: Stages.json is invalid\n");
		return;
	}

	for (const auto& entry : root["Stages"])
	{
		if (!entry.is_object())
			continue;

		StageEntry stage;
		stage.path = entry.value("path", "");
		if (stage.path.empty())
			continue; //JSONのパスがないステージは、選べても始められないので、飛ばす

		stage.name = StringUtility::ToWide(entry.value("name", stage.path));
		_stages.push_back(std::move(stage));
	}
}

void StageRoster::Select(int index)
{
	if (_stages.empty())
	{
		_selectedIndex = 0;
		return;
	}
	_selectedIndex = std::clamp(index, 0, GetCount() - 1);
}

const std::string& StageRoster::GetSelectedPath() const
{
	static const std::string empty;
	if (_stages.empty())
		return empty;
	return _stages[static_cast<size_t>(_selectedIndex)].path;
}
