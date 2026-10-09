#pragma once
#include <unordered_map>
#include <string>
#include <iostream>
#include "GameObject.h"

#include "Datas.h"

//使ってないスクリプト
class ModelManager
{
public:
	ModelManager() = default;

	const GameObject* LoadedObjectData(const CharacterType& Ctype)const;
	void SetObjectData(const GameObject& object, const CharacterType& Ctype);
private:
	std::unordered_map<CharacterType, std::unique_ptr<GameObject>> _models;
};