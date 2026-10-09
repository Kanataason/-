#pragma once
#include <iostream>
#include <unordered_map>
#include <string>


#include "MaterialLoader.h"
#include "TextureManager.h"

class Material;

class MaterialManager
{
public :
	MaterialManager(MaterialLoader& loader,TextureManager& texManager)
		:_loader(loader),_textureManager(texManager) {}

	void Initialize();
	void CreateDefaultMaterial();
	//マテリアルやテクスチャのロード状態を見る
	std::shared_ptr<Material> CheckMaterial(const std::string& path);
	std::shared_ptr<Texture> FbxCheckTexture(const std::wstring& path);

private:
	std::unordered_map <std::string, std::shared_ptr<Material>> _materials;
	std::unordered_map<std::string, std::string> _materialPath;

	TextureManager& _textureManager;
	MaterialLoader& _loader;
};