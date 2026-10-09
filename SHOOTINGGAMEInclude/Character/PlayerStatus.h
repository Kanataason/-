#pragma once
#include "Component.h"
#include "Datas.h"
#include "CharacterStatus.h"
#include "Primitive.h"

#include "PlayerProvider.h"

//円の半径を持つ。描画と移動制限に使う
class CircleShape : public Primitive
{
public:
	void SetRadius(float radius) { _radius = radius; }
	float GetRadius() const override { return _radius; }
private:
	float _radius = 0.0f;
};

//プレイヤーのHPと生存状態を持つ
class PlayerStatus : public Component, public CharacterStatus
{
public:
	Event<SoundId> OnHitSound;
	std::unique_ptr<Component> Clone()const override
	{
		return std::make_unique<PlayerStatus>(*this);
	}
	void Initialize(ServiceLocator& locator)override { _provider = locator.playerProvider; };
	void Update(float deltaTime)override {};

	void OnColliderEnter(float damage)override
	{
		if (!GetIsActive()) return;//死亡演出中はダメージを受けない。無敵中は当たり判定自体が無効になる
		ApplyDamage(damage);
		OnHitSound.Invoke(SoundId::PlayerHit);
	}
	void OnDead()override 
	{
		SetIsActive(false);
		//敵が撃つのを止めるために、生存状態を共有する
		_provider->SetPlayerActive(false);
	}

private:
	PlayerProvider* _provider = nullptr;
};

