#pragma once
#include <vector>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <wrl.h>

#include "Vertex.h"
using Microsoft::WRL::ComPtr;

struct MeshInfo
{
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;

	const std::vector<Vertex> GetVertices() const{ return vertices; }
	const std::vector<UINT> GetIndices()const { return indices; }
};

class Mesh
{
	public:
		template<typename T>
		static Mesh Create(float size)
		{
			return T::Create(size);
		}

		~Mesh()
		{ 
			if (_vertexBuffer)_vertexBuffer->Release();
			if (_indexBuffer)_indexBuffer->Release();
		}

		//頂点バッファとインデックスバッファを作成する。
		void CreateMeshInfo(ID3D11Device* device);

		//ゲッターとセッター

	    ID3D11Buffer* GetVertexBuffer() const { return _vertexBuffer.Get(); }
	    ID3D11Buffer* GetIndexBuffer() const { return _indexBuffer.Get(); }

		 UINT GetIndexCount()const { return _indexCount; }

		 void SetMeshInfo(const MeshInfo& meshinfo) { _meshInfo = meshinfo; }
		 MeshInfo& GetMeshInfos() { return _meshInfo; }
		 const MeshInfo& GetMeshInfos() const { return _meshInfo; }
private:
	//頂点バッファを作成する
	void CreateDrawBuffer(ID3D11Device* device);

	//インデックスバッファを作成する
	void CreateIndexBuffer(ID3D11Device* device);

private:
	MeshInfo _meshInfo;

	ComPtr<ID3D11Buffer> _vertexBuffer = nullptr;
	ComPtr<ID3D11Buffer> _indexBuffer = nullptr;

	UINT _indexCount = 0;
};

//図形の頂点データとインデックス構造体
namespace Primitive
{
	struct TriangleMesh
	{
		static Mesh Create(float size)
		{
			Mesh mesh;
			MeshInfo meshinfo;
			float h = size * 0.5f;

			meshinfo.vertices =
			{
	{ {-h,  h, 0.0f}, {0,0,-1}, {0,0} },
	{ { h,  h, 0.0f}, {0,0,-1}, {1,0} },
	{ {-h, -h, 0.0f}, {0,0,-1}, {0,1} },
			};

			meshinfo.indices =
			{
				   0,1,2
			};
			mesh.SetMeshInfo(meshinfo);
			return mesh;
		}
	};
	struct QuadMesh
	{
		static Mesh Create(float size)
		{
			Mesh mesh;
			MeshInfo meshinfo;
			float h = size * 0.5f;

			meshinfo.vertices =
			{
	{ {-h,  h, 0.0f},  {0,0,-1}, {0,0} },
	{ { h,  h, 0.0f},  {0,0,-1}, {1,0} },
	{ {-h, -h, 0.0f},  {0,0,-1}, {0,1} },
	{ { h, -h, 0.0f},  {0,0,-1}, {1,1} },
			};

			meshinfo.indices =
			{
				  0, 1, 2,
				  2, 1, 3
			};

			mesh.SetMeshInfo(meshinfo);

			return mesh;
		}
	};
	struct BoxMesh
	{
		static Mesh Create(float size)
		{
			Mesh mesh;
			MeshInfo meshinfo;
			float h = size * 0.5f;
			meshinfo.vertices =
			{
				//Front
				{ {-h,-h,-h}, {0,0,-1}, {0,1} },
				{ {-h, h,-h}, {0,0,-1}, {0,0} },
				{ { h, h,-h}, {0,0,-1}, {1,0} },
				{ { h,-h,-h}, {0,0,-1}, {1,1} },

				//Back
				{ {-h,-h, h}, {0,0,1}, {0,1} },
				{ { h,-h, h}, {0,0,1}, {1,1} },
				{ { h, h, h}, {0,0,1}, {1,0} },
				{ {-h, h, h}, {0,0,1}, {0,0} },

				//Up
				{ {-h, h,-h}, {0,1,0}, {0,1} },
                { {-h, h, h}, {0,1,0}, {0,0} },
                { { h, h, h}, {0,1,0}, {1,0} },
                { { h, h,-h}, {0,1,0}, {1,1} },

				//Right
                {{ h,-h,-h}, {1,0,0}, {0,1}},
                {{ h, h,-h}, {1,0,0}, {0,0}},
                {{ h, h, h}, {1,0,0}, {1,0}},
                {{ h,-h, h}, {1,0,0}, {1,1}},

				//Left
                {{-h,-h,-h}, {-1,0,0}, {0,1}},
                {{-h, h,-h}, {-1,0,0}, {0,0}},
                {{-h, h, h}, {-1,0,0}, {1,0}},
                {{-h,-h, h}, {-1,0,0}, {1,1}},
				//Down
                { {-h,-h, h}, {0,-1,0}, {0,1} },
                { { h,-h, h}, {0,-1,0}, {1,1} },
                { { h,-h,-h}, {0,-1,0}, {1,0} },
                { {-h,-h,-h}, {0,-1,0}, {0,0} },
			};

			meshinfo.indices =
			{
				//Front
				0,1,2,
				0,2,3,
				//Back
				4,5,6,
				4,6,7,

				//Up
				8,9,10,
				8,10,11,

				//Right
				12,13,14,
				12,14,15,

				//Left
				16,18,17,
				16,19,18,

				//Down(下から見て時計回りになる順番。他の面と同じ向きにそろえる)
				20,22,21,
				20,23,22,
			};

			mesh.SetMeshInfo(meshinfo);
			return mesh;
		}
	};

}