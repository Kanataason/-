#include "ObjectBuilders.h"

#include "GameObject.h"
#include "CannonController.h"
#include "MeshRenderer.h"

//大砲：揺れて発射する操作 + 見た目
void ObjectBuilders::BuildCannon(GameObject& obj, const ObjectData& /*data*/, const std::shared_ptr<ModelDatas>& model)
{
    obj.AddComponent<CannonController>();
    auto meshRenderer = obj.AddComponent<MeshRenderer>();
    meshRenderer->SetModel(model);
}
