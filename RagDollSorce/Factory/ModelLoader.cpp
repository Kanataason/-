#include <vector>
#include <filesystem>

#include "ModelLoader.h"
#include "Skeleton.h"
#include "Material.h"
#include "ConsoleUtility.h"
#include "MatrixUtility.h"
#include "BoneNameUtility.h"


void ModelLoader::GetFbxObjectData(MeshInfo& meshinfo, const aiMesh* aimesh, const std::unordered_map<std::string, int>& boneMap)
{
    //既存メッシュの頂点数を基準に、このメッシュの頂点位置を記録
    const UINT offset =
        static_cast<UINT>(meshinfo.vertices.size());

    //必要な容量をあらかじめ確保し、vectorの再確保を抑える
    meshinfo.vertices.reserve(
        meshinfo.vertices.size() + aimesh->mNumVertices);

    meshinfo.indices.reserve(
        meshinfo.indices.size() + aimesh->mNumFaces * 3);

    for (unsigned i = 0; i < aimesh->mNumVertices; ++i)
    {
        Vertex vertex{};

        vertex.Position =
        {
            aimesh->mVertices[i].x,
            aimesh->mVertices[i].y,
            aimesh->mVertices[i].z
        };

        vertex.Normal =
        {
            aimesh->mNormals[i].x,
            aimesh->mNormals[i].y,
            aimesh->mNormals[i].z
        };

        //UVが存在しないメッシュにはデフォルト値を設定
        if (aimesh->HasTextureCoords(0))
        {
            //FBXのUVは、下がV=0(OpenGL式)。DirectXで読み込んだ画像は、上がV=0なので、Vを反転する
            //(しないと、テクスチャが上下逆に貼られる: 頭に足の模様、など)
            vertex.UV =
            {
                aimesh->mTextureCoords[0][i].x,
                1.0f - aimesh->mTextureCoords[0][i].y
            };
        }
        else
        {
            vertex.UV = { 0.0f, 0.0f };
        }

        meshinfo.vertices.push_back(vertex);
    }

    //ボーンウェイトを各頂点に設定
    for (unsigned boneIndex = 0;
        boneIndex < aimesh->mNumBones;
        ++boneIndex)
    {
        const aiBone* bone = aimesh->mBones[boneIndex];

        const std::string boneName =
            bone->mName.C_Str();

        auto it = boneMap.find(boneName);

        if (it == boneMap.end())
            continue;

        const int skeletonIndex = it->second;

        for (unsigned j = 0;
            j < bone->mNumWeights;
            ++j)
        {
            const unsigned vertexId =
                bone->mWeights[j].mVertexId;

            const float weight =
                bone->mWeights[j].mWeight;

            Vertex& vertex =
                meshinfo.vertices[offset + vertexId];

            //最大4つのボーンウェイトを頂点に格納
            if (vertex.Weight.x == 0.0f)
            {
                vertex.BoneIndex.x = skeletonIndex;
                vertex.Weight.x = weight;
            }
            else if (vertex.Weight.y == 0.0f)
            {
                vertex.BoneIndex.y = skeletonIndex;
                vertex.Weight.y = weight;
            }
            else if (vertex.Weight.z == 0.0f)
            {
                vertex.BoneIndex.z = skeletonIndex;
                vertex.Weight.z = weight;
            }
            else if (vertex.Weight.w == 0.0f)
            {
                vertex.BoneIndex.w = skeletonIndex;
                vertex.Weight.w = weight;
            }
        }
    }

    //Triangulate済みなので、各面は3頂点としてインデックスを追加
    for (unsigned i = 0; i < aimesh->mNumFaces; ++i)
    {
        const aiFace& face = aimesh->mFaces[i];

        meshinfo.indices.push_back(face.mIndices[0] + offset);
        meshinfo.indices.push_back(face.mIndices[1] + offset);
        meshinfo.indices.push_back(face.mIndices[2] + offset);
    }

}



