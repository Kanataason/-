#pragma once
#include <d3d11.h>
#include <unordered_map>
#include <string>
#include <iostream>

#include "Mesh.h"
#include "ModelLoader.h"
#include "ObjectType.h"
#include "ModelInfo.h"
#include "AnimationLoader.h"

class ModelManager
{
public :

	ModelManager(ModelLoader& modeloader,AnimationLoader& animaLoader) :_modelLoader(modeloader),_animationLoader(animaLoader) {}

	void SetDevice(ID3D11Device* device);

	//すでにロードされているかを確認する
	std::shared_ptr<ModelDatas> CheckModel(const ObjectData& data);

private :
	std::unordered_map<PrimitiveShape, std::shared_ptr<ModelDatas>> primitives;
	std::unordered_map<std::string, std::shared_ptr<ModelDatas>> models;

	ID3D11Device* _device = nullptr;
	ModelLoader& _modelLoader;
	AnimationLoader& _animationLoader;
};