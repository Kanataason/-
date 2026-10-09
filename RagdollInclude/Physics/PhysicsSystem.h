#pragma once
#include <vector>
#include <string>

#include "CollisionManager.h"
#include "RigidBody.h"
#include "Collider.h"
#include "PhysicsBody.h"
#include "RagdollSystem.h"

class DebugDraw;

constexpr int SubStepCount = 5;//サブステップの回数

class PhysicsSystem
{
public:
    PhysicsSystem(DebugDraw* debug) : _collisionManager(debug) {}

    void Initialize();

    //毎フレーム呼び出す物理更新のエントリーポイント
    void FixedUpdate(float deltaTime);

    //RigidBody / Collider / Jointの登録
    //RagdollBoneがunique_ptrを持つためPhysicsBodyはコピーできない。値で受け取り、内部でmoveする
    void AddPhysicsBody(PhysicsBody body);

    //RigidBodyのデストラクタから、自動的に呼ばれる(手動で呼ぶ必要はない)
    void RemoveRigidBody(RigidBody* rigidBody);

    //Colliderのデストラクタから、自動的に呼ばれる(手動で呼ぶ必要はない)
    //CollisionManager側の登録解除と、_rigidBodies内の参照の後始末を両方行う
    //(デストラクタの中から呼ばれるので、コライダーの形(GetMinなど)は、触らない)
    void RemoveCollider(Collider* collider);

    //生きているコライダーを外して、下の支えを失った、眠っている箱だけを起こす(的を飛ばすときなど)
    void RemoveColliderAndWakeUnsupported(Collider* collider);

    //登録時に、Joint側にも登録先を覚えさせる(Jointの所有者はRagdoll。ここは参照するだけ)
    void AddJoint(Joint* joint);

    //Jointのデストラクタから、自動的に呼ばれる(手動で呼ぶ必要はない)
    //消し忘れると、シーンを作り直したあとのSolveJointsで、破棄済みのJointを触って落ちる
    void RemoveJoint(Joint* joint);

    void AddCollider(Collider* collider) { collider->SetRegistry(this); _collisionManager.AddCollider(collider); }

    CollisionManager* GetCollisionManager() { return &_collisionManager; }

    bool RayCast(const XMFLOAT3& origin, const XMFLOAT3& direction, const float& distance, XMFLOAT3& hitPos);

    //ragdollのボーンごとの、colliderのずれ(ローカルオフセット・回転オフセット)と、
    //重心オフセットが信頼できるかどうかを更新する
    void UpdateRagdollBoneOffsets(RigidBody* rigidBody,
        const Vector3& localOffset, const Quaternion& localRotOffset, bool hasReliableComOffset);

    //体ぜんたいで「ぴょん」と跳ねる設定(キャラクター固有)。発射のたびに、設定し直す
    //restitution: 着地の速さの何倍で跳ね上がるか / count: 跳ねられる回数(0なら、ふつうの、ボーンごとのバウンド)
    void SetRagdollHop(float restitution, int count);

    void ResetRagdollTimer() { _ragdollElapsedTime = 0.0f; _ragdollSystem.ResetRest(); }
    //休止(倒れて静止したら止める処理)を使うかを切り替える。吊り下げ中は false にする
    void SetRagdollRestEnabled(bool enabled) { _ragdollSystem.SetRestEnabled(enabled); }
private:
    //---- FixedUpdate内部で使うステップ処理 ----

    //動かない物でも、GameObject側で動かされたときに追従できるようにしている
    void SyncStaticColliders();

    //GameObjectのTransformに合わせる
    void SyncKinematicBodies();

    //位置だけを書き出す
    void SyncGameObjectTransforms();

    //RigidBodyの位置・回転から、colliderの位置・回転を計算して設定する
    void SyncCollidersFromRigidBodies();

    //全ボーンの重力・速度による位置更新と、
    //ボーンごとの重力トルクを加える
    void IntegrateDynamics(float subDeltaTime);

    //すべてのジョイントの拘束を解く
    //片方だけに解いても影響が伝わらないのを防止のため、両方を解く
    void SolveJoints(float subDeltaTime);

    //接触判定
    //衝突ペアごとに全ボーンを走査するので、計算量は「ペア数 × ボーン数」
    void SolveCollisions(float deltaTime, float subDeltaTime);

    //同じサブステップで回転に反映する
    //キネマティックとスリープ中は対象外
    void IntegrateRotations(float subDeltaTime);

    //1つのRigidBodyの位置に、疑似速度 * 時間 を足す
    void ApplyPseudoVelocityToPosition(RigidBody& rigidBody, float subDeltaTime);

    //衝突で溜めた疑似速度(めり込みの押し戻し)を、全ボーンの位置に反映する
    void ApplyPseudoVelocityToPositions(float subDeltaTime);

    //このボーンが、今回の衝突ペアの当事者かを調べる
    bool TryGetContactInfo(const PhysicsBody& body, const CollisionResult& result,
        Vector3& contactNormal, bool& isGround) const;

    //ボーン同士の接触の処理
    void ResolveBoneContact(PhysicsBody& body, const CollisionResult& result,
        const Vector3& contactNormal, float subDeltaTime);

    //接触ペアの「相手側」のRigidBodyを探して返す
    RigidBody* FindOtherRigidBody(PhysicsBody& body, const CollisionResult& result);

    //地面との接触の処理
    void ResolveGroundContact(PhysicsBody& body, const CollisionResult& result,
        const Vector3& contactNormal, float deltaTime, float subDeltaTime);

    //ボーン同士がぶつかった衝撃を、角速度に変える
    void ApplyBoneImpactAngularImpulse(PhysicsBody& body, const CollisionResult& result,
        const Vector3& contactNormal, float velocityAlongNormalBefore);

    //胴体が地面に当たったとき、入ってきた向きで反射させて、全ボーンを、同じ速度の変化で、まとめて跳ね上げる
    //跳ねたら true
    bool TryRagdollHop(RigidBody& trigger, const Vector3& incomingVelocity, const Vector3& contactNormal);

    void LockRigidBodyAxis();

    //ほとんど動いていない箱などを、スリープさせる(ラグドールのボーンは、RagdollSystemが眠らせる)
    void UpdateBodySleep(float deltaTime);

    //眠っている箱などのうち、下に支えがない物を起こす(下の箱が消えて、上の箱が落ちるときなどに使う)
    //起こした箱の上の箱も、支えを失うので、変化がなくなるまで繰り返す
    void WakeUnsupportedBodies();

    //この箱の下に、支えがあるか(床や眠っている箱が、真下にあり、重心が支えの範囲の中にあるか)
    //fallingColliders: いま起こした、落ちる側の箱(支えにしない)
    bool HasSupport(const PhysicsBody& body, const std::vector<const Collider*>& fallingColliders) const;
private:
    float _sleepSpeed = 0.2f; //この速さ(m/s)より遅い間は、「動いていない」とみなす
    float _sleepSeconds = 0.5f; //動いていない状態が、この秒数続いたら、スリープさせる

    //体ぜんたいで跳ねる設定
    float _hopRestitution = 1.0f;
    int _hopCountLeft = 0; //残りの回数。0なら、この仕組みは使わない
    float _hopCooldown = 0.0f; //次に跳ねられるまでの、残りの秒数

    float _ragdollElapsedTime = 0.0f;
    std::vector<PhysicsBody> _rigidBodies;
    //所有者はRagdoll(RagdollBone::joint)。ここでは、解くために参照するだけ
    std::vector<Joint*> _jointList;

    RagdollSystem _ragdollSystem;
    CollisionManager _collisionManager;
};