int ModelLoader::CreateBoneNode(const aiNode* node,Skeleton& nodes, int parentIndex)
{
    BoneNode boneNode;

    //mixamorig9: などの番号つきの名前は、mixamorig: にそろえる(ラグドール・IKが名前で探すため)
    boneNode.Name =
        BoneNameUtility::NormalizeMixamoPrefix(node->mName.C_Str());

    const XMFLOAT4X4 localTransform =
        MatrixUtility::ConvertMatrix(node->mTransformation);

    const XMMATRIX localBind =
        XMLoadFloat4x4(&localTransform);

    boneNode.BindTransform = localTransform;
    boneNode.LocalTransform = localTransform;
    if (parentIndex >= 0)
    {
        //親の累積済みBindTransformを掛けて、モデル空間での累積値にする
        XMMATRIX parentBind =
            XMLoadFloat4x4(&nodes.GetNode()[parentIndex].BindGlobalTransform);

        XMStoreFloat4x4(
            &boneNode.BindGlobalTransform,
            XMMatrixMultiply(localBind, parentBind));
    }
    else
    {
        //ルートは親が無いのでローカル=モデル空間
        XMStoreFloat4x4(&boneNode.BindGlobalTransform, localBind);
    }
    boneNode.ParentIndex =
        parentIndex;

    int index =
        static_cast<int>(nodes.GetNode().size());

    nodes.SetNode(boneNode);
    //回帰処理ですべての子供に対して処理をする
    for (unsigned i = 0;
        i < node->mNumChildren;
        ++i)
    {
        CreateBoneNode(
            node->mChildren[i],
            nodes,
            index);
    }

    return index;
}



void ModelLoader::GetFbxMaterialData(std::shared_ptr<Material>& material, aiMaterial* aimaterial)
{

    //Diffuse Color
    aiColor4D color;

    if (aiGetMaterialColor(
        aimaterial,
        AI_MATKEY_COLOR_DIFFUSE,
        &color) == AI_SUCCESS)
    {
        material->SetColor(
            {
                color.r,
                color.g,
                color.b,
                color.a
            });
    }


    material->SetTexturePath(L"");

    aiString path;
    //Diffuseテクスチャが無ければここで終了
    if (aimaterial->GetTexture(
        aiTextureType_DIFFUSE,
        0,
        &path) != AI_SUCCESS)
    {
        return;
    }

    //FBX側のパスは無視してファイル名だけ使用し、プロジェクトのAsset配下として組み立てる
    std::filesystem::path texturePath(path.C_Str());
    const std::filesystem::path fileName = texturePath.filename();
    const std::filesystem::path resultPath = std::filesystem::path(L"Asset/Texture") / fileName;

    material->SetTexturePath(
        resultPath.lexically_normal().wstring());
}



void ModelLoader::BuildSkeleton(aiNode* node, Skeleton& skeleton,
    const std::unordered_map<std::string, aiBone*>& boneMap, std::unordered_map<std::string, int>& boneIndexMap,int parentIndex)
{
    std::string name = node->mName.C_Str();

    auto it = boneMap.find(name);
    //このノードがボーンとして使われているか確認
    if (it != boneMap.end())
    {
        //boneMap / boneIndexMap は、FBXの元の名前で引く。Skeletonの中の名前は、そろえた名前
        const std::string normalizedName = BoneNameUtility::NormalizeMixamoPrefix(name);

        Bone bone;
        bone.Name = normalizedName;
        bone.ParentIndex = parentIndex;
        bone.OffsetMatrix = MatrixUtility::ConvertMatrix(it->second->mOffsetMatrix);
        bone.LocalTransform = MatrixUtility::ConvertMatrix(node->mTransformation);
        bone.NodeIndex = skeleton.FindNode(normalizedName);

        const int currentIndex = static_cast<int>(skeleton.GetBones().size());
        boneIndexMap[name] = currentIndex;

        skeleton.SetBone(bone); //TODO: AddBoneなど意図が伝わる名前を検討

        //子ノードもこのボーンを親として探索
        for (unsigned i = 0; i < node->mNumChildren; ++i)
        {
            BuildSkeleton(
                node->mChildren[i],
                skeleton,
                boneMap,
                boneIndexMap,
                currentIndex);
        }
        return;
    }

    //ボーンでなくても、その子にボーンがいる可能性があるので探索を続ける
    for (unsigned i = 0; i < node->mNumChildren; ++i)
    {
        BuildSkeleton(
            node->mChildren[i],
            skeleton,
            boneMap,
            boneIndexMap,
            parentIndex);
    }
}

