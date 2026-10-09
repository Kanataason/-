#pragma once
#include <vector>
#include <chrono>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include "VectorUtility.h"

#include "Component.h"
#include "Animation.h"
#include "Joint.h"
//RagdollBoneがunique_ptrで持つので、完全な型が必要(破棄のときに中身が要る)
#include "RigidBody.h"
#include "CapsuleCollider.h"
#include "Event.h"

class PhysicsSystem;
class Skeleton;
class GameObject;

struct WorldPose
{
    Vector3 position{};
    Quaternion rotation{};
};
//共通処理
namespace RagdollData
{
    constexpr float referenceHeight = 3.088f;//基準のサイズ

    inline constexpr static const char* boneNames[] =
    {
        "mixamorig:Hips",

    "mixamorig:Spine",
    "mixamorig:Spine1",
    "mixamorig:Spine2",

    "mixamorig:Head",

    "mixamorig:LeftArm",
    "mixamorig:LeftForeArm",

    "mixamorig:RightArm",
    "mixamorig:RightForeArm",

    "mixamorig:LeftUpLeg",
    "mixamorig:LeftLeg",
    "mixamorig:LeftFoot",

    "mixamorig:RightUpLeg",
    "mixamorig:RightLeg",
    "mixamorig:RightFoot",

    //手(前腕の先)
    "mixamorig:LeftHand",
    "mixamorig:RightHand"
    };
    inline constexpr static int parentIndices[] =
    {
       -1, //0 Hips

     0, //1 Spine      → Hips
     1, //2 Spine1     → Spine
     2, //3 Spine2     → Spine1

     3, //4 Head       → Spine2

     3, //5 LeftArm    → Spine2
     5, //6 LeftForeArm → LeftArm

     3, //7 RightArm   → Spine2
     7, //8 RightForeArm → RightArm

     0, //9 LeftUpLeg  → Hips
     9, //10 LeftLeg    → LeftUpLeg
    10, //11 LeftFoot   → LeftLeg

     0, //12 RightUpLeg → Hips
    12, //13 RightLeg   → RightUpLeg
    13, //14 RightFoot  → RightLeg

     6, //15 LeftHand   ← LeftForeArm
     8 //16 RightHand  ← RightForeArm
    };
    inline std::unordered_map<std::string, float> boneRadius =
    {
        { "mixamorig:Hips",        0.15f },
        { "mixamorig:Spine",       0.15f },
        { "mixamorig:Spine1",      0.16f },
        { "mixamorig:Spine2",      0.17f },
        { "mixamorig:Head",        0.10f },

        { "mixamorig:LeftFoot",  0.07f },
        { "mixamorig:RightFoot", 0.07f },

        { "mixamorig:LeftHand",  0.04f },
        { "mixamorig:RightHand", 0.04f },

        { "mixamorig:LeftArm",     0.06f },
        { "mixamorig:LeftForeArm", 0.05f },
        { "mixamorig:RightArm",    0.06f },
        { "mixamorig:RightForeArm",0.05f },

        { "mixamorig:LeftUpLeg",   0.11f },
        { "mixamorig:LeftLeg",     0.06f },
        { "mixamorig:RightUpLeg",  0.11f },
        { "mixamorig:RightLeg",    0.06f },
    };
    //各ボーンの角度制限単位は度  設定時にラジアンに変換される
    inline std::unordered_map<std::string, BallJointLimit> ballLimits =
    {
{ "mixamorig:Hips",    { -45.0f, 45.0f, 25.0f } }, //-20/20 → -45/45
{ "mixamorig:Spine",   { -35.0f, 35.0f, 25.0f } }, //-15/15 → -35/35
{ "mixamorig:Spine1",  { -35.0f, 35.0f, 25.0f } }, //-15/15 → -35/35
{ "mixamorig:Spine2",  { -35.0f, 35.0f, 25.0f } }, //-15/15 → -35/35

//首
{ "mixamorig:Head",    { -60.0f, 60.0f, 60.0f } },

//肩
{ "mixamorig:LeftArm",  { -30.0f, 30.0f, 100.0f } }, //40 → 100
{ "mixamorig:RightArm", { -30.0f, 30.0f, 100.0f } }, //40 → 100

//股関節
{ "mixamorig:LeftUpLeg",  { -30.0f, 30.0f, 50.0f } },
{ "mixamorig:RightUpLeg", { -30.0f, 30.0f, 50.0f } },

//足首
{ "mixamorig:LeftFoot",  { -10.0f, 10.0f, 60.0f } },
{ "mixamorig:RightFoot", { -10.0f, 10.0f, 60.0f } },

//手首
{ "mixamorig:LeftHand",  { -15.0f, 15.0f, 70.0f } },
{ "mixamorig:RightHand", { -15.0f, 15.0f, 70.0f } },
    };

