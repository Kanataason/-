#include "PlayerMovement.h"
#include "PlayerStatus.h"

#include "GameObject.h"

void PlayerMovement::Initialize(ServiceLocator& locator)
{
    _sizeStatus = GetOwner()->GetComponent<CircleShape>();
    _gameWindow = &locator.gameWindow->GetBounds();
}

void PlayerMovement::MovementRestrictions()
{
    //左右の端にいったら進行方向とは反対の端にワープする
    Vector2 position = GetOwner()->GetTransform().Position;
    position = _gameWindow->Wrap(position);

    //Y軸はワープはせずに画面外に出ないように補正する
    position.y = _gameWindow->ClampY(position.y, 0.0f, _gameWindow->height, _sizeStatus->GetRadius());

    GetOwner()->GetTransform().Position = position;
}

void PlayerMovement::Move(Vector2 direction, float speed)
{
    //動きはシンプルによけやすく
    float length = direction.Length();

    //0除算を防ぐ
    if (length <= 0.0f)
        return;

    Vector2 velocity = direction.Normalized(length) * speed;

    //後ろ方向の動きは遅くする
    if (direction.y > 0.0f)
        velocity.y *=_damping;

    GetOwner()->GetTransform().Position += velocity;
}