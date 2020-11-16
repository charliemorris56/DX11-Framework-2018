#include "Application.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

Application::Application()
{
	_hInst = nullptr;
	_hWnd = nullptr;
	_driverType = D3D_DRIVER_TYPE_NULL;
	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	_pd3dDevice = nullptr;
	_pImmediateContext = nullptr;
	_pSwapChain = nullptr;
	_pRenderTargetView = nullptr;
	_pVertexShader = nullptr;
	_pPixelShader = nullptr;
	_pVertexLayout = nullptr;
	_pVertexBuffer = nullptr;
	_pIndexBuffer = nullptr;
    _pVertexBufferPyramid = nullptr;
    _pIndexBufferPyramid = nullptr;
    _pVertexBufferFloor = nullptr;
    _pIndexBufferFloor = nullptr;
	_pConstantBuffer = nullptr;
    _pTextureRV = nullptr;
    _pSamplerLinear = nullptr;
    _camera = nullptr;
    _transparency = nullptr;
}

Application::~Application()
{
	Cleanup();
}

HRESULT Application::Initialise(HINSTANCE hInstance, int nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
        return E_FAIL;
	}

    RECT rc;
    GetClientRect(_hWnd, &rc);
    _WindowWidth = rc.right - rc.left;
    _WindowHeight = rc.bottom - rc.top;

    if (FAILED(InitDevice()))
    {
        Cleanup();

        return E_FAIL;
    }

	// Initialize the world matrix
	XMStoreFloat4x4(&_world, XMMatrixIdentity());

    // Eye Position in World
    XMFLOAT3 eyePosW = XMFLOAT3(0.0f, 10.0f, -10.0f); //Where the camera is
    XMFLOAT3 at = XMFLOAT3(0.0f, 0.0f, 0.0f); //What it is looking at
    XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f); //The orintaion

    // Initialize the Camera
    _camera = new Camera(eyePosW, at, up, _WindowWidth, _WindowHeight, FLOAT(0.0f), FLOAT(0.0f));

    // Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(_camera->getEye().x, _camera->getEye().y, _camera->getEye().z, 0.0f);
	XMVECTOR At = XMVectorSet(_camera->getAt().x, _camera->getAt().y, _camera->getAt().z, 0.0f);
	XMVECTOR Up = XMVectorSet(_camera->getUp().x, _camera->getUp().y, _camera->getUp().z, 0.0f);
       
    // Light direction from surface (XYZ)
    lightDirection = XMFLOAT3(0.25f, 0.5f, -1.0f);
    // Diffuse material properties (RGBA)
    diffuseMaterial = XMFLOAT4(0.2f, 0.2f, 0.2f, 0.0f);
    // Diffuse light colour (RGBA)
    diffuseLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); 
    // Ambient meterial properties (RGBA)
    ambientMeterial = XMFLOAT4(0.2f, 0.2f, 0.2f, 0.2f);
    // Ambient light colour (RGBA)
    ambientLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    // Specular meterial properties (RGBA)
    specularMeterial = XMFLOAT4(0.2f, 0.2f, 1.0f, 0.2f);
    // Specular light colour (RGBA)
    specularLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    // Specular Power
    specularPower = 1.0f;

    // Texture loading
    CreateDDSTextureFromFile(_pd3dDevice, L"Crate_COLOR.dds", nullptr, &_pTextureRV);
    _pImmediateContext->PSSetShaderResources(0, 1, &_pTextureRV);

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    _pd3dDevice->CreateSamplerState(&sampDesc, &_pSamplerLinear);
    _pImmediateContext->PSSetSamplers(0, 1, &_pSamplerLinear);

    objMeshData = OBJLoader::Load("star.obj", _pd3dDevice);

	return S_OK;
}

HRESULT Application::InitShadersAndInputLayout()
{
	HRESULT hr;

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "VS", "vs_4_0", &pVSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the vertex shader
	hr = _pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);

	if (FAILED(hr))
	{	
		pVSBlob->Release();
        return hr;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "PS", "ps_4_0", &pPSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the pixel shader
	hr = _pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
	pPSBlob->Release();

    if (FAILED(hr))
        return hr;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        //{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
	hr = _pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
                                        pVSBlob->GetBufferSize(), &_pVertexLayout);
	pVSBlob->Release();

	if (FAILED(hr))
        return hr;

    // Set the input layout
    _pImmediateContext->IASetInputLayout(_pVertexLayout);

	return hr;
}

