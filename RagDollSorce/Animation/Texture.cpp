#include "Texture.h"
bool Texture::Load(ID3D11Device* device,ID3D11DeviceContext* context,const std::wstring& path)
{
	//ファイルを開いてviewを作成
	HRESULT hr = DirectX::CreateWICTextureFromFile(
		device,
		context,
		path.c_str(),
		nullptr,
		_shaderview.ReleaseAndGetAddressOf());

	if (FAILED(hr))
	{
		OutputDebugStringW((L"Texture Load Failed : " + path + L"\n").c_str());

		wchar_t buffer[64];
		swprintf_s(buffer, L"HRESULT : 0x%08X\n", hr);
		OutputDebugStringW(buffer);

		return false;
	}

	return true;
}
void Texture::Bind(ID3D11DeviceContext* context)
{
	//テクスチャをGPUに送る
	context->PSSetShaderResources(
		0,
		1,
		_shaderview.GetAddressOf());
}