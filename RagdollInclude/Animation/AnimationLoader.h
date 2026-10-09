#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Animation.h"
#include "Skeleton.h"
#include "BoneNameUtility.h"

class AnimationLoader
{
public:
    std::shared_ptr<Animation> Load(
        const AnimationData& animationData,
        Skeleton& skeleton)
    {
        if (animationData.AnimationPath.empty())return nullptr;

        auto found = _cacheList.find(animationData.AnimationPath);

        if (found != _cacheList.end())
        {
            OutputDebugStringA("キャッシュ使用\n");
            return found->second;
        }

        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            animationData.AnimationPath,
            aiProcess_Triangulate |
            aiProcess_LimitBoneWeights);

        if (scene == nullptr || scene->mNumAnimations == 0)
        {
            return nullptr;
        }

        const aiAnimation* aiAnim = scene->mAnimations[0];

        auto animation = std::make_shared<Animation>();

        animation->sourceName = aiAnim->mName.C_Str();
        animation->animationName = animationData.AnimationName;
        animation->Duration = aiAnim->mDuration;
        animation->TicksPerSecond = aiAnim->mTicksPerSecond;

        for (unsigned i = 0; i < aiAnim->mNumChannels; ++i)
        {
            const aiNodeAnim* channel = aiAnim->mChannels[i];

            const std::string nodeName =
                channel->mNodeName.C_Str();

            auto resultName = NormalizeBoneName(nodeName);
            //Boneだけでなく全Nodeと照合する
            if (skeleton.FindNode(resultName) < 0)
            {
                continue;
            }

            BoneAnimation nodeAnimation;
            nodeAnimation.BoneName = resultName;

            for (unsigned k = 0;
                k < channel->mNumPositionKeys;
                ++k)
            {
                const auto& key = channel->mPositionKeys[k];

                nodeAnimation.Positions.push_back({
                    key.mTime,
                    { key.mValue.x, key.mValue.y, key.mValue.z }
                    });
            }

            for (unsigned k = 0;
                k < channel->mNumRotationKeys;
                ++k)
            {
                const auto& key = channel->mRotationKeys[k];

                nodeAnimation.Rotations.push_back({
                    key.mTime,
                    { key.mValue.x, key.mValue.y,
                      key.mValue.z, key.mValue.w }
                    });
            }

            for (unsigned k = 0;
                k < channel->mNumScalingKeys;
                ++k)
            {
                const auto& key = channel->mScalingKeys[k];

                nodeAnimation.Scales.push_back({
                    key.mTime,
                    { key.mValue.x, key.mValue.y, key.mValue.z }
                    });
            }

            animation->Bones.push_back(
                std::move(nodeAnimation));
        }

        _cacheList[animationData.AnimationPath] = animation;
        return animation;
    }
    std::string NormalizeBoneName(std::string name)
    {
        constexpr std::string_view assimpMarker = "*$AssimpFbx$";

        const size_t pos = name.find(assimpMarker);
        if (pos != std::string::npos)
        {
            name.erase(pos);

            //LeftArm_$AssimpFbx$*Rotation → LeftArm
            if (!name.empty() && name.back() == '_')
                name.pop_back();
        }

        //mixamorig9:Hips → mixamorig:Hips(モデル側の名前も、同じようにそろえている)
        return BoneNameUtility::NormalizeMixamoPrefix(std::move(name));
    }

private:
    std::unordered_map<
        std::string,
        std::shared_ptr<Animation>
    > _cacheList;
};