HRESULT Application::InitVertexBuffer()
{
	HRESULT hr;
    
    D3D11_BUFFER_DESC bd;
    D3D11_SUBRESOURCE_DATA InitData;

    // Create vertex buffer Cube
    //SimpleVertex vertices[] =
    //{
    //    { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT4( 1.0f, 0.0f, 1.0f, 1.0f ) }, //0
    //    { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT4( 1.0f, 0.0f, 1.0f, 1.0f ) }, //1
    //    { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 0.0f, 1.0f, 1.0f ) }, //2
    //    { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) }, //3
    //    { XMFLOAT3( -1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, //4
    //    { XMFLOAT3( 1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, //5
    //    { XMFLOAT3( 1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) }, //6
    //    { XMFLOAT3( -1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) }, //7
    //};

    SimpleVertex vertices[] =
    {
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), NormalCalc(XMFLOAT3(-1.0f, 1.0f, -1.0f)), XMFLOAT2(0.0f, 0.0f) }, //0
        { XMFLOAT3(1.0f, 1.0f, -1.0f), NormalCalc(XMFLOAT3(1.0f, 1.0f, -1.0f)), XMFLOAT2(1.0f, 0.0f) }, //1
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), NormalCalc(XMFLOAT3(-1.0f, -1.0f, -1.0f)), XMFLOAT2(0.0f, 1.0f) }, //2
        { XMFLOAT3(1.0f, -1.0f, -1.0f), NormalCalc(XMFLOAT3(1.0f, -1.0f, -1.0f)), XMFLOAT2(1.0f, 1.0f) }, //3
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), NormalCalc(XMFLOAT3(-1.0f, -1.0f, 1.0f)), XMFLOAT2(0.0f, 0.0f) }, //4
        { XMFLOAT3(1.0f, -1.0f, 1.0f), NormalCalc(XMFLOAT3(1.0f, -1.0f, 1.0f)), XMFLOAT2(0.0f, 1.0f) }, //5
        { XMFLOAT3(1.0f, 1.0f, 1.0f), NormalCalc(XMFLOAT3(1.0f, 1.0f, 1.0f)), XMFLOAT2(1.0f, 1.0f) }, //6
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), NormalCalc(XMFLOAT3(-1.0f, 1.0f, 1.0f)), XMFLOAT2(1.0f, 0.0f) }, //7
    };

    int vertexCount = 8;

	ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * vertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pVertexBuffer);

    if (FAILED(hr))
        return hr;

    // Create vertex buffer Pyarid
    //SimpleVertex verticesPyramid[] =
    //{
    //    { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, //0
    //    { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, //1
    //    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, //2
    //    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, //3
    //    { XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, //4
    //};

    //SimpleVertex verticesPyramid[] =
    //{
    //    { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.6f, 0.0f) }, //0
    //    { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, //1
    //    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.3f, -0.6f, 0.6f) }, //2
    //    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -0.6f, 1.0f) }, //3
    //    { XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.3f, 0.0f, -0.3f) }, //4
    //};

    SimpleVertex verticesPyramid[] =
    {
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), NormalCalc(XMFLOAT3(-1.0f, -1.0f, 1.0f)), XMFLOAT2(0.0f, 0.0f) }, //0
        { XMFLOAT3(1.0f, -1.0f, 1.0f), NormalCalc(XMFLOAT3(1.0f, -1.0f, 1.0f)), XMFLOAT2(1.0f, 0.0f) }, //1
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), NormalCalc(XMFLOAT3(-1.0f, -1.0f, -1.0f)), XMFLOAT2(0.0f, 1.0f) }, //2
        { XMFLOAT3(1.0f, -1.0f, -1.0f), NormalCalc(XMFLOAT3(1.0f, -1.0f, -1.0f)), XMFLOAT2(1.0f, 1.0f) }, //3
        { XMFLOAT3(0.0f, 1.0f, 0.0f), NormalCalc(XMFLOAT3(0.0f, 1.0f, 0.0f)), XMFLOAT2(1.0f, 0.0f) }, //4
    };

    int vertexCountPyramid = 5;

    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * vertexCountPyramid;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = verticesPyramid;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pVertexBufferPyramid);

    if (FAILED(hr))
        return hr;

    // Create vertex buffer Floor
    SimpleVertex verticesFloor[] =
    //{
    //    { XMFLOAT3(-4.0f, -2.0f, 4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //0 topleft
    //    { XMFLOAT3(-2.0f, -2.0f, 4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //1
    //    { XMFLOAT3(0.0f, -2.0f, 4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //2
    //    { XMFLOAT3(2.0f, -2.0f, 4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //3
    //    { XMFLOAT3(4.0f, -2.0f, 4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //4 topright
    //    { XMFLOAT3(-4.0f, -2.0f, 2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //5
    //    { XMFLOAT3(-2.0f, -2.0f, 2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //6
    //    { XMFLOAT3(0.0f, -2.0f, 2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //7
    //    { XMFLOAT3(2.0f, -2.0f, 2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //8
    //    { XMFLOAT3(4.0f, -2.0f, 2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //9
    //    { XMFLOAT3(-4.0f, -2.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //10 middleleft
    //    { XMFLOAT3(-2.0f, -2.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //11
    //    { XMFLOAT3(0.0f, -2.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //12 middlemiddle
    //    { XMFLOAT3(2.0f, -2.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //13
    //    { XMFLOAT3(4.0f, -2.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //14 middleright
    //    { XMFLOAT3(-4.0f, -2.0f, -2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //15
    //    { XMFLOAT3(-2.0f, -2.0f, -2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //16
    //    { XMFLOAT3(0.0f, -2.0f, -2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //17
    //    { XMFLOAT3(2.0f, -2.0f, -2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //18
    //    { XMFLOAT3(4.0f, -2.0f, -2.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //19
    //    { XMFLOAT3(-4.0f, -2.0f, -4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //20 bottomleft
    //    { XMFLOAT3(-2.0f, -2.0f, -4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //21
    //    { XMFLOAT3(0.0f, -2.0f, -4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //22
    //    { XMFLOAT3(2.0f, -2.0f, -4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //23
    //    { XMFLOAT3(4.0f, -2.0f, -4.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, //24 bottomright
    //};
    {
        { XMFLOAT3(-4.0f, -2.0f, 4.0f), NormalCalc(XMFLOAT3(-4.0f, -2.0f, 4.0f)) }, //0 topleft
        { XMFLOAT3(-2.0f, -2.0f, 4.0f), NormalCalc(XMFLOAT3(-2.0f, -2.0f, 4.0f)) }, //1
        { XMFLOAT3(0.0f, -2.0f, 4.0f), NormalCalc(XMFLOAT3(0.0f, -2.0f, 4.0f)) }, //2
        { XMFLOAT3(2.0f, -2.0f, 4.0f), NormalCalc(XMFLOAT3(2.0f, -2.0f, 4.0f)) }, //3
        { XMFLOAT3(4.0f, -2.0f, 4.0f), NormalCalc(XMFLOAT3(4.0f, -2.0f, 4.0f)) }, //4 topright
        { XMFLOAT3(-4.0f, -2.0f, 2.0f), NormalCalc(XMFLOAT3(-4.0f, -2.0f, 2.0f)) }, //5
        { XMFLOAT3(-2.0f, -2.0f, 2.0f), NormalCalc(XMFLOAT3(-2.0f, -2.0f, 2.0f)) }, //6
        { XMFLOAT3(0.0f, -2.0f, 2.0f), NormalCalc(XMFLOAT3(0.0f, -2.0f, 2.0f)) }, //7
        { XMFLOAT3(2.0f, -2.0f, 2.0f), NormalCalc(XMFLOAT3(2.0f, -2.0f, 2.0f)) }, //8
        { XMFLOAT3(4.0f, -2.0f, 2.0f), NormalCalc(XMFLOAT3(4.0f, -2.0f, 2.0f)) }, //9
        { XMFLOAT3(-4.0f, -2.0f, 0.0f), NormalCalc(XMFLOAT3(-4.0f, -2.0f, 0.0f)) }, //10 middleleft
        { XMFLOAT3(-2.0f, -2.0f, 0.0f), NormalCalc(XMFLOAT3(-2.0f, -2.0f, 0.0f)) }, //11
        { XMFLOAT3(0.0f, -2.0f, 0.0f), NormalCalc(XMFLOAT3(0.0f, -2.0f, 0.0f)) }, //12 middlemiddle
        { XMFLOAT3(2.0f, -2.0f, 0.0f), NormalCalc(XMFLOAT3(2.0f, -2.0f, 0.0f)) }, //13
        { XMFLOAT3(4.0f, -2.0f, 0.0f), NormalCalc(XMFLOAT3(4.0f, -2.0f, 0.0f)) }, //14 middleright
        { XMFLOAT3(-4.0f, -2.0f, -2.0f), NormalCalc(XMFLOAT3(-4.0f, -2.0f, -2.0f)) }, //15
        { XMFLOAT3(-2.0f, -2.0f, -2.0f), NormalCalc(XMFLOAT3(-2.0f, -2.0f, -2.0f)) }, //16
        { XMFLOAT3(0.0f, -2.0f, -2.0f), NormalCalc(XMFLOAT3(0.0f, -2.0f, -2.0f)) }, //17
        { XMFLOAT3(2.0f, -2.0f, -2.0f), NormalCalc(XMFLOAT3(2.0f, -2.0f, -2.0f)) }, //18
        { XMFLOAT3(4.0f, -2.0f, -2.0f), NormalCalc(XMFLOAT3(4.0f, -2.0f, -2.0f)) }, //19
        { XMFLOAT3(-4.0f, -2.0f, -4.0f), NormalCalc(XMFLOAT3(-4.0f, -2.0f, -4.0f)) }, //20 bottomleft
        { XMFLOAT3(-2.0f, -2.0f, -4.0f), NormalCalc(XMFLOAT3(-2.0f, -2.0f, -4.0f)) }, //21
        { XMFLOAT3(0.0f, -2.0f, -4.0f), NormalCalc(XMFLOAT3(0.0f, -2.0f, -4.0f)) }, //22
        { XMFLOAT3(2.0f, -2.0f, -4.0f), NormalCalc(XMFLOAT3(2.0f, -2.0f, -4.0f)) }, //23
        { XMFLOAT3(4.0f, -2.0f, -4.0f), NormalCalc(XMFLOAT3(4.0f, -2.0f, -4.0f)) }, //24 bottomright
    };
    int vertexCountFloor = 45;

    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * vertexCountFloor;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = verticesFloor;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pVertexBufferFloor);

    if (FAILED(hr))
        return hr;

	return S_OK;
}

