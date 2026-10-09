#include "ObjectBuilders.h"

#include "GameObject.h"
#include "MeshRenderer.h"
#include "BoxCollider.h"

//静的な置き物：見た目 + 衝突用のBoxCollider
void ObjectBuilders::BuildProp(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model)
{
    auto meshRenderer = obj.AddComponent<MeshRenderer>();
    meshRenderer->SetModel(model);

    auto coll = obj.AddComponent<BoxCollider>();
    coll->SetSize(data.transform.Scale);
    coll->SetCollisionGroup(CollisionGroup::Ground);
    coll->SetPosition(data.transform.Position);
}
