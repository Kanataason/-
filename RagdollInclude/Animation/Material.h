#pragma once
#include <DirectXMath.h>
#include "Texture.h"
using namespace DirectX;
class Material
{
public :

    const XMFLOAT4& GetColor() const{ return _color; }
    void SetColor(const XMFLOAT4& color)
    {
        _color = color;
    }

    const std::wstring& GetTexturePath() const { return _texturePath; }
    void SetTexturePath(const std::wstring& path)
    {
        _texturePath = path; 
    }

    const std::shared_ptr<Texture>& GetTexture() const{ return _texture; }
    void SetTexture(const std::shared_ptr<Texture> texture)
    {
        _texture = texture;
    }

    int GetUnlit()const { return _unlit; }
    void SetUnlit(bool unlit)
    {
        _unlit = unlit;
    }

private:
    //初期はなにもなしloaderでデフォルトのパスがふられる
    std::wstring _texturePath = L"";
    std::shared_ptr<Texture> _texture = nullptr;
    XMFLOAT4 _color = { 1,1,1,1 };
    int _unlit = 0;
};