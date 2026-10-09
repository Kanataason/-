#include "CharacterRoster.h"

#include <algorithm>
#include <fstream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <ThirdParty/nlohmann/json.hpp>

#include "StringUtility.h"

using json = nlohmann::json;

void CharacterRoster::Load(const std::string& jsonPath)
{
	_characters.clear();
	_selectedIndex = 0;

	std::ifstream file(jsonPath);
	if (!file.is_open())
	{
		OutputDebugStringA("CharacterRoster: Characters.json not found\n");
		return;
	}

	//書き方を間違えていても、例外を投げずに空で返す
	const json root = json::parse(file, nullptr, false);
	if (root.is_discarded() || !root.is_object() || !root.contains("Characters") || !root["Characters"].is_array())
	{
		OutputDebugStringA("CharacterRoster: Characters.json is invalid\n");
		return;
	}

	for (const auto& entry : root["Characters"])
	{
		if (!entry.is_object())
			continue;

		CharacterData character;
		character.modelPath = entry.value("path", "");
		if (character.modelPath.empty())
			continue; //モデルのないキャラクターは、選べても出せないので、飛ばす

		character.name = StringUtility::ToWide(entry.value("name", character.modelPath));
		character.scale = entry.value("scale", character.scale);
		character.colliderScale = entry.value("colliderScale", character.colliderScale);
		character.bounceRestitution = entry.value("bounceRestitution", character.bounceRestitution);
		character.bounceCount = entry.value("bounceCount", character.bounceCount);

		if (entry.contains("animations") && entry["animations"].is_array())
		{
			for (const auto& animation : entry["animations"])
			{
				if (!animation.is_object())
					continue;

				const std::string name = animation.value("name", "");
				const std::string path = animation.value("path", "");
				if (!name.empty() && !path.empty())
					character.animations.push_back({ name, path });
			}
		}

		if (entry.contains("sounds") && entry["sounds"].is_object())
		{
			for (const auto& item : entry["sounds"].items())
			{
				if (item.value().is_string())
					character.sounds[item.key()] = item.value().get<std::string>();
			}
		}

		_characters.push_back(std::move(character));
	}
}

std::string CharacterRoster::ResolveSoundName(const std::string& eventName) const
{
	if (const CharacterData* character = GetSelected())
	{
		const auto it = character->sounds.find(eventName);
		if (it != character->sounds.end())
			return it->second;
	}
	return eventName;
}

const CharacterData* CharacterRoster::GetSelected() const
{
	if (_characters.empty())
		return nullptr;
	return &_characters[static_cast<size_t>(_selectedIndex)];
}

void CharacterRoster::Select(int index)
{
	if (_characters.empty())
		return;
	_selectedIndex = (std::max)(0, (std::min)(index, GetCount() - 1));
}

void CharacterRoster::SelectNext()
{
	if (_characters.empty())
		return;
	_selectedIndex = (_selectedIndex + 1) % GetCount();
}

void CharacterRoster::SelectPrevious()
{
	if (_characters.empty())
		return;
	_selectedIndex = (_selectedIndex + GetCount() - 1) % GetCount();
}

void CharacterRoster::ApplyToPlayer(ObjectData& playerData) const
{
	const CharacterData* character = GetSelected();
	if (!character)
		return;

	playerData.attribute = Attribute::File;
	playerData.modelPath = character->modelPath;
	playerData.transform.Scale = { character->scale, character->scale, character->scale };
	playerData.animationDatas = character->animations;
	playerData.colliderScale = character->colliderScale;
	playerData.bounceRestitution = character->bounceRestitution;
	playerData.bounceCount = character->bounceCount;
}
