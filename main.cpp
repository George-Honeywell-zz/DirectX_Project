//--------------------------------------------------------------------------------------
// File: Tutorial03.cpp
//
// This application displays a triangle using Direct3D 11
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729720.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"
#include "UserInput.h"
#include <WICTextureLoader.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include "Timer.h"
#include "ErrorLogger.h"
#include <random>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
	XMFLOAT2 Texture;
	XMFLOAT3 Normal;
};

struct ConstantBuffer
{
	XMMATRIX mWorld; //Changes every frame
	XMMATRIX mView; //Never changes
	XMMATRIX mProjection; //Changes on Resize
	XMFLOAT4 LightDir;
	XMFLOAT4 LightColour;
	XMFLOAT4 LightDir2;
	XMFLOAT4 LightColour2;
	XMFLOAT4 OutputColour;
};

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE					g_hInst = nullptr;
HWND						g_hWnd = nullptr;
D3D_DRIVER_TYPE				g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL			g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*				g_pd3dDevice = nullptr;
ID3D11Device1*				g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*		g_pImmediateContext = nullptr;
ID3D11DeviceContext1*		g_pImmediateContext1 = nullptr;
ID3D11RasterizerState*		g_rasterState = nullptr;
ID3D11BlendState*			g_blendState = nullptr;
IDXGISwapChain*				g_pSwapChain = nullptr;
IDXGISwapChain1*			g_pSwapChain1 = nullptr;
ID3D11RenderTargetView*		g_pRenderTargetView = nullptr;
ID3D11VertexShader*			g_pVertexShader = nullptr;
ID3D11PixelShader*			g_pPixelShader = nullptr;
ID3D11PixelShader*			g_pPixelShaderSolid = nullptr;
ID3D11InputLayout*			g_pVertexLayout = nullptr;
ID3D11Buffer*				g_pVertexBuffer = nullptr;
ID3D11Buffer*				g_pIndexBuffer = nullptr;
ID3D11Buffer*				g_pConstantBuffer = nullptr;
ID3D11Buffer*				g_FrameBuffer = nullptr;
XMMATRIX					g_World;
XMMATRIX					g_World2;
XMMATRIX					g_View;
XMMATRIX					g_Projection;
ID3D11DepthStencilView*		g_depthStencilView = nullptr;
ID3D11DepthStencilState*	g_depthStencilState = nullptr;
ID3D11Texture2D*			g_depthBuffer = nullptr;
ID3D11ShaderResourceView*	g_Texture = nullptr;
ID3D11ShaderResourceView*	g_Texture2 = nullptr;
ID3D11SamplerState*			g_TextureSamplerState = nullptr;
ID3D11SamplerState*			g_TextureSamplerState2 = nullptr;
std::unique_ptr<SpriteBatch>spriteBatch;
std::unique_ptr<SpriteFont> spriteFont;

XMMATRIX WVP;
XMMATRIX cube1;
XMMATRIX cube2;
XMMATRIX camView;
XMMATRIX camProjection;

XMVECTOR camPosition;
XMVECTOR camTarget;
XMVECTOR camUp;
XMMATRIX Translation;
Timer fpsTimer;

UserInput input;
Timer timer;

int addHit = 0;
int addMissed = 0;
int difference = 0;

double endTime;
const double globalTime = 30000.0;


enum cubeType { Red, Green, Blue };

class CubeSpawner
{
public:

	XMFLOAT3 vecMin;
	XMFLOAT3 vecMax;
	XMFLOAT3 cubePos{ rand() % 14 + -7 , 10 , 2 };

	void UpdateAABB() {
		//-1 and +1 from the origin of the cube (centre)
		vecMin.x = cubePos.x - 1.0f;
		vecMax.x = cubePos.x + 1.0f;
		vecMin.y = cubePos.y - 1.0f;
		vecMax.y = cubePos.y + 1.0f;
		vecMin.z = cubePos.z - 1.0f;
		vecMax.z = cubePos.z + 1.0f;
	}

	void random()
	{
		cubePos.x = rand() % 14 + -7;
	}

	//Constructor for the cubes.
	//Used to set values BEFORE initialize 
	CubeSpawner()
	{
		cubePos = XMFLOAT3(rand() % 14 + -7 , 10 , 2 );
		UpdateAABB();
	}

	//Set ConstantBuffer for all cubes
	ConstantBuffer enemyBuffer;
};

