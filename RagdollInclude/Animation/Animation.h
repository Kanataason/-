#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

//スケルトンの階層構造を表す1ノード
struct BoneNode
{
    std::string Name;

    XMFLOAT4X4 BindGlobalTransform; //バインドポーズでのワールド(モデル)空間変換
    XMFLOAT4X4 BindTransform; //バインドポーズでのローカル変換
    XMFLOAT4X4 LocalTransform; //現在のローカル変換(親からの相対)
    XMFLOAT4X4 GlobalTransform; //現在のワールド(モデル)空間変換

    int ParentIndex = -1; //親ノードのインデックス(ルートは-1)

    BoneNode()
    {
        //各行列は単位行列で初期化
        XMStoreFloat4x4(
            &GlobalTransform,
            XMMatrixIdentity());

        XMStoreFloat4x4(
            &LocalTransform,
            XMMatrixIdentity());


        XMStoreFloat4x4(
            &BindTransform,
            XMMatrixIdentity());

        XMStoreFloat4x4(
            &BindGlobalTransform,
            XMMatrixIdentity());

    }
};

//スキニングに使う実際のボーン(BoneNodeのうち、メッシュに影響するものだけを持つ)
struct Bone
{
    std::string Name;

    int NodeIndex = -1; //対応するBoneNodeのインデックス
    int ParentIndex = -1; //親Boneのインデックス

    XMFLOAT4X4 OffsetMatrix; //バインドポーズのモデル空間->ボーンローカル空間への変換
    XMFLOAT4X4 LocalTransform;
    XMFLOAT4X4 GlobalTransform;
    XMFLOAT4X4 FinalTransform; //スキニングでシェーダーに渡す最終行列

    Bone()
    {
        //各行列は単位行列で初期化
        XMStoreFloat4x4(
            &OffsetMatrix,
            XMMatrixIdentity());

        XMStoreFloat4x4(
            &LocalTransform,
            XMMatrixIdentity());

        XMStoreFloat4x4(
            &GlobalTransform,
            XMMatrixIdentity());

        XMStoreFloat4x4(
            &FinalTransform,
            XMMatrixIdentity());
    }
};


//アニメーションの位置キーフレーム(時刻と値)
struct PositionKey
{
    double Time;
    XMFLOAT3 Value;
};

//アニメーションの回転キーフレーム(クォータニオン)
struct RotationKey
{
    double Time;
    XMFLOAT4 Value;
};

//アニメーションのスケールキーフレーム
struct ScaleKey
{
    double Time;
    XMFLOAT3 Value;
};

//1ボーン分のキーフレーム列
struct BoneAnimation
{
    std::string BoneName;

    std::vector<PositionKey> Positions;
    std::vector<RotationKey> Rotations;
    std::vector<ScaleKey> Scales;
};

//1つのアニメーションクリップ全体
struct Animation
{
    std::string sourceName; //FBX側の元の名前
    std::string animationName; //ゲーム内で参照する名前

    double Duration = 0.0;
    double TicksPerSecond = 0.0;

    std::vector<BoneAnimation> Bones;
};

//キーフレーム列から任意時刻の値を補間して求めるユーティリティ
class AnimationMath
{
public:
    //指定時刻の位置を、前後のキーフレーム間で線形補間して返す
    static XMFLOAT3 InterpolatePosition(const std::vector<PositionKey>& keys, double currentTime)
    {
        //最初のキーより前なら先頭の値をそのまま返す
        if (currentTime <= keys.front().Time)
        {
            return keys.front().Value;
        }

        //キーが1つしかなければ補間できないのでそのまま返す
        if (keys.size() == 1)
        {
            return keys[0].Value;
        }

        for (size_t i = 0; i < keys.size() - 1; i++)
        {
            const auto& key1 = keys[i];
            const auto& key2 = keys[i + 1];

            //currentTimeがkey1とkey2の間にある
            if (currentTime >= key1.Time &&
                currentTime <= key2.Time)
            {
                //区間内での正規化された時間(0～1)
                double time =
                    (currentTime - key1.Time) /
                    (key2.Time - key1.Time);

                float t = static_cast<float>(time);

                XMVECTOR pos1 =
                    XMLoadFloat3(&key1.Value);

                XMVECTOR pos2 =
                    XMLoadFloat3(&key2.Value);

                XMVECTOR result =
                    XMVectorLerp(pos1, pos2, t);

                XMFLOAT3 position;

                XMStoreFloat3(&position, result);

                return position;
            }
        }

        //最後のKeyを超えていた場合は末尾の値を返す
        return keys.back().Value;
    }

    //指定時刻の回転を、前後のキーフレーム間で球面線形補間(Slerp)して返す
    static DirectX::XMFLOAT4 InterpolateRotation(const std::vector<RotationKey>& keys, double currentTime)
    {
        //最初のキーより前なら先頭の値をそのまま返す
        if (currentTime <= keys.front().Time)
        {
            return keys.front().Value;
        }

        //キーが1つしかなければ補間できないのでそのまま返す
        if (keys.size() == 1)
        {
            return keys[0].Value;
        }

        for (size_t i = 0; i < keys.size() - 1; i++)
        {
            const auto& key1 = keys[i];
            const auto& key2 = keys[i + 1];

            //currentTimeがkey1とkey2の間にある
            if (currentTime >= key1.Time &&
                currentTime <= key2.Time)
            {
                //区間内での正規化された時間(0～1)
                double time =
                    (currentTime - key1.Time) /
                    (key2.Time - key1.Time);

                float t = static_cast<float>(time);

                XMVECTOR q1 = XMLoadFloat4(&key1.Value);
                XMVECTOR q2 = XMLoadFloat4(&key2.Value);

                //補間前に正規化しておくことで誤差の蓄積を防ぐ
                q1 = XMQuaternionNormalize(q1);
                q2 = XMQuaternionNormalize(q2);

                XMVECTOR result =
                    XMQuaternionSlerp(q1, q2, t);

                result = XMQuaternionNormalize(result);

                XMFLOAT4 rotation;
                XMStoreFloat4(&rotation, result);

                return rotation;
            }
        }

        //最後のKeyを超えていた場合は末尾の値を返す
        return keys.back().Value;
    }

    //指定時刻のスケールを、前後のキーフレーム間で線形補間して返す
    static DirectX::XMFLOAT3 InterpolateScale(const std::vector<ScaleKey>& keys, double currentTime)
    {
        //最初のキーより前なら先頭の値をそのまま返す
        if (currentTime <= keys.front().Time)
        {
            return keys.front().Value;
        }

        //キーが1つしかなければ補間できないのでそのまま返す
        if (keys.size() == 1)
        {
            return keys[0].Value;
        }

        for (size_t i = 0; i < keys.size() - 1; i++)
        {
            const auto& key1 = keys[i];
            const auto& key2 = keys[i + 1];

            //currentTimeがkey1とkey2の間にある
            if (currentTime >= key1.Time &&
                currentTime <= key2.Time)
            {
                //区間内での正規化された時間(0～1)
                double time =
                    (currentTime - key1.Time) /
                    (key2.Time - key1.Time);

                float t = static_cast<float>(time);

                XMVECTOR pos1 =
                    XMLoadFloat3(&key1.Value);

                XMVECTOR pos2 =
                    XMLoadFloat3(&key2.Value);

                XMVECTOR result =
                    XMVectorLerp(pos1, pos2, t);

                XMFLOAT3 position;

                XMStoreFloat3(&position, result);

                return position;
            }
        }

        //最後のKeyを超えていた場合は末尾の値を返す
        return keys.back().Value;
    }
};