//--------------------------------------------------------------------------------------
// File: lecture 8.cpp
//
// Clouds, voxels, and octres
//
//--------------------------------------------------------------------------------------

#include "render_to_texture.h"
#include "shaderheader.h"
#include <fstream>
unsigned int put_in_octree(XMFLOAT3 pos);
//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11RasterizerState*				g_pRasterState = NULL;

ID3D11ShaderResourceView*           g_pTextureSun = NULL;

RenderTextureClass					RenderToTexture;
RenderTextureClass					LightSourcePass;
RenderTextureClass					VolumetricPass;
RenderTextureClass					BrightPass;
RenderTextureClass					BloomPass;

RenderTextureClass					Voxel_GI;
RenderTextureClass					Octree_RW;
RenderTextureClass					VFL;
RenderTextureClass					const_count;

ID3D11VertexShader*                 g_pScreenVS = NULL;
ID3D11PixelShader*                  g_pScreenPS = NULL;

ID3D11PixelShader*                  g_pSkyPS = NULL;
ID3D11PixelShader*                  g_pCloudPS = NULL;

ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;

ID3D11VertexShader*                 g_pEffectVS = NULL;
ID3D11PixelShader*                  g_pEffectPS = NULL;

ID3D11ComputeShader*                g_pOctreeCS = NULL;	//NEW

ID3D11VertexShader*                 g_pShadowVS = NULL;
ID3D11PixelShader*                  g_pShadowPS = NULL;
ID3D11PixelShader*                  g_pSunPS = NULL;

ID3D11PixelShader*                  g_pBrightPS = NULL;
ID3D11VertexShader*                 g_pBloomVS = NULL;
ID3D11PixelShader*                  g_pBloomPS = NULL;

ID3D11VertexShader*                 g_pVoxelVS = NULL;
ID3D11GeometryShader*				g_pVoxelGS = NULL;
ID3D11PixelShader*                  g_pVoxelPS = NULL;
voxel_								voxel;

ID3D11InputLayout*                  g_pVertexLayout = NULL;
ID3D11Buffer*                       g_pVertexBuffer = NULL;
ID3D11Buffer*                       g_pVertexBuffer_sky = NULL;
ID3D11Buffer*                       g_pVertexBuffer_screen = NULL;
int									vertex_count;

//states for turning off and on the depth buffer
ID3D11DepthStencilState				*ds_on, *ds_off;
ID3D11BlendState*					g_BlendState;

ID3D11Buffer*                       g_pCBuffer = NULL;

ID3D11Buffer*						g_pCBufferCS = NULL;

ID3D11ShaderResourceView*           g_pTextureRV = NULL;
ID3D11ShaderResourceView*           g_pSkyboxTex = NULL; //skybox
ID3D11ShaderResourceView*           normaltex = NULL;

ID3D11SamplerState*                 g_pSamplerLinear = NULL;

XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);

camera								cam;
std::vector<billboard*>				smokeray;
XMFLOAT3							rocket_position;

billboard							sun;
billboard							testcloud;
billboard							testcloud2;
int									plane = 0;
int									voxeldraw = 0;
int									numberOfClouds = 10;
float								offsetx = 2;
float								offsety = 0;
float								offsetz = 2;
bool                                madeClouds = false;

#define ROCKETRADIUS				10

