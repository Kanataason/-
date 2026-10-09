#pragma once
#include <iostream>
#include <fstream>
#include <ThirdParty/nlohmann/json.hpp>

#include <string>
#include <DirectXMath.h>

#include "Material.h"
using json = nlohmann::json;
class MaterialLoader
{
public :
	std::shared_ptr<Material> LoadMaterial(const std::string& path);

	//Jsonからカラーとテクスチャを取得
	DirectX::XMFLOAT4 LoadColor(const json& root);
	std::wstring LoadTexturePath(const json& root);
	int LoadUnlit(const json& root);
};