HRESULT Application::InitIndexBuffer()
{
	HRESULT hr;
    D3D11_BUFFER_DESC bd;
    D3D11_SUBRESOURCE_DATA InitData;

    // Create index buffer Cube
    WORD indices[] =
    {
        0,1,2, //front1
        2,1,3, //front2
        4,3,5, //bottom1
        4,2,3, //bottom2
        3,6,5, //right1
        3,1,6, //right2
        2,7,0, //left1
        2,4,7, //left2
        7,4,5, //back1
        5,6,7, //back2
        0,7,1, //top1
        7,6,1, //top2
    };

	ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * indexCountCube;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = indices;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pIndexBuffer);

    if (FAILED(hr))
        return hr;

    // Create index buffer Pyramid
    WORD indicesPyramid[] =
    {
        0,1,2, //bottom1
        2,3,1, //bottom2
        1,4,3, //rightface1
        3,4,2, //frontface1
        2,4,0, //leftface1
        0,4,1, //backface1
    };

    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * indexCountPyramid;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = indicesPyramid;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pIndexBufferPyramid);

    if (FAILED(hr))
        return hr;

    // Create index buffer Floor
    WORD indicesFloor[] =
    {
        0,1,5, //toprow
        5,6,1, 
        1,2,6, 
        6,2,7, 
        7,2,3, 
        3,7,8, 
        8,3,4, 
        4,8,9, 
        9,14,13, //2ndrow 
        13,9,8, 
        8,13,12, 
        12,8,7, 
        7,12,11, 
        11,7,6, 
        6,11,10, 
        10,5,6, 
        10,11,15, //3rdrow 
        15,16,11,
        11,12,16,
        16,17,12,
        12,17,13,
        13,17,18,
        18,13,14,
        14,18,19,
        19,24,23, //4throw
        23,19,18,
        18,23,22,
        22,18,17,
        17,22,21,
        21,17,16,
        16,21,20,
        20,16,15,
    };

    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * indexCountFloor;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = indicesFloor;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pIndexBufferFloor);

    if (FAILED(hr))
        return hr;

	return S_OK;
}

