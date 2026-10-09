#pragma once
#include <unordered_map>
#include <string>

#include "VectorUtility.h"

struct BallJointLimit
{
    float minTwist; 
    float maxTwist;
    float maxSwing;
};

struct HingeLimit
{
    float min;
    float max;
    Vector3 axis;
};

namespace RagdollTuning
{
   //ジョイントで使用

    //---- 拘束の強さ ----

    //拘束で消す速度の割合(1 = 全部消す)
    constexpr float velocityRemovalRatio = 1.0f;

    //この角度(rad)より小さい制限違反は、補正しない
    constexpr float rejectedAngleDeadband = 0.01f;

    //速度拘束の強さ
    constexpr float velocityConstraintStrength = 0.5f;

    //角度制限の補正の強さ(beta) 制限の内側・外側・その他で使い分ける
    constexpr float angularBetaInLimit = 0.1f; //制限の範囲内
    constexpr float angularBetaOutOfLimit = 0.6f; //制限の範囲外(強く戻す)
    constexpr float angularBeta = 0.4f; //上記以外の標準値

    //可動範囲を、この角度(rad。約20度)より大きく超えたときは、少しずつではなく、一気に範囲の中へ戻す
    //(速く飛んだときに、折れ曲がったまま固まるのを防ぐ)
    constexpr float angularSnapAngle = 0.35f;


    //ジョイントのアンカーのずれを利用して親ボディを回転させる。
    //これにより、脚や胴体にかかる負荷によって姿勢を傾けられる。
    constexpr bool kAnchorRotation = false;

    //1回の補正のうち、実際に適用する割合。
    //1に近いほど補正が強くなるが、震えやすくなる
    constexpr float kAnchorRotationWeight = 0.1f;

    //1回の呼び出しで回す角度の上限(ラジアン)
    constexpr float kAnchorRotationMaxAngle = 0.05f;

    //このずれより小さければ補正しない
    //極小のずれに反応して、ボディが細かく震え続けるのを防ぐ
    constexpr float kAnchorRotationDeadband = 0.006f;

    //これより短い腕(重心からアンカーまで)は、回転で動かせる量がほぼ0で、
    //割り算も不安定になるため補正しない(二乗の長さ)
    constexpr float kMinAnchorArmSqLength = 1e-6f;

    //回転軸がほぼ0(腕とずれが平行)なら、
    //回す方向が決まらないため補正しない
    constexpr float kMinRotationAxisLength = 1e-6f;

    //---- BallJoint のひねり軸 ----
    //BallJointのツイスト軸
    //親から子への方向ではなく、ジョイント自身のボーン(自身の子)に沿った軸を使用する
    constexpr bool kTwistAxisAlongOwnBone = true;


    //RigidBody 側で使う

    //角速度の減衰
    //値が大きいほど回転が早く収まる
    constexpr float kAngularDamping = 2.5f;

    //線形速度の減衰
    constexpr float linearDamping = 0.4f;

    //角速度の上限
    constexpr float maxAngularSpeed = 15.0f;

    //線形速度の上限
    constexpr float maxVelocitySpeed = 20.0f;

    //大砲で発射して飛んでいる間だけ使う、線形速度の上限。
    //通常の上限(maxVelocitySpeed)のままだと、発射の初速がそこで頭打ちになる
    constexpr float kLaunchMaxVelocitySpeed = 80.0f;


    //重力トルク

    //地面に接触しているボーンにも、重力トルクを適用する
    //(地面から回転方向の反作用を受けないため、回転や振動が発生する場合がある)
    constexpr bool kGravityTorqueWhenGrounded = true;

    //足と手は、地面に接触しているあいだ、重力トルクを使わない。
    //慣性が小さいので、重力で下へ回す力と、地面の押し戻しが毎ステップぶつかって、震えるため。
    //true にすると、足と手も、接地中に重力トルクを使う(kGravityTorqueWhenGrounded が true のとき)
    constexpr bool kExtremityGravityTorqueWhenGrounded = false;

    //ルートに対する重力トルク。体全体の重心位置によって発生する
    constexpr bool kRootGravityTorque = false;

    //末端ボーン(頭、前腕、足)にも重力トルクを適用する
    //無効にすると、頭が地面に落ちにくくなる
    constexpr bool kLeafBoneGravityTorque = false;


    //地面との接触(PhysicsSystem)
    
