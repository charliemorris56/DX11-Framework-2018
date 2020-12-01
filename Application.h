#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"
#include "Structures.h"
#include "OBJLoader.h"
#include "DDSTextureLoader.h"
#include "Camera.h"
#include "rapidxml.hpp"

using namespace DirectX;
using namespace rapidxml;

class Application
{
private:
	HINSTANCE               _hInst;
	HWND                    _hWnd;
	D3D_DRIVER_TYPE         _driverType;
	D3D_FEATURE_LEVEL       _featureLevel;
	ID3D11Device*           _pd3dDevice;
	ID3D11DeviceContext*    _pImmediateContext;
	IDXGISwapChain*         _pSwapChain;
	ID3D11RenderTargetView* _pRenderTargetView;
	ID3D11VertexShader*     _pVertexShader;
	ID3D11PixelShader*      _pPixelShader;
	ID3D11InputLayout*      _pVertexLayout;
	ID3D11Buffer*           _pVertexBuffer;
	ID3D11Buffer*           _pIndexBuffer;
	ID3D11Buffer*			_pVertexBufferPyramid;
	ID3D11Buffer*			_pIndexBufferPyramid;
	ID3D11Buffer*			_pVertexBufferFloor;
	ID3D11Buffer*			_pIndexBufferFloor;
	ID3D11Buffer*           _pConstantBuffer;
	XMFLOAT4X4              _sun, _planet1, _planet2, _moon1, _moon2, _floor, _sphere, _car;
	XMFLOAT4X4              _view;
	XMFLOAT4X4              _projection;

	XMFLOAT3				lightDirection;
	XMFLOAT4				diffuseMaterial;
	XMFLOAT4				diffuseLight;
	XMFLOAT4				ambientLight;
	XMFLOAT4				ambientMeterial;
	XMFLOAT4				specularMeterial;
	XMFLOAT4				specularLight;
	FLOAT					specularPower;
	XMFLOAT3				eyePosW;

	ID3D11ShaderResourceView* _pTextureRV;
	ID3D11SamplerState*		_pSamplerLinear;

	MeshData				starObjMeshData;
	MeshData				carObjMeshData;

	Camera*					_camera;
	Camera*					_cameraStatic;
	Camera*					_cameraTopDown;
	Camera*					_cameraFirstPerson;
	Camera*					_cameraThirdPerson;

	XMFLOAT3				carPos;
	XMFLOAT3				sunPos;
	XMFLOAT3				planet1Pos;
	XMFLOAT3				planet2Pos;
	XMFLOAT3				moon1Pos;
	XMFLOAT3				moon2Pos;
	XMFLOAT3				floorPos;
	XMFLOAT3				spherePos;

	XMFLOAT4				speed;

	POINT					cursorPoint;
	XMFLOAT2				cursorPointXY;

	ID3D11BlendState*		_transparency;
	
	int indexCountCube = 36;
	int indexCountPyramid = 18;
	int indexCountFloor = 72;

	//float gTime;

private:
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
	HRESULT InitDevice();
	void Cleanup();
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT InitShadersAndInputLayout();
	HRESULT InitVertexBuffer();
	HRESULT InitIndexBuffer();

	XMFLOAT3 NormalCalc(XMFLOAT3 vec);

	void XML();

	UINT _WindowHeight;
	UINT _WindowWidth;

	ID3D11DepthStencilView* _depthStencilView;
	ID3D11Texture2D* _depthStencilBuffer;

	ID3D11RasterizerState* _wireFrame;

public:
	Application();
	~Application();

	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);

	void Update();
	void Draw();
};