bool AABBtoAABB(UserInput& box1, CubeSpawner& box2)
{
	//Check the min and max extents of the AABB
	if (!(box1.vecMin.x > box2.vecMax.x ||
		box1.vecMax.x < box2.vecMin.x ||
		box1.vecMax.y < box2.vecMin.y ||
		box1.vecMin.y > box2.vecMax.y ||
		box1.vecMin.z > box2.vecMax.z ||
		box1.vecMax.z < box2.vecMin.z))
	{

		//The DIRECTION of each cube is equal to BOX2 LOCATION minus (-) BOX1 LOCATION
		//Then get BOX1 LOCATION and then add the DIRECTION (NORMALISED) to the specific axis.

		//Get the direction of each cube, and minus the position of each
		XMFLOAT3 Direction = {
		box2.cubePos.x - box1.xPos,
		box2.cubePos.y - box1.yPos,
		box2.cubePos.z - box1.zPos
		};

		XMFLOAT3 Direction1 = {
		box1.xPos - box2.cubePos.x,
		box1.yPos - box2.cubePos.y,
		box1.zPos - box2.cubePos.z
		};

		//When colliding, push the cube away by .001.
		box1.xPos += Direction1.x * .001f;
		box1.yPos += Direction1.y * .001f;
		box1.zPos += Direction1.z * .001f;
		box2.cubePos.x += Direction.x * .001f;
		box2.cubePos.y += Direction.y * .001f;
		box2.cubePos.z += Direction.z * .001f;
		return true;
	}
	return false;
}