int GIarr[100000];

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
bool LoadCatmullClark(LPCTSTR filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count);
void Render();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}
	srand(time(0));

	for (int i = 0; i < 100000; i++) {
		GIarr[i] = 0;
	}
	
	std::ofstream tfile;
	tfile.open("test.txt");

	put_in_octree(XMFLOAT3(-.1,-.1,-.1));
	put_in_octree(XMFLOAT3(.1, .1, .1));

	for (int i = 0; i < 100000; i++) {
		tfile << "Octree_RW[" << i << "] = " << GIarr[i] << ";\n";
	}

	sun.position.x = 0;
	sun.position.y = 0;
	sun.position.z = 1000;
	sun.scale = 50;
	sun.rotation = 0;

	testcloud.position.x = -2;
	testcloud.position.y = -2;
	testcloud.position.z = 0;
	testcloud.scale = 1;
	testcloud.rotation = 0;

	testcloud2.position.x = -2;
	testcloud2.position.y = -2;
	testcloud2.position.z = -2;
	testcloud2.scale = 1;
	testcloud2.rotation = 0;

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
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
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 1280, 720 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Octree Clouds", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

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
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	/////////// Shaders ///////////
	ID3DBlob* pVSBlob = NULL;
	ID3DBlob* pGSBlob = NULL;
	ID3DBlob* pPSBlob = NULL;

	// Voxel vertex shader
	hr = CompileShaderFromFile(L"voxel.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVoxelVS);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Voxel geometry shader
	hr = CompileShaderFromFile(L"voxel.fx", "GS", "gs_4_0", &pGSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &g_pVoxelGS);
	pGSBlob->Release();
	if (FAILED(hr))
		return hr;
	
	// Voxel pixel shader
	hr = CompileShaderFromFile(L"voxel.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pVoxelPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Scene vertex shader
	pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Scene pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_5_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Cloud pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PScloud", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pCloudPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Skybox pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSsky", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pSkyPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Screen vertex shader
	pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VS_screen", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pScreenVS);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Screen pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS_screen", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pScreenPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Bloom vertex shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"bloomshader.fx", "VS", "vs_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreateVertexShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pBloomVS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Bloom pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"bloomshader.fx", "PS_bloom", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pBloomPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Bright pass pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"bloomshader.fx", "PS_bright", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pBrightPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Volumetric lighting vertex shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"vlshader.fx", "VS", "vs_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreateVertexShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pEffectVS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Volumetric lighting pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"vlshader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pEffectPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Shadow pass scene vertex shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shadowshader.fx", "VS", "vs_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreateVertexShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pShadowVS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Shadow pass scene pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shadowshader.fx", "PSscene", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pShadowPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Shadow pass sun pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shadowshader.fx", "PSsun", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pSunPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compute shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"compute.fx", "CS", "cs_5_0", &pPSBlob);
	if(FAILED(hr))
		{
		MessageBox(NULL,
				   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
		}
	hr = g_pd3dDevice->CreateComputeShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pOctreeCS);
	pPSBlob->Release();
	if(FAILED(hr))
		return hr;

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	/////////// Vertex buffers ///////////
	// Create skysphere vertex buffer
	LoadCatmullClark(L"ccsphere.cmp", g_pd3dDevice, &g_pVertexBuffer_sky, &vertex_count);

	// Create screen vertex buffer
	SimpleVertex vertices_screen[] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0,0) ,XMFLOAT3(0.0f, 0.0f, 1.0f), },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1,0) ,XMFLOAT3(0.0f, 0.0f, 1.0f), },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0,1) ,XMFLOAT3(0.0f, 0.0f, 1.0f), },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0,1) ,XMFLOAT3(0.0f, 0.0f, 1.0f), },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1,0) ,XMFLOAT3(0.0f, 0.0f, 1.0f), },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1,1) ,XMFLOAT3(0.0f, 0.0f, 1.0f), },
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices_screen;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer_screen);
	if (FAILED(hr))
		return hr;

	// Create double sided square vertex buffer
	SimpleVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },

		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) ,XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f),XMFLOAT3(0.0f, 0.0f, 1.0f) }
	};

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 12;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Create voxel vertex buffer
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * voxel.anz;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = voxel.v;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &voxel.vbuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffers
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBuffer);
	if (FAILED(hr))
		return hr;

	// Create the constant buffers for the CS
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBufferCS);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBufferCS);
	if (FAILED(hr))
		return hr;



	// Load skybox texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"skybox3.png", NULL, NULL, &g_pSkyboxTex, NULL);
	if (FAILED(hr))
		return hr;

	// Load cloud texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"smoke.dds", NULL, NULL, &g_pTextureRV, NULL);
	if (FAILED(hr))
		return hr;

	// Load cloud normal map texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"smoke_NormalMap.png", NULL, NULL, &normaltex, NULL);
	if (FAILED(hr))
		return hr;

	// Load sun texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"sun.png", NULL, NULL, &g_pTextureSun, NULL);
	if (FAILED(hr))
		return hr;

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
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr))
		return hr;

	// Initialize the world matrices
	g_World = XMMatrixIdentity();

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, -0.01f, 0.0f);//camera position
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);//look at
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);// normal vector on at vector (always up)
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 10000.0f);

	// Initialize the constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose(g_View);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
	constantbuffer.info = XMFLOAT4(1, 1, 1, 1);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	ConstantBufferCS constantbufferCS;
	constantbufferCS.values = XMFLOAT4(0,0,0,0);
	g_pImmediateContext->UpdateSubresource(g_pCBufferCS, 0, NULL, &constantbufferCS, 0, 0);

	// Create the blend state
	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_BlendState);

	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_BlendState, blendFactor, sampleMask);

	// Create the depth stencil states for turning the depth buffer on and of:
	D3D11_DEPTH_STENCIL_DESC		DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;
	// Stencil test parameters
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	// Stencil operations if pixel is front-facing
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Stencil operations if pixel is back-facing
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Create depth stencil state
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	g_pd3dDevice->CreateDepthStencilState(&DS_ON, &ds_on);
	g_pd3dDevice->CreateDepthStencilState(&DS_OFF, &ds_off);

	D3D11_RASTERIZER_DESC rasterizerState;
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.CullMode = D3D11_CULL_NONE;
	rasterizerState.FrontCounterClockwise = false;
	rasterizerState.DepthBias = 0;
	rasterizerState.DepthBiasClamp = 0.0f;
	rasterizerState.SlopeScaledDepthBias = 0.0f;
	rasterizerState.DepthClipEnable = true;
	rasterizerState.ScissorEnable = false;
	rasterizerState.MultisampleEnable = false;
	rasterizerState.AntialiasedLineEnable = false;
	g_pd3dDevice->CreateRasterizerState(&rasterizerState, &g_pRasterState);
	g_pImmediateContext->RSSetState(g_pRasterState);

	rocket_position = XMFLOAT3(0, 0, ROCKETRADIUS);

	// RenderTarget textures
	RenderToTexture.Initialize_2DTex(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM, TRUE);
	Voxel_GI.Initialize_3DTex(g_pd3dDevice, 512, 512, 512, TRUE, DXGI_FORMAT_R8G8B8A8_UNORM, TRUE);
	Octree_RW.Initialize_1DTex(g_pd3dDevice, g_hWnd, -1, TRUE, DXGI_FORMAT_R32_UINT, TRUE);
	VFL.Initialize_1DTex(g_pd3dDevice, g_hWnd, -1, TRUE, DXGI_FORMAT_R32G32B32A32_FLOAT, TRUE);
	const_count.Initialize_1DTex(g_pd3dDevice, g_hWnd, 1, TRUE, DXGI_FORMAT_R32_UINT, TRUE);
	LightSourcePass.Initialize_2DTex(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM, FALSE);
	VolumetricPass.Initialize_2DTex(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM, FALSE);
	BrightPass.Initialize_2DTex(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM, FALSE);
	BloomPass.Initialize_2DTex(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM, FALSE);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	if (g_pTextureRV) g_pTextureRV->Release();
	if (g_pCBuffer) g_pCBuffer->Release();
	if (g_pCBufferCS) g_pCBufferCS->Release();
	
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pScreenVS) g_pScreenVS->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pCloudPS) g_pCloudPS->Release();
	if (g_pEffectVS) g_pEffectVS->Release();
	if (g_pEffectPS) g_pEffectPS->Release();
	if (g_pShadowVS) g_pShadowVS->Release();
	if (g_pShadowPS) g_pShadowPS->Release();
	if (g_pSunPS) g_pSunPS->Release();
	if (g_pBrightPS) g_pBrightPS->Release();
	if (g_pBloomPS) g_pBloomPS->Release();
	if (g_pBloomVS) g_pBloomVS->Release();
	if (g_pVoxelVS) g_pVoxelVS->Release();
	if (g_pVoxelGS) g_pVoxelGS->Release();
	if (g_pVoxelPS) g_pVoxelPS->Release();
	if (g_pVertexBuffer_sky) g_pVertexBuffer_sky->Release();
	if (g_pVertexBuffer_screen) g_pVertexBuffer_screen->Release();
	if (ds_on) ds_on->Release();
	if (ds_off) ds_off->Release();
	if (g_BlendState) g_BlendState->Release();
	if (g_pSkyboxTex) g_pSkyboxTex->Release();
	if (normaltex) normaltex->Release();
	if (g_pScreenPS) g_pScreenPS->Release();
	if (g_pSkyPS) g_pSkyPS->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}

