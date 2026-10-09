#pragma once
#include <WICTextureLoader.h>
#include <d3d11.h>

#include <wrl.h>
#include <string>

using Microsoft::WRL::ComPtr;

class Texture
{
public:
	~Texture() = default;
	//テクスチャをロードする
	bool Load(ID3D11Device* device, ID3D11DeviceContext* context,const std::wstring& path);

	//GPCにテクスチャを反映
	void Bind(ID3D11DeviceContext* context);

private:

	ComPtr<ID3D11ShaderResourceView> _shaderview;
};