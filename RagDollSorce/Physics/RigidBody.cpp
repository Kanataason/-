#include "RigidBody.h"
#include "GameObject.h"
#include "Collider.h"
#include "PhysicsSystem.h"

void RigidBody::Initialize(ServiceLocator& locator)
{
    isGravity = true;
}

//登録済みのPhysicsSystemがあれば、自動的に登録を解除する
//(GameObjectがunique_ptrで破棄されるとき、ここで自動的に呼ばれるので、
//呼び出し側で手動でRemoveを呼び忘れる心配がなくなる)
RigidBody::~RigidBody()
{
    if (_registry != nullptr)
    {
        _registry->RemoveRigidBody(this);
    }
}

void RigidBody::FixedUpdate(float deltaTime)
{
    UpdateGravity(deltaTime);
}


void RigidBody::UpdateGravity(float deltaTime)
{
    if (isGravity)
        Velocity.y += GRAVITY * deltaTime;

    //空気抵抗の代わりの減衰
    //周期が変わっても同じ減衰になるようにしている
    constexpr float linearDamping = RagdollTuning::linearDamping;
    Velocity *= std::exp(-linearDamping * deltaTime);

    //速度の上限。ジョイントや衝突の補正で速度が発散したときに、
    //ラグドールが吹き飛んだり、1フレームで壁をすり抜けたりするのを防ぐ安全弁。
    const float maxSpeed = _maxSpeed;
    if (Vector::SqMagnitude(Velocity) > maxSpeed * maxSpeed)
    {
        //上限を超えたときだけNormalizeする。
        //二乗のまま比較しているのは、毎フレーム平方根を計算しないため。
        Velocity.Normalize();
        Velocity = Velocity * maxSpeed;
    }

    Position += Velocity * deltaTime;

}

void RigidBody::RemoveVelocityAlongNormal(const Vector3& normal)
{
    float velocityAlongNormal = Vector::Dot(Velocity, normal);

    //normal は押し出し方向。内積が負なら押し出しとは逆向き、つまり面へ向かっている
    //正(面から離れている)のときは何もしない。消してしまうと面に貼りつくような動きになるため
    if (velocityAlongNormal < EPSILON)
        Velocity -= normal * velocityAlongNormal;
}


void RigidBody::ApplyTorque(float deltaTime)
{
    const float angularDamping = RagdollTuning::kAngularDamping;
    Vector3 angularAcceleration = Torque / Inertia;
    AngularVelocity += angularAcceleration * deltaTime;

    //並進側の減衰と同じ理由で exp を使い、周期が変わっても減衰の強さが変わらないようにしている
    //角速度の減衰がないと、ジョイントに繋がった部位がいつまでも揺れ続ける
    AngularVelocity *= std::exp(-angularDamping * deltaTime);

    //角速度の上限。ジョイント補正でトルクが跳ね上がったとき、
    //回転が発散してラグドールが暴れるのを防ぐ安全弁
    constexpr float maxAngularSpeed = RagdollTuning::maxAngularSpeed;
    if (Vector::SqMagnitude(AngularVelocity) > maxAngularSpeed * maxAngularSpeed)
    {
        AngularVelocity.Normalize();
        AngularVelocity = AngularVelocity * maxAngularSpeed;
    }

    //溜めたトルクは1フレーム分の力なので、使い終わったら必ず 0 に戻す
    //戻さないと次のフレームにも同じトルクがかかり続ける
    ClearTorque();
}


void RigidBody::IntegrateRotation(float deltaTime)
{
    float angularSpeed = AngularVelocity.Length();
    float angle = angularSpeed * deltaTime;

    //ほぼ回っていないときは何もしない
    //回転軸を作るときに angularSpeed で割るので、0 に近いと割り算が不安定になるのを避ける
    if (angle > EPSILON)
    {
        Vector3 rotationAxis = AngularVelocity / angularSpeed;
        Quaternion deltaRotation = QuaternionMath::CreateFromAxisAngle(rotationAxis, angle);

        //角速度はワールド座標の軸で持っているので、増分を左から掛ける
        //(右から掛けるとローカル軸まわりの回転になってしまう)
        Rotation = deltaRotation * Rotation;

        //掛け算を繰り返すと誤差でクォータニオンの長さが 1 からずれていくので、毎回正規化して戻す
        Rotation.Normalize();
    }
}

void RigidBody::ApplyFriction(const Vector3& normal, float frictionDecayPerSecond, float deltaTime)
{
    float velocityAlongNormal =
        Vector::Dot(Velocity, normal);

    //速度を、法線方向の成分と接線方向の成分に分ける
    //摩擦は接線方向にだけ働くので、分けてから接線側だけを減らす
    Vector3 normalVelocity =
        normal * velocityAlongNormal;

    Vector3 tangentVelocity =
        Velocity - normalVelocity;

    //接線速度が1フレームで残る割合(1に近いほど滑る)
    //「毎回一定割合を削る」方式だと FixedUpdate の周期で摩擦の強さが変わってしまうので、
    //時間ベースの減衰にして周期に依存しないようにしている
    float tangentRemainRatio = std::exp(-frictionDecayPerSecond * deltaTime);

    //法線成分はそのまま残し、接線成分だけ残る割合をかけて合成し直す
    Velocity = normalVelocity + tangentVelocity * tangentRemainRatio;
}

void RigidBody::ApplyGravityTorqueWorld(const Vector3& worldComOffset, float totalMass, float deltaTime)
{
    if (!isGravity)
        return;

    //トルクの強さを調整する倍率
    constexpr float scale = RagdollTuning::kRootGravityTorqueScale;

    Vector3 gravityForce = Vector3(0.0f, GRAVITY, 0.0f) * totalMass;
    Vector3 torque = Vector::Cross(worldComOffset, gravityForce);

    AngularVelocity += torque * (scale * deltaTime / Inertia);
}

void RigidBody::ApplyGravityTorque(const Vector3& centerOfMassOffsetLocal, float deltaTime)
{
    if (!isGravity)
        return;

    //ピボットと重心がほぼ同じ位置なら、トルクは 0 になるので計算を省く
    //長さの二乗で比較しているのは平方根を避けるため(1e-8 は長さにして 0.0001)
    if (Vector::SqMagnitude(centerOfMassOffsetLocal) < EPSILON)
        return;

    //オフセットはオブジェクトのローカル座標で持っているので、
    //重力(ワールド座標)と外積を取れるように、現在の回転でワールド座標へ変換する
    Vector3 worldOffset = Rotation * centerOfMassOffsetLocal;

    //重力による力は 質量 × 重力加速度。UpdateGravity と同じ GRAVITY を使い、
    //並進側と回転側で重力の強さがずれないようにしている
    Vector3 gravityForce = Vector3(0.0f, GRAVITY, 0.0f) * _mass;

    //トルク = 支点から重心までのベクトル × 力
    Vector3 torque = Vector::Cross(worldOffset, gravityForce);

    //トルクを慣性で割ると角加速度。それに時間をかけて角速度へ加える
    //ApplyTorque を通さずここで直接足しているので、減衰と速度制限はこの後の ApplyTorque で一緒にかかる
    AngularVelocity += torque * deltaTime / Inertia;
}