#include "ModelProvider.h"

//モデルを取得し、各メッシュにマテリアル/テクスチャを割り当てる
std::shared_ptr<ModelDatas> ModelProvider::Prepare(const ObjectData& data)
{
    auto model = _modelManager.CheckModel(data);

    //モデル読み込み失敗（未対応のPrimitiveやFBX読み込みエラー）
    if (model == nullptr)
        return nullptr;

    for (auto& mesh : model->meshes)
    {
        if (mesh.material != nullptr)
        {
            //マテリアルにテクスチャパスが指定されていれば読み込んで設定
            auto& path = mesh.material->GetTexturePath();

            if (!path.empty())
            {
                auto texture = _materialManager.FbxCheckTexture(path);

                if (texture != nullptr)
                {
                    mesh.material->SetTexture(texture);
                }
                else
                {
                    OutputDebugStringA("None Texture");
                }
            }
        }
        else
        {
            //マテリアル未設定のメッシュには指定名のマテリアルを割り当てる
            mesh.material = _materialManager.CheckMaterial(data.materialName);
        }
    }

    return model;
}
