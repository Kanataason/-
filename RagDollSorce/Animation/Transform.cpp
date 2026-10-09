
#include "Transform.h"
 
using namespace DirectX;

namespace
{
    //Rotationから回転行列を作成する共通処理
    XMMATRIX MakeRotationMatrix(const XMFLOAT3& rotation)
    {
        return XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(rotation.x),
            XMConvertToRadians(rotation.y),
            XMConvertToRadians(rotation.z));
    }
}

DirectX::XMMATRIX Transform::GetWorldMatrix() const
{
    XMMATRIX scale =
        XMMatrixScaling(
            Scale.x,
            Scale.y,
            Scale.z);

    //ラジアン変換
    XMMATRIX rotation = MakeRotationMatrix(Rotation);

    XMMATRIX translation =
        XMMatrixTranslation(
            Position.x,
            Position.y,
            Position.z);

    //見た目だけの回転は、拡大のあと・ゲーム上の回転の前に掛ける(モデルの中の向きを直してから、物体として回す)
    XMMATRIX modelRotation = MakeRotationMatrix(ModelRotation);

    return scale * modelRotation * rotation * translation;
}
DirectX::XMFLOAT3 Transform::GetForward() const
{

    XMMATRIX rot = MakeRotationMatrix(Rotation);

    XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rot);

    XMFLOAT3 result;
    XMStoreFloat3(&result, forward);
    return result;
}
DirectX::XMFLOAT3 Transform::GetRight() const
{

    XMMATRIX rot = MakeRotationMatrix(Rotation);
    //ローカルの+X軸を回転行列で変換
    XMVECTOR right =
        XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0),rot);

    XMFLOAT3 result;
    XMStoreFloat3(&result, right);
    return result;
}
DirectX::XMFLOAT3 Transform::GetUp()const
{
    XMMATRIX rot = MakeRotationMatrix(Rotation);

    XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0,1, 0, 0), rot);

    XMFLOAT3 result;
    XMStoreFloat3(&result, forward);
    return result;
}