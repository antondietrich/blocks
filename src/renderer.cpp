#include "renderer.h"

namespace Blocks
{

extern ConfigType Config;

bool Renderer::isInstantiated_ = false;

Renderer::Renderer()
:
colorShader()
{
	assert( !isInstantiated_ );
	isInstantiated_ = true;
	vsync_ = 0;

	device_ = 0;
	context_ = 0;
	swapChain_ = 0;
	backBufferView_ = 0;
	triangleVB_ = 0;
}

Renderer::~Renderer()
{
	swapChain_->SetFullscreenState( FALSE, 0 );

#ifdef _DEBUG_
	ID3D11Debug* DebugDevice = nullptr;
	device_->QueryInterface(__uuidof(ID3D11Debug), (void**)(&DebugDevice));
#endif

	RELEASE( triangleVB_ );
	RELEASE( defaultRasterizerState_ );
	RELEASE( backBufferView_ );
	RELEASE( swapChain_ );
	RELEASE( context_ );
	RELEASE( device_ );

#ifdef _DEBUG_
	DebugDevice->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
	RELEASE(DebugDevice);
#endif
	
}

bool Renderer::Start( HWND wnd )
{
	HRESULT hr;

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_SOFTWARE
	};
	unsigned int totalDriverTypes = ARRAYSIZE( driverTypes );

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	unsigned int totalFeatureLevels = ARRAYSIZE( featureLevels );

	// Device and context
	unsigned int deviceFlags = 0;
	#ifdef _DEBUG_
		deviceFlags = D3D11_CREATE_DEVICE_DEBUG;
	#endif

