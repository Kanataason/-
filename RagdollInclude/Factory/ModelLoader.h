#pragma once
#include <string>
#include <iostream>
#include <functional>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "ObjectType.h"
#include "Mesh.h"
#include "ModelInfo.h"
#include "Animation.h"

class Skeleton;
class Material;

class ModelLoader
{
public :

	//外部からのFBXを読み取る
	std::shared_ptr<ModelDatas> FBXLoadModel(const std::string& path);

	//もともと設定してある図形を読み取る
	std::shared_ptr<ModelDatas> PrimitiveLoadModel(const PrimitiveShape& shape);

	//Primitivのテーブルでの関数定義で読みやすくするために宣言
	static std::shared_ptr<Mesh> CreateBottomPivotBox();
private:

	//1メッシュ分の頂点・インデックス・ボーンウェイトをMeshInfoに追加
	void GetFbxObjectData(MeshInfo& mesh, const aiMesh* aimesh, const std::unordered_map<std::string, int>& boneMap);

	//マテリアルのDiffuseカラーとテクスチャパスを取得
	void GetFbxMaterialData(std::shared_ptr<Material>& material, aiMaterial* aimaterial);

	//FBXの先頭アニメーションをキーフレームデータとして取得
	void GetFbxAnimationData(const aiScene* scene, Animation& animation, Skeleton& skeleton);

	//ノード階層からボーンとして使われているものだけを抽出し
	void BuildSkeleton(aiNode* node, Skeleton& skeleton, const std::unordered_map<std::string, aiBone*>& boneMap,
		std::unordered_map<std::string, int>& boneIndexMap, int parentIndex);

	//ノード階層を再帰的に辿り、Skeletonへ全ノードを登録する
	int CreateBoneNode(const aiNode* node, Skeleton&, int parentIndex);

	//全メッシュを走査し、ボーン名→aiBoneの対応表を作成
	void CollectBones(const aiScene* scene, std::unordered_map<std::string, aiBone*>& boneMap);
};