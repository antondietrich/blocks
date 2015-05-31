#include "renderer.h"

namespace Blocks
{

using namespace DirectX;

extern ConfigType Config;

bool Renderer::isInstantiated_ = false;

Renderer::Renderer()
//:
//colorShader()
{
	assert( !isInstantiated_ );
	isInstantiated_ = true;
	vsync_ = 0;

	device_ = 0;
	context_ = 0;
	swapChain_ = 0;
	backBufferView_ = 0;
	defaultRasterizerState_ = 0;
	for( int i = 0; i < NUM_BLEND_MODES; i++ ) {
		blendStates_[i] = 0;
	}
	globalConstantBuffer_ = 0;
	frameConstantBuffer_ = 0;
	modelConstantBuffer_ = 0;
	linearSampler_ = 0;

	for( int i = 0; i < MAX_SHADERS; i++ )
	{
		shaders_[i] = Shader();
	}

	for( int i = 0; i < MAX_TEXTURES; i++ )
	{
		textures_[i] = Texture();
	}

	for( int i = 0; i < MAX_MESHES; i++ )
	{
		meshes_[i] = Mesh();
	}

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
	RELEASE( linearSampler_ );
	RELEASE( modelConstantBuffer_ );
	RELEASE( frameConstantBuffer_ );
	RELEASE( globalConstantBuffer_ );
	for( int i = 0; i < NUM_BLEND_MODES; i++ ) {
		RELEASE( blendStates_[i] );
	}
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

	// rasterizer states
	D3D11_RASTERIZER_DESC rasterizerStateDesc;
	ZeroMemory( &rasterizerStateDesc, sizeof( rasterizerStateDesc ) );
	rasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
	// rasterizerStateDesc.FillMode = D3D11_FILL_WIREFRAME;
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

	context_->RSSetState( defaultRasterizerState_ );

	// blend modes
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory( &blendDesc, sizeof( blendDesc ) );
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device_->CreateBlendState( &blendDesc, &blendStates_[ BM_DEFAULT ] );

	// float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT sampleMask   = 0xffffffff;
	context_->OMSetBlendState( blendStates_[ BM_DEFAULT], NULL, sampleMask );

	ZeroMemory( &blendDesc, sizeof( blendDesc ) );
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device_->CreateBlendState( &blendDesc, &blendStates_[ BM_ALPHA ] );

	// viewport
	screenViewport_.Width = static_cast<float>( Config.screenWidth );
	screenViewport_.Height = static_cast<float>( Config.screenHeight );
	screenViewport_.MinDepth = 0.0f;
	screenViewport_.MaxDepth = 1.0f;
	screenViewport_.TopLeftX = 0.0f;
	screenViewport_.TopLeftY = 0.0f;

	context_->RSSetViewports( 1, &screenViewport_ );

	// global constant buffer
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory( &cbDesc, sizeof( cbDesc ) );
	cbDesc.ByteWidth = sizeof( GlobalCB );
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	GlobalCB cbData;
	cbData.screenToNDC = XMFLOAT4X4(
		2.0f / screenViewport_.Width,	 0.0f,							0.0f,	-1.0f,
		0.0f,							-2.0f / screenViewport_.Height,	0.0f,	 1.0f,
		0.0f,							 0.0f,							1.0f,	 0.0f,
		0.0f,							 0.0f,							0.0f,	 1.0f
	);

	D3D11_SUBRESOURCE_DATA cbInitData;
	cbInitData.pSysMem = &cbData;
	cbInitData.SysMemPitch = sizeof( GlobalCB );
	cbInitData.SysMemSlicePitch = 0;

	hr = device_->CreateBuffer( &cbDesc, &cbInitData, &globalConstantBuffer_ );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create overlay constant buffer!" );
		return false;
	}

	context_->VSSetConstantBuffers( 0, 1, &globalConstantBuffer_ );

	// frame constant buffer
	ZeroMemory( &cbDesc, sizeof( cbDesc ) );
	cbDesc.ByteWidth = sizeof( FrameCB );
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	FrameCB frameCBData;

