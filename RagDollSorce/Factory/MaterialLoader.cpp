#include "MaterialLoader.h"
std::shared_ptr<Material> MaterialLoader::LoadMaterial(
    const std::string& path)
{
    //マテリアルファイルを読み込む
    std::ifstream file(path);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open material file: " + path);
    }

    json root;
    file >> root;

    auto material = std::make_shared<Material>();

    //JSONからマテリアル情報を設定
    material->SetColor(LoadColor(root));
    material->SetTexturePath(LoadTexturePath(root));
    material->SetUnlit(LoadUnlit(root));

    return material;
}

int MaterialLoader::LoadUnlit(const json& root) { return root.value("unlit", false); }

DirectX::XMFLOAT4 MaterialLoader::LoadColor(
    const json& root)
{
    //色が設定されていない場合はデフォルト値を使用
    if (!root.contains("color"))
        return { 1.0f, 1.0f, 1.0f, 1.0f };

    const auto& color = root["color"];

    if (!color.is_array() || color.size() < 4)
        return { 1.0f, 1.0f, 1.0f, 1.0f };

    return
    {
        color[0].get<float>(),
        color[1].get<float>(),
        color[2].get<float>(),
        color[3].get<float>()
    };
}

std::wstring MaterialLoader::LoadTexturePath(
    const json& root)
{
    //テクスチャが設定されていない場合は空文字を返す
    if (!root.contains("texture"))
        return L"";

    const std::string path =
        root["texture"].get<std::string>();

    //DirectXで扱うためstd::wstringへ変換
    return std::wstring(path.begin(), path.end());
}