//Set a constant INT for number of cubes to be displayed
const int numberOfCubes = 3;
CubeSpawner CUBES[numberOfCubes];

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();
void Update();
void TextManager();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
	srand(time(0));

    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};

	//ErrorLogger::Log(S_OK, "TEST MCMESSAGE");

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{	
			Render();
			Update();
		}
	}
    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{



    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"DX11WindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"DX11WindowClass", L"CT5PRGAP - DirectX 11",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile( szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob );
    if( FAILED(hr) )
    {
        if( pErrorBlob )
        {
            OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
            pErrorBlob->Release();
        }
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	timer.Start();
	fpsTimer.Start();

	//Players Start Position
	input.xPos = 0.0f;
	input.yPos = -4.0f;
	input.zPos = 2.0f;

    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
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
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );

        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                                    D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        }

        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface( __uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice) );
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent( __uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory) );
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface( __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2) );
    if ( dxgiFactory2 )
    {
        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface( __uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1) );
        if (SUCCEEDED(hr))
        {
            (void) g_pImmediateContext->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1) );
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd( g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1 );
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain) );
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd = {};
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

        hr = dxgiFactory->CreateSwapChain( g_pd3dDevice, &sd, &g_pSwapChain );
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation( g_hWnd, DXGI_MWA_NO_ALT_ENTER );

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;
	
	//Describe the Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC dbDesc = {};
	dbDesc.Width = width;
	dbDesc.Height = height;
	dbDesc.MipLevels = 1;
	dbDesc.ArraySize = 2;
	dbDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dbDesc.SampleDesc.Count = 1;
	dbDesc.SampleDesc.Quality = 0;
	dbDesc.Usage = D3D11_USAGE_DEFAULT;
	dbDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dbDesc.CPUAccessFlags = 0;
	dbDesc.MiscFlags = 0;

	//Create Depth View
	hr = g_pd3dDevice->CreateTexture2D(&dbDesc, nullptr, &g_depthBuffer);
	if (FAILED(hr)) {
		MessageBox(NULL, L"Error.", L"Error", MB_OK);
	}
	
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};

	descDSV.Format = dbDesc.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	g_pd3dDevice->CreateDepthStencilView(g_depthBuffer, &descDSV, &g_depthStencilView);
	if (FAILED(hr)) {
		MessageBox(NULL, L"Error.", L"Error", MB_OK);
	}

	//Set BLEND State
	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget->BlendEnable = false;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_depthStencilView ); 

	//RASTERIZER SET
	D3D11_VIEWPORT viewPort;
	viewPort.Width = 800.0f;
	viewPort.Height = 600.0f;
	viewPort.MinDepth = 0;
	viewPort.MaxDepth = 1;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports( 1, &viewPort);
	
	D3D11_RECT rects[1];
	rects[0].left = 0;
	rects[0].right = 800;
	rects[0].top = 0;
	rects[0].bottom = 600;
	g_pImmediateContext->RSSetScissorRects(1, rects);

	//Set Rasterizer State
	D3D11_RASTERIZER_DESC rasterizerState;
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.CullMode = D3D11_CULL_FRONT;
	rasterizerState.FrontCounterClockwise = true;
	rasterizerState.DepthBias = false;
	rasterizerState.DepthBiasClamp = 0;
	rasterizerState.SlopeScaledDepthBias = 0;
	rasterizerState.DepthClipEnable = true;
	rasterizerState.ScissorEnable = false;
	rasterizerState.MultisampleEnable = false;
	rasterizerState.AntialiasedLineEnable = false;
	g_pd3dDevice->CreateRasterizerState(&rasterizerState, &g_rasterState);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to Create Rasterizer State Mode!", L"Error", MB_OK);
		return hr;
	}

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile( L"Tutorial03.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled. Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "INSTANCECOLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    };
	UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
	hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
        return hr;

    // Set the input layout
    //g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile( L"Tutorial03.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

   //  Create vertex buffer
	SimpleVertex vertices[] =
	{			/* Position */			/* Texture Coords */	   /*Normal Coords*/
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f ) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
	};

    D3D11_BUFFER_DESC bd = {};
	ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * ARRAYSIZE(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
	if (FAILED(hr))
	{
		printf("Failed ro Create Vertex Buffer.");
		return hr;
	}

	//Create index buffer
	WORD indices[] =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.ByteWidth = sizeof(WORD) * 36;		
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
	if (FAILED(hr))
		return hr;

	//Set Index buffer
	g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	//Create the constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pConstantBuffer );
	if ( FAILED( hr ))
		return hr;

	//Load texture(s)
	hr = CreateWICTextureFromFile(g_pd3dDevice, L"Data\\Textures\\Texture_Green.jpg", nullptr, &g_Texture, 0);
	if (FAILED(hr)) {
		MessageBox(NULL, L"Unable to load textures!\nCheck the 'data' folder is in the main directory of the .exe file.", L"ERROR", MB_ICONEXCLAMATION);
	}
	
	//CreateWIC enables the support of other picture file formats other than .DDS
	hr = CreateWICTextureFromFile(g_pd3dDevice, L"Data\\Textures\\Texture_Red.jpg", nullptr, &g_Texture2, 0);
	if (FAILED(hr)) {
		MessageBox(NULL, L"Unable to load textures!\nCheck the 'data' folder is in the main directory of the .exe file.", L"ERROR", MB_ICONEXCLAMATION);
	}


	//Create Sampler State
	D3D11_SAMPLER_DESC sampDesc = {};
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_TextureSamplerState);
	if (FAILED(hr))
		return hr;

	// Initialize the world matrix
	g_World = XMMatrixIdentity();

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet( 0.0f, 1.0f, -5.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	g_View = XMMatrixLookAtLH( Eye, At, Up );

	//Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f );
	
	//Setup SpriteFont
	spriteBatch = std::make_unique<SpriteBatch>(g_pImmediateContext);
	spriteFont = std::make_unique<SpriteFont>(g_pd3dDevice, L"Data\\Fonts\\game_font.spritefont");

}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if (g_pImmediateContext) g_pImmediateContext->ClearState();
    if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pPixelShader) g_pPixelShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain1) g_pSwapChain1->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext1) g_pImmediateContext1->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice1) g_pd3dDevice1->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
	if (g_depthBuffer) g_depthBuffer->Release();
	if (g_depthStencilView) g_depthStencilView->Release();
	if (g_Texture) g_Texture->Release();
	if (g_TextureSamplerState) g_TextureSamplerState->Release();
	if (g_FrameBuffer) g_FrameBuffer->Release();
	if (g_rasterState) g_rasterState->Release();
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;
	if (message == WM_KEYDOWN) {
		switch (wParam)
		{
			// First Cube Controls
		case 39:
			input.isMovingRight = true;
			break;
		case 37:
			input.isMovingLeft = true;
			break;
		case 38:
			input.isMovingUp = true;
			break;
		case 40:
			input.isMovingDown = true;
			break;
		}
	}

	if (message == WM_KEYUP) {
		switch (wParam)
		{
			//First Cube Controls
		case 39:
			input.isMovingRight = false;
			break;
		case 37:
			input.isMovingLeft = false;
			break;
		case 38:
			input.isMovingUp = false;
			break;
		case 40:
			input.isMovingDown = false;
			break;
		}
	}

    switch( message )
    {
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;
    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
		break;
    }
    return 0;
}

