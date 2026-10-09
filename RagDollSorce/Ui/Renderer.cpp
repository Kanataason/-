#include <algorithm>
#include <string>

#include "Renderer.h"
#include "Vertex.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"

using BoneMatrix = DirectX::XMFLOAT4X4;

constexpr size_t MaxBones = 128;

namespace
{
    //リソース作成失敗時の共通エラー表示
    void ShowCreationError(const wchar_t* target)
    {
        std::wstring message = std::wstring(target) + L"作成失敗";
        MessageBox(nullptr, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
    }
}

void Renderer::CreateRasterizer()
{
    D3D11_RASTERIZER_DESC desc = {};

    //通常のポリゴン描画
    desc.FillMode = D3D11_FILL_SOLID;

    //裏面をカリング
    desc.CullMode = D3D11_CULL_BACK;

    //頂点の並び順を時計回りとして扱う
    desc.FrontCounterClockwise = FALSE;

    //深度範囲外のプリミティブをクリップ
    desc.DepthClipEnable = TRUE;

    HRESULT hr = _device->CreateRasterizerState(
        &desc,
        _rasterizer.GetAddressOf());

    if (FAILED(hr))
    {
        ShowCreationError(L"RasterizerState");
        return;
    }

    //作成したラスタライザステートを描画に使用
    _context->RSSetState(_rasterizer.Get());
}


void Renderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC desc = {};

    desc.ByteWidth = sizeof(ConstantBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = _device->CreateBuffer(
        &desc,
        nullptr,
        _constantBuffer.GetAddressOf());

    if (FAILED(hr))
    {
        ShowCreationError(L"ConstantBuffer");
    }
}


//アニメーション用ボーンバッファの作成
void Renderer::CreateAnimationBoneBuffer()
{
    D3D11_BUFFER_DESC desc = {};

    //最大128本のボーン行列を格納
    desc.ByteWidth =
        sizeof(BoneMatrix) * MaxBones;

    desc.Usage =
        D3D11_USAGE_DEFAULT;

    //VertexShaderからShaderResourceとして参照
    desc.BindFlags =
        D3D11_BIND_SHADER_RESOURCE;

    //構造化バッファとして使用
    desc.MiscFlags =
        D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    desc.StructureByteStride =
        sizeof(BoneMatrix);

    HRESULT hr = _device->CreateBuffer(
        &desc,
        nullptr,
        _boneBuffer.GetAddressOf());

    if (FAILED(hr))
    {
        ShowCreationError(L"AnimationBone Buffer");
        return;
    }

    //ShaderResourceViewの設定
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    //構造化バッファなのでUNKNOWNを指定
    srvDesc.Format =
        DXGI_FORMAT_UNKNOWN;

    srvDesc.ViewDimension =
        D3D11_SRV_DIMENSION_BUFFER;

    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = MaxBones;

    //VertexShaderからボーン行列を参照するためのView
    hr = _device->CreateShaderResourceView(
        _boneBuffer.Get(),
        &srvDesc,
        _boneSRVBuffer.GetAddressOf());

    if (FAILED(hr))
    {
        ShowCreationError(L"AnimationBone SRV");
    }
}


void Renderer::CreateSampleState()
{
    D3D11_SAMPLER_DESC desc = {};

    //ミップマップを含めた線形補間
    desc.Filter =
        D3D11_FILTER_MIN_MAG_MIP_LINEAR;

    //テクスチャ座標が範囲外の場合は繰り返す
    desc.AddressU =
        D3D11_TEXTURE_ADDRESS_WRAP;

    desc.AddressV =
        D3D11_TEXTURE_ADDRESS_WRAP;

    desc.AddressW =
        D3D11_TEXTURE_ADDRESS_WRAP;

    desc.MaxLOD =
        D3D11_FLOAT32_MAX;

    HRESULT hr = _device->CreateSamplerState(
        &desc,
        _sampleState.GetAddressOf());

    if (FAILED(hr))
    {
        ShowCreationError(L"SamplerState");
    }
}



bool Renderer::CompileShader()
{
    ComPtr<ID3DBlob> vsError;
    ComPtr<ID3DBlob> psError;

    //VertexShaderをコンパイル
    HRESULT hr = D3DCompileFromFile(
        L"Shaders/VertexShader.hlsl",
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        0,
        0,
        _verticesRenderer.GetAddressOf(),
        vsError.GetAddressOf());

    if (!CheckCompileFile(hr, vsError.Get()))
    {
        return false;
    }

    //PixelShaderをコンパイル
    hr = D3DCompileFromFile(
        L"Shaders/PixelShader.hlsl",
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        0,
        0,
        _pixelRenderer.GetAddressOf(),
        psError.GetAddressOf());

    if (!CheckCompileFile(hr, psError.Get()))
    {
        return false;
    }

    return true;
}


bool Renderer::CheckCompileFile(
    HRESULT hr,
    ID3DBlob* errorBlob)
{
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA(
                static_cast<const char*>(
                    errorBlob->GetBufferPointer()));
        }

        return false;
    }

    return true;
}


void Renderer::CreateShader()
{
    HRESULT hr = _device->CreateVertexShader(
        _verticesRenderer->GetBufferPointer(),
        _verticesRenderer->GetBufferSize(),
        nullptr,
        _verticesShader.ReleaseAndGetAddressOf());

    if (FAILED(hr))
    {
        ShowCreationError(L"VertexShader");
        return;
    }

    hr = _device->CreatePixelShader(
        _pixelRenderer->GetBufferPointer(),
        _pixelRenderer->GetBufferSize(),
        nullptr,
        _pixelShader.ReleaseAndGetAddressOf());

    if (FAILED(hr))
    {
        ShowCreationError(L"PixelShader");
    }
}



