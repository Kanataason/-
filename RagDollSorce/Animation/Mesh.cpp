#include "Mesh.h"

void Mesh::CreateMeshInfo(ID3D11Device* device)
{
	CreateIndexBuffer(device);
    CreateDrawBuffer(device);
}
void Mesh::CreateDrawBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC vertexDesc = {};

    //頂点データを読み取り専用の参照として取得
    const std::vector<Vertex>& verticesList = _meshInfo.vertices;

    //頂点1個分のサイズ × 頂点数でバッファサイズを計算
    vertexDesc.ByteWidth =
        sizeof(Vertex) * static_cast<UINT>(verticesList.size());

    //頂点バッファとして使用することを指定
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    //GPU側で使用するバッファ。
    //CPUから直接書き換える必要がないため DEFAULT を使用する。
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;

    //GPUへ初期データとして渡す頂点データを指定
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = verticesList.data();

    //頂点バッファを作成
    device->CreateBuffer(
        &vertexDesc,
        &data,
        _vertexBuffer.GetAddressOf()
    );
}
void Mesh::CreateIndexBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC indexDesc = {};

    //インデックスデータを読み取り専用の参照として取得
    const std::vector<UINT>& indices = _meshInfo.indices;

    //UINT 1個分のサイズ × インデックス数で
    //バッファサイズを計算
    indexDesc.ByteWidth =
        sizeof(UINT) * static_cast<UINT>(indices.size());

    //インデックスバッファとして使用することを指定
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    //GPU側で使用するバッファ。
    //CPUから直接書き換える必要がないため DEFAULT を使用する。
    indexDesc.Usage = D3D11_USAGE_DEFAULT;

    //GPUへ初期データとして渡すインデックスデータを指定
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = indices.data();

    //インデックスバッファを作成
    device->CreateBuffer(
        &indexDesc,
        &data,
        _indexBuffer.GetAddressOf()
    );

    //描画時に使用するインデックス数を保存
    _indexCount = static_cast<UINT>(indices.size());
}