//Update each Frame
void Update()
{
	double dt = timer.GetMillisecondsElapsed();

	timer.Restart();

	endTime += dt;

	//10,000 = 10 seconds
	//When the game ends, write the 'score' to a .csv file (Comma Separated Values)
	if (endTime > globalTime)
	{
		std::chrono::system_clock::time_point p = std::chrono::system_clock::now();

		std::time_t t = std::chrono::system_clock::to_time_t(p);

		std::ofstream scoreFile;
		scoreFile.open("GameScore.csv");
		scoreFile << "Hit, Missed, Variance, Time Stamp\n";
		scoreFile << addHit << "," << addMissed << "," << difference << "," << std::ctime(&t);
		scoreFile.close();
		DestroyWindow(g_hWnd);
	}

	for(int i = 0; i < numberOfCubes; i++)
	{
		AABBtoAABB(input, CUBES[i]);
		input.UpdatePos();
		CUBES[i].UpdateAABB();
		//If cubes collide, set them back to the start
		if (AABBtoAABB(input, CUBES[i]) == true)
		{
			CUBES[i].random();
			CUBES[i].cubePos.y = 10.0f;
		}
	}

	XMMATRIX mTranslate = XMMatrixTranslation(input.xPos, input.yPos, input.zPos);
	g_World = mTranslate;

	float moveDistance = 0.008f;

	//Cube ONE Controls
	if (input.isMovingRight == true) {
		input.xPos += moveDistance;
	}
	if (input.isMovingLeft == true) {
		input.xPos -= moveDistance;
	}
	if (input.isMovingUp == true) {
		input.yPos += moveDistance;
	}
	if (input.isMovingDown == true) {
		input.yPos -= moveDistance;
	}

	//If the player goes off screen, reset their position
	if (input.xPos >= 8.0f) {
		input.xPos = 0.0f;
		input.yPos = -4.0f;
	}

	if (input.xPos <= -8.0f) {
		input.xPos = 0.0f;
		input.yPos = -4.0f;
	}

	if (input.yPos >= 6.0f) {
		input.xPos = 0.0f;
		input.yPos = -4.0f;
	}

	if (input.yPos <= -6.0f) {
		input.xPos = 0.0f;
		input.yPos = -4.0f;
	}


}


//Manages SpriteBatch / SpriteFont
void TextManager()
{
	//Draw Text
	static int fpsCounter = 0;
	static std::string fpsString = "FPS: 0";
	fpsCounter += 1;
	if (fpsTimer.GetMillisecondsElapsed() > 1000.0)
	{
		fpsString = "FPS: " + std::to_string(fpsCounter);
		fpsCounter = 0;
		fpsTimer.Restart();
	}
	spriteBatch->Begin();
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(fpsString).c_str(), DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::Colors::White);
	spriteFont->DrawString(spriteBatch.get(), L"UP: 843814", XMFLOAT2(0.0f, 15.0f), Colors::White);
	spriteFont->DrawString(spriteBatch.get(), L"Collect as many cubes as possible in 30 seconds!", XMFLOAT2(0.0f, 30.0f), Colors::GhostWhite);





	static std::string Cube1xPos = "X: 0";
	static std::string Cube1yPos = "Y: 0";
	static std::string Cube1zPos = "Z: 0";

	if (input.xPos > 0.0f || input.xPos < 0.0f || input.yPos > 0.0f)
	{
		Cube1xPos = "X: " + std::to_string(input.xPos);
		Cube1yPos = "Y: " + std::to_string(input.yPos);
		Cube1zPos = "Z: " + std::to_string(input.zPos);
	}

	spriteFont->DrawString(spriteBatch.get(), L"CUBE 1 POSITION", XMFLOAT2(650.0f, 0.0f), Colors::GhostWhite);
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(Cube1xPos).c_str(), XMFLOAT2(650.0f, 15.0f), Colors::GhostWhite);
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(Cube1yPos).c_str(), XMFLOAT2(650.0f, 30.0f), Colors::GhostWhite);
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(Cube1zPos).c_str(), XMFLOAT2(650.0f, 45.0f), Colors::GhostWhite);


	/*--- Score Management ---*/

	
	static std::string PlayerScore = "Cubes Collected: 0";
	for (int i = 0; i < numberOfCubes; i++) {
		//Add ONE point if collision is successful
		if (AABBtoAABB(input, CUBES[i]) == true)
		{
			addHit++;
			PlayerScore = "Cubes Collected: " + std::to_string(addHit);
		} 
	}
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(PlayerScore).c_str(), XMFLOAT2(0.0f, 60.0f), Colors::GhostWhite);

	static std::string MissedCubes = "Cubes Missed: 0";
	for (int i = 0; i < numberOfCubes; i++)
	{
		if (CUBES[i].cubePos.y == 10.0f)
		{
			addMissed++;
			MissedCubes = "Cubes Missed: " + std::to_string(addMissed);
		}
	}
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(MissedCubes).c_str(), XMFLOAT2(0.0f, 75.0f), Colors::GhostWhite);

	static std::string Variance = "Variance: 0";
	difference = addHit - addMissed;
	Variance = "Variance: " + std::to_string(difference);
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(Variance).c_str(), XMFLOAT2(0.0f, 90.0f), Colors::GhostWhite);

	static std::string tLeft = "Time Left: 30";
	int timeLeft = (globalTime - endTime) / 1000;
	tLeft = "Time Left: " + std::to_string(timeLeft);
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(tLeft).c_str(), XMFLOAT2(0.0f, 45.0f), Colors::Red);


	spriteBatch->End();
}


