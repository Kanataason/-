#pragma once
#include <iostream>

#include "Component.h"
#include "ModelInfo.h"
#include "Renderer.h"

class Animator;

class MeshRenderer:public Component
{
public :
	void Initialize(ServiceLocator& locator)override;

	void Draw(Renderer& renderer) ;

	void SetModel(std::shared_ptr<ModelDatas> model) { _model = model; }

private:
	Animator* _animator = nullptr;

	std::shared_ptr<ModelDatas> _model;
};