	XMVECTOR eye = XMVectorSet( 0.0f, 14.0f, 0.0f, 0.0f );
	XMVECTOR direction = XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f );
	XMVECTOR up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	XMMATRIX view =  XMMatrixLookToLH( eye, direction, up );
	XMMATRIX projection = XMMatrixPerspectiveFovLH( XMConvertToRadians( 60.0f ), screenViewport_.Width / screenViewport_.Height, 1.0f, 1000.0f );
	// XMStoreFloat4x4( &frameCBData.vp, XMMatrixMultiply( view, projection ) );
	XMStoreFloat4x4( &frameCBData.vp, XMMatrixTranspose( XMMatrixMultiply( view, projection ) ) );

	D3D11_SUBRESOURCE_DATA frameCBInitData;
	frameCBInitData.pSysMem = &frameCBData;
	frameCBInitData.SysMemPitch = sizeof( FrameCB );
	frameCBInitData.SysMemSlicePitch = 0;

	hr = device_->CreateBuffer( &cbDesc, &frameCBInitData, &frameConstantBuffer_ );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create overlay constant buffer!" );
		return false;
	}

	context_->VSSetConstantBuffers( 1, 1, &frameConstantBuffer_ );

	// model constant buffer
	ZeroMemory( &cbDesc, sizeof( cbDesc ) );
	cbDesc.ByteWidth = sizeof( ModelCB );
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = device_->CreateBuffer( &cbDesc, NULL, &modelConstantBuffer_ );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create overlay constant buffer!" );
		return false;
	}

	context_->VSSetConstantBuffers( 2, 1, &modelConstantBuffer_ );

	// Load textures
	if( !textures_[0].Load( L"assets/textures/dirt.dds", device_ ) ) {
		OutputDebugStringA( "Failed to load texture!" );
		return false;
	}
	if( !textures_[1].Load( L"assets/textures/grass.dds", device_ ) ) {
		OutputDebugStringA( "Failed to load texture!" );
		return false;
	}

	// Load shaders
	if( !shaders_[0].Load( L"assets/shaders/global.fx", device_ ) ) {
		OutputDebugStringA( "Failed to load global shader!" );
		return false;
	}

	//	if( !meshes_[0].Load( vertices, numVertices, device_ ) )
