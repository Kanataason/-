#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <fstream>

#include "Animator.h"
#include "Vertex.h"
#include "ConsoleUtility.h"
#include "MatrixUtility.h"
#include "MathUtility.h"
#include "GameObject.h"

#include "InputSystem.h"
#include "CollisionManager.h"
#include "PhysicsSystem.h"
#include "Ragdoll.h"

namespace
{
    //腰のノード名。Assimpがノードを分割するモデルでは、位置は、…_Translation という別のノードが持つ
    const std::string hipsName = "mixamorig:Hips";
    const std::string hipsTranslationNodeName = "mixamorig:Hips_$AssimpFbx$_Translation";
}

void Animator::Initialize(ServiceLocator& locator)
{
    _physicsSystem = locator.physicsSystem;
    _ragDoll = GetOwner()->GetComponent<Ragdoll>();

    _hasHipsTranslationNode = _skeleton && _skeleton->FindNode(hipsTranslationNodeName) >= 0;

    PlayAnimation("Idle",0.4f);
    SetIk();
}

void Animator::SetIk()
{
    //Ikように右足と左足のnodeをとる
    _leftChain.rootNode = _skeleton->FindNode("mixamorig:LeftUpLeg");
    _leftChain.middleNode = _skeleton->FindNode("mixamorig:LeftLeg");
    _leftChain.endNode = _skeleton->FindNode("mixamorig:LeftFoot");

    _rightChain.rootNode = _skeleton->FindNode("mixamorig:RightUpLeg");
    _rightChain.middleNode = _skeleton->FindNode("mixamorig:RightLeg");
    _rightChain.endNode = _skeleton->FindNode("mixamorig:RightFoot");
}
void Animator::Update(float deltaTime)
{
    if (!_animation || !_skeleton)
        return;

    if (_isStopAnimation)
        return;

    //再生時間を進める
    UpdateAnimationTime(_animation->TicksPerSecond, _animation->Duration, _currentTime, deltaTime);

    if (!_ragDoll->IsActive())
    {
        //通常アニメーション
        UpdateLocalTransform(deltaTime);
        UpdateGlobalTransform();

        UpdateIk(_rightChain);
        UpdateIk(_leftChain);

        UpdateGlobalTransform();
    }
    else
    {
        //ラグドール中は物理シミュレーション結果をボーンに反映
        _ragDoll->UpdateBonesFromPhysics();
    }

    //スキニング用の最終行列を計算
    UpdateFinalTransform();

    //ブレンド完了判定
    if (_isBlending && _blendTime >= _blendDuraction)
    {
        _isBlending = false;
        _blendSource.reset();
        _blendAnimationMap.clear();
    }
}

