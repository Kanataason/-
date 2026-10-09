#pragma once
#include <DirectXMath.h>
#include <unordered_map>
#include <string>

#include "Component.h"
#include "Animation.h"
#include "Skeleton.h"

using namespace DirectX;

class PhysicsSystem;
class InputSystem;
class CollisionManager;
class Ragdoll;

class Animator:public Component
{
	struct IkChain
	{
		int rootNode;
		int middleNode;
		int endNode;
	};
public: 
	void Initialize(ServiceLocator& locator)override;

	void SetAnimationInfo(const std::shared_ptr<Animation> animation, const std::shared_ptr<Skeleton>& skeleton,
		const std::vector<std::shared_ptr<Animation>>& animations)
	{
		_animation = animation;
		_skeleton = skeleton;
	
		AddAnimation(animations);
		SetMap(animation->Bones);
	}
	void PlayAnimation(const std::string& name,const float& duration);


	void Update(float deltaTime)override;

	//ゲッターとセッター

	void SetStopAnimation(bool isActive) { _isStopAnimation = isActive; };
	const std::vector<XMFLOAT4X4>& GetFinalTransforms() const { return _finalTransforms; }

private:
	//アニメーションの再生時間を計算
	void UpdateAnimationTime(const double& tickPerSecond, const double& duration, float& time, const float& deltaTime);
	void UpdateLocalTransform(const float& deltaTime);
	void UpdateGlobalTransform();
	void UpdateFinalTransform();
	XMMATRIX SampleLocalTransform(const BoneNode& node, const BoneAnimation* nodeAnimation, const float& time)const;

	//Ikの処理
	void UpdateIk(const IkChain& chain);
	void SolveIk(const IkChain& chain, const XMFLOAT3& pos, const XMFLOAT4X4& root, const XMFLOAT4X4& middle, const XMFLOAT4X4& end);

	void SetIk();
	void SetMap(const std::vector<BoneAnimation>& animationKey)
	{
		_animationMap.clear();

		for (const BoneAnimation& bone : animationKey)
		{
			_animationMap[bone.BoneName] = bone;
		}

	}
	//複数のアニメーションを保持
	void AddAnimation(const std::vector<std::shared_ptr<Animation>>& animations)
	{
		if (animations.empty())return;

		for (const auto& animation : animations)
		{
			if (animation == nullptr)continue;

			_animations[animation->animationName] = animation;
		}
	}

private:
	bool _isStopAnimation = false;
	//再生可能なアニメーション
	std::unordered_map<std::string, std::shared_ptr<Animation>> _animations;

	//各アニメーションの値
	std::unordered_map<std::string, BoneAnimation> _animationMap;

	//ブレンド用のアニメーションマップ
	std::unordered_map<std::string, BoneAnimation> _blendAnimationMap;

	//ボーンの名前マップ
	std::unordered_map < std::string, std::string> _boneNameMap;

	float _blendSorceTime = 0.0f;
	float _currentTime = 0.0f;
    std::shared_ptr<Animation> _animation;//現在再生中のアニメーション
	std::shared_ptr<Animation> _blendSource;//遷移先のアニメーション

	bool _isBlending = false;
	float _blendTime = 0.0f;
	float _blendDuraction = 0.0f;

	std::vector<XMFLOAT4X4>_finalTransforms;
	std::shared_ptr<Skeleton> _skeleton;

	PhysicsSystem* _physicsSystem = nullptr;
	InputSystem* _input = nullptr;
	Ragdoll* _ragDoll = nullptr;

	bool _hasHipsTranslationNode = false; //腰の位置が、別ノード(…_Translation)に分かれているモデルか

	IkChain _rightChain{};
	IkChain _leftChain{};
};
