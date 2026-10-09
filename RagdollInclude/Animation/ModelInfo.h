#pragma once
#include <string>
#include <iostream>

#include "Mesh.h"
#include "Material.h"
#include "Skeleton.h"
#include "Animation.h"

//ロードしたオブジェクトのデータを保持構造体
struct MeshData
{
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
};
//キャラクターの情報構造体
struct ModelDatas
{
    std::vector<MeshData> meshes;

    std::shared_ptr<Skeleton> skeleton;

    std::vector<std::shared_ptr<Animation>> animations;
    std::shared_ptr<Animation> animation;
};