void ModelLoader::GetFbxAnimationData(const aiScene* scene,Animation& animation,Skeleton& skeleton)
{
    //アニメーションが存在しない場合は終了
    if (scene->mNumAnimations == 0)
        return;

    aiAnimation* aiAnim = scene->mAnimations[0];

    animation.sourceName =
        aiAnim->mName.C_Str();

    animation.Duration =
        aiAnim->mDuration;

    animation.TicksPerSecond =
        aiAnim->mTicksPerSecond;

    //アニメーションチャンネル数分の領域を事前確保
    animation.Bones.reserve(aiAnim->mNumChannels);

    //ボーンごとのキーフレームチャンネルを変換
    for (unsigned i = 0; i < aiAnim->mNumChannels; ++i)
    {
        aiNodeAnim* channel = aiAnim->mChannels[i];
        const std::string nodeName = BoneNameUtility::NormalizeMixamoPrefix(channel->mNodeName.C_Str());

        //Skeletonに存在しないNodeは無視
        const int nodeIndex = skeleton.FindNode(nodeName);
        if (nodeIndex < 0)
            continue;

        BoneAnimation boneAnimation;
        boneAnimation.BoneName = nodeName;

        boneAnimation.Positions.reserve(channel->mNumPositionKeys);
        boneAnimation.Rotations.reserve(channel->mNumRotationKeys);
        boneAnimation.Scales.reserve(channel->mNumScalingKeys);

        //Position
        for (unsigned k = 0; k < channel->mNumPositionKeys; ++k)
        {
            const auto& key = channel->mPositionKeys[k];

            PositionKey keyframe;
            keyframe.Time = key.mTime;
            keyframe.Value = { key.mValue.x, key.mValue.y, key.mValue.z };

            boneAnimation.Positions.push_back(keyframe);
        }

        //Rotation
        for (unsigned k = 0; k < channel->mNumRotationKeys; ++k)
        {
            const auto& key = channel->mRotationKeys[k];

            RotationKey keyframe;
            keyframe.Time = key.mTime;
            keyframe.Value = { key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w };

            boneAnimation.Rotations.push_back(keyframe);
        }

        //Scale
        for (unsigned k = 0; k < channel->mNumScalingKeys; ++k)
        {
            const auto& key = channel->mScalingKeys[k];

            ScaleKey keyframe;
            keyframe.Time = key.mTime;
            keyframe.Value = { key.mValue.x, key.mValue.y, key.mValue.z };

            boneAnimation.Scales.push_back(keyframe);
        }

        animation.Bones.push_back(std::move(boneAnimation));
    }

}