void Animator::PlayAnimation(const std::string& name,const float& duration =0.0f)
{
    auto found = _animations.find(name);

    if (found == _animations.end())
    {
        OutputDebugStringA("Not Find Animation");
        return;
    }

    const auto& nextAnimation = found->second;

    //初回再生 or ブレンド無しの場合は時間をリセット
    if (_animation == nullptr || duration <= 0.0f)
    {
        _currentTime = 0.0f;
        _isBlending = false;
    }

    //同じアニメーションを再生し直す必要はない
    if (_animation == nextAnimation)
        return;

    //ブレンド元として現在の状態を退避
    _blendSorceTime = _currentTime;
    _blendAnimationMap = std::move(_animationMap);
    _blendSource = _animation;

    //新しいアニメーションに切り替え
    _animation = nextAnimation;
    _currentTime = 0.0f;
    SetMap(_animation->Bones);

    _blendTime = 0.0f;
    _blendDuraction = duration;
    _isBlending = true;
}
void Animator::UpdateAnimationTime(const double& tickPerSecond, const double& duration, float& time,const float& deltaTime)
{
    float tick = static_cast<float>(tickPerSecond);

    if (tick <= 0.0f)
    {
        tick = 25.0f;
    }
    //秒からチックに変換をする
    time += deltaTime * tick;

    if (duration > 0.0)
    {
        time = std::fmod(time, static_cast<float>(duration));
    }
}
void Animator::UpdateIk(const IkChain& chain)
{
    //ボーンの名前が見つからなかった(-1)キャラクターは、足のIKをしない
    if (chain.rootNode < 0 || chain.middleNode < 0 || chain.endNode < 0)
        return;

    XMFLOAT3 hitPos, footPos;
    auto& nodes = _skeleton->GetNode();

    //足先のワールド座標を算出
    const auto& mat = XMLoadFloat4x4(&nodes[chain.endNode].GlobalTransform);
    const auto& pos = GetOwner()->GetTransform().GetWorldMatrix();
    const auto& world = XMMatrixMultiply(mat, pos);
    XMVECTOR foots = world.r[3];
    XMStoreFloat3(&footPos, foots);
    footPos.y += 0.2f; //レイの開始位置を少し浮かせる

    //足元に地面があるかレイキャストで確認
    if (_physicsSystem->RayCast(footPos, { 0.0f, -1.0f, 0.0f }, 1.0f, hitPos))
    {
        XMFLOAT4X4 root, middle, end;
        XMStoreFloat4x4(&root, XMMatrixMultiply(XMLoadFloat4x4(&nodes[chain.rootNode].GlobalTransform), pos));
        XMStoreFloat4x4(&middle, XMMatrixMultiply(XMLoadFloat4x4(&nodes[chain.middleNode].GlobalTransform), pos));
        XMStoreFloat4x4(&end, world);

        SolveIk(chain, hitPos, root, middle, end);
    }
}
void Animator::SolveIk(const IkChain& chain, const XMFLOAT3& pos,const XMFLOAT4X4& root, const XMFLOAT4X4& middle, const XMFLOAT4X4& end)
{
    auto& nodes = _skeleton->GetNode();

    //Root/Middle/EndのワールドPositionを取得
    XMFLOAT3 r, m, e;
    r = MatrixUtility::GetTransform<MatrixUtility::ReturnTransform::Position>(root);
    m = MatrixUtility::GetTransform<MatrixUtility::ReturnTransform::Position>(middle);
    e = MatrixUtility::GetTransform<MatrixUtility::ReturnTransform::Position>(end);
    XMVECTOR rootPos = XMLoadFloat3(&r);
    XMVECTOR middlePos = XMLoadFloat3(&m);
    XMVECTOR endPos = XMLoadFloat3(&e);

    //ターゲット位置（少し浮かせて地面にめり込まないように）
    XMVECTOR targetPos = XMLoadFloat3(&pos) + XMVectorSet(0.0f, 0.3f, 0.0f, 0.0f);

    float upperLength = XMVectorGetX(XMVector3Length(middlePos - rootPos));
    float lowerLength = XMVectorGetX(XMVector3Length(endPos - middlePos));
    float targetLength = XMVectorGetX(XMVector3Length(targetPos - rootPos));

    //脚の長さで届かない場合は補正しない
    if (targetLength > upperLength + lowerLength)
        return;

    if (targetLength < fabs(upperLength - lowerLength))
    {
        OutputDebugStringA("近い\n");
        return;
    }

    //Middle→End と Middle→Target のなす角を求め、その差分だけMiddleを回転
    XMVECTOR toEnd = MathUtility::Normalize(middlePos, endPos);
    XMVECTOR toTarget = MathUtility::Normalize(middlePos, targetPos);

    XMVECTOR axis = XMVector3Normalize(MathUtility::CrossVector3(toEnd, toTarget));
    float dot = std::clamp(XMVectorGetX(XMVector3Dot(toEnd, toTarget)), -1.0f, 1.0f);
    float angle = acosf(dot);
    XMVECTOR rotation = XMQuaternionRotationAxis(axis, angle);

    //ワールド回転量をMiddleNodeのローカル空間に変換して適用
    XMMATRIX middleLocal = XMLoadFloat4x4(&nodes[chain.middleNode].LocalTransform);
    XMVECTOR currentRotation = XMQuaternionRotationMatrix(middleLocal);

    int parentIndex = nodes[chain.middleNode].ParentIndex;
    if (parentIndex < 0)
        return;
    XMFLOAT4 pG = MatrixUtility::GetTransform<MatrixUtility::ReturnTransform::Rotation>(nodes[parentIndex].GlobalTransform);
    XMVECTOR parentWorldRotation = XMLoadFloat4(&pG);

    XMVECTOR parentInverse = XMQuaternionInverse(parentWorldRotation);

    XMVECTOR localIkRotation =
        XMQuaternionMultiply(XMQuaternionMultiply(parentInverse, rotation), parentWorldRotation);

    XMVECTOR newRotation = XMQuaternionNormalize(XMQuaternionMultiply(currentRotation, localIkRotation));

    //Scale/Positionは維持したままRotationだけ差し替えてLocalTransformを再構築
    XMFLOAT3 scale = MatrixUtility::GetTransform<MatrixUtility::ReturnTransform::Scale>(nodes[chain.middleNode].LocalTransform);
    XMFLOAT3 position = MatrixUtility::GetTransform<MatrixUtility::ReturnTransform::Position>(nodes[chain.middleNode].LocalTransform);

    XMMATRIX newLocal =
        XMMatrixScaling(scale.x, scale.y, scale.z)
        * XMMatrixRotationQuaternion(newRotation)
        * XMMatrixTranslation(position.x, position.y, position.z);

    XMStoreFloat4x4(&nodes[chain.middleNode].LocalTransform, newLocal);
}