///////////////////////////////////
// Keyboard interaction functions
///////////////////////////////////
void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) { }
void OnRBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) { }
void OnChar(HWND hwnd, UINT ch, int cRepeat) { }
void OnLBU(HWND hwnd, int x, int y, UINT keyFlags) { }
void OnRBU(HWND hwnd, int x, int y, UINT keyFlags) { }

///////////////////////////////////
// Mouse interaction functions
///////////////////////////////////
void mHideCursor() 	//hides cursor
{
	while (plane >= 0)
		plane = ShowCursor(FALSE);
}

void mShowCursor() 	//shows it again
{
	while (plane<0)
		plane = ShowCursor(TRUE);
}

void OnMM(HWND hwnd, int x, int y, UINT keyFlags)
{
	static int holdx = x, holdy = y;
	static int reset_cursor = 0;
	RECT rc;
	GetWindowRect(hwnd, &rc); 	//retrieves the window size
	int border = 20;
	rc.bottom -= border;
	rc.right -= border;
	rc.left += border;
	rc.top += border;
	ClipCursor(&rc);

	if ((keyFlags & MK_LBUTTON) == MK_LBUTTON)
	{
	}

	if ((keyFlags & MK_RBUTTON) == MK_RBUTTON)
	{
	}
	if (reset_cursor == 1)
	{
		reset_cursor = 0;
		holdx = x;
		holdy = y;
		return;
	}
	int diffx = holdx - x;
	int diffy = holdy - y;
	float angle_y = (float)diffx / 300.0f;
	float angle_x = (float)diffy / 300.0f;
	cam.rotation.x += angle_x;
	cam.rotation.y += angle_y;

	int midx = (rc.left + rc.right) / 2;
	int midy = (rc.top + rc.bottom) / 2;
	SetCursorPos(midx, midy);
	reset_cursor = 1;

	mHideCursor(); //hide cursor
}

BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
	RECT rc;
	GetWindowRect(hwnd, &rc); 	//retrieves the window size
	int border = 5;
	rc.bottom -= border;
	rc.right -= border;
	rc.left += border;
	rc.top += border;
	ClipCursor(&rc);
	int midx = (rc.left + rc.right) / 2;
	int midy = (rc.top + rc.bottom) / 2;
	SetCursorPos(midx, midy);
	return TRUE;
}

void OnTimer(HWND hwnd, UINT id) { }

//*************************************************************************
void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch (vk)
	{
	case 65:cam.a = 0; //a
		break;
	case 68: cam.d = 0; //d
		break;
	case 32: //space
		break;
	case 87: cam.w = 0; //w
		break;
	case 83:cam.s = 0; //s
		break;
	case 81: cam.q = 0; //w
		break;
	case 69: cam.e = 0; //s
		break;
	default:break;
	}
}

void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{

	switch (vk)
	{
	default:break;
	case 65: cam.a = 1;//a
		break;
	case 68: cam.d = 1;//d
		break;
	case 32: //space
		if (voxeldraw == 0)
			voxeldraw = 1;
		else
			voxeldraw = 0;
		break;
	case 87: cam.w = 1; //w
		break;
	case 83: cam.s = 1; //s
		break;
	case 81: cam.q = 1; //w
		break;
	case 69: cam.e = 1; //s
		break;
	case 27: 
		PostQuitMessage(0);//escape
		break;
	}
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
#include <windowsx.h>
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		HANDLE_MSG(hWnd, WM_LBUTTONDOWN, OnLBD);
		HANDLE_MSG(hWnd, WM_LBUTTONUP, OnLBU);
		HANDLE_MSG(hWnd, WM_MOUSEMOVE, OnMM);
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hWnd, WM_KEYDOWN, OnKeyDown);
		HANDLE_MSG(hWnd, WM_KEYUP, OnKeyUp);
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

