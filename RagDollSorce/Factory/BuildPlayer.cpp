#include "ObjectBuilders.h"

#include "GameObject.h"
#include "VectorUtility.h"
#include "Player.h"
#include "RigidBody.h"
#include "Ragdoll.h"
#include "MeshRenderer.h"
#include "Animator.h"

//プレイヤー：操作・物理・ラグドール・見た目・アニメーションをまとめて構築
void ObjectBuilders::BuildPlayer(GameObject& obj, const ObjectData& data, const std::shared_ptr<ModelDatas>& model)
{
    obj.AddComponent<Player>();

    //通常時はキネマティックとして扱い、物理演算に押されないようにする
    auto rigi = obj.AddComponent<RigidBody>();
    rigi->EnableKinematic();
    rigi->SetPosition(Vector::fromF3ToV3(data.transform.Position));

    //ラグドール切り替え用にSkeletonと所有者を紐付け
    auto doll = obj.AddComponent<Ragdoll>();
    doll->SetSkeleton(model->skeleton);
    doll->SetParent(obj);
    doll->SetColliderScale(data.colliderScale);
    doll->SetBounce(data.bounceRestitution, data.bounceCount);

    auto meshRenderer = obj.AddComponent<MeshRenderer>();
    meshRenderer->SetModel(model);

    //アニメーション再生に必要な情報一式を渡す
    auto anim = obj.AddComponent<Animator>();
    anim->SetAnimationInfo(model->animation, model->skeleton, model->animations);
}
