#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "Animation.h"

class Skeleton
{
public:
    Skeleton()
    {
        XMStoreFloat4x4(&_globalInverseTransform, XMMatrixIdentity());
    }
    std::vector<Bone>& GetBones(){return _bones;}
    const std::vector<Bone>& GetBones()const { return _bones; }
    void SetBone(const Bone& bone) { _bones.push_back(bone); }

    std::vector<BoneNode>& GetNode() { return _nodes; }
    const std::vector<BoneNode>& GetNode()const { return _nodes; }
    void SetNode(const BoneNode& node) { _nodes.push_back(node); }

    //モデル空間からボーン空間への変換に使用する
    DirectX::XMFLOAT4X4& GetGlobalInverseTransform() { return _globalInverseTransform; }
    const DirectX::XMFLOAT4X4& GetGlobalInverseTransform() const { return _globalInverseTransform; }


    //名前からBoneのインデックスを検索
    //見つからない場合は-1を返す
    int FindBone(const std::string& name)
    {
        for (int i = 0; i < static_cast<int>(_bones.size()); ++i)
        {
            if (_bones[i].Name == name)
                return i;
        }

        return -1;
    }

    //名前からNodeのインデックスを検索
    //見つからない場合は-1を返す
    int FindNode(const std::string& name)
    {
        for (int i = 0; i < static_cast<int>(_nodes.size()); ++i)
        {
            if (_nodes[i].Name == name)
                return i;
        }

        return -1;
    }

private:
    //モデル全体の逆変換行列
    DirectX::XMFLOAT4X4 _globalInverseTransform;

    //アニメーション階層を構成するNode
    std::vector<BoneNode> _nodes;

    //スキニングに使用するBone
    std::vector<Bone> _bones;
};