//	{
//		OutputDebugStringA( "Failed to load mesh!" );
//		return false;
//	}
	int numVertices = 36;
	VertexPosNormalTexcoord cubeVertices[] = 
	{
		// face 1 / -Z
		{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
		{ { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },
		{ {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
		{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
		{ {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
		{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },
		// face 2 / +X
		{ {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
		{ {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
		{ {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
		{ {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
		{ {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
		{ {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
		// face 3 / +Z
		{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
		{ {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },
		{ { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
		{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
		{ { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
		{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },
		// face 4 / -X
		{ { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
		{ { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
		{ { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
		{ { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
		{ { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
		// face 5 / +Y
		{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
		{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },
		{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
		{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
		{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
		{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },
		// face 6 / -Y
		{ { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
		{ { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },
		{ {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
		{ {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
		{ {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },
	};

	if( !meshes_[0].Load( cubeVertices, numVertices, device_ ) ) {
		OutputDebugStringA( "Failed to load cube vertices!" );
		return false;
	}

	// create texture sampler
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory( &samplerDesc, sizeof( samplerDesc ) );
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;

	hr = device_->CreateSamplerState( &samplerDesc, &linearSampler_ );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create sampler state!" );
		return false;
	}

	// Temp triangle
	numVertices = 3;
	VertexPosition vertices[] = {
		{ 0.0f, 0.5f, 0.5f },
		{ -0.5f, -0.5f, 0.5f },
		{ 0.5f, -0.5f, 0.5f },
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

	

//	if( !colorShader.Load( L"assets/shaders/color.fx", device_ ) )
//	{
//		OutputDebugStringA( "Failed to load shaders! " );
//		return false;
//	}
	

	context_->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	return true;
}

void Renderer::Begin()
{
	float clearColor[ 4 ] = { 0.53f, 0.81f, 0.98f, 1.0f };
	context_->ClearRenderTargetView( backBufferView_, clearColor );
}

void Renderer::DrawCube( XMFLOAT3 offset )
{
	ModelCB modelCBData;
	modelCBData.translate = XMFLOAT4( offset.x, offset.y, offset.z, 0.0f );
	context_->UpdateSubresource( modelConstantBuffer_, 0, NULL, &modelCBData, sizeof( ModelCB ), 0 );

	context_->VSSetConstantBuffers( 1, 1, &frameConstantBuffer_ );
	context_->VSSetConstantBuffers( 2, 1, &modelConstantBuffer_ );


	SetMesh( meshes_[0] );

	context_->PSSetSamplers( 0, 1, &linearSampler_ );
	SetTexture( textures_[0] );

	SetShader( shaders_[0] );
	context_->Draw( 36, 0 );
}

void Renderer::End()
{
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

	screenViewport_.Width = (float)desc.BufferDesc.Width;
	screenViewport_.Height = (float)desc.BufferDesc.Height;
	screenViewport_.MinDepth = 0.0f;
	screenViewport_.MaxDepth = 1.0f;
	screenViewport_.TopLeftX = 0;
	screenViewport_.TopLeftY = 0;
	context_->RSSetViewports( 1, &screenViewport_ );

	// update the screen to NDC matrix, since the view dimensions have changed
	GlobalCB cbData;
	cbData.screenToNDC = XMFLOAT4X4(
		2.0f / screenViewport_.Width,	 0.0f,							0.0f,	-1.0f,
		0.0f,							-2.0f / screenViewport_.Height,	0.0f,	 1.0f,
		0.0f,							 0.0f,							1.0f,	 0.0f,
		0.0f,							 0.0f,							0.0f,	 1.0f
	);

	if( globalConstantBuffer_ ) {
		context_->UpdateSubresource( globalConstantBuffer_, 0, NULL, &cbData, sizeof( GlobalCB ), 0 );
	}

	return true;
}

void Renderer::SetBlendMode( BLEND_MODE bm )
{
	UINT sampleMask   = 0xffffffff;
	context_->OMSetBlendState( blendStates_[ bm ], NULL, sampleMask );
}

void Renderer::SetMesh( const Mesh& mesh )
{
	unsigned int stride = sizeof( VertexPosNormalTexcoord );
	unsigned int offset = 0;
	context_->IASetVertexBuffers( 0, 1, &mesh.vertexBuffer_, &stride, &offset );
}

void Renderer::SetShader( const Shader& shader )
{
	context_->IASetInputLayout( shader.inputLayout_ );
	context_->VSSetShader( shader.vertexShader_, NULL, 0 );
	context_->PSSetShader( shader.pixelShader_, NULL, 0 );
}

void Renderer::SetTexture( const Texture& texture )
{
	context_->PSSetShaderResources( 0, 1, &texture.textureView_ );
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

	// input layout	
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
// Texture
//********************************
Texture::Texture()
{
	textureView_ = 0;
}

Texture::~Texture()
{
	RELEASE( textureView_ );
}

bool Texture::Load( wchar_t* filename, ID3D11Device *device )
{
	HRESULT hr = CreateDDSTextureFromFile( device,
		                                   filename,
		                                   NULL,
		                                   &textureView_
		                                 );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to load texture!" );
		return false;
	}

	return true;
}

//********************************
// Mesh
//********************************
Mesh::Mesh()
{
	vertexBuffer_ = 0;
}

Mesh::~Mesh()
{
	RELEASE( vertexBuffer_ );
}

bool Mesh::Load( const VertexPosNormalTexcoord *vertices, int numVertices, ID3D11Device *device )
{
	D3D11_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof( desc ) );
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = sizeof( VertexPosNormalTexcoord ) * numVertices;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory( &resourceData, sizeof( resourceData ) );
	resourceData.pSysMem = vertices;

	HRESULT hr = device->CreateBuffer( &desc, &resourceData, &vertexBuffer_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create vertex buffer!" );
		return false;
	}
	return true;
}

//********************************
// Debug overlay
//********************************
Overlay::Overlay()
:
shader_(),
texture_()
{
	renderer_ = 0;
	vb_ = 0;
//	textureView_ = 0;
	sampler_ = 0;
	constantBuffer_ = 0;

	lineNumber_ = 0;
	lineOffset_ = 0;

	currentColor_ = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
}

Overlay::~Overlay()
{
	RELEASE( constantBuffer_ );
	RELEASE( sampler_ );
//	RELEASE( textureView_ );
	RELEASE( vb_ );
}

bool Overlay::Start( Renderer *renderer )
{
	HRESULT hr;

	renderer_ = renderer;
	if( !shader_.Load( L"assets/shaders/overlay.fx", renderer_->device_ ) )
		return false;

	// create vertex buffer
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
		return false;
	}

	// load texture
	if( !texture_.Load( L"assets/textures/droidMono.dds", renderer_->device_ ) )
	{
		OutputDebugStringA( "Failed to load Droid Mono texture!" );
		return false;
	}

	// create texture sampler
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory( &samplerDesc, sizeof( samplerDesc ) );
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 0;

	hr = renderer_->device_->CreateSamplerState( &samplerDesc, &sampler_ );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create sampler state!" );
		return false;
	}

	// create constant buffer
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory( &cbDesc, sizeof( cbDesc ) );
	cbDesc.ByteWidth = sizeof( OverlayShaderCB );
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	OverlayShaderCB cbData;
	cbData.color = currentColor_;

	D3D11_SUBRESOURCE_DATA cbInitData;
	cbInitData.pSysMem = &cbData;
	cbInitData.SysMemPitch = sizeof( OverlayShaderCB );
	cbInitData.SysMemSlicePitch = 0;

	hr = renderer_->device_->CreateBuffer( &cbDesc, &cbInitData, &constantBuffer_ );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create overlay constant buffer!" );
		return false;
	}


	return true;
}

void Overlay::Reset()
{
	lineNumber_ = 0;
	lineOffset_ = 0;
}

void Overlay::WriteLine( const char* text )
{
	Write( text );
	lineOffset_ = 0;
	lineNumber_++;
}

void Overlay::Write( const char* text )
{
	int textLength = (int)strlen( text );

	if( textLength > MAX_CHARS_IN_LINE )
	{
		lineOffset_ = 0;
		lineNumber_++;

		int numWrappedLines = textLength / MAX_CHARS_IN_LINE;
		int lastLineLength = textLength % MAX_CHARS_IN_LINE;

		char buffer[ MAX_CHARS_IN_LINE + 1 ];

		for( int wrappedLineIndex = 0; wrappedLineIndex < numWrappedLines; wrappedLineIndex++ )
		{
			strncpy( buffer, text + wrappedLineIndex * MAX_CHARS_IN_LINE, MAX_CHARS_IN_LINE );
			buffer[ MAX_CHARS_IN_LINE ] = '\0';
			DisplayText( TEXT_PADDING + lineOffset_, TEXT_PADDING + lineNumber_ * ( LINE_HEIGHT + LINE_SPACING ), buffer, XMFLOAT4( 1.0f, 1.0f, 0.0f, 1.0f ) );
			lineNumber_++;
		}
		strncpy( buffer, text + numWrappedLines * MAX_CHARS_IN_LINE, lastLineLength );
		buffer[ lastLineLength ] = '\0';
		DisplayText( TEXT_PADDING + lineOffset_, TEXT_PADDING + lineNumber_ * ( LINE_HEIGHT + LINE_SPACING ), buffer, XMFLOAT4( 1.0f, 1.0f, 0.0f, 1.0f ) );
		lineOffset_ += ( lastLineLength ) * CHAR_WIDTH;
	}
	else
	{
		DisplayText( TEXT_PADDING + lineOffset_, TEXT_PADDING + lineNumber_ * ( LINE_HEIGHT + LINE_SPACING ), text, XMFLOAT4( 1.0f, 1.0f, 0.0f, 1.0f ) );
		lineOffset_ += ( textLength ) * CHAR_WIDTH;
	}
}

void Overlay::DisplayText( int x, int y, const char* text, XMFLOAT4 color )
{
	// cut text if too long
	int textLength = (int)strlen( text );
	if( textLength > MAX_OVERLAY_CHARS ) {
		OutputDebugStringA( "Overlay text too long, clamping!\n" );
		textLength = MAX_OVERLAY_CHARS;
	}

	char shortText[ MAX_OVERLAY_CHARS + 1 ];
	strncpy( shortText, text, textLength );
	shortText[ textLength ] = '\0';

	OverlayVertex vertices[ MAX_OVERLAY_CHARS * 6 ]; // 6 vertices per sqaure/char
	int charOffsetInLine = 0;

	for( int i = 0; i < textLength; i++ )
	{
		char c = shortText[i];

		// handle newline
//		if( c == '\n' ) {
//			lineNumber_++;
//			charOffsetInLine = 0;
//			continue;
//		}

		float texcoord = GetCharOffset( c );

		// wrap at 64 chars
//		if( charOffsetInLine != 0 && charOffsetInLine % 64 == 0 ) {
//			lineNumber_++;
//			charOffsetInLine = 0;
//		}

		vertices[i * 6 + 0] = { { (float)( x + CHAR_WIDTH * ( charOffsetInLine + 0 ) ), (float)( y + 0 ) 			 }, { texcoord, 0.0f } };
		vertices[i * 6 + 1] = { { (float)( x + CHAR_WIDTH * ( charOffsetInLine + 0 ) ), (float)( y + LINE_HEIGHT ) }, { texcoord, 1.0f } };
		vertices[i * 6 + 2] = { { (float)( x + CHAR_WIDTH * ( charOffsetInLine + 1 ) ), (float)( y + LINE_HEIGHT ) }, { texcoord + NORMALIZED_CHAR_WIDTH, 1.0f } };
		vertices[i * 6 + 3] = { { (float)( x + CHAR_WIDTH * ( charOffsetInLine + 0 ) ), (float)( y + 0 ) 			 }, { texcoord, 0.0f } };
		vertices[i * 6 + 4] = { { (float)( x + CHAR_WIDTH * ( charOffsetInLine + 1 ) ), (float)( y + LINE_HEIGHT ) }, { texcoord + NORMALIZED_CHAR_WIDTH, 1.0f } };
		vertices[i * 6 + 5] = { { (float)( x + CHAR_WIDTH * ( charOffsetInLine + 1 ) ), (float)( y + 0 ) 			 }, { texcoord + NORMALIZED_CHAR_WIDTH, 0.0f } };
	
		charOffsetInLine++;
	}

	/*
	NOTE:
	A common use of these two flags involves filling dynamic index/vertex buffers with geometry 
	that can be seen from the camera's current position. The first time that data is entered 
	into the buffer on a given frame, Map is called with D3D11_MAP_WRITE_DISCARD; 
	doing so invalidates the previous contents of the buffer. The buffer is then filled with all available data.
	Subsequent writes to the buffer within the same frame should use D3D11_MAP_WRITE_NO_OVERWRITE. 
	This will enable the CPU to access a resource that is potentially being used by the GPU as long 
	as the restrictions described previously are respected.
	*/
	// TODO: build a single vertex buffer per frame 
	D3D11_MAPPED_SUBRESOURCE mapResource;
	HRESULT hr = renderer_->context_->Map( vb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapResource );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to map subresource!");
		return;
	}
	memcpy( mapResource.pData, vertices, sizeof( OverlayVertex ) * textLength * 6 );
	renderer_->context_->Unmap( vb_, 0 );

	unsigned int stride = sizeof( OverlayVertex );
	unsigned int offset = 0;

	// update constant buffer
	if( color.x != currentColor_.x ||
		color.y != currentColor_.y || 
		color.z != currentColor_.z ||
		color.w != currentColor_.w )
	{
		OutputDebugStringA( "Color updated!" );
		OverlayShaderCB	cbData;
		cbData.color = color;
		currentColor_ = color;
		renderer_->context_->UpdateSubresource( constantBuffer_, 0, NULL, &cbData, sizeof( OverlayShaderCB ), 0 );
	}

	renderer_->SetBlendMode( BM_ALPHA );
	renderer_->context_->IASetVertexBuffers( 0, 1, &vb_, &stride, &offset );
	// renderer_->context_->PSSetShaderResources( 0, 1, &textureView_ );
	renderer_->SetTexture( texture_ );
	renderer_->context_->PSSetSamplers( 0, 1, &sampler_ );
	renderer_->context_->PSSetConstantBuffers( 1, 1, &constantBuffer_ );
	renderer_->SetShader( shader_ );
	renderer_->context_->Draw( textLength * 6, 0 );
	renderer_->SetBlendMode( BM_DEFAULT );
}

float Overlay::GetCharOffset( char c )
{
	std::string alpha = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890+-*/=><()_\"';:.,!?@#[] ";

	int index = (int)alpha.find_first_of( c );

	return (float)index * NORMALIZED_CHAR_WIDTH;
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