void Animator::UpdateLocalTransform(const float& deltaTime)
{ 
    float blendRate = 1.0f;

    //ブレンド中はソース側のアニメーション時間も進める
    if (_isBlending)
    {
        _blendTime += deltaTime;
        UpdateAnimationTime(_blendSource->TicksPerSecond, _blendSource->Duration, _blendSorceTime, deltaTime);
        blendRate = std::min(_blendTime / _blendDuraction, 1.0f);
    }

    for (auto& node : _skeleton->GetNode())
    {
        const BoneAnimation* currentTrack = nullptr;
        const BoneAnimation* sorceTrack = nullptr;

        node.LocalTransform = node.BindTransform; //一旦バインド姿勢に戻す

        //腰の位置だけを持つ別ノード(…_Translation)は、アニメーションの名前が、本体の 'mixamorig:Hips' なので、そちらの動きを使う
        const std::string& trackName = (node.Name == hipsTranslationNodeName) ? hipsName : node.Name;

        if (const auto it = _animationMap.find(trackName); it != _animationMap.end())
            currentTrack = &it->second;

        if (_isBlending)
        {
            if (const auto it = _blendAnimationMap.find(trackName); it != _blendAnimationMap.end())
                sorceTrack = &it->second;
        }

        const XMMATRIX targetLocal = SampleLocalTransform(node, currentTrack, _currentTime);

        if (!_isBlending)
        {
            XMStoreFloat4x4(&node.LocalTransform, targetLocal);
            continue;
        }

        //ブレンド元とブレンド先をSRTに分解して補間（回転のみSlerp）
        const XMMATRIX sorceLocal = SampleLocalTransform(node, sorceTrack, _blendSorceTime);

        XMVECTOR sorceScale, sorceRotation, sorcePosition;
        XMVECTOR targetScale, targetRotation, targetPosition;
        XMMatrixDecompose(&sorceScale, &sorceRotation, &sorcePosition, sorceLocal);
        XMMatrixDecompose(&targetScale, &targetRotation, &targetPosition, targetLocal);

        const auto scale = XMVectorLerp(sorceScale, targetScale, blendRate);
        const auto rotation = XMQuaternionSlerp(sorceRotation, targetRotation, blendRate);
        const auto position = XMVectorLerp(sorcePosition, targetPosition, blendRate);

        const XMMATRIX blendLocal =
            XMMatrixScalingFromVector(scale) *
            XMMatrixRotationQuaternion(rotation) *
            XMMatrixTranslationFromVector(position);

        XMStoreFloat4x4(&node.LocalTransform, blendLocal);
    }
}
void Animator::UpdateGlobalTransform()
{
    //親から子へ順にLocal×ParentGlobalでGlobalを算出
    for (size_t i = 0; i < _skeleton->GetNode().size(); i++)
    {
        BoneNode& node = _skeleton->GetNode()[i];
        XMMATRIX local = XMLoadFloat4x4(&node.LocalTransform);

        if (node.ParentIndex < 0)
        {
            XMStoreFloat4x4(&node.GlobalTransform, local);
        }
        else
        {
            BoneNode& parent = _skeleton->GetNode()[node.ParentIndex];
            XMMATRIX global = local * XMLoadFloat4x4(&parent.GlobalTransform);
            XMStoreFloat4x4(&node.GlobalTransform, global);
        }
    }

    //計算したNodeのGlobalTransformをBoneにも反映
    auto& nodes = _skeleton->GetNode();
    for (auto& bone : _skeleton->GetBones())
    {
        bone.GlobalTransform = nodes[bone.NodeIndex].GlobalTransform;
    }
}
void Animator::UpdateFinalTransform()
{
    //スキニング行列 = Offset * Global * GlobalInverse（転置してシェーダ用に格納）
    _finalTransforms.resize(_skeleton->GetBones().size());

    for (size_t i = 0; i < _skeleton->GetBones().size(); ++i)
    {
        Bone& bone = _skeleton->GetBones()[i];

        XMMATRIX global = XMLoadFloat4x4(&bone.GlobalTransform);
        XMMATRIX offset = XMLoadFloat4x4(&bone.OffsetMatrix);
        XMMATRIX globalInverse = XMLoadFloat4x4(&_skeleton->GetGlobalInverseTransform());

        XMMATRIX final = XMMatrixTranspose(offset * global * globalInverse);

        XMStoreFloat4x4(&_finalTransforms[i], final);
        XMStoreFloat4x4(&bone.FinalTransform, final);
    }
}