std::shared_ptr<ModelDatas> ModelLoader::FBXLoadModel(const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |//すべてのポリゴンを三角形に変換
        aiProcess_LimitBoneWeights |//1頂点に影響するボーン数を制限
        aiProcess_JoinIdenticalVertices |//同じ頂点を結合して頂点数を削減
        aiProcess_GenSmoothNormals//滑らかな法線ベクトルを生成
    );

    if (!scene)
    {
        OutputDebugStringA(importer.GetErrorString());
        return nullptr;
    }

    //動かない物(ボーンもアニメーションもない)は、ノードの位置・回転・拡大を、頂点に焼き込む。
    //焼き込まないと、複数のパーツ(車輪・砲身など)が、全部、原点に同じ向きで重なって描かれる
    //(このゲームは、メッシュの頂点だけを読み、ノードの変換は使わないため)。
    //ボーンのあるモデル(プレイヤー)は、ノードの変換を、スケルトンとして使うので、そのままにする
    bool hasBones = false;
    for (unsigned i = 0; i < scene->mNumMeshes; ++i)
    {
        if (scene->mMeshes[i]->mNumBones > 0)
            hasBones = true;
    }
    if (!hasBones && scene->mNumAnimations == 0)
    {
        scene = importer.ApplyPostProcessing(aiProcess_PreTransformVertices);
        if (!scene)
        {
            OutputDebugStringA(importer.GetErrorString());
            return nullptr;
        }
    }

    auto data = std::make_shared<ModelDatas>();
    std::unordered_map<std::string, aiBone*> boneNames;
    std::unordered_map<std::string, int> boneIndexMap;

    //ボーン情報を収集
    CollectBones(scene, boneNames);

    //全ノードを登録してからSkeleton(ボーンのみ)を構築
    auto skeleton = std::make_shared<Skeleton>();
    CreateBoneNode(scene->mRootNode, *skeleton, -1);
    BuildSkeleton(scene->mRootNode, *skeleton, boneNames, boneIndexMap, -1);

    //ルートのBindTransformからモデル空間への逆行列を算出
    auto rootBind = XMLoadFloat4x4(&skeleton->GetNode()[0].BindTransform);
    XMStoreFloat4x4(&skeleton->GetGlobalInverseTransform(), XMMatrixInverse(nullptr, rootBind));

    //アニメーションデータを取得
    auto animation = std::make_shared<Animation>();
    GetFbxAnimationData(scene, *animation, *skeleton);
    data->animation = animation;
    data->skeleton = skeleton;

    //メッシュ・マテリアルを1つずつ変換して登録
    for (unsigned i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* sourceMesh = scene->mMeshes[i];

        auto material = std::make_shared<Material>();
        GetFbxMaterialData(material, scene->mMaterials[sourceMesh->mMaterialIndex]);

        MeshInfo info;
        GetFbxObjectData(info, sourceMesh, boneIndexMap);

        //ボーンウェイトの合計が1になるよう正規化
        for (auto& vertex : info.vertices)
        {
            const float total =
                vertex.Weight.x + vertex.Weight.y +
                vertex.Weight.z + vertex.Weight.w;

            if (total > 0.0f)
            {
                vertex.Weight.x /= total;
                vertex.Weight.y /= total;
                vertex.Weight.z /= total;
                vertex.Weight.w /= total;
            }
        }

        auto mesh = std::make_shared<Mesh>();
        mesh->SetMeshInfo(info);

        MeshData meshData;
        meshData.mesh = mesh;
        meshData.material = material;

        data->meshes.push_back(meshData);
    }

    return data;
}

void ModelLoader::CollectBones(const aiScene* scene,std::unordered_map<std::string, aiBone*>& boneMap)
{
    for (unsigned i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* aiMesh = scene->mMeshes[i];

        for (unsigned j = 0; j < aiMesh->mNumBones; j++)
        {
            aiBone* aibone = aiMesh->mBones[j];
            boneMap.emplace(aibone->mName.C_Str(), aibone);
        }
    }
}

std::shared_ptr<ModelDatas> ModelLoader::PrimitiveLoadModel(const PrimitiveShape& shape)
{
    //形ごとのメッシュ生成関数(役割=ObjectTypeには依存しない)
    static const std::unordered_map<PrimitiveShape, std::function<std::shared_ptr<Mesh>()>> creators =
    {
        { PrimitiveShape::Triangle,       []() { return std::make_shared<Mesh>(Mesh::Create<Primitive::TriangleMesh>(1.0)); } },
        { PrimitiveShape::Box,            []() { return std::make_shared<Mesh>(Mesh::Create<Primitive::BoxMesh>(1.0)); } },
        { PrimitiveShape::Quad,           []() { return std::make_shared<Mesh>(Mesh::Create<Primitive::QuadMesh>(1.0)); } },
        { PrimitiveShape::BoxBottomPivot, CreateBottomPivotBox }
    };
    //形が登録されているかどうか確認
    auto found = creators.find(shape);
    if (found == creators.end())
        return nullptr;

    auto data = std::make_shared<ModelDatas>();

    MeshData meshData;
    meshData.mesh = found->second();

    data->meshes.push_back(meshData);
    return data;
}
std::shared_ptr<Mesh> ModelLoader::CreateBottomPivotBox()
{
    auto mesh = std::make_shared<Mesh>(Mesh::Create<Primitive::BoxMesh>(1.0f));

    //箱の頂点は、中心が原点(Yが -0.5～0.5)。上に0.5ずらして、下端(Y=0)を原点にする
    for (auto& vertex : mesh->GetMeshInfos().vertices)
        vertex.Position.y += 0.5f;

    return mesh;
}
