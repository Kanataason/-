#include "BulletManager.h"
#include "GameObject.h"
#include "GameWindow.h"

void BulletManager::Initialize(ServiceLocator& locator)
{
	_screenBounds = &locator.gameWindow->GetBounds();
}

void BulletManager::Update(float deltaTime)
{
	//移動処理だけをする
	_bulletPool.ForEachActive([this, deltaTime](Bullet& bullet)
		{
			BulletMovement(bullet,deltaTime);
			if (ShouldRelease(bullet, deltaTime))
				_bulletPool.Release(&bullet);
		});
}
void BulletManager::BulletMovement(Bullet& bullet,float deltaTime)
{
	bullet.position += bullet.velocity * deltaTime;
	Reflect(bullet);
}

void BulletManager::DrawBullet(const Bullet& bullet)
{
	const Vector2 tailPosition = bullet.GetTail();

	DrawLine(static_cast<int>(bullet.position.x), static_cast<int>(bullet.position.y),
		static_cast<int>(tailPosition.x), static_cast<int>(tailPosition.y),
		bullet.color, static_cast<int>(bullet.thickness));
}
void BulletManager::Draw()
{
	//描画処理だけをする
	_bulletPool.ForEachActive([this](Bullet& bullet) {DrawBullet(bullet);});
}

void BulletManager::FireBullet(const Vector2& position, const Vector2& velocity, const ShotSetting& data, int color, Team team)
{
	//満杯ならnullptrが返るが、使わないのでそのまま無視してよい
	_bulletPool.Acquire(position, velocity, data.bulletLife,
		data.bulletDamage, data.length, color, team, data.thickness,data.bounceCount);
}
void BulletManager::Reflect(Bullet& bullet)
{
	if (bullet.bounceCount <= 0) return;

	const float width = _screenBounds->width;
	const float height = _screenBounds->height;

	//各壁に対して反射処理をする
	if (bullet.position.x < 0.0f || bullet.position.x > width)
	{
		bullet.position.x = std::clamp(bullet.position.x, 0.0f, width);   //壁の外にはみ出した分を戻す
		bullet.velocity.x = -bullet.velocity.x;
		bullet.direction.x = -bullet.direction.x;   //描画と当たり判定の向きも反転する
		--bullet.bounceCount;
	}
	if (bullet.position.y < 0.0f || bullet.position.y > height)
	{
		bullet.position.y = std::clamp(bullet.position.y, 0.0f, height);   //壁の外にはみ出した分を戻す
		bullet.velocity.y = -bullet.velocity.y;
		bullet.direction.y = -bullet.direction.y;   //描画と当たり判定の向きも反転する
		--bullet.bounceCount;
	}
}

bool BulletManager::ShouldRelease(Bullet& bullet, float deltaTime)
{
	bullet.lifeTime -= deltaTime;
	if (bullet.lifeTime <= 0.0f) return true;
	return !_screenBounds->Contains(bullet.position, 10.0f);
}