//--------------------------------------------------------------------------------------
// noise functions
//--------------------------------------------------------------------------------------
double findnoise2d(double x, double y) {

	int n = (int)x + (int)y * 57;
	n = (n << 13) ^ n;
	int nn = (n*(n*n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
	return 1.0 - ((double)nn / 1073741824.0);
}

double interpolate(double a, double b, double x) {

	double ft = x * 3.1415927;
	double f = (1.0 - cos(ft))* 0.5;
	return a*(1.0 - f) + b*f;
}

double noise(double x, double y) {

	double floorx = (double)((int)x);
	double floory = (double)((int)y);
	double s, t, u, v;

	s = findnoise2d(floorx, floory);
	t = findnoise2d(floorx + 1, floory);
	u = findnoise2d(floorx, floory + 1);//Get the surrounding pixels to calculate the transition.	
	v = findnoise2d(floorx + 1, floory + 1);

	double int1 = interpolate(s, t, x - floorx);//Interpolate between the values.
	double int2 = interpolate(u, v, x - floorx);//Here we use x-floorx, to get 1st dimension. Don't mind the x-floorx thingie, it's part of the cosine formula.
	return interpolate(int1, int2, y - floory);//Here we use y-floory, to get the 2nd dimension.
}

//--------------------------------------------------------------------------------------
// sphere loader
//--------------------------------------------------------------------------------------
bool LoadCatmullClark(LPCTSTR filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count)
{
	struct CatmullVertex
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 tex;
	};
	HANDLE file;
	std::vector<SimpleVertex> data;
	DWORD burn;

	file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	SetFilePointer(file, 80, NULL, FILE_BEGIN);
	ReadFile(file, vertex_count, 4, &burn, NULL);

	for (int i = 0; i < *vertex_count; ++i)
	{
		CatmullVertex vertData;
		ReadFile(file, &vertData, sizeof(CatmullVertex), &burn, NULL);
		SimpleVertex sv;
		sv.Pos = vertData.pos;
		sv.Normal = vertData.normal;
		sv.Tex = vertData.tex;
		data.push_back(sv);
	}

	D3D11_BUFFER_DESC desc = {
		sizeof(SimpleVertex)* *vertex_count,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0, 0,
		sizeof(SimpleVertex)
	};
	D3D11_SUBRESOURCE_DATA subdata = {
		&(data[0]), 0, 0
	};
	HRESULT hr = g_pd3dDevice->CreateBuffer(&desc, &subdata, ppVertexBuffer);

	if (FAILED(hr))
		return false;
	
	return true;
}

XMFLOAT3 mul(XMFLOAT3 vec, XMMATRIX &M)
{
	XMFLOAT3 erg;
	XMFLOAT3 forward = vec;
	XMVECTOR f = XMLoadFloat3(&forward);
	f = XMVector3TransformCoord(f, M);
	XMStoreFloat3(&erg, f);
	return erg;
}

XMFLOAT2 get2dPoint(XMFLOAT3 point3D, XMMATRIX &viewMatrix, XMMATRIX &projectionMatrix, int width = 0, int height = 0)
{
	XMMATRIX viewProjectionMatrix = viewMatrix*projectionMatrix;
	//transform world to clipping coordinates
	point3D = mul(point3D, viewProjectionMatrix);
	return XMFLOAT2(point3D.x, point3D.y);
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
//############################################################################################################
void Render_to_texture(long elapsed)
{
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // red, green, blue, alpha
	ID3D11RenderTargetView*			RenderTarget;
	RenderTarget = RenderToTexture.GetRenderTarget();

	// Clear render target & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTarget, g_pDepthStencilView);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	// Update constant buffer constants
	ConstantBuffer constantbuffer;
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);
	XMMATRIX w = sun.get_matrix(view);
	constantbuffer.SunPos = XMFLOAT4(w._41, w._42, w._43, 1);

	// Sky sphere day and night cycle
	static float f = 0.1;
	f = f + 0.0001f;
	f += elapsed / 10000000.0f;
	constantbuffer.DayTimer.x = cos(f);

	// Render skybox
	XMMATRIX M, vv;
	M = XMMatrixRotationY(0);
	vv = view;
	vv._41 = vv._42 = vv._43 = 0;
	constantbuffer.World = XMMatrixTranspose(M);
	constantbuffer.View = XMMatrixTranspose(vv);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	
	g_pImmediateContext->PSSetShader(g_pSkyPS, NULL, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(2, 1, &g_pSkyboxTex);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);

	g_pImmediateContext->Draw(vertex_count, 0);

	// Render clouds
	M = XMMatrixIdentity();

	ID3D11ShaderResourceView* d3dtexture = Voxel_GI.GetShaderResourceView();
	g_pImmediateContext->GenerateMips(d3dtexture);
	ID3D11ShaderResourceView* octexture = Octree_RW.GetShaderResourceView();

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	
	g_pImmediateContext->PSSetShader(g_pCloudPS, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetShaderResources(3, 1, &d3dtexture);
	g_pImmediateContext->PSSetShaderResources(6, 1, &octexture);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);

	for (int ii = 0; ii < smokeray.size(); ii++)
	{
		M = smokeray[ii]->get_matrix(view);
		constantbuffer.World = XMMatrixTranspose(M);
		constantbuffer.View = XMMatrixTranspose(view);
		constantbuffer.info = XMFLOAT4(smokeray[ii]->transparency, 1, 1, 1);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->Draw(12, 0);
	}

	// Render voxels
	if (voxeldraw)
	{
		constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
		constantbuffer.View = XMMatrixTranspose(view);
		constantbuffer.Projection = XMMatrixTranspose(g_Projection);
		constantbuffer.info.x = 1;
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->VSSetShader(g_pVoxelVS, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->GSSetShader(g_pVoxelGS, NULL, 0);
		g_pImmediateContext->GSSetShaderResources(0, 1, &d3dtexture);
		g_pImmediateContext->GSSetShaderResources(1, 1, &octexture);
		g_pImmediateContext->GSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->GSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->PSSetShader(g_pVoxelPS, NULL, 0);
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->IASetVertexBuffers(0, 1, &voxel.vbuffer, &stride, &offset);
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);

		g_pImmediateContext->Draw(voxel.anz, 0);
	}

	g_pSwapChain->Present(0, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->GSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
}


void Render_to_texture_test(long elapsed)
{
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // red, green, blue, alpha
	ID3D11RenderTargetView*			RenderTarget;
	RenderTarget = RenderToTexture.GetRenderTarget();

	// Clear render target & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTarget, g_pDepthStencilView);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	// Update constant buffer constants
	ConstantBuffer constantbuffer;
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);
	XMMATRIX w = sun.get_matrix(view);
	constantbuffer.SunPos = XMFLOAT4(w._41, w._42, w._43, 1);

	// Sky sphere day and night cycle
	static float f = 0.1;
	f = f + 0.0001f;
	f += elapsed / 10000000.0f;
	constantbuffer.DayTimer.x = cos(f);

	// Render skybox
	XMMATRIX M, vv;
	M = XMMatrixRotationY(0);
	vv = view;
	vv._41 = vv._42 = vv._43 = 0;
	constantbuffer.World = XMMatrixTranspose(M);
	constantbuffer.View = XMMatrixTranspose(vv);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);

	g_pImmediateContext->PSSetShader(g_pSkyPS, NULL, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(2, 1, &g_pSkyboxTex);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);

	g_pImmediateContext->Draw(vertex_count, 0);

	// Render clouds
	M = XMMatrixIdentity();

	ID3D11ShaderResourceView* d3dtexture = Voxel_GI.GetShaderResourceView();
	g_pImmediateContext->GenerateMips(d3dtexture);
	ID3D11ShaderResourceView* octexture = Octree_RW.GetShaderResourceView();

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);

	g_pImmediateContext->PSSetShader(g_pCloudPS, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetShaderResources(3, 1, &d3dtexture);
	g_pImmediateContext->PSSetShaderResources(6, 1, &octexture);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);

	//M = smokeray[ii]->get_matrix(view);
	constantbuffer.World = XMMatrixTranspose(testcloud.get_matrix(view));
	constantbuffer.View = XMMatrixTranspose(view);
	//constantbuffer.info = XMFLOAT4(smokeray[ii]->transparency, 1, 1, 1);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->Draw(6, 0);

	/*constantbuffer.World = XMMatrixTranspose(testcloud2.get_matrix(view));
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	g_pImmediateContext->Draw(12, 0);*/

	// Render voxels
	if (voxeldraw)
	{
		constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
		constantbuffer.View = XMMatrixTranspose(view);
		constantbuffer.Projection = XMMatrixTranspose(g_Projection);
		constantbuffer.info.x = 1;
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->VSSetShader(g_pVoxelVS, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->GSSetShader(g_pVoxelGS, NULL, 0);
		g_pImmediateContext->GSSetShaderResources(0, 1, &d3dtexture);
		g_pImmediateContext->GSSetShaderResources(1, 1, &octexture);
		g_pImmediateContext->GSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->GSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->PSSetShader(g_pVoxelPS, NULL, 0);
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->IASetVertexBuffers(0, 1, &voxel.vbuffer, &stride, &offset);
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);

		g_pImmediateContext->Draw(voxel.anz, 0);
	}

	g_pSwapChain->Present(0, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->GSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
}