    //左stifness 右damping
    inline std::unordered_map<std::string, std::pair<float, float>> stiffnessTable =
    {
 { "mixamorig:Spine",  { 0.2f,  1.5f } },
    { "mixamorig:Spine1", { 0.2f, 2.0f } },
    { "mixamorig:Spine2", { 0.2f, 2.0f } },
    { "mixamorig:Head",   { 0.3f,  1.5f } },

    { "mixamorig:LeftArm",     { 0.0f, 1.2f } },
    { "mixamorig:RightArm",    { 0.0f, 1.2f } },
    { "mixamorig:LeftUpLeg",   { 0.0f, 2.0f } },
    { "mixamorig:RightUpLeg",  { 0.0f, 2.0f } },

    { "mixamorig:LeftFoot",  { 0.0f, 0.3f } },
    { "mixamorig:RightFoot", { 0.0f, 0.3f } },
    { "mixamorig:LeftHand",  { 0.0f, 0.3f } },
    { "mixamorig:RightHand", { 0.0f, 0.3f } },

    { "mixamorig:LeftLeg",     { 0.0f, 1.0f } },
    { "mixamorig:RightLeg",    { 0.0f, 1.0f } },
    { "mixamorig:LeftForeArm",  { 0.0f, 0.6f } },
    { "mixamorig:RightForeArm", { 0.0f, 0.6f } },
    };

    //第一引数が最低角度、２が最大角度、３が回転軸
    inline std::unordered_map<std::string, HingeLimit> hingeLimits =
    {
        //膝
 { "mixamorig:LeftLeg",
    { 0.0f, 140.0f, Vector3(-1,0,0) } },

{ "mixamorig:RightLeg",
    { 0.0f, 140.0f, Vector3(-1,0,0) } },

    //肘
{ "mixamorig:LeftForeArm",
{ 0.0f, 145.0f, Vector3(0,0,1) } },

{ "mixamorig:RightForeArm",
    { 0.0f, 145.0f, Vector3(0,0,-1) } },
    };

    inline std::unordered_map<std::string, float> boneMass =
    {
        { "mixamorig:Hips",        5.0f },
        { "mixamorig:Spine",       3.0f },
        { "mixamorig:Spine1",      3.0f },
        { "mixamorig:Spine2",      2.5f },
        { "mixamorig:Head",        1.0f },

        { "mixamorig:LeftArm",     1.0f },
        { "mixamorig:LeftForeArm", 0.7f },
        { "mixamorig:RightArm",    1.0f },
        { "mixamorig:RightForeArm",0.7f },

        { "mixamorig:LeftUpLeg",   2.0f },
        { "mixamorig:LeftLeg",     1.2f },
        { "mixamorig:LeftFoot",    0.5f },
        { "mixamorig:RightUpLeg",  2.0f },
        { "mixamorig:RightLeg",    1.2f },
        { "mixamorig:RightFoot",   0.5f },
        { "mixamorig:LeftHand",    0.4f },
        { "mixamorig:RightHand",   0.4f },
    };

    //末端ボーンの長さ(m)。ここにないボーンは RagdollTuning::leafHeight を使う
    inline std::unordered_map<std::string, float> leafHeights =
    {
        { "mixamorig:LeftHand",  0.12f },
        { "mixamorig:RightHand", 0.12f },
    };

}