HRESULT Application::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    _hInst = hInstance;
    RECT rc = {0, 0, 640, 480};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    _hWnd = CreateWindow(L"TutorialWindowClass", L"DX11 Framework", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                         nullptr);
    if (!_hWnd)
		return E_FAIL;

    ShowWindow(_hWnd, nCmdShow);

    return S_OK;
}

HRESULT Application::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob != nullptr)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

        if (pErrorBlob) pErrorBlob->Release();

        return hr;
    }

    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

HRESULT Application::InitDevice()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = _WindowWidth;
    sd.BufferDesc.Height = _WindowHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = _hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        _driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                           D3D11_SDK_VERSION, &sd, &_pSwapChain, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FAILED(hr))
        return hr;

    hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
    pBackBuffer->Release();

    if (FAILED(hr))
        return hr;

    // Define depth/stencil buffer
    D3D11_TEXTURE2D_DESC depthStencilDesc;
    depthStencilDesc.Width = _WindowWidth;
    depthStencilDesc.Height = _WindowHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    // Create the depth/stencil buffer
    _pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &_depthStencilBuffer);
    _pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);

    _pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, _depthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)_WindowWidth;
    vp.Height = (FLOAT)_WindowHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _pImmediateContext->RSSetViewports(1, &vp);

	InitShadersAndInputLayout();

	InitVertexBuffer();
    
	InitIndexBuffer();
    
    // Set primitive topology
    _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &_pConstantBuffer);

    // Define the rasterizer state
    D3D11_RASTERIZER_DESC wfdesc;
    ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
    wfdesc.FillMode = D3D11_FILL_SOLID; //D3D11_FILL_WIREFRAME
    wfdesc.CullMode = D3D11_CULL_NONE; //D3D11_CULL_BACK
    hr = _pd3dDevice->CreateRasterizerState(&wfdesc, &_wireFrame);
    _pImmediateContext->RSSetState(_wireFrame);

    // Define the blend state
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));

    D3D11_RENDER_TARGET_BLEND_DESC rtbd;
    ZeroMemory(&rtbd, sizeof(rtbd));

    rtbd.BlendEnable = true;
    rtbd.SrcBlend = D3D11_BLEND_SRC_COLOR;
    rtbd.DestBlend = D3D11_BLEND_BLEND_FACTOR;
    rtbd.BlendOp = D3D11_BLEND_OP_ADD;
    rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
    rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
    rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rtbd.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.RenderTarget[0] = rtbd;

    _pd3dDevice->CreateBlendState(&blendDesc, &_transparency);

    if (FAILED(hr))
        return hr;

    return S_OK;
}