/////Light vs. shadow mapping pass
void Render_to_texture2(long elapsed)
{
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // red, green, blue, alpha
	ID3D11RenderTargetView*			RenderTarget;
	RenderTarget = LightSourcePass.GetRenderTarget();

	// Clear render target & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTarget, g_pDepthStencilView);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	// Update constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);
	XMMATRIX w = sun.get_matrix(view);
	constantbuffer.SunPos = XMFLOAT4(w._41, w._42, w._43, 1);

	// Sky sphere day and night cycle
	static float f = 0.1;
	f = f + 0.0001f;
	f += elapsed / 10000000.0f;
	constantbuffer.DayTimer.x = cos(f);

	// Render sun
	XMMATRIX worldmatrix = XMMatrixIdentity();
	sun.rotation += elapsed / 5500000.0f;
	worldmatrix = sun.get_matrix(view);
	constantbuffer.World = XMMatrixTranspose(worldmatrix);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.info.x = 1;
	constantbuffer.info.y = 0;
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->VSSetShader(g_pShadowVS, NULL, 0);
	g_pImmediateContext->VSSetShaderResources(0, 1, &g_pTextureSun);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->PSSetShader(g_pSunPS, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureSun);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);

	g_pImmediateContext->Draw(12, 0);

	// Render clouds
	ID3D11ShaderResourceView* d3dtexture = Voxel_GI.GetShaderResourceView();	
	g_pImmediateContext->GenerateMips(d3dtexture);

	g_pImmediateContext->VSSetShader(g_pShadowVS, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);

	g_pImmediateContext->PSSetShader(g_pShadowPS, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetShaderResources(3, 1, &d3dtexture);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);

	for (int ii = 0; ii < smokeray.size(); ii++)
	{
		ConstantBuffer constantbuffer;
		constantbuffer.info.x = 0;
		constantbuffer.info.y = 1;

		worldmatrix = smokeray[ii]->get_matrix(view);
		constantbuffer.World = XMMatrixTranspose(worldmatrix);
		constantbuffer.View = XMMatrixTranspose(view);
		constantbuffer.Projection = XMMatrixTranspose(g_Projection);
		constantbuffer.info = XMFLOAT4(smokeray[ii]->transparency, 1, 1, 1);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->Draw(12, 0);
	}

	// Render voxels
	if (voxeldraw)
	{
		constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
		constantbuffer.View = XMMatrixTranspose(view);
		constantbuffer.Projection = XMMatrixTranspose(g_Projection);
		constantbuffer.info.x = 1;
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
		
		g_pImmediateContext->VSSetShader(g_pVoxelVS, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);
		
		g_pImmediateContext->GSSetShader(g_pVoxelGS, NULL, 0);
		g_pImmediateContext->GSSetShaderResources(5, 1, &d3dtexture);
		g_pImmediateContext->GSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->GSSetSamplers(0, 1, &g_pSamplerLinear);
		
		g_pImmediateContext->PSSetShader(g_pVoxelPS, NULL, 0);
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		
		g_pImmediateContext->IASetVertexBuffers(0, 1, &voxel.vbuffer, &stride, &offset);
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);

		g_pImmediateContext->Draw(voxel.anz, 0);
	}
	g_pSwapChain->Present(0, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
}

/////////Volumetric lighting approximation pass
void Render_to_texture3(long elapsed)
{
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // red, green, blue, alpha
	ID3D11RenderTargetView*			RenderTarget = VolumetricPass.GetRenderTarget();
	ID3D11ShaderResourceView*		texture = LightSourcePass.GetShaderResourceView();

	// Clear render target & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTarget, g_pDepthStencilView);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	XMMATRIX w = sun.get_matrix(view);
	XMFLOAT3 lpos = XMFLOAT3(w._41, w._42, w._43);
	XMMATRIX ip = g_Projection;
	XMMATRIX iv = view;
	XMFLOAT2 screencam = get2dPoint(lpos, iv, ip);
	screencam.y = -screencam.y;

	// Update constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.World = XMMatrixIdentity();
	constantbuffer.View = view;
	constantbuffer.Projection = g_Projection;
	constantbuffer.info.x = screencam.x;
	constantbuffer.info.y = screencam.y;
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0); 

	g_pImmediateContext->VSSetShader(g_pEffectVS, NULL, 0);
	g_pImmediateContext->VSSetShaderResources(0, 1, &texture);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->PSSetShader(g_pEffectPS, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &texture);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_screen, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);

	g_pImmediateContext->Draw(6, 0);

	// Present our back buffer to our front buffer
	g_pSwapChain->Present(0, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);

}

/////bright pass
void Render_to_texturebright(long elapsed)
{
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // red, green, blue, alpha
	ID3D11RenderTargetView*			RenderTarget;
	RenderTarget = BrightPass.GetRenderTarget();

	ID3D11ShaderResourceView*		texture = RenderToTexture.GetShaderResourceView();

	// Clear render target & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTarget, g_pDepthStencilView);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	XMMATRIX worldmatrix = XMMatrixIdentity();
	XMMATRIX T, S, Rx, Ry;

	// Update constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0); //update constant buffer

	g_pImmediateContext->VSSetShader(g_pBloomVS, NULL, 0); //set vertex shader
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer); //set constant buffer for vertex shader
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->PSSetShader(g_pBrightPS, NULL, 0); //set pixel shader
	g_pImmediateContext->PSSetShaderResources(0, 1, &texture); //set texture resource for pixel shader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear); //set sampler for pixel and vertex buffers

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_screen, &stride, &offset); //set desired model's vertex buffer
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);

	g_pImmediateContext->Draw(6, 0);

	g_pSwapChain->Present(0, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
}