struct RagdollBone
{
    std::string name;

    int nodeIndex =-1;
    int parentIndex = -1;

    bool hasReliableComOffset = false;

    Vector3 colliderLocalOffset{}; //RigidBody基準のローカル位置オフセット
    Quaternion colliderLocalRotOffset{}; //RigidBody基準のローカル回転オフセット
    Quaternion initialRelativeRotation{}; //初期の回転位置

    //所有者はRagdoll(このボーンを持つRagdollコンポーネント)だけ
    std::unique_ptr<RigidBody> rigidBody;
    std::unique_ptr<CapsuleCollider> collider;

    std::unique_ptr<Joint> joint;
    };

class Ragdoll : public Component
{
public:
    //発射して、静止したときに1回だけ呼ばれる
    Event<> OnSettled;

    //発射したあと、体のどこかが、空中から地面に着いた瞬間(跳ねて着地するたびに出る)
    Event<> OnGroundHit;

    Ragdoll() = default;
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton) { _skeleton = skeleton; }
    void SetParent(GameObject& owner) { _owner = &owner; }

    void Initialize(ServiceLocator& loacator)override;

    void FixedUpdate(float deltaTime);

    //飛んでいる最中に、どれかのボーンが地面に着いたら、飛行中を終える
    void Update(float deltaTime) override;

    //あたりはじめだけ処理する
    void OnCollisionEnter(Collider* other, const CollisionResult& result) override;

    //ラグドールの切り替え関数
    bool IsActive() { return _enabled; }
    void Enable() 
    {
        _enabled = true;

        for (auto& bone : _bones)
        {
        bone.rigidBody->SetIsGravity(_enabled);
        bone.rigidBody->SetSleeping(false);
        }
    };
    void Disable() 
    {
        _enabled = false;

        for (auto& bone : _bones)
        {
            bone.rigidBody->SetIsGravity(_enabled);
            bone.rigidBody->SetSleeping(true);
        }
    };


    void InitializePhysicsTransform();

    //---------------ラグドールの開始・終了と、吊り下げ(体全体を浮かせて移動させる)-------------
    //重力で地面に倒れる。ボーンを今のアニメーションのポーズに合わせて、ラグドールを有効にする。
    void StartFall();

    //吊る骨(RagdollTuning::kHangingBoneName)を Kinematic にして、体を吊り下げる。
    //ほかの骨は重力でぶら下がる。倒れている最中でも、アニメーション中でも呼べる
    //(倒れている最中なら、その姿勢のまま持ち上げる)
    void StartHanging();

    //吊り下げをやめて、また重力で落とす(ラグドールは有効のまま)
    void ReleaseHanging();

    //吊る骨を入力で動かす。ほかの骨は、ジョイントに引っ張られて、ついてくる。
    //delta:今フレームの移動量, deltaTime:今フレームの時間(秒)
    void MoveHangingBone(const Vector3& delta, float deltaTime);

    //吊り下げ中、毎フレーム入力のあとに呼ぶ。飛ばされた勢いを吊る骨に反映して、子を引っ張る速度を物理に伝える
    void UpdateHanging(float deltaTime);

    //吊り下げているか
    bool IsHangingStarted() const { return _hangingBoneIndex >= 0; }

    //ラグドールを終えて、アニメーション再生(通常)に戻す。
    //ラグドール中は Transform を動かしていないので、Hips が動いた分だけ Transform を動かして、
    //アニメーションの体を、ラグドールの体があった位置に移す。
    //includeHeight:true = 高さも移す(吊り下げて浮いていたとき)、false = 水平だけ移す(地面に倒れていたとき)
    void StopRagdoll(bool includeHeight);

  
    //計算結果をスケルトンに反映する
    void UpdateBonesFromPhysics();

    //大砲でragDoll全体に初速を入れて飛ばす関数
    //向きは正規化済み、
    void Launch(const Vector3& direction,float speed);

    //発射されて、まだ着地していないか
    bool IsFlying() const { return _isFlying; }

    //発射してから、全ボーンが静止する(スリープする)まで待っている間はtrue
    bool IsWaitingSettle() const { return _isWaitingSettle; }

    //体の中心(腰)の世界座標。ラグドール中は物理のボーンの位置、そうでなければPlayerのTransformの位置
    //(ラグドール中はTransformを動かさないので、カメラの追従などは、これを見る)
    Vector3 GetRootPosition() const;

    //CameraFollowでカメラの引き具合を決めるためにrootの速さをとる
    Vector3 GetRootVelocity() const;

    //体のY軸大きさ
    float GetBodyScale()const { return _bodyScale; }
    float GetHipHeight()const { return _hipHeight; }
    void SetColliderScale(float scale) { _colliderScale = scale; }

    //地面でのバウンドを強くする(よく跳ねるキャラクター用)。発射のたびに、全ボーンへ設定される
    //restitution: 強く跳ねるときの反発係数(1より大きいと、元の速さより強く跳ねる) / count: 強く跳ねられる回数
    void SetBounce(float restitution, int count) { _bounceRestitution = restitution; _bounceCount = count; }

    //コライダーに対応するボーンの名前。このラグドールのコライダーでなければ、空文字
    std::string GetBoneName(const Collider* collider) const;

