#include <filesystem>
#include "Material.h"
#include "MaterialManager.h"
namespace fs = std::filesystem;
void MaterialManager::Initialize()
{
    //あらかじめマテリアルを取得する
	for (const auto& file : std::filesystem::directory_iterator("Asset/Material"))
	{
		if (file.path().extension() == ".mat")
		{
			std::string name =
				file.path().stem().string();

			_materialPath[name] = file.path().string();
		}
	}
    //デフォルトのマテリアルを取得する
    CreateDefaultMaterial();
}
void MaterialManager::CreateDefaultMaterial()
{
    auto mate = std::make_shared<Material>();
    auto texture = _textureManager.CheckTexture(L"Asset/Texture/default.png");
    mate->SetTexture(texture);
    mate->SetColor({ 1,1,1,1 });

    _materials["default"] = mate;
}
std::shared_ptr<Texture> MaterialManager::FbxCheckTexture(const std::wstring& path)
{
    return _textureManager.CheckTexture(path);
}

std::shared_ptr<Material> MaterialManager::CheckMaterial(const std::string& materialName)
{
    std::string name = materialName.empty() ? "default" : materialName;
    //マテリアルがすでにロードされているか確認
    auto material = _materials.find(name);
    if (material != _materials.end())
    {
        return material->second;
    }

    //そもそも名前が登録されているか確認
    auto path = _materialPath.find(name);
    if (path == _materialPath.end())
    {
        return _materials["default"];
    }

    //マテリアルをロード
    auto mate = _loader.LoadMaterial(path->second);

    //テクスチャのパスをロード
    auto& texturePath = mate->GetTexturePath();

    //パスがとれなかったらデフォルトを設定
    if (!texturePath.empty())
    {
        mate->SetTexture(
            _textureManager.CheckTexture(texturePath)
        );
    }
    else
    {
        mate->SetTexture(
            _textureManager.CheckTexture(
                L"Asset/Texture/default.png")
        );
    }

    _materials[name] = mate;

    return mate;
}