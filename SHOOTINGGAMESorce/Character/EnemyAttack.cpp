#include "EnemyAttack.h"
#include "PlayerProvider.h"
#include "PlayerStatus.h"
#include "BulletManager.h"
#include "Primitive.h"

#include "GameWindow.h"
#include "GameObject.h"



void EnemyAttack::Initialize(ServiceLocator& locator)
{
	_bulletManager = locator.bulletManager;
	_gameWindow = locator.gameWindow;
	_provider = locator.playerProvider;
}

void EnemyAttack::InitializeShotSetting(int attackType)
{
	_shape = GetOwner()->GetComponent<Primitive>();
	if (const ShotSetting* setting = FindShotSetting(attackType))
		_gunData = *setting;
}

void EnemyAttack::LongRangeShot()
{
	if (!CanShoot()) return;
	const GameObject* player = _provider->GetPlayer();

	const Vector2& shooterPosition = GetOwner()->GetTransform().Position;
	Vector2 toPlayer = player->GetTransform().Position - shooterPosition;
	float distance = toPlayer.Length();
	if (distance <= 0.0001f) return; //ほぼ同じ位置なら撃たない

	//弾を出す場所は形によって変える
	Vector2 direction = toPlayer.Normalized(distance);
	_bulletManager->FireBullet(GetMuzzlePosition(direction), direction * _gunData.bulletSpeed,
		_gunData, _bulletColor, Team::Enemy);
}

void EnemyAttack::ReflectShot()
{
	if (!CanShoot()) return;

	Vector2 angleRange = GetNearerHorizontalAngle();
	int bulletCount = _gunData.bulletCount;
	if (bulletCount <= 0) return;

	//1発なら中央、2発以上なら両端を含めて均等に並べる
	float angleStep = (bulletCount > 1)
		? (angleRange.y - angleRange.x) / (bulletCount - 1)
		: 0.0f;
	float startDegrees = (bulletCount > 1)
		? angleRange.x
		: (angleRange.x + angleRange.y) / 2.0f;

	const Vector2& shooterPosition = GetOwner()->GetTransform().Position;
	for (int bulletIndex = 0; bulletIndex < bulletCount; ++bulletIndex)
	{
		Vector2 direction = DirectionFromAngle(startDegrees + angleStep * bulletIndex);
		_bulletManager->FireBullet(shooterPosition, direction * _gunData.bulletSpeed,
			_gunData, _bulletColor, Team::Enemy);
	}
}
void EnemyAttack::SpreadShot()
{
	if (!CanShoot()) return;

	int bulletCount = _gunData.bulletCount;
	if (bulletCount <= 0) return;

	float angleStep = 360.0f / static_cast<float>(bulletCount);

	const Vector2& shooterPosition = GetOwner()->GetTransform().Position;
	for (int bulletIndex = 0; bulletIndex < bulletCount; ++bulletIndex)
	{
		float degrees = angleStep * static_cast<float>(bulletIndex);
		float radians = degrees * DX_PI_F / 180.0f;
		Vector2 direction = { std::cos(radians), std::sin(radians) };

		_bulletManager->FireBullet(shooterPosition, direction * _gunData.bulletSpeed,
			_gunData, _bulletColor, Team::Enemy);
	}
}
void EnemyAttack::SwirlShot(float baseDegrees)
{
	if (!CanShoot()) return;

	int armCount = _gunData.bulletCount;//渦の腕の数
	if (armCount <= 0) return;

	float angleStep = 360.0f / static_cast<float>(armCount);

	const Vector2& shooterPosition = GetOwner()->GetTransform().Position;
	for (int armIndex = 0; armIndex < armCount; ++armIndex)
	{
		float radians = (baseDegrees + angleStep * armIndex) * DX_PI_F / 180.0f;
		Vector2 direction = { std::cos(radians), std::sin(radians) };

		_bulletManager->FireBullet(shooterPosition, direction * _gunData.bulletSpeed,
			_gunData, _bulletColor, Team::Enemy);
	}
}

bool EnemyAttack::CanShoot() const
{
	return _provider->GetPlayer() != nullptr && _provider->GetActive();
}
Vector2 EnemyAttack::GetMuzzlePosition(const Vector2& direction) const
{
	const Vector2& center = GetOwner()->GetTransform().Position;
	if (_shape == nullptr) return center;

	//円なら半径、四角形なら向いている方向で先に当たる辺までの距離
	float offset = _shape->GetRadius();
	Vector2 half = _shape->GetHalfSize();
	if (half.x > 0.0f || half.y > 0.0f)
	{
		float toX = (direction.x != 0.0f) ? half.x / std::abs(direction.x) : FLT_MAX;
		float toY = (direction.y != 0.0f) ? half.y / std::abs(direction.y) : FLT_MAX;
		offset = (std::min)(toX, toY);
	}
	return center + direction * offset;
}

Vector2 EnemyAttack::DirectionFromAngle(float degrees)const
{
	const float radians = degrees * DX_PI_F / 180.0f;

	return { -std::cos(radians), -std::sin(radians) };
}
Vector2 EnemyAttack::GetNearerHorizontalAngle()
{
    float positionX = GetOwner()->GetTransform().Position.x;
	float width = static_cast<float>(_gameWindow->GetBounds().width);

	float distanceToLeft  = positionX;
	float distanceToRight = width - positionX;

	//遠い側の壁に向かって、斜め下に撃つ
	return (distanceToLeft <= distanceToRight)
		? Vector2{ 200.0f, 260.0f }  //右下寄り
	: Vector2{ 280.0f, 340.0f }; //左下寄り
}