/////////bloom pass
void Render_to_texturebloom(long elapsed)
{
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // red, green, blue, alpha
	ID3D11RenderTargetView*			RenderTarget = BloomPass.GetRenderTarget();
	ID3D11ShaderResourceView*		texture = BrightPass.GetShaderResourceView();
	ID3D11ShaderResourceView*		texture1 = RenderToTexture.GetShaderResourceView();

	// Clear render target & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTarget, g_pDepthStencilView);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	XMFLOAT3 lpos = XMFLOAT3(25, 50, 500);
	XMMATRIX ip = g_Projection;
	XMMATRIX iv = view;
	XMFLOAT2 screencam = get2dPoint(lpos, iv, ip);
	screencam.y = -screencam.y;

	ConstantBuffer constantbuffer;
	constantbuffer.World = XMMatrixIdentity();
	constantbuffer.View = view;
	constantbuffer.Projection = g_Projection;
	constantbuffer.info.x = screencam.x;
	constantbuffer.info.y = screencam.y;
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0); //update constant buffer

	g_pImmediateContext->VSSetShader(g_pBloomVS, NULL, 0);			//set vertex shader
	g_pImmediateContext->VSSetShaderResources(0, 1, &texture);		//set texture resource for vertex shader
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);	//set constant buffer for vertex shader
	g_pImmediateContext->VSSetShaderResources(1, 1, &texture1);		//set texture resource for vertex shader
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);	//set sampler for vertex shader

	g_pImmediateContext->PSSetShader(g_pBloomPS, NULL, 0);			//set pixel shader
	g_pImmediateContext->PSSetShaderResources(0, 1, &texture);
	g_pImmediateContext->PSSetShaderResources(1, 1, &texture1);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_screen, &stride, &offset); //set desired model's vertex buffer
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);

	g_pImmediateContext->Draw(6, 0);

	// Present our back buffer to our front buffer
	g_pSwapChain->Present(0, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
}

/*void MakeClouds()
{
	offsetx = rand() % 50 * noise(rand() % 50, rand() % 50);
	offsety = rand() % 50 * noise(rand() % 50, rand() % 50);
	offsetz = rand() % 50 * noise(rand() % 50, rand() % 50);
	for (int i = 0; i < 500; i++) {
		billboard *new_bill = new billboard;

		new_bill->position.x = rocket_position.x + offsetx + 10 * noise(rand() % 100, rand() % 100);
		new_bill->position.y = rocket_position.y + offsety + noise(rand() % 100, rand() % 100);
		new_bill->position.z = rocket_position.z + offsetz + 10 * noise(rand() % 100, rand() % 100);

		new_bill->scale = 1. + (float)(rand() % 100) / 300.;
		new_bill->rotation = 0;
		smokeray.push_back(new_bill);
	}
}*/

void MakeClouds()
{
	offsetx = rand() % 50 * noise(rand() % 50, rand() % 50);
	offsetz = rand() % 50 * noise(rand() % 50, rand() % 50);
	for (int i = 0; i < 30; i++) {
		billboard *new_bill = new billboard;

		new_bill->position.x = rocket_position.x + offsetx + 2 * noise(rand() % 100, rand() % 100);
		new_bill->position.y = rocket_position.y + noise(rand() % 100, rand() % 100);
		new_bill->position.z = rocket_position.z + offsetz + 2 * noise(rand() % 100, rand() % 100);

		new_bill->scale = 1. + (float)(rand() % 100) / 300.;
		new_bill->rotation = 0;
		smokeray.push_back(new_bill);
	}
}

/*
void Render_to_3dtexture(long elapsed)
{
	ID3D11RenderTargetView*			RenderTarget = RenderToTexture.GetRenderTarget();
	ID3D11UnorderedAccessView *uav[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	uav[0] = Voxel_GI.GetUAV();
	float ClearColorRT[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
	float ClearColorUAV[4] = { 1,1,0,0 }; // red, green, blue, alpha

	// Clear render target, UAV & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColorRT);
	g_pImmediateContext->ClearUnorderedAccessViewFloat(uav[0], ClearColorUAV);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, &RenderTarget, 0, 1, 1, uav, 0);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	// Update constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);

	XMMATRIX worldmatrix;
	worldmatrix = XMMatrixIdentity();

	if (!madeClouds)
	{
		for (int i = 0; i < numberOfClouds; i++)
			MakeClouds();

		madeClouds = true;

	}

	//REORDER THE BILLBOARDS, FURTHEST AWAY FIRST, CLOSEST LAST
	//This can be optimized to be a lot faster, but W/E
	bool swapped;
	billboard* temp;
	do {
		swapped = false;
		for (int i = 1; i < smokeray.size(); i++)
		{
			double dist1 = sqrt((-cam.position.x - smokeray[i - 1]->position.x) * (-cam.position.x - smokeray[i - 1]->position.x)
				+ (-cam.position.y - smokeray[i - 1]->position.y) * (-cam.position.y - smokeray[i - 1]->position.y)
				+ (-cam.position.z - smokeray[i - 1]->position.z) * (-cam.position.z - smokeray[i - 1]->position.z));

			double dist2 = sqrt((-cam.position.x - smokeray[i]->position.x) * (-cam.position.x - smokeray[i]->position.x)
				+ (-cam.position.y - smokeray[i]->position.y) * (-cam.position.y - smokeray[i]->position.y)
				+ (-cam.position.z - smokeray[i]->position.z) * (-cam.position.z - smokeray[i]->position.z));

			if (dist1 < dist2) {
				temp = smokeray[i - 1];
				smokeray[i - 1] = smokeray[i];
				smokeray[i] = temp;
				swapped = true;
			}
		}
	} while (swapped);
	
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->VSSetSamplers(0, 1, &SamplerScreen);

	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);

	for (int ii = 0; ii < smokeray.size(); ii++)
	{
		worldmatrix = smokeray[ii]->get_matrix(view);
		constantbuffer.World = XMMatrixTranspose(worldmatrix);
		constantbuffer.info = XMFLOAT4(smokeray[ii]->transparency, 1, 1, 1);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->Draw(12, 0);
	}

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pSwapChain->Present(0, 0);

	ID3D11RenderTargetView* RT;
	RT = 0;
	uav[0] = 0;
	g_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, &RT, 0, 1, 1, uav, 0);

}*/
//############################################################################################################

