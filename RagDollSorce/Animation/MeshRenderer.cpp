#include "MeshRenderer.h"

#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "Animator.h"

void MeshRenderer::Initialize(ServiceLocator& locator)
{
	_animator = GetOwner()->GetComponent<Animator>();
	//描写のベースを作る
	for (auto& mesh : _model->meshes)
	{
		mesh.mesh->CreateMeshInfo(locator.device);
	}

}
void MeshRenderer::Draw(Renderer& renderer)
{
	//メッシュごとに処理をする
	for (auto& mesh : _model->meshes)
	{
		if (_animator)
		{
			renderer.SetAnimationBone(_animator->GetFinalTransforms());
		}
			
		//描画する
		renderer.Draw(*mesh.mesh, *mesh.material, GetOwner()->GetTransform());
	}
}
