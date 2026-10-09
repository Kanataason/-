#include "GameObject.h"

void GameObject::Initialize(ServiceLocator& locator)
{
	for (auto& com : _components)
		com->Initialize(locator);
}
void GameObject::Update(float deltaTime)
{
	for (auto& com : _components)
		com->Update(deltaTime);
}
void GameObject::Draw()
{
	for (auto& com : _components)
		com->Draw();
}

void GameObject::OnColliderEnter(float damage)
{
	for (auto& com : _components)
		com->OnColliderEnter(damage);
}