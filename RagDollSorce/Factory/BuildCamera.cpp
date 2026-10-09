#include "ObjectBuilders.h"

#include "GameObject.h"
#include "BaseCamera.h"
#include "CameraFollow.h"

//カメラ：視点制御 + 当たり判定用のSphereCollider（物理挙動あり）
void ObjectBuilders::BuildCamera(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& /*model*/)
{
    obj.AddComponent<Camera>();
    //発射したラグドールを追いかける(追う相手を渡されるまでは、何もしない)
    obj.AddComponent<CameraFollow>();
}