void Application::Cleanup()
{
    if (_pImmediateContext) _pImmediateContext->ClearState();
    if (_pConstantBuffer) _pConstantBuffer->Release();
    if (_pVertexBuffer) _pVertexBuffer->Release();
    if (_pIndexBuffer) _pIndexBuffer->Release();
    if (_pVertexBufferPyramid) _pVertexBufferPyramid->Release();
    if (_pIndexBufferPyramid) _pIndexBufferPyramid->Release();
    if (_pVertexBufferFloor) _pVertexBufferFloor->Release();
    if (_pIndexBufferFloor) _pIndexBufferFloor->Release();
    if (_pVertexLayout) _pVertexLayout->Release();
    if (_pVertexShader) _pVertexShader->Release();
    if (_pPixelShader) _pPixelShader->Release();
    if (_pRenderTargetView) _pRenderTargetView->Release();
    if (_pSwapChain) _pSwapChain->Release();
    if (_pImmediateContext) _pImmediateContext->Release();
    if (_pd3dDevice) _pd3dDevice->Release();
    if (_depthStencilView) _depthStencilView->Release();
    if (_depthStencilBuffer) _depthStencilBuffer->Release();
    if (_wireFrame) _wireFrame->Release();
    if (_pTextureRV) _pTextureRV->Release();
    if (_pSamplerLinear) _pSamplerLinear->Release();
    if (_camera) _camera->~Camera();
    if (_transparency) _transparency->Release();
}