private:
    void InitializeBoneLength();
    void SetLockZ(bool lock);

    //飛行中の切り替え。飛んでいる間は重力トルクを止め、着地したら戻す
    void SetFlying(bool flying);

    //全ボーンがスリープしているか
    bool AllBonesSleeping() const;

    //---- 生成・開始姿勢
    void AddRagDoll();

    //物体を押された方向に動かす
    void ApplyPushImpulse(const Vector3& pushDir, float speed, RagdollBone& hitBone, const Vector3& contactPoint);

    //吹き飛ばす(全ボーンをまとめて飛ばす)
    void ApplyLaunchImpulse(const Vector3& pushDir, RagdollBone& hitBone, const Vector3& contactPoint);

    //押せる物体かの確認関数
    bool IsPushCollider(const Collider* other) const;

    //------------------生成-------------------

    bool IsLegBone(const std::string& boneName)
    {
        return boneName.find("Leg") != std::string::npos ||
            boneName.find("Foot") != std::string::npos;
    }
    bool IsLeftBone(const std::string& boneName)
    {
        return boneName.find("Left") != std::string::npos;
    }
    //左脚と右脚の組み合わせか
    bool AreLegsOfDifferentSides(const std::string& boneNameA, const std::string& boneNameB)
    {
        return IsLegBone(boneNameA) && IsLegBone(boneNameB) &&
            IsLeftBone(boneNameA) != IsLeftBone(boneNameB);
    }
    //ボーンを1本作って、PhysicsSystem に登録する
    void CreateBone(int boneNameIndex);




    //親から自分への向きと長さを求める。距離がほぼ 0 なら false
    bool CalculateBoneAxis(const Vector3& nodePosition, const Vector3& parentPosition,
        float& outLength, Vector3& outAxis);

    //このボーンが、同じ親を持つボーンのうち最初のものか
    bool IsPrimaryChild(size_t boneIndex) const;


    //ボーンのワールド空間での位置と回転を返す
    WorldPose GetWorldPose(const BoneNode& node, const XMMATRIX& worldMatrix);

    //自分の capsule(関節から先へ伸びる短いもの)を設定する
    Quaternion ComputeCapsuleRotation(const Vector3& axisFromParent);

    //ジョイントの基準(静止)回転を、Bind姿勢から求める
    Quaternion ComputeBindRelativeRotation(const BoneNode& parentNode, const BoneNode& node);

    //開始時のランダムな押し(倒れる向きを毎回変える)
    void ApplyRandomStartPush();

    //全ボーンの位置・回転・capsule を、現在のアニメーション姿勢に合わせる
    void InitializeBonePoses();



    //---------------------Joint-----------------------
    
    //テーブルにあれば、ジョイントの硬さ(stiffness)を設定する。
    void ApplyJointStiffness(RagdollBone& child);

    //ヒンジ,1軸の回転だけを許すジョイントを生成する
    void CreateHingeJoint(RagdollBone& parentBone, RagdollBone& child, const auto& hingeLimit);

    //ボール,ひねり(twist)と振り(swing)の範囲を制限するジョイントを制限する
    void CreateBallJoint(RagdollBone& parentBone, RagdollBone& child);

    //親を持つ全ボーンにジョイントを作り、PhysicsSystem に登録する。
    void CreateJoints();
    //ひねりの軸を、「親→自分」ではなく「自分のボーンの向き(自分の最初の子に向かう向き)」にする。
    void AlignTwistWithOwnBone(size_t boneIndex);



    //-----------------------生成-----------------------

    //衝突を無視するボーンの組み合わせ(親子・左右の脚)を登録する
    void RegisterIgnoredCollisionPairs();
    
    //ルート(親のいないボーン)を、ノードの位置・回転にそのまま置く
    void InitializeRootBone(RagdollBone& bone, const WorldPose& nodePose);

    //親のcapsuleの情報を設定する
    void SetupParentCapsule(RagdollBone& parent, WorldPose& parentPose,
        const Vector3& nodePosition, float lengthFromParent, const Quaternion& capsuleRotation);

    //自分のcapsuleを設定する
    void SetupLeafCapsule(RagdollBone& bone, WorldPose& nodePose, const Vector3& axisFromParent);

    //----------物理からスケルトンに反映-------------------


    //ワールド空間の位置・回転を、オーナー(GameObject)基準のローカル行列に変換する。

    XMMATRIX WorldPoseToOwnerLocalMatrix(const Vector3& worldPosition, const Quaternion& worldRotation,
        const XMMATRIX& ownerInverseWorld, const XMVECTOR& ownerWorldRotation);
    //ragdollのボーンの、物理のワールド位置・回転を、スケルトンのノード(GlobalTransform)へ書き込む。
    void WritePhysicsPoseToNodes();

    //ragdoll以外のノードを、親のグローバル行列 * 自分のローカル行列で更新する。
    void UpdateNonRagdollNodes();

    //計算結果をノードのGlobalにコピーする
    void CopyNodeTransformsToSkeletonBones();
