#include "ObjectBuilders.h"
#include "VectorUtility.h"

#include "GameObject.h"
#include "ObjectStatus.h"
#include "MeshRenderer.h"
#include "RigidBody.h"
#include "BoxCollider.h"

//点数が入る物(的)：状態 + 見た目 + 衝突用のBoxCollider
void ObjectBuilders::BuildScoreObject(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model)
{
    auto status = obj.AddComponent<ObjectStatus>();
    status->SetScore(data.score); //JSONの"score"(省略時は100)
    auto meshRenderer = obj.AddComponent<MeshRenderer>();
    meshRenderer->SetModel(model);

    auto coll = obj.AddComponent<BoxCollider>();
    coll->SetSize(data.transform.Scale);
    coll->SetCollisionGroup(CollisionGroup::Target);
    coll->SetPosition(data.transform.Position);

    auto rigid = obj.AddComponent<RigidBody>();
    rigid->DisableKinematic();
    rigid->SetPosition(Vector::fromF3ToV3(data.transform.Position));
    //最初から眠らせる。積んだ箱は、置いた時点で落ち着いているので、落ち着くのを待たなくていい
    if (data.startAsleep)
    {
        rigid->SetSleeping(true);
    }

    //質量が0のままだと、衝突の計算で 1/質量 が無限大になり、NaN(位置が消える)になる
    //慣性は、立方体として 質量 × 辺の長さ^2 / 6 で求める
    constexpr float crateMass = 2.0f;
    const float edge = data.transform.Scale.x;
    rigid->SetMass(crateMass);
    rigid->SetInertia(crateMass * edge * edge / 6.0f);
}