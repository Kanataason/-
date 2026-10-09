#include <DirectXMath.h>


#include "BaseCamera.h"
#include "GameObject.h"
using namespace DirectX;


DirectX::XMMATRIX Camera::GetProjectionMatrix() const
{
    return DirectX::XMMatrixPerspectiveFovLH(
        fov, //垂直方向の視野角
        aspect, //画面のアスペクト比
        nearZ, //ニアクリップ面
        farZ //ファークリップ面
    );
}
DirectX::XMMATRIX Camera::GetViewMatrix() const
{
    //カメラを所有しているGameObjectのTransformを取得
    auto& transform = GetOwner()->GetTransform();

    //カメラが現在向いている方向を取得
    auto forward = transform.GetForward();

    //カメラのワールド座標を取得
    XMVECTOR eye = XMLoadFloat3(&transform.Position);

    //カメラの位置 + 前方向から注視点を計算
    XMVECTOR target =
        XMVectorAdd(
            eye,
            XMLoadFloat3(&forward)
        );

    //左手座標系のビュー行列を作成
    //カメラの位置
    //カメラが見る位置
    //カメラの上方向
    return DirectX::XMMatrixLookAtLH(
        eye,
        target,
        DirectX::XMVectorSet(
            0.0f, 1.0f, 0.0f, 0.0f
        )
    );
}