//Render a frame
void Render() {
	
	float bgColour[] = { 0.0f, 0.1f, 0.5f, 0.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, bgColour);
	g_pImmediateContext->ClearDepthStencilView(g_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_depthStencilView);
	D3D11_VIEWPORT viewPort;
	viewPort.Width = 800.0f;
	viewPort.Height = 600.0f;
	viewPort.MinDepth = 0;
	viewPort.MaxDepth = 1;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &viewPort);

	//*--- Lighting Parameters --- */

	//Position / Direction of the light
	XMFLOAT4 LightDirs =
	{
		//Light 1 Position/Direction
		XMFLOAT4(0.0f, -4.0f, 2.0f, 5.0f),
	};

	//Colours of the light
	XMFLOAT4 LightColours =
	{
		//Light 1 Colour
		XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f),
	};


	ConstantBuffer cb1;
	cb1.mWorld = XMMatrixTranspose(g_World);
	cb1.mView = XMMatrixTranspose(g_View);
	cb1.mProjection = XMMatrixTranspose(g_Projection);
	cb1.LightDir = LightDirs;
	cb1.LightColour = LightColours;
	cb1.OutputColour = XMFLOAT4(0, 0, 0, 0);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb1, 0, 0);



	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pImmediateContext->RSSetState(g_rasterState);
	g_pImmediateContext->OMSetDepthStencilState(g_depthStencilState, 0);
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_Texture);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_TextureSamplerState);
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	//Cube ONE
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_pImmediateContext->DrawIndexed(36, 0, 0);


	for (int i = 0; i < numberOfCubes; i++)
	{
		CUBES[i].enemyBuffer.mWorld= XMMatrixTranspose(XMMatrixTranslation(CUBES[i].cubePos.x, CUBES[i].cubePos.y, CUBES[i].cubePos.z));
		CUBES[i].enemyBuffer.mView = XMMatrixTranspose(g_View);
		CUBES[i].enemyBuffer.mProjection = XMMatrixTranspose(g_Projection);
		CUBES[i].enemyBuffer.LightDir = LightDirs;
		CUBES[i].enemyBuffer.LightColour = LightColours;
		CUBES[i].enemyBuffer.OutputColour = XMFLOAT4(0, 0, 0, 0);
		g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &CUBES[i].enemyBuffer, 0, 0);

		g_pImmediateContext->PSSetShaderResources(0, 1, &g_Texture2);
		g_pImmediateContext->DrawIndexed(36, 0, 0);
		
		//Move Cubes down by the chosen amount
		CUBES[i].cubePos.y += -0.004f;

		//When cubes hit -10 on Y, reset them to 10 on Y & randomize their X position
		if (CUBES[i].cubePos.y < -10.0f) {
			CUBES[i].cubePos.y = 10.0f;
			CUBES[i].random();
		}
		
	}

	//Draw Text
	TextManager();

	//Present our back buffer to the front buffer
	g_pSwapChain->Present(0, NULL);

}

