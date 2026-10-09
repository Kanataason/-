#pragma once
#include <iostream>
#include <unordered_map>
#include <string>
#include <d3dcompiler.h>
#include <d3d11.h>

class Texture;

class TextureManager
{
public:

	void SetDeviceAndContext(ID3D11Device* device, ID3D11DeviceContext* context) 
	{
		_device = device;
		_context = context;
	}
	std::shared_ptr<Texture> CheckTexture(const std::wstring& path);

private:
	//これはbaseSceneの物を借りてくるからComPtrにしなくていいしRemoveもしなくていい
	ID3D11Device* _device = nullptr;
	ID3D11DeviceContext* _context = nullptr;

	std::shared_ptr<Texture> _defaultTexture;
	std::unordered_map<std::wstring, std::shared_ptr<Texture>> _textures;
};