    //1回のサブステップで補正するめり込みの割合。
    constexpr float penetrationCorrectionRate = 0.3f;

    //これ未満のめり込みは無視する。接触時の細かいがたつきを防止する。
    constexpr float penetrationSlop = 0.005f;

    //地面に接触している間の、フレームごとの角速度減衰係数。
    //値が小さいほど強く減衰する(1フレーム分の値。サブステップ分には時間の比でべき乗して直す)。
    constexpr float kGroundAngularDampPerFrame = 0.6f;

    //地面の摩擦係数(接触1回あたり、サブステップ単位)。1 = 即座に滑りを止める。
    constexpr float kGroundFriction = 0.7f;

    //地面でのバウンド
    //反発係数。0 = 跳ねない、1 = ぶつかったときと同じ速さで跳ね返る。
    constexpr float kGroundRestitution = 1.0f;
    //これより遅い接触(m/s)では跳ねない。止まりかけの小さな震えを防ぐ。
    //超えた分だけ跳ねるので、遅くなるほど自然に跳ねなくなる。
    constexpr float kGroundBounceMinSpeed = 1.5f;
    //体ぜんたいで「ぴょん」と跳ねるキャラクター用(PhysicsSystemの、RagdollHop)
    //跳ね上がる速さの上限(m/s)。上限がないと、跳ねるたびに、速さが増えて、手に負えなくなる
    constexpr float hopMaxSpeed = 12.0f;
    //これより遅い着地(m/s)では、跳ねない(止まりかけの、小さな接触で跳ねないように)
    constexpr float hopMinImpactSpeed = 2.0f;
    //1回跳ねたら、この秒数のあいだは、次の跳ねを起こさない(胴体のあちこちが、続けて地面に当たっても、1回だけ跳ねる)
    //この間は、地面の摩擦も、かけない(摩擦で横の速さが消えると、斜めに跳ね返らない)
    constexpr float hopCooldownSeconds = 0.25f;
    //跳ねるときに、地面に沿った速さを、減らす割合(0 = そのまま、1 = 全部消す)
    constexpr float hopTangentLoss = 0.1f;


    //ボーン同士の接触(PhysicsSystem)


    //ボーン同士の押し出し処理(腕が胴体内部に入り込むなど)。

    //サブステップごとの補正割合
    constexpr float kBoneContactBeta = 0.8f;

    //無視するめり込み量(m)
    constexpr float kBoneContactSlop = 0.0025f;

    //1サブステップで補正する最大めり込み量
    constexpr float kBoneContactMaxPush = 0.02f;

    //ボーン同士の摩擦係数(接触1回あたり、サブステップ単位)
    constexpr float kBoneFriction = 0.2f;

    //左右の脚ボーン同士の衝突を無視する
    //脚同士が交差した際に押し合って、腰が持ち上がるのを防ぐ
    constexpr bool kIgnoreLegLegCollision = true;


    //ragdollの生成(Ragdoll)

    //慣性モーメント = 質量 * この値。
    //衝突時のインパルスによってボーンが激しく回転していた。
    //カプセルを端を中心に回転させる場合は、おおよそ m*L*L/3。
    constexpr float kInertiaPerMass = 0.03f;

    //足先や手先の固定の長さ
    constexpr float leafHeight = 0.4f;

    //ラグドール開始時に胸・頭へ加えるランダムな水平方向の押し出し速度(m/s)。
    //0 = 無効。開始するたびに倒れる方向が変わる。
    constexpr float kStartPushSpeed = 0.4f;

    constexpr float kRootGravityTorqueScale = 0.1f;

    //衝撃を受けたときの反応(Ragdoll)

    //衝撃を受けたときの反応。
    //true = ラグドール全体を吹き飛ばす。
    //false = 衝撃を受けたボーンだけを軽く押す。
    constexpr bool kLaunchOnHit = true;

    constexpr float kLaunchCooldown = 0.7f; //seconds during which further hits are ignored after a launch
    constexpr float kLaunchSpeed = 8.0f; //水平方向の速度(m/s)
    constexpr float kLaunchUpSpeed = 3.0f; //上方向の速度(m/s)
    constexpr float kLaunchSpinScale = 0.3f; //衝撃を受けたボーンの回転量
    constexpr float kLaunchTumbleSpeed = 6.0f; //吹き飛んでいる間の全身の回転速度(rad/s)