	unsigned int driverType = 0;
	for( driverType = 0; driverType < totalDriverTypes; ++driverType )
	{
		hr = D3D11CreateDevice( NULL, driverTypes[driverType], NULL, deviceFlags, featureLevels, 
								totalFeatureLevels, D3D11_SDK_VERSION, &device_, NULL, &context_ );
		if( SUCCEEDED( hr ) )
		{
			break;
		}
	}
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create DirectX device!" );
		return false;
	}
	
	// Swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory( &swapChainDesc, sizeof( swapChainDesc ) );
	swapChainDesc.BufferDesc.Width = Config.screenWidth;
	swapChainDesc.BufferDesc.Height = Config.screenHeight;
	// TODO: get these values from the device
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = BACK_BUFFER_FORMAT;

	if( Config.multisampling == 2 || Config.multisampling == 4 || Config.multisampling == 8 || Config.multisampling == 16 )
	{
		swapChainDesc.SampleDesc.Count = (UINT)Config.multisampling;
		unsigned int quality;
		device_->CheckMultisampleQualityLevels( swapChainDesc.BufferDesc.Format, swapChainDesc.SampleDesc.Count, &quality );
		if( quality > 0 )
		{
			// NOTE: using the highest available quality for now. Add quality setting?
			swapChainDesc.SampleDesc.Quality = quality - 1;
		}
		else
		{
			OutputDebugStringA( "Multisampling level not supported, disabling!" );
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
		}
	}
	else
	{
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
	}

	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = wnd;
	swapChainDesc.Windowed = TRUE;
	// TODO: look into this more https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	vsync_ = Config.vsync;

	// Retrieve the Factory from the device
	IDXGIDevice *dxgiDevice = 0;
	hr = device_->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to retrieve IDXGIDevice!" );
		return false;
	}
	      
	IDXGIAdapter *dxgiAdapter = 0;
	hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&dxgiAdapter);
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to retrieve IDXGIAdapter!" );
		return false;
	}

	IDXGIFactory *dxgiFactory = 0;
	hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&dxgiFactory);
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to retrieve DXGIFactory!" );
		return false;
	}

	dxgiFactory->CreateSwapChain( device_, &swapChainDesc, &swapChain_ );

	// Handle Alt-Enter ourselves
	hr = dxgiFactory->MakeWindowAssociation( wnd, DXGI_MWA_NO_ALT_ENTER );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "MakeWindowAssociation call failed!" );
		return false;
	}

	RELEASE( dxgiFactory );
	RELEASE( dxgiAdapter );
	RELEASE( dxgiDevice );
	
	// NOTE: as a side-effect, sets the back buffer (consider making a separate function)
	ResizeBuffers();

	if( Config.fullscreen ) {
		ToggleFullscreen();
	}

	// rasterizer state
	D3D11_RASTERIZER_DESC rasterizerStateDesc;
	ZeroMemory( &rasterizerStateDesc, sizeof( rasterizerStateDesc ) );
	rasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerStateDesc.CullMode = D3D11_CULL_BACK;
	rasterizerStateDesc.FrontCounterClockwise = TRUE;
	rasterizerStateDesc.DepthBias = 0;
	rasterizerStateDesc.DepthBiasClamp = 0.0f;
	rasterizerStateDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerStateDesc.DepthClipEnable = TRUE;
	rasterizerStateDesc.ScissorEnable = FALSE;
	rasterizerStateDesc.MultisampleEnable = FALSE;
	if( Config.multisampling == 2 || Config.multisampling == 4 || Config.multisampling == 8 || Config.multisampling == 16 ) {
		rasterizerStateDesc.MultisampleEnable = TRUE;
	}
	rasterizerStateDesc.AntialiasedLineEnable = FALSE;

	hr =  device_->CreateRasterizerState( &rasterizerStateDesc, &defaultRasterizerState_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create default rasterizer state!" );
		return false;
	}

	// context_->RSSetState( defaultRasterizerState_ );

	// viewport
	screenViewport_.Width = static_cast<float>( Config.screenWidth );
	screenViewport_.Height = static_cast<float>( Config.screenHeight );
	screenViewport_.MinDepth = 0.0f;
	screenViewport_.MaxDepth = 1.0f;
	screenViewport_.TopLeftX = 0.0f;
	screenViewport_.TopLeftY = 0.0f;

	context_->RSSetViewports( 1, &screenViewport_ );

	// Temp triangle
	const int numVertices = 3;
	VertexPosition vertices[] = {
		{ 0.0f, 0.5f, 0.5f },
		{ 0.5f, -0.5f, 0.5f },
		{ -0.5f, -0.5f, 0.5f },
	};

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory( &vertexBufferDesc, sizeof( vertexBufferDesc ) );
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof( VertexPosition ) * numVertices;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory( &resourceData, sizeof( resourceData ) );
	resourceData.pSysMem = vertices;

	hr = device_->CreateBuffer( &vertexBufferDesc, &resourceData, &triangleVB_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create vertex buffer!" );
		return false;
	}

	unsigned int stride = sizeof( VertexPosition );
	unsigned int offset = 0;

	context_->IASetVertexBuffers( 0, 1, &triangleVB_, &stride, &offset );

	if( !colorShader.Load( L"assets/shaders/color.fx", device_ ) )
	{
		OutputDebugStringA( "Failed to load shaders! " );
		return false;
	}
	SetShader( colorShader );

	context_->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	return true;
}

void Renderer::Present()
{
	float clearColor[ 4 ] = { 0.53f, 0.81f, 0.98f, 1.0f };
	context_->ClearRenderTargetView( backBufferView_, clearColor );
	context_->Draw( 3, 0 );
	swapChain_->Present( vsync_, 0 );
}

bool Renderer::Fullscreen()
{
	BOOL fs;
	swapChain_->GetFullscreenState( &fs, NULL );
	return 0 != fs; // avoid warning C4800
}

void Renderer::ToggleFullscreen()
{
	if( !Fullscreen() ) {
		swapChain_->SetFullscreenState( TRUE, 0 );
	}
	else {
		swapChain_->SetFullscreenState( FALSE, 0 );
	}
	ResizeBuffers();
}