void Render_to_3dtexture(long elapsed)
{
	ID3D11RenderTargetView*			RenderTarget = RenderToTexture.GetRenderTarget();
	ID3D11UnorderedAccessView *uav[3] = { NULL, NULL, NULL };
	
	//uav[0] = Voxel_GI.GetUAV();
	uav[0] = Octree_RW.GetUAV();
	uav[1] = VFL.GetUAV();
	uav[2] = const_count.GetUAV();
	float ClearColorRT[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
	float ClearColorUAV[4] = { 0,0,0,0 }; // red, green, blue, alpha
	unsigned int ClearColorUAVint[1] = { 0 }; // red

	// Clear render target, UAV & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColorRT);
	g_pImmediateContext->ClearUnorderedAccessViewFloat(uav[1], ClearColorUAV);
	g_pImmediateContext->ClearUnorderedAccessViewUint(uav[0], ClearColorUAVint);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, &RenderTarget, 0, 1, 2, uav, 0);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);
	//XMMATRIX viewsub;




	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	// Update constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);

	XMMATRIX worldmatrix;
	worldmatrix = XMMatrixIdentity();

	if (!madeClouds) {
		for (int i = 0; i < numberOfClouds; i++)
			MakeClouds();

		madeClouds = true;
	}

	//REORDER THE BILLBOARDS, FURTHEST AWAY FIRST, CLOSEST LAST
	//This can be optimized to be a lot faster, but W/E
	bool swapped;
	billboard* temp;
	do {
		swapped = false;
		for (int i = 1; i < smokeray.size(); i++)
		{
			double dist1 = sqrt((-cam.position.x - smokeray[i - 1]->position.x) * (-cam.position.x - smokeray[i - 1]->position.x)
				+ (-cam.position.y - smokeray[i - 1]->position.y) * (-cam.position.y - smokeray[i - 1]->position.y)
				+ (-cam.position.z - smokeray[i - 1]->position.z) * (-cam.position.z - smokeray[i - 1]->position.z));

			double dist2 = sqrt((-cam.position.x - smokeray[i]->position.x) * (-cam.position.x - smokeray[i]->position.x)
				+ (-cam.position.y - smokeray[i]->position.y) * (-cam.position.y - smokeray[i]->position.y)
				+ (-cam.position.z - smokeray[i]->position.z) * (-cam.position.z - smokeray[i]->position.z));

			if (dist1 < dist2) {
				temp = smokeray[i - 1];
				smokeray[i - 1] = smokeray[i];
				smokeray[i] = temp;
				swapped = true;
			}
		}
	} while (swapped);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
	
	for (int ii = 0; ii < smokeray.size(); ii++)
	{
		worldmatrix = smokeray[ii]->get_matrix(view);

		//viewsub = smokeray[ii]->get_matrix(view);
		constantbuffer.World = XMMatrixTranspose(worldmatrix);
		constantbuffer.info = XMFLOAT4(smokeray[ii]->transparency, 1, 1, 1);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->Draw(12, 0);
	}

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pSwapChain->Present(0, 0);

	ID3D11RenderTargetView* RT;
	RT = 0;
	uav[0] = 0;
	g_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, &RT, 0, 1, 1, uav, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);

}



void Render_to_test(long elapsed)
{
	ID3D11RenderTargetView*			RenderTarget = RenderToTexture.GetRenderTarget();
	ID3D11UnorderedAccessView *uav[2] = { NULL, NULL };

	uav[0] = Octree_RW.GetUAV();
	float ClearColorRT[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
														  //float ClearColorUAV[4] = { 0,0,0,0 }; // red, green, blue, alpha
	unsigned int ClearColorUAVint[1] = { 0 }; // red

											  // Clear render target, UAV & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColorRT);
	g_pImmediateContext->ClearUnorderedAccessViewUint(uav[0], ClearColorUAVint);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, &RenderTarget, 0, 1, 1, uav, 0);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX view = cam.get_matrix(&g_View);
	//XMMATRIX viewsub;

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	// Update constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);

	XMMATRIX worldmatrix;
	worldmatrix = XMMatrixIdentity();

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);

	constantbuffer.World = XMMatrixTranspose(testcloud.get_matrix(view));
	//constantbuffer.info = XMFLOAT4(smokeray[ii]->transparency, 1, 1, 1);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	g_pImmediateContext->Draw(6, 0);

	/*constantbuffer.World = XMMatrixTranspose(testcloud2.get_matrix(view));
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	g_pImmediateContext->Draw(12, 0);*/

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pSwapChain->Present(0, 0);

	ID3D11RenderTargetView* RT;
	RT = 0;
	uav[0] = 0;
	g_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, &RT, 0, 1, 1, uav, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);

}

//############################################################################################################

