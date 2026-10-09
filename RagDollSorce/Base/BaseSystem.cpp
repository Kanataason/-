#include <d3d11.h>
#include <Windows.h>

#include"BaseSystem.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

void BaseSystem::CreateDepthStencil()
{
    //深度・ステンシルバッファの設定を作成
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = SCREEN_WIDTH;
    depthDesc.Height = SCREEN_HEIGHT;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    //深度・ステンシル用テクスチャをGPU上に作成
    ComPtr<ID3D11Texture2D> depthTexture;
    HRESULT hr = _device->CreateTexture2D(
        &depthDesc,
        nullptr,
        &depthTexture);

    if (FAILED(hr))
    {
        OutputDebugStringA("DepthStencil Texture作成失敗\n");
        return;
    }

    //深度・ステンシルテクスチャを描画に使用するためのViewを作成
    hr = _device->CreateDepthStencilView(
        depthTexture.Get(),
        nullptr,
        _depthStencil.GetAddressOf());

    if (FAILED(hr))
    {
        OutputDebugStringA("DepthStencilView作成失敗\n");
    }

}
void BaseSystem::ClearScreen()
{
    //カラーバッファをクリア
    const float clearColor[4] =
    {
        0.0f,
        0.0f,
        1.0f,
        0.0f
    };

    _context->ClearRenderTargetView(
        _renderTargetView.Get(),
        clearColor);

    //深度バッファを初期化
    //1.0fは「最も遠い深度」を表す
    _context->ClearDepthStencilView(
        _depthStencil.Get(),
        D3D11_CLEAR_DEPTH,
        1.0f,
        0);

    //今フレームで使用する描画先を設定
    _context->OMSetRenderTargets(
        1,
        _renderTargetView.GetAddressOf(),
        _depthStencil.Get());
}
//スクリーンの情報をセットする処理
void BaseSystem::SetViewPort(float width, float height, float minDeph, float maxDepth, float topX, float topY)
{
    _viewport.Width = width;
    _viewport.Height = height;
    _viewport.MinDepth = minDeph;
    _viewport.MaxDepth = maxDepth;
    _viewport.TopLeftX = topX;
    _viewport.TopLeftY = topY;

    _context->RSSetViewports(1, &_viewport);
}

void BaseSystem::SetScreenInfo(int width, int height, HWND hwnd)
{
    _swapChainDesc = {};

    _swapChainDesc.BufferDesc.Width = width;
    _swapChainDesc.BufferDesc.Height = height;
    _swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    //バックバッファを描画先として使用
    _swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    _swapChainDesc.BufferCount = 1;

    //描画対象となるウィンドウ
    _swapChainDesc.OutputWindow = hwnd;

    //マルチサンプリングは使用しない
    _swapChainDesc.SampleDesc.Count = 1;

    //ウィンドウモードで起動
    _swapChainDesc.Windowed = TRUE;

    //Present時にバックバッファを破棄する方式
    _swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
}


void BaseSystem::Present()
{
    if (_swapChain)
    {
        //垂直同期を1回待って画面を更新
        _swapChain->Present(1, 0);
    }
}
void BaseSystem::Release()//cmtpを使っているので不要
{
}

//DirectXの初期化
void BaseSystem::Initialize(HWND hwnd)
{
    //スワップチェーンに使用する画面情報を設定
    SetScreenInfo(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        hwnd);

    //GPUデバイス・コンテキスト・スワップチェーンを作成
    CreateDeviceAndSwapChain();

    if (!_device || !_context || !_swapChain)
    {
        OutputDebugStringA("DirectX11初期化失敗\n");
        return;
    }

    //深度バッファを作成
    CreateDepthStencil();

    //描画領域を設定
    SetViewPort(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0.0f,
        1.0f,
        0.0f,
        0.0f);

    OutputDebugStringA("BaseSystem Initialize完了\n");
}
IDXGIAdapter1* BaseSystem::FindBestAdapter()
{
    //窓口を作成
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))//型を間違えないためにIID使用
        return nullptr;

    SIZE_T maxMemory = 0;
    IDXGIAdapter1* bestAdapter = nullptr;

    //すべてのGPUを見る
    for (UINT i = 0;; ++i)
    {
        IDXGIAdapter1* temp = nullptr;

        if (factory->EnumAdapters1(i, &temp) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc;
        temp->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            temp->Release();
            continue;
        }

        if (desc.DedicatedVideoMemory > maxMemory)
        {
            if (bestAdapter)
                bestAdapter->Release();

            bestAdapter = temp;
            maxMemory = desc.DedicatedVideoMemory;
        }
        else
        {
            temp->Release();
        }
    }
    factory->Release();
    return bestAdapter;
}
void BaseSystem::CreateDeviceAndSwapChain()
{
    D3D_FEATURE_LEVEL featureLevel;

    auto bestAdapter = FindBestAdapter();//アダプターを取得
    if (!bestAdapter)
    {
        OutputDebugStringA("GPUアダプターの取得に失敗\n");
        return;
    }
    //ここで基盤を初期化
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        bestAdapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &_swapChainDesc,
        _swapChain.GetAddressOf(),
        _device.GetAddressOf(),
        &featureLevel,
        _context.GetAddressOf()
    );
 
    bestAdapter->Release();

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"DirectX11 初期化失敗", L"Error", MB_OK);
        return;
    }

    hr = _swapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(_backBuffer.GetAddressOf()));
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"GetBuffer失敗", L"Error", MB_OK);
        return;
    }
    //画像を最適な設定で描画命令を生成元に通知
    hr = _device->CreateRenderTargetView(
        _backBuffer.Get(),
        nullptr,
        _renderTargetView.GetAddressOf()
    );
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"RTV作成失敗", L"Error", MB_OK);
        return;
    }

}