private:

    PhysicsSystem* _physicsSystem = nullptr;
    std::shared_ptr<Skeleton> _skeleton = nullptr;

    GameObject* _owner = nullptr;
    RigidBody* _hipsRigidBody = nullptr;

    float _bodyScale = 1.0f;
    float _colliderScale = 1.0f;
    float _bounceRestitution = 1.0f;
    int _bounceCount = 0; //0 = 強く跳ねない(ふつうのキャラクター)
    float _hipHeight = 0.0f;

    bool _enabled = false;
    bool _isFlying = false;
    bool _isWaitingSettle = false; //発射して、静止するのを待っているか
    float _settleElapsed = 0.0f; //発射してからの経過時間(止まらない場合の保険に使う)
    bool _wasGrounded = false; //前のフレームで、どれかのボーンが地面に着いていたか(着地の瞬間を見つける用)
    float _groundHitCooldown = 0.0f; //着地の通知を、連続で出さないための待ち時間

    //---- 吊り下げモード ----
    int _hangingBoneIndex = -1; //吊る骨(_bones の番号) -1 = まだ始めていない
    Vector3 _hangingInputVelocity{}; //入力で動かした速度 ジョイントが子を引っ張るために使う
    Vector3 _hangingDriftVelocity{}; //飛ばされた勢い 時間とともに弱まる
    Vector3 _ragdollStartRootPosition{}; //ラグドールを始めたときの、Hips(根っこ)の位置 通常に戻るとき、動いた距離を求めるために使う

    std::chrono::steady_clock::time_point _lastLaunchTime{};
    bool _hasLaunched = false;

    std::vector<RagdollBone> _bones;
    std::unordered_set<int> ragdollNodeIndices;
};