void Render_to_screen(long elapsed)
{
	float ClearColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f }; // red, green, blue, alpha
	ConstantBuffer constantbuffer;
	XMMATRIX view = cam.get_matrix(&g_View);
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);

	// Clear render target & shaders, set render target & primitive topology
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11ShaderResourceView*           texture = RenderToTexture.GetShaderResourceView();
	ID3D11ShaderResourceView*			texture2 = VolumetricPass.GetShaderResourceView();
	ID3D11ShaderResourceView*           vx = Voxel_GI.GetShaderResourceView();
	ID3D11ShaderResourceView*			texture3 = BloomPass.GetShaderResourceView();

	constantbuffer.World = XMMatrixIdentity();
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->VSSetShader(g_pScreenVS, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->PSSetShader(g_pScreenPS, NULL, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &texture);
	g_pImmediateContext->PSSetShaderResources(3, 1, &vx);
	g_pImmediateContext->PSSetShaderResources(4, 1, &texture2);
	g_pImmediateContext->PSSetShaderResources(5, 1, &texture3);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_screen, &stride, &offset);
	
	g_pImmediateContext->Draw(6, 0);

	g_pSwapChain->Present(0, 0);

	ID3D11ShaderResourceView* srvs[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
	g_pImmediateContext->VSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	g_pImmediateContext->PSSetShaderResources(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
}
//############################################################################################################

unsigned int put_in_octree(XMFLOAT3 pos) {
	unsigned int level = 0;
	unsigned int octdex = 1;
	unsigned int currdex = 0;
	XMFLOAT3 midpt = XMFLOAT3(0, 0, 0);
	float midsize = vxarea / 4.;
	unsigned int idx = 0;
	if (GIarr[0] == 0)
		GIarr[0] = 9;

	while (level != maxlevel) {
		if (pos.x < midpt.x) {
			if (pos.y < midpt.y) {
				if (pos.z < midpt.z) idx = 0;
				else idx = 6;
			}
			else {
				if (pos.z < midpt.z) idx = 2;
				else idx = 4;
			}
		}
		else {
			if (pos.y < midpt.y) {
				if (pos.z < midpt.z) idx = 1;
				else idx = 7;
			}
			else {
				if (pos.z < midpt.z) idx = 3;
				else idx = 5;
			}
		}

		currdex = octdex + idx;
		if (level + 1 != maxlevel) {
			if (GIarr[currdex] > 1)
				octdex = GIarr[currdex];
			else {
				octdex = GIarr[0];
				GIarr[0] += 8;
				GIarr[currdex] = octdex;
			}
		}
		else GIarr[currdex] = 1;

		if (idx % 2 == 0) midpt.x -= midsize;
		else midpt.x += midsize;

		if (idx < 4) midpt.z -= midsize;
		else midpt.z += midsize;

		if (idx < 6 && idx > 1) midpt.y += midsize;
		else midpt.y -= midsize;

		midsize = midsize / 2.;

		level += 1;
	}

	return currdex;
}

/////////////////////////		NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW 
void Reset_CS()
	{
	ID3D11UnorderedAccessView*	ppUAVssNULL[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11ShaderResourceView*	ppSRVsNULL[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	ID3D11Buffer*				ppBuffsNULL[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	UINT initCounts = 0;
	int actualnum_uav = 1;//count of uav's = read/write stuff
	int actualnum_srv = 1;//count of shader ressource view = txtures for input
	int actualnum_buffs = 1;//count of constant buffer = what you get from the C++ side

	if(actualnum_uav)	g_pImmediateContext->CSSetUnorderedAccessViews(0, actualnum_uav, ppUAVssNULL, &initCounts);
	if(actualnum_srv)	g_pImmediateContext->CSSetShaderResources(0, actualnum_srv, ppSRVsNULL);
	if(actualnum_buffs)	g_pImmediateContext->CSSetConstantBuffers(0, actualnum_buffs, ppBuffsNULL);

	actualnum_buffs = actualnum_uav = actualnum_srv = 0;
	g_pImmediateContext->CSSetShader(NULL, NULL, 0);
	}
//------------------------------------------------------- NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW 
/*
void update_constants()
	{
	ID3D11ShaderResourceView* ppSRVNULL[1] = { NULL };
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	float *dataPtr=NULL;
	//const_count
	//	DX11::ConstantBufferDesc::CB_PER_CALL_DEFAULT* dataPtr;
	g_pImmediateContext->Map(const_count.GetTexture1D(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	
	dataPtr = (float*)mappedResource.pData;

	dataPtr[1] = dataPtr[2];		//updates after CS2
	dataPtr[2] = dataPtr[4];		//updates after CS2

	g_pImmediateContext->Unmap(const_count.GetTexture1D(), 0);
	}*/

void run_compute_shader(long elapsed)
	{
	 
	//resetting all from last iteration
	ID3D11ShaderResourceView* pNullSRV = NULL;
	ID3D11UnorderedAccessView* pNullUAV = NULL;
	for(int ii = 0; ii < 8; ii++)
		{
		g_pImmediateContext->CSSetShaderResources(ii, 1, &pNullSRV);
		g_pImmediateContext->CSSetUnorderedAccessViews(ii, 1, &pNullUAV, NULL);
		}

	Reset_CS();

	
	//I do a lot of resetting = set things to NULL, I don't know it this all is necessary!!!!!
	g_pImmediateContext->VSSetShader(NULL, NULL, 0);
	g_pImmediateContext->PSSetShader(NULL, NULL, 0);
	g_pImmediateContext->HSSetShader(NULL, NULL, 0);
	g_pImmediateContext->DSSetShader(NULL, NULL, 0);
	g_pImmediateContext->GSSetShader(NULL, NULL, 0);
	
	g_pImmediateContext->CSSetShader(g_pOctreeCS, NULL, 0);

	//				SET YOUR READ/WRITE STUFF:
	/*
	int initCounts = 0;
	ID3D11UnorderedAccessView *uav = Voxel_list.GetUAV(); //Voxel_list can be a texture class object and set to uav, you know the drill...
	g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, uav, &initCounts);
	ID3D11ShaderResourceView* srv[3];
	srv[0] = .. set here some texture aka input stuff, in this example I set 3 textures, can be more, can be less
	srv[1] = 
	srv[2] = 
	g_pImmediateContext-
	>CSSetShaderResources(0, 3, srv);
	*/
	ConstantBufferCS constantbufferCS;
	constantbufferCS.values = XMFLOAT4(0,0,0,0);
	g_pImmediateContext->UpdateSubresource(g_pCBufferCS, 0, NULL, &constantbufferCS, 0, 0);
	g_pImmediateContext->CSSetConstantBuffers(0, 1, &g_pCBufferCS);

	for (int i = 0; i < maxlevel; i++)			//maxlevel = 8
	{
		constantbufferCS.values = XMFLOAT4((float)i+EPS, 0, 0, 0);
		g_pImmediateContext->UpdateSubresource(g_pCBufferCS, 0, NULL, &constantbufferCS, 0, 0);
		g_pImmediateContext->CSSetConstantBuffers(0, 1, &g_pCBufferCS);

		//run CS(VFL, count[]);
		{
			//flag all the voxel nodes in the list into the Octree_RW array
			//everytime you run: count[1] = previous index, and next index = count[2]
		}
		//run CS2(Octree_RW, count[1,2]);
		//update_constants();		//updates after CS2
								//updates after CS2

	}

	//update your constant buffer
	//note: I would do a new constant buffer from scratch including an int vvariable for the current octree level
	ConstantBuffer constantbuffer;
	XMMATRIX view = cam.get_matrix(&g_View);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);
	constantbuffer.World = XMMatrixIdentity();	
	
	// RUNNNNNNN!!!!!!!!!
	g_pImmediateContext->Dispatch(256, 1, 1);

	// make a second compute shader
	// run both of them in a loop as many times as octree levels
	// ???
	// success

	Reset_CS();
}

void Render()
{
	static StopWatchMicro_ stopwatch;
	long elapsed = stopwatch.elapse_micro();
	stopwatch.start(); //restart

	cam.animation(elapsed);
	Render_to_3dtexture(elapsed);
	//Render_to_test(elapsed);
	Render_to_texture(elapsed);
	//Render_to_texture_test(elapsed);
	Render_to_texture2(elapsed);
	
	stopwatch.start(); //restart
	run_compute_shader(elapsed);	//could be anywhere in the render pipeline //NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW 
	elapsed = stopwatch.elapse_micro();

	Render_to_texturebright(elapsed);	//bright
	Render_to_texturebloom(elapsed);	//bloom
	Render_to_texture3(elapsed);
	Render_to_screen(elapsed);
}