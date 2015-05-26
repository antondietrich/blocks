#include "renderer.h"

namespace Blocks
{

using namespace DirectX;

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
//	D3D11_INPUT_ELEMENT_DESC layout[] =
//	{
//	    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
//	};
//	UINT numElements = ARRAYSIZE(layout);
//	hr = device->CreateInputLayout( layout, numElements, vShaderBytecode->GetBufferPointer(), vShaderBytecode->GetBufferSize(), &inputLayout_ );
//	if( FAILED( hr ) )
//	{
//		OutputDebugStringA( "Input layout creation failed!" );
//		RELEASE( vShaderBytecode );
//		RELEASE( inputLayout_ );
//		return false;
//	}
	hr = CreateInputLayoutFromShaderBytecode( vShaderBytecode, device, &inputLayout_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create input layout from shader bytecode!" );
		if( vShaderBytecode )
			vShaderBytecode->Release();
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
// Debug overlay
//********************************
Overlay::Overlay()
:
shader_()
{
	renderer_ = 0;
	vb_ = 0;
}

Overlay::~Overlay()
{
	RELEASE( vb_ );
}

bool Overlay::Start( Renderer *renderer )
{
	HRESULT hr;

	renderer_ = renderer;
	if( !shader_.Load( L"assets/shaders/overlay.fx", renderer_->device_ ) )
		return false;

	D3D11_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof( desc ) );
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = sizeof( OverlayVertex ) * MAX_OVERLAY_CHARS;

	hr = renderer_->device_->CreateBuffer( &desc, NULL, &vb_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create overlay vertex buffer!" );
	}

	return true;
}

void Overlay::Print( const char* text )
{
	DisplayText( 10, 10, text, XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) );
}

void Overlay::DisplayText( int x, int y, const char* text, DirectX::XMFLOAT4 color )
{
	// cut text if too long
	int textLength = (int)strlen( text );
	if( textLength > MAX_OVERLAY_CHARS ) {
		OutputDebugStringA( "Overlay text too long, clamping!\n" );
		textLength = MAX_OVERLAY_CHARS;
	}

	char shortText[ MAX_OVERLAY_CHARS + 1 ];
	strncpy ( shortText, text, textLength );
	shortText[ textLength ] = '\0';

	OverlayVertex vertices[ MAX_OVERLAY_CHARS ];
	for( int i = 0; i < textLength; i++ )
	{
		char letterTemp[2];
		letterTemp[0] = shortText[i];
		letterTemp[1] = '\0';
		OutputDebugStringA( letterTemp );
	}
	OutputDebugStringA( "\n" );
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

HRESULT CreateInputLayoutFromShaderBytecode( ID3DBlob* shaderBytecode, ID3D11Device* device, ID3D11InputLayout** inputLayout )
{
	HRESULT hr;
	// Reflect shader info
	ID3D11ShaderReflection* vShaderReflection = NULL;
	if( FAILED( hr = D3DReflect( shaderBytecode->GetBufferPointer(), shaderBytecode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&vShaderReflection ) ) )
	{
		return hr;
	}

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	vShaderReflection->GetDesc( &shaderDesc );

	// Read input layout description from shader info
	unsigned int numInputParameters = shaderDesc.InputParameters;
	D3D11_INPUT_ELEMENT_DESC *inputLayoutDesc = new D3D11_INPUT_ELEMENT_DESC[ numInputParameters ];
	for( unsigned int i = 0; i < numInputParameters; ++i )
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		vShaderReflection->GetInputParameterDesc( i, &paramDesc );

		// fill out input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if( paramDesc.Mask == 1 )
		{
			if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if( paramDesc.Mask <= 3 )
		{
			if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if( paramDesc.Mask <= 7 )
		{
			if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if( paramDesc.Mask <= 15 )
		{
			if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		//save element desc
		inputLayoutDesc[ i ] = elementDesc;
	}

	// Try to create Input Layout
	hr = device->CreateInputLayout( inputLayoutDesc, numInputParameters, shaderBytecode->GetBufferPointer(),
									shaderBytecode->GetBufferSize(), inputLayout );

	//Free allocation shader reflection memory
	delete[] inputLayoutDesc;
	vShaderReflection->Release();

	return hr;
}

}