XMMATRIX Animator::SampleLocalTransform(const BoneNode& node, const BoneAnimation* nodeAnimation,const float& time)const
{
    XMVECTOR scale, rotation, position;

    //まずバインド姿勢をSRTに分解（アニメーションが無いボーン用のデフォルト値）
    XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&node.BindTransform));

    //アニメーションデータがあれば各成分を上書き
    if (nodeAnimation != nullptr)
    {
        //・Assimpが、ノードを分割するモデル: 位置は '…Hips_$AssimpFbx$_Translation' が持ち、本体の位置は0
        //このとき、アニメーションの位置を本体に足すと、位置が二重になり、腰が、ありえない高さに出る
        const bool isHipsTranslationNode = (node.Name == hipsTranslationNodeName);
        const bool holdsHipsTranslation =
            isHipsTranslationNode || (node.Name == hipsName && !_hasHipsTranslationNode);

        if (!nodeAnimation->Positions.empty() && holdsHipsTranslation)
        {
            //骨の長さや単位が違うキャラクターは、腕や足が伸び縮みする。
            //そこで、体の高さ(Hips)以外は、キャラクター自身の骨の位置(バインド姿勢)を使い、回転だけをアニメーションに従う
            auto p = AnimationMath::InterpolatePosition(nodeAnimation->Positions, time);

            //Hipsは、体ぜんたいの高さの動き。キャラクターの腰の高さと、アニメーションの腰の高さの比で、大きさを合わせる
            const float animationHeight = nodeAnimation->Positions.front().Value.y;
            const float bindHeight = XMVectorGetY(position); //このノードの、バインドの高さ(位置を持つノードなので、腰の高さ)
            float ratio = 1.0f;
            if (std::fabs(animationHeight) > 1e-4f && std::fabs(bindHeight) > 1e-4f)
            {
                ratio = bindHeight / animationHeight;
                if (std::fabs(ratio - 1.0f) < 0.15f)
                    ratio = 1.0f;
            }
            position = XMVectorSet(p.x * ratio, p.y * ratio, p.z * ratio, 0.0f);
        }
        //それ以外のノードは、positionをバインド姿勢のままにする

        //位置だけを担当するノードには、回転と大きさを入れない(それらは、本体のノードに入る)
        if (!isHipsTranslationNode)
        {
            if (!nodeAnimation->Rotations.empty())
            {
                auto r = AnimationMath::InterpolateRotation(nodeAnimation->Rotations, time);
                rotation = XMLoadFloat4(&r);
            }

            if (!nodeAnimation->Scales.empty())
            {
                auto s = AnimationMath::InterpolateScale(nodeAnimation->Scales, time);
                scale = XMLoadFloat3(&s);
            }
        }
    }

    return XMMatrixScalingFromVector(scale) *
        XMMatrixRotationQuaternion(rotation) *
        XMMatrixTranslationFromVector(position);
}
