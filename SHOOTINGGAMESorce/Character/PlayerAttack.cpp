#include <DxLib.h>
#include <algorithm>

#include "PlayerAttack.h"
#include "BulletManager.h"
#include "GameObject.h"

void PlayerAttack::Initialize(ServiceLocator& locator)
{
	_bulletManager = locator.bulletManager;
}
void PlayerAttack::InitializeShotSetting(int type)
{
    if (const ShotSetting* setting = FindShotSetting(type))
        _currentShotData = *setting;
}

void PlayerAttack::LongShot()
{
    //上方向固定なのでdirectionは固定
	auto direction = Vector2(0, -1);
	_bulletManager->FireBullet(GetOwner()->GetTransform().Position, direction * _currentShotData.bulletSpeed,
        _currentShotData,_bulletColor, Team::Player);
}
void PlayerAttack::SpreadShot()
{

    Vector2 firePosition = GetOwner()->GetTransform().Position;

    for (int bulletNumber = 0; bulletNumber < _currentShotData.bulletCount; ++bulletNumber)
    {
        Vector2 bulletDirection = GetRandomDirection();

        _bulletManager->FireBullet(firePosition, bulletDirection *_currentShotData.bulletSpeed,
            _currentShotData, _bulletColor,Team::Player);
    }
}
Vector2 PlayerAttack::GetRandomDirection()const
{
    //ばらつきを持たせてランダム性を持たせる
    const float randomDegrees = _minDegrees + GetRand(static_cast<int>((_maxDegrees - _minDegrees) * 100.0f)) / 100.0f;
    const float randomAngle = randomDegrees * DX_PI_F / 180.0f;

    return { -std::cos(randomAngle), -std::sin(randomAngle) };
}