void Renderer::CreateInputLayout()
{
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            0,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },

        {
            "NORMAL",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            12,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },

        {
            "TEXCOORD",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            24,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },

        {
            "BLENDINDICES",
            0,
            DXGI_FORMAT_R32G32B32A32_UINT,
            0,
            32,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },

        {
            "BLENDWEIGHT",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            0,
            48,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        }
    };

    HRESULT hr = _device->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        _verticesRenderer->GetBufferPointer(),
        _verticesRenderer->GetBufferSize(),
        _inputLayout.GetAddressOf());

    if (FAILED(hr))
    {
        ShowCreationError(L"InputLayout");
    }
}


void Renderer::UpdateConstantBuffer(
    const Material& material,
    const Transform& transform)
{
    _ConstantBufferData.material.Unlit = material.GetUnlit();

    _ConstantBufferData.material.Color =
        material.GetColor();

    _ConstantBufferData.world =
        XMMatrixTranspose(
            transform.GetWorldMatrix());

    //CPU側のデータをGPUへ転送
    _context->UpdateSubresource(
        _constantBuffer.Get(),
        0,
        nullptr,
        &_ConstantBufferData,
        0,
        0);

    ID3D11Buffer* buffer =
        _constantBuffer.Get();

    //VertexShader / PixelShaderから
    //ConstantBufferを参照できるように設定
    _context->VSSetConstantBuffers(
        0,
        1,
        &buffer);

    _context->PSSetConstantBuffers(
        0,
        1,
        &buffer);
}


void Renderer::Draw(
    const Mesh& mesh,
    const Material& material,
    const Transform& transform)
{
    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff); //標準の、不透明合成に戻す
    _context->OMSetDepthStencilState(nullptr, 0);
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    //テクスチャサンプラーを設定
    _context->PSSetSamplers(
        0,
        1,
        _sampleState.GetAddressOf());

    //マテリアルにテクスチャがあれば設定
    auto& texture = material.GetTexture();

    if (texture)
    {
        texture->Bind(_context);
    }

    ID3D11Buffer* vertexBuffer =
        mesh.GetVertexBuffer();

    ID3D11Buffer* indexBuffer =
        mesh.GetIndexBuffer();

    UINT indexCount =
        mesh.GetIndexCount();

    //オブジェクトごとの描画情報をGPUへ転送
    UpdateConstantBuffer(
        material,
        transform);

    //頂点バッファを設定
    _context->IASetVertexBuffers(
        0,
        1,
        &vertexBuffer,
        &stride,
        &offset);

    //インデックスバッファを設定
    _context->IASetIndexBuffer(
        indexBuffer,
        DXGI_FORMAT_R32_UINT,
        0);

    //頂点入力レイアウトを設定
    _context->IASetInputLayout(
        _inputLayout.Get());

    //三角形単位で描画
    _context->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //シェーダーを設定
    _context->VSSetShader(
        _verticesShader.Get(),
        nullptr,
        0);

    _context->PSSetShader(
        _pixelShader.Get(),
        nullptr,
        0);

    //インデックスバッファを使用して描画
    _context->DrawIndexed(
        indexCount,
        0,
        0);
}


void Renderer::Initialize(
    ID3D11Device* device,
    ID3D11DeviceContext* context)
{
    SetDeviceAndContext(
        device,
        context);

    //GPU描画に必要なリソースを作成
    CreateRasterizer();
    CreateAnimationBoneBuffer();
    CreateConstantBuffer();
    CreateSampleState();

    //HLSLをコンパイル
    if (!CompileShader())
    {
        return;
    }

    //コンパイルしたシェーダーから
    //GPUで使用するオブジェクトを作成
    CreateShader();

    //頂点データとシェーダーの入力形式を対応付け
    CreateInputLayout();
}


//ここから先は ConstantBufferの情報をアップデートする関数


//ライト情報の設定
void Renderer::SetLight(const Light& light)
{
    _ConstantBufferData.light.Color =
        light.GetColor();

    if (light.GetLightType() ==
        LightType::Directional)
    {
        const DirectionalLight& directionalLight =
            static_cast<const DirectionalLight&>(light);

        _ConstantBufferData.light.Direction =
            directionalLight.GetDirection();
    }
}


void Renderer::SetCamera(const Camera& camera)
{
    _ConstantBufferData.projection =
        XMMatrixTranspose(
            camera.GetProjectionMatrix());

    _ConstantBufferData.view =
        XMMatrixTranspose(
            camera.GetViewMatrix());
}



void Renderer::SetAnimationBone(
    const std::vector<XMFLOAT4X4>& finalTransforms)
{
    XMFLOAT4X4 matrices[MaxBones] = {};

    //GPU側の最大ボーン数を超えないように制限
    const size_t count =
        std::min(
            finalTransforms.size(),
            MaxBones);

    //CPU側の最終ボーン行列をGPU転送用配列へコピー
    std::copy_n(
        finalTransforms.begin(),
        count,
        matrices);

    //ボーン行列をGPUへ転送
    _context->UpdateSubresource(
        _boneBuffer.Get(),
        0,
        nullptr,
        matrices,
        0,
        0);

    //VertexShaderのt1スロットへ設定
    _context->VSSetShaderResources(
        1,
        1,
        _boneSRVBuffer.GetAddressOf());
}