bool Renderer::ResizeBuffers()
{
	HRESULT hr;

	context_->OMSetRenderTargets( 0, NULL, NULL );

	OutputDebugStringA( "Window size changed: resizing buffers\n" );

	RELEASE( backBufferView_ );

	// Let DXGI figure out buffer size and format
	swapChain_->ResizeBuffers( 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0 );

	ID3D11Texture2D* backBufferTexture = 0;
	hr = swapChain_->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&backBufferTexture );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to get the swap chain buffer!" );
		return false;
	}

	hr = device_->CreateRenderTargetView( backBufferTexture, 0, &backBufferView_ );

	RELEASE( backBufferTexture );

	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create the render target view!" );
		RELEASE( backBufferView_ );
		return false;
	}

	DXGI_SWAP_CHAIN_DESC desc;
	swapChain_->GetDesc( &desc );

	context_->OMSetRenderTargets( 1, &backBufferView_, NULL );

//	screenViewport_.Width = (float)width;
//	screenViewport_.Height = (float)height;
	screenViewport_.Width = (float)desc.BufferDesc.Width;
	screenViewport_.Height = (float)desc.BufferDesc.Height;
	screenViewport_.MinDepth = 0.0f;
	screenViewport_.MaxDepth = 1.0f;
	screenViewport_.TopLeftX = 0;
	screenViewport_.TopLeftY = 0;
	context_->RSSetViewports( 1, &screenViewport_ );

	return true;
}

void Renderer::SetShader( const Shader& shader )
{
	context_->IASetInputLayout( shader.inputLayout_ );
	context_->VSSetShader( shader.vertexShader_, NULL, 0 );
	context_->PSSetShader( shader.pixelShader_, NULL, 0 );
}


//******************
// Shader
//******************

Shader::Shader()
{
	vertexShader_ = 0;
	pixelShader_ = 0;
	inputLayout_ = 0;
}

Shader::~Shader()
{
	RELEASE( vertexShader_ );
	RELEASE( pixelShader_ );
	RELEASE( inputLayout_ );
}

bool Shader::Load( wchar_t* filename, ID3D11Device *device )
{
	HRESULT hr;
	ID3DBlob *vShaderBytecode = 0;

	// Vertex shader
	if( !LoadShader( filename, VERTEX_SHADER_ENTRY, "vs_4_0", &vShaderBytecode ) )
	{
		OutputDebugStringA( "Shader compiltaion failed!" );
		RELEASE( vShaderBytecode );
		return false;
	}

	hr = device->CreateVertexShader( vShaderBytecode->GetBufferPointer(), vShaderBytecode->GetBufferSize(), NULL, &vertexShader_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Shader creation failed!" );
		RELEASE( vertexShader_ );
		RELEASE( vShaderBytecode );
		return false;
	}

	// Input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
	    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
	};
	UINT numElements = ARRAYSIZE(layout);

	hr = device->CreateInputLayout( layout, numElements, vShaderBytecode->GetBufferPointer(), vShaderBytecode->GetBufferSize(), &inputLayout_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Input layout creation failed!" );
		RELEASE( vShaderBytecode );
		RELEASE( inputLayout_ );
		return false;
	}
	RELEASE( vShaderBytecode );

	ID3DBlob *pShaderBytecode = 0;
	if( !LoadShader( filename, PIXEL_SHADER_ENTRY, "ps_4_0", &pShaderBytecode ) )
	{
		OutputDebugStringA( "Shader compiltaion failed!" );
		if( pShaderBytecode )
			pShaderBytecode->Release();
		return false;
	}

	hr = device->CreatePixelShader( pShaderBytecode->GetBufferPointer(), pShaderBytecode->GetBufferSize(), NULL, &pixelShader_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Shader creation failed!" );
		RELEASE( pixelShader_ );
		RELEASE( pShaderBytecode );
		return false;
	}
	RELEASE( pShaderBytecode );
	return true;
}


//********************************
// Function definitions
//********************************
bool LoadShader( wchar_t *filename, const char *entry, const char *shaderModel, ID3DBlob **buffer )
{
	HRESULT hr;
	ID3DBlob *errorBuffer = 0;

	hr = D3DCompileFromFile( filename, 0, 0, entry, shaderModel, 0, 0, buffer, &errorBuffer );
	if( FAILED( hr ) )
	{
		if( errorBuffer )
		{
			OutputDebugStringA( (char*)errorBuffer->GetBufferPointer() );
			errorBuffer->Release();
		}
		return false;
	}

	if( errorBuffer )
		errorBuffer->Release();

	return true;
}

}