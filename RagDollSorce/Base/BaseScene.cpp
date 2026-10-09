#include "BaseScene.h"

#include "ObjectType.h"
#include "RigidBody.h"

#include "DebugUtility.h"
#include "PhysicsSystem.h"
#include "GameObject.h"
#include "BaseLight.h"
#include "BaseCamera.h"

using namespace DirectX;


void BaseScene::Draw(Renderer& renderer)
{
	//描写処理にカメラの情報を渡す
	if(_camera)
    renderer.SetCamera(*_camera);

	//オブジェクトごとに描画をする
	for (const auto& obj : _objects)
	{
		if (auto light = obj->GetComponent<Light>())
		{
			renderer.SetLight(*light);
		}
		obj->Draw(renderer);
	}
}

void BaseScene::Initialize(ServiceLocator& locator)//シーンの情報初期化
{
	_physicsSystem = locator.physicsSystem;
	_uiRenderer = locator.uiRenderer;
	_sceneManager = locator.sceneManager;
	SetObjectInfo(locator);
}
void BaseScene::SetObjectInfo(ServiceLocator& locator)
{
	for (const auto& obj : _objects)
	{
		obj->Initialize(locator);

		auto rigid = obj->GetComponent<RigidBody>();
		auto collider = obj->GetComponent<Collider>();

		//物理システムのほうに当たり判定と物理の処理をセットで送る
		if (rigid || collider)
		{
			PhysicsBody body;

			body.rigidBody = rigid;
			body.collider = collider;

			_physicsSystem->AddPhysicsBody(std::move(body));
		}
		
		if (auto light = obj->GetComponent<Light>())
			_lights.push_back(light);

		if (auto camera = obj->GetComponent<Camera>())
			_camera = camera;
	}
}

void BaseScene::Update(float deltaTime)
{
	for (const auto& obj : _objects)
	{
		obj->Update(deltaTime);
	}
}
void BaseScene::Release()
{
	for (const auto& obj : _objects)
	{
		obj->Release();
	}
}
void BaseScene::AddObject(std::unique_ptr<GameObject> object)
{
	//生成に失敗したObject(モデルの読み込みエラーなど)は、nullptrで返ってくる。
	//そのまま入れると、Initialize/Update/Drawで全オブジェクトを回すときに落ちる
	if (!object)
		return;

	_objects.push_back(std::move(object));
}