void Application::Update()
{
    // Update our time
    static float t = 0.0f;

    if (_driverType == D3D_DRIVER_TYPE_REFERENCE)
    {
        t += (float) XM_PI * 0.0125f;
    }
    else
    {
        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();

        if (dwTimeStart == 0)
            dwTimeStart = dwTimeCur;

        t = (dwTimeCur - dwTimeStart) / 1000.0f;
    }

    //For shader
    //gTime = t;

    //
    // Animate the objects
    //
	XMStoreFloat4x4(&_world, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixRotationY(t * 0.1f) * XMMatrixRotationX(t * 0.1f) * XMMatrixTranslation(0.0f, 5.0f, 0.0f)); //sun
    XMStoreFloat4x4(&_world2, XMMatrixRotationY(t * 0.3f) * XMMatrixTranslation(5.0f, 0.0f, 0.0f) * XMMatrixRotationY(t)); //planet1
    XMStoreFloat4x4(&_world3, XMMatrixRotationY(t * 0.7f) * XMMatrixTranslation(10.0f, 0.0f, 0.0f) * XMMatrixRotationY(t * 0.3f)); //planet2
    XMStoreFloat4x4(&_world4, XMMatrixRotationZ(t * 5.0f) * XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixTranslation(2.0f, 0.0f, 0.0f) * XMMatrixRotationY(t) * XMMatrixTranslation(5.0f, 0.0f, 0.0f) * XMMatrixRotationY(t)); //moon1
    XMStoreFloat4x4(&_world5, XMMatrixScaling(0.15f, 0.15f, 0.15f) * XMMatrixTranslation(1.5f, 0.0f, 0.0f) * XMMatrixRotationY(t * 0.8f) * XMMatrixTranslation(10.0f, 0.0f, 0.0f) * XMMatrixRotationY(t * 0.3f)); //moon2
    XMStoreFloat4x4(&_floor, XMMatrixTranslation(0.0f, 0.0f, 0.0f)); //floor
    XMStoreFloat4x4(&_sphere, XMMatrixTranslation(0.0f, 0.0f, 0.0f)); //sphere

    // Keyboard input https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    // use 0x0001 for a key toggle, or 0x8000 for a held down
    if (GetKeyState('D') & 0x8000)
    {
        _camera->setEye(XMFLOAT3(_camera->getEye().x + 0.005f, _camera->getEye().y, _camera->getEye().z));
    }
    if (GetKeyState('A') & 0x8000)
    {
        _camera->setEye(XMFLOAT3(_camera->getEye().x - 0.005f, _camera->getEye().y, _camera->getEye().z));
    }
    if (GetKeyState('W') & 0x8000)
    {
        _camera->setEye(XMFLOAT3(_camera->getEye().x, _camera->getEye().y + 0.005f, _camera->getEye().z));
    }    
    if (GetKeyState('S') & 0x8000)
    {
        _camera->setEye(XMFLOAT3(_camera->getEye().x, _camera->getEye().y - 0.005f, _camera->getEye().z));
    }
    if (GetKeyState('Q') & 0x8000)
    {
        _camera->setEye(XMFLOAT3(_camera->getEye().x, _camera->getEye().y, _camera->getEye().z + 0.005f));
    }
    if (GetKeyState('E') & 0x8000)
    {
        _camera->setEye(XMFLOAT3(_camera->getEye().x, _camera->getEye().y, _camera->getEye().z - 0.005f));
    }

    // Wireframe toggling
    if (GetKeyState('K') & 0x8000) //To Wireframe
    {
        HRESULT hr = S_OK;
        D3D11_RASTERIZER_DESC wfdesc;
        ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
        wfdesc.FillMode = D3D11_FILL_WIREFRAME;
        wfdesc.CullMode = D3D11_CULL_NONE; //D3D11_CULL_BACK
        hr = _pd3dDevice->CreateRasterizerState(&wfdesc, &_wireFrame);
        _pImmediateContext->RSSetState(_wireFrame);
    }
    if (GetKeyState('L') & 0x8000) //To Solid
    {
        HRESULT hr = S_OK;
        D3D11_RASTERIZER_DESC wfdesc;
        ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
        wfdesc.FillMode = D3D11_FILL_SOLID;
        wfdesc.CullMode = D3D11_CULL_NONE; //D3D11_CULL_BACK
        hr = _pd3dDevice->CreateRasterizerState(&wfdesc, &_wireFrame);
        _pImmediateContext->RSSetState(_wireFrame);
    }
}

