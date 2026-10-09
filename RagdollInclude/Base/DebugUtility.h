#pragma once
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <wrl.h>
#include <vector>

#include "Vertex.h"
#include "ServiceLocator.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Camera;

class DebugDraw
{
    struct DebugConstantBuffer
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
    };
public:
    void Initialize(ServiceLocator& locator);

    //球体を3つの平面(XY/XZ/YZ)の円で近似してワイヤーフレーム描画
    void DrawSphere(const XMFLOAT3& position, float radius, const XMFLOAT4& color);
    void DrawBox(const XMFLOAT3& min, const XMFLOAT3& max, const XMFLOAT4& color);

    //カプセル(始点・終点・半径)をワイヤーフレームで描画
    //円柱部分の側面線・両端の半球(近似のリング)で構成される
    void DrawCapsule(const XMFLOAT3& s, const XMFLOAT3& e, const float& radius, const XMFLOAT4& color);

    //1本の線分を頂点リストに追加する(全てのDraw～関数はこれを組み合わせて描く)
    void DrawLine( const XMFLOAT3& start, const XMFLOAT3& end,const XMFLOAT4& color);

    //始点から方向×最大距離の位置まで1本の線を引く(RayCastの可視化用)
    void DrawRay(const XMFLOAT3& origin, const XMFLOAT3& direction,const float& maxDistance, const XMFLOAT4& color);

    //溜まった線分データをまとめてGPUに送り、線描画してクリアする
    void Render(Camera& camera);
    void ClearList() { _vertices.clear(); };
    size_t GetList() const
    {
        return _vertices.size();
    }
private:

    void SetDevice(ID3D11Device* device) { _device = device; }
    void SetContext(ID3D11DeviceContext* context) { _context = context; }

    //カメラのview/projection行列をConstantBufferへ書き込む(worldは常に単位行列)
    void UpdateConstantBuffer(Camera& camera);

    //現在の_verticesの数に合わせて、CPUから書き換え可能な動的VertexBufferを作成
    void CreateVertexBuffer();
    void CreateInputLayout();

    //コンパイル済みシェーダーバイナリからVertex/PixelShaderオブジェクトを作成
    void CreateShader();

    //デバッグ描画用のVertex/PixelShaderをHLSLファイルからコンパイル
    void CompileShader();

    //カメラ行列(world/view/projection)を送るためのConstantBufferを作成
    void CreateConstantBuffer();

private:
    std::vector<DebugDrawVertex> _vertices;

    ID3D11Device* _device = nullptr;
    ID3D11DeviceContext* _context = nullptr;

    DebugConstantBuffer _constantData = {};

    ComPtr<ID3D11InputLayout> _inputLayout;
    ComPtr<ID3DBlob> _vertexShaderBlob;
    ComPtr<ID3DBlob> _pixelShaderBlob;

    ComPtr<ID3D11Buffer> _vertecesBlob;

    ComPtr<ID3D11Buffer> _constantBuffer;

    ComPtr<ID3D11VertexShader> _verticesShader;
    ComPtr<ID3D11PixelShader> _pixelShader;

    size_t _vertexCapacity = 0;
};