    //ボーンごとに追加するランダムな回転速度(rad/s)。手足をばたつかせるために使用する。
    constexpr float kLaunchFlailSpeed = 6.0f;

    //---- 大砲で発射するときの、ランダム性(ぐちゃぐちゃ度) ----
    //値を大きくするほど、毎回の飛び方がばらつく。0にすると、そのランダム性は無くなる
    constexpr float kLaunchSpeedJitter = 0.05f; //初速の±割合(0.05 = ±5%)
    constexpr float kLaunchAngleJitterDeg = 3.0f; //発射の向きの±度
    constexpr float kLaunchTumbleJitter = 0.5f; //全身の回転速度の±割合(0.5 = 0.5倍～1.5倍)
    constexpr float kLaunchSpinReverseChance = 0.25f; //回転が逆向きになる確率(0～1)
    constexpr float kLaunchScatterSpeed = 2.5f; //ボーンごとに足す、ランダムな速度(m/s)
    constexpr float kLaunchExtremityScatterScale = 2.0f; //頭・手・足を、何倍に散らすか


    //休止(ragdollが倒れて静止したら止める)(RagdollSystem)

    //---- 全体のスイッチ ----

    //地面上で静止したラグドールを停止させる機能のマスタースイッチ
    //false = 停止せずシミュレーションを続ける
    constexpr bool kEnableRest = true;

    //最終的にすべてのボーンを停止(スリープ)させる
    //false = 滑らかな減衰のみ行い、突然停止させない
    constexpr bool kFreezeAtEnd = true;

    //---- 「静止」の基準 ----

    //ラグドールが地面上でこの速度以下の状態を kRestTime 秒間維持した場合、
    //すべてのボーンを停止させる。
    constexpr float kRestLinearSpeed = 0.05f; //線形速度(m/s)
    constexpr float kRestAngularSpeed = 0.15f; //角速度(rad/s)

    //完全静止がこの時間(秒)続いたら凍結する。
    constexpr float kRestTime = 1.0f;

    //---- 「倒れた」の判定 ----

    //ルートが一定量下がった場合も「倒れている」と判定する
    //false = 胸・頭などが地面に接触した場合のみ倒れていると判定する
    constexpr bool kRestUseRootDrop = false;

    //初回の地面接触時のルート位置から、この値(m)以上下がった場合、
    //ラグドールが「倒れている」と判定する
    constexpr float kRestRootDrop = 0.35f;

    //最も速く動いているボーンがこの速度(m/s)を超えた場合、
    //倒れている状態を解除する。例:外部から押された場合
    constexpr float kRestUnlatchSpeed = 3.0f;

    //---- 「ほぼ静止」と減衰 ----

    //「ほぼ静止」の基準 = 静止の基準(kRestLinearSpeed / kRestAngularSpeed)× この係数
    constexpr float kSettleSpeedFactor = 40.0f;

    //減衰の強さ
    constexpr float kSettleDamping = 0.8f;

    //滑らかに停止させるための減衰処理。
    //ほぼ静止している状態がこの時間(秒)続く間、
    //減衰を1から kSettleDamping まで徐々に強くする。
    constexpr float kSettleRampTime = 2.0f;

    //この時間(秒)ほぼ静止している状態が続いた場合、
    //小さな振動が残っていてもすべて停止させる。
    constexpr float kForceRestTime = 3.0f;



    //---- 吊り下げモード(体全体を浮かせて移動させる) ----

    //吊り下げ(体全体を浮かせて移動させる)の設定。
    //倒れているラグドールを、吊る骨(kHangingBoneName)で持ち上げて、ほかの全身を重力でぶら下げる。
    //吊り下げているあいだは、静止して停止する機能(RagdollSystem::UpdateRest)は使わない。

    //吊る骨。この骨だけを Kinematic にして、入力で動かす
    constexpr const char* kHangingBoneName = "mixamorig:Spine2";

    //飛ばされたときの勢いが、1秒あたりに弱まる強さ。
    //吊る骨は Kinematic で物理に動かせないので、飛ばされた勢いは Ragdoll 側で別に管理する
    constexpr float kHangingDriftDamping = 2.0f;

    //倒れている体を吊り下げる(または、吊り下げをやめて落とす)キー。
    //カメラ(W A S D R T)や、倒れる(P)・通常に戻る(Q)キーとは別のキーにしている
    constexpr int kHangingToggleKey = 'B';

    //デバッグの切り替え変数

    constexpr bool IsDebug = true;
}