void Application::Draw()
{
    //
    // Clear the back buffer
    //
    float ClearColor[4] = {0.0f, 0.125f, 0.3f, 1.0f}; // red,green,blue,alpha
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);
    _pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;

	XMMATRIX world = XMLoadFloat4x4(&_world); //Sun
	XMMATRIX view = XMLoadFloat4x4(&_camera->getView());
	XMMATRIX projection = XMLoadFloat4x4(&_camera->getProjection());
    
    //
    // Update variables
    //
    ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose(world);
	cb.mView = XMMatrixTranspose(view);
	cb.mProjection = XMMatrixTranspose(projection);
    //cb.gTime = gTime;
    cb.DiffuseLight = diffuseLight;
    cb.DiffuseMtrl = diffuseMaterial;
    cb.LightVecW = lightDirection;
    cb.buffer = NULL;
    cb.AmbientLight = ambientLight;
    cb.AmbientMtrl = ambientMeterial;
    cb.SpecularMtrl = specularMeterial;
    cb.SpecularLight = specularLight;
    cb.SpecularPower = specularPower;
    cb.EyePosW = eyePosW;

	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

    // Opaque objects
    _pImmediateContext->OMSetBlendState(0, 0, 0xffffffff); // Set the default blend state (no blending) for opaque objects
    
    //Set buffers to Pyramid
    _pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBufferPyramid, &stride, &offset); 
    _pImmediateContext->IASetIndexBuffer(_pIndexBufferPyramid, DXGI_FORMAT_R16_UINT, 0); 

    //
    // Renders a triangle
    //
	_pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
	_pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
    _pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
	_pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
	_pImmediateContext->DrawIndexed(indexCountPyramid, 0, 0);

    world = XMLoadFloat4x4(&_world2); //Planet1
    cb.mWorld = XMMatrixTranspose(world);
    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->DrawIndexed(indexCountPyramid, 0, 0);

    // Transparent objects
    float blendFactor[] = { 0.75f, 0.75f, 0.75f, 1.0f }; //blending equation
    _pImmediateContext->OMSetBlendState(_transparency, blendFactor, 0xffffffff); //Set the blend state for transparent object
       
    world = XMLoadFloat4x4(&_world3); //Planet2
    cb.mWorld = XMMatrixTranspose(world);
    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->DrawIndexed(indexCountPyramid, 0, 0);

    //Set buffers to Cube
    _pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBuffer, &stride, &offset); 
    _pImmediateContext->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0); 

    world = XMLoadFloat4x4(&_world4); //Moon1
    cb.mWorld = XMMatrixTranspose(world);
    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->DrawIndexed(indexCountCube, 0, 0);

    world = XMLoadFloat4x4(&_world5); //Moon2
    cb.mWorld = XMMatrixTranspose(world);
    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->DrawIndexed(indexCountCube, 0, 0);

    //Set buffers to Floor
    _pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBufferFloor, &stride, &offset);
    _pImmediateContext->IASetIndexBuffer(_pIndexBufferFloor, DXGI_FORMAT_R16_UINT, 0);

    world = XMLoadFloat4x4(&_floor); //Floor
    cb.mWorld = XMMatrixTranspose(world);
    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->DrawIndexed(indexCountFloor, 0, 0);

    //Set buffers to Sphere
    _pImmediateContext->IASetVertexBuffers(0, 1, &objMeshData.VertexBuffer, &stride, &offset);
    _pImmediateContext->IASetIndexBuffer(objMeshData.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    world = XMLoadFloat4x4(&_sphere); //Floor
    cb.mWorld = XMMatrixTranspose(world);
    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->DrawIndexed(objMeshData.IndexCount, 0, 0);

    //
    // Present our back buffer to our front buffer
    //
    _pSwapChain->Present(0, 0);
}

XMFLOAT3 Application::NormalCalc(XMFLOAT3 vec)
{
    float length = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    float x = vec.x / length;
    float y = vec.y / length;
    float z = vec.z / length;

    return XMFLOAT3(x,y,z);
}