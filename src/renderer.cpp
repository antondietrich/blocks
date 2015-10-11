#include "renderer.h"

namespace Blocks
{

using namespace DirectX;

extern ConfigType Config;

void MakeGlobalCBInitData( GlobalCB *cbData, D3D11_SUBRESOURCE_DATA *cbInitData, float screenWidth, float screenHeight );

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
	depthStencilView_ = 0;
	defaultRasterizerState_ = 0;
	globalConstantBuffer_ = 0;
	frameConstantBuffer_ = 0;
	modelConstantBuffer_ = 0;
//	linearSampler_ = 0;

	for( int i = 0; i < NUM_SAMPLER_TYPES; i++ ) {
		samplers_[i] = 0;
	}
	for( int i = 0; i < NUM_BLEND_MODES; i++ ) {
		blendStates_[i] = 0;
	}
	for( int i = 0; i < NUM_DEPTH_BUFFER_MODES; i++ ) {
		depthStencilStates_[i] = 0;
	}

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

	blockVB_ = 0;
	blockCache_ = new BlockVertex[ MAX_VERTS_PER_BATCH ];
	numCachedBlocks_ = 0;
	numCachedVerts_ = 0;

	viewPosition_ = { 0, 0, 0 };
	viewDirection_ = { 0, 0, 0 };
	viewUp_ = { 0, 0, 0 };
}

Renderer::~Renderer()
{
	swapChain_->SetFullscreenState( FALSE, 0 );

#ifdef _DEBUG_
	ID3D11Debug* DebugDevice = nullptr;
	device_->QueryInterface(__uuidof(ID3D11Debug), (void**)(&DebugDevice));
#endif

	delete[] blockCache_;
	RELEASE( blockVB_ );
	for( int i = 0; i < NUM_DEPTH_BUFFER_MODES; i++ ) {
		RELEASE( depthStencilStates_[i] );
	}
	for( int i = 0; i < NUM_BLEND_MODES; i++ ) {
		RELEASE( blendStates_[i] );
	}
	for( int i = 0; i < NUM_SAMPLER_TYPES; i++ ) {
		RELEASE( samplers_[i] );
	}
//	RELEASE( linearSampler_ );
	RELEASE( modelConstantBuffer_ );
	RELEASE( frameConstantBuffer_ );
	RELEASE( globalConstantBuffer_ );
	RELEASE( defaultRasterizerState_ );
	RELEASE( depthStencilView_ );
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
	uint totalDriverTypes = ARRAYSIZE( driverTypes );

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	uint totalFeatureLevels = ARRAYSIZE( featureLevels );

	// Device and context
	uint deviceFlags = 0;
	#ifdef _DEBUG_
		deviceFlags = D3D11_CREATE_DEVICE_DEBUG;
	#endif

	uint driverType = 0;
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
		uint quality;
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

	hr = dxgiFactory->CreateSwapChain( device_, &swapChainDesc, &swapChain_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create swap chain!" );
		return false;
	}

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
		

	// NOTE: as a side-effect, sets the back buffer and the depth buffer (consider making separate functions)
	ResizeBuffers();

	if( Config.fullscreen ) {
		ToggleFullscreen();
	}

	// rasterizer states
	D3D11_RASTERIZER_DESC rasterizerStateDesc;
	ZeroMemory( &rasterizerStateDesc, sizeof( rasterizerStateDesc ) );
	rasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
	//rasterizerStateDesc.FillMode = D3D11_FILL_WIREFRAME;
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
	rasterizerStateDesc.AntialiasedLineEnable = TRUE;

	hr =  device_->CreateRasterizerState( &rasterizerStateDesc, &defaultRasterizerState_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create default rasterizer state!" );
		return false;
	}

	context_->RSSetState( defaultRasterizerState_ );

	// texture samplers
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory( &samplerDesc, sizeof( samplerDesc ) );
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;
	// samplerDesc.MaxLOD = 6;
	hr = device_->CreateSamplerState( &samplerDesc, &samplers_[ SAMPLER_POINT ] );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create point sampler state!" );
		return false;
	}

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = device_->CreateSamplerState( &samplerDesc, &samplers_[ SAMPLER_LINEAR ] );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create linear sampler state!" );
		return false;
	}

	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = Config.filtering;
	hr = device_->CreateSamplerState( &samplerDesc, &samplers_[ SAMPLER_ANISOTROPIC ] );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create anisotropic sampler state!" );
		return false;
	}

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

	// depth buffer modes
	D3D11_DEPTH_STENCIL_DESC depthDesc;
	ZeroMemory( &depthDesc, sizeof( depthDesc ) );
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthDesc.StencilEnable = FALSE;
	depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depthDesc.BackFace.StencilFunc = depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthDesc.BackFace.StencilFailOp = depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilPassOp = depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilDepthFailOp = depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	hr = device_->CreateDepthStencilState( &depthDesc, &depthStencilStates_[ DB_ENABLED ] );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create depth-stencil state!" );
		return false;
	}

	depthDesc.DepthEnable = FALSE;
	hr = device_->CreateDepthStencilState( &depthDesc, &depthStencilStates_[ DB_DISABLED ] );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create depth-stencil state!" );
		return false;
	}

	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	hr = device_->CreateDepthStencilState( &depthDesc, &depthStencilStates_[ DB_READ ] );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create depth-stencil state!" );
		return false;
	}

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
	D3D11_SUBRESOURCE_DATA cbInitData;

	MakeGlobalCBInitData( &cbData, &cbInitData, screenViewport_.Width, screenViewport_.Height );
	hr = device_->CreateBuffer( &cbDesc, &cbInitData, &globalConstantBuffer_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create global constant buffer!" );
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
	direction = XMVector4Normalize( direction );
	XMVECTOR up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	XMMATRIX view =  XMMatrixLookToLH( eye, direction, up );
	XMMATRIX projection = XMMatrixPerspectiveFovLH( XMConvertToRadians( 60.0f ), screenViewport_.Width / screenViewport_.Height, 0.1f, 1000.0f );
	// XMStoreFloat4x4( &frameCBData.vp, XMMatrixMultiply( view, projection ) );
	XMStoreFloat4x4( &frameCBData.vp, XMMatrixTranspose( XMMatrixMultiply( view, projection ) ) );
	XMStoreFloat4x4( &view_, view );
	XMStoreFloat4x4( &projection_, projection );

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
	if( !textures_[2].Load( L"assets/textures/test.dds", device_ ) ) {
		OutputDebugStringA( "Failed to load texture!" );
		return false;
	}
	if( !textures_[3].Load( L"assets/textures/grass2.dds", device_ ) ) {
		OutputDebugStringA( "Failed to load texture!" );
		return false;
	}
	if( !textures_[4].Load( L"assets/textures/rock.dds", device_ ) ) {
		OutputDebugStringA( "Failed to load texture!" );
		return false;
	}
	if( !textures_[5].Load( L"assets/textures/wood.dds", device_ ) ) {
		OutputDebugStringA( "Failed to load texture!" );
		return false;
	}
	if( !textures_[6].Load( L"assets/textures/atlas01.dds", device_ ) ) {
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

#if 0
	int numVertices = VERTS_PER_BLOCK;
	VertexPosNormalTexcoord cubeVertices[] = 
	{
		// face 1 / -Z
		{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } }, // 0
		{ { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } }, // 1
		{ {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } }, // 2
		{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } }, // 0
		{ {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } }, // 2
		{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } }, // 3
		// face 2 / +X
		{ {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } }, // 3
		{ {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }, // 2
		{ {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } }, // 4
		{ {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } }, // 3
		{ {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } }, // 4
		{ {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }, // 5
		// face 3 / +Z
		{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } }, // 5
		{ {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } }, // 4
		{ { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } }, // 6
		{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } }, // 5
		{ { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } }, // 6
		{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } }, // 7
		// face 4 / -X
		{ { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } }, // 7
		{ { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }, // 6
		{ { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } }, // 1
		{ { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } }, // 7
		{ { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } }, // 1
		{ { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }, // 0
		// face 5 / +Y
		{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } }, // 7
		{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } }, // 0
		{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } }, // 3
		{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } }, // 7
		{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } }, // 3
		{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } }, // 5
		// face 6 / -Y
		{ { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } }, // 1
		{ { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } }, // 6
		{ {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } }, // 4
		{ { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } }, // 1
		{ {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } }, // 4
		{ {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } }, // 2
	};

	for( int i = 0; i < VERTS_PER_BLOCK; i++ )
	{
		block_[i] = cubeVertices[i];
	}

	if( !meshes_[0].Load( cubeVertices, numVertices, device_ ) ) {
		OutputDebugStringA( "Failed to load cube vertices!" );
		return false;
	}
#endif

		// vertex buffer to batch blocks
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory( &vertexBufferDesc, sizeof( vertexBufferDesc ) );
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof( BlockVertex ) * MAX_VERTS_PER_BATCH; // ~256MB

	hr = device_->CreateBuffer( &vertexBufferDesc, NULL, &blockVB_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create vertex buffer!" );
		return false;
	}
	context_->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	return true;
}

void Renderer::Begin()
{
	float clearColor[ 4 ] = { 0.53f, 0.81f, 0.98f, 1.0f };
	context_->ClearRenderTargetView( backBufferView_, clearColor );
	context_->ClearDepthStencilView( depthStencilView_, D3D11_CLEAR_DEPTH, 1.0f, 0 );


	context_->VSSetConstantBuffers( 1, 1, &frameConstantBuffer_ );
	context_->VSSetConstantBuffers( 2, 1, &modelConstantBuffer_ );
	SetSampler( SAMPLER_ANISOTROPIC );
	SetTexture( textures_[6], ST_FRAGMENT, 0 );
	SetShader( shaders_[0] );
	SetDepthBufferMode( DB_ENABLED );

	uint stride = sizeof( BlockVertex );
	uint offset = 0;
	context_->IASetVertexBuffers( 0, 1, &blockVB_, &stride, &offset );

	numBatches_ = 0;
	numCachedVerts_ = 0;
}

void Renderer::Flush()
{
	if( numCachedVerts_ ) {
		Draw( numCachedVerts_ );
		numCachedVerts_ = 0;
	}
}

void Renderer::Draw( uint vertexCount, uint startVertexOffset )
{
//	assert( numCachedBlocks_ * VERTS_PER_BLOCK == numPrimitives );
//	D3D11_MAPPED_SUBRESOURCE mapResource;
//	HRESULT hr = context_->Map( blockVB_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapResource );
//	if( FAILED( hr ) )
//	{
//		OutputDebugStringA( "Failed to map subresource!");
//		return;
//	}
//	memcpy( mapResource.pData, blockCache_, sizeof( BlockVertex ) * numPrimitives );
//	context_->Unmap( blockVB_, 0 );

	numCachedBlocks_ = 0;
	
	uint stride = sizeof( BlockVertex );
	uint offset = 0;
	context_->IASetVertexBuffers( 0, 1, &blockVB_, &stride, &offset );

//	uint stride = sizeof( BlockVertex );
//	uint offset = 0;
//	context_->IASetVertexBuffers( 0, 1, &blockVB_, &stride, &offset );

	SetShader( shaders_[0] );

	context_->Draw( vertexCount, startVertexOffset );

	numBatches_++;
}

void Renderer::DrawCube( XMFLOAT3 offset )
{
	ModelCB modelCBData;
	modelCBData.translate = XMFLOAT4( offset.x, offset.y, offset.z, 0.0f );
	context_->UpdateSubresource( modelConstantBuffer_, 0, NULL, &modelCBData, sizeof( ModelCB ), 0 );

	context_->VSSetConstantBuffers( 1, 1, &frameConstantBuffer_ );
	context_->VSSetConstantBuffers( 2, 1, &modelConstantBuffer_ );


	SetMesh( meshes_[0] );
	SetTexture( textures_[0], ST_FRAGMENT );
	SetShader( shaders_[0] );
	context_->Draw( 36, 0 );
}

void Renderer::DrawChunk( int x, int z, BlockVertex *vertices, int numVertices )
{
	assert( numCachedVerts_ + numVertices < MAX_VERTS_PER_BATCH );

	D3D11_MAP mappingType = D3D11_MAP_WRITE_DISCARD;
	if( numCachedVerts_ == 0 )
	{
		mappingType = D3D11_MAP_WRITE_DISCARD;
	}
	else
	{
		mappingType = D3D11_MAP_WRITE_NO_OVERWRITE;
	}

	ProfileStart( "MAP" );
	D3D11_MAPPED_SUBRESOURCE mapResource;
	HRESULT hr = context_->Map( blockVB_, 0, mappingType, 0, &mapResource );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to map subresource!");
		return;
	}

	ProfileStart( "MEMCPY" );
	memcpy( (BlockVertex*)mapResource.pData + numCachedVerts_, vertices, sizeof( BlockVertex ) * numVertices );
	ProfileStop();

	context_->Unmap( blockVB_, 0 );
	ProfileStop();

	ModelCB modelCBData;
	modelCBData.translate = XMFLOAT4( (float)x, 0.0f, (float)z, 0.0f );
	context_->UpdateSubresource( modelConstantBuffer_, 0, NULL, &modelCBData, sizeof( ModelCB ), 0 );

	ProfileStart( "Draw" );
	Draw( numVertices, numCachedVerts_ );
	ProfileStop();

	numCachedVerts_ += numVertices;
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
	RELEASE( depthStencilView_ );

	// Let DXGI figure out buffer size and format
	hr = swapChain_->ResizeBuffers( 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0 );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to resize back buffers!" );
		return false;
	}

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

	// depth buffer
	ID3D11Texture2D* depthTexture = 0;
	D3D11_TEXTURE2D_DESC depthTexDesc;
	ZeroMemory( &depthTexDesc, sizeof( depthTexDesc ) );
	depthTexDesc.Width = desc.BufferDesc.Width;
	depthTexDesc.Height = desc.BufferDesc.Height;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthTexDesc.SampleDesc.Count = desc.SampleDesc.Count;
	depthTexDesc.SampleDesc.Quality = desc.SampleDesc.Quality;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.MiscFlags = 0;
	hr = device_->CreateTexture2D( &depthTexDesc, NULL, &depthTexture );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create Z-Buffer texture!" );
		RELEASE( depthTexture );
		return false;
	}

	D3D11_DSV_DIMENSION accessMode;
	if( Config.multisampling == 2 || Config.multisampling == 4 || Config.multisampling == 8 || Config.multisampling == 16 )
	{
		accessMode = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		accessMode = D3D11_DSV_DIMENSION_TEXTURE2D;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc;
	ZeroMemory( &dsViewDesc, sizeof( dsViewDesc ) );
	dsViewDesc.Format = depthTexDesc.Format;
	dsViewDesc.ViewDimension = accessMode;
	dsViewDesc.Texture2D.MipSlice = 0;
	hr = device_->CreateDepthStencilView( depthTexture, &dsViewDesc, &depthStencilView_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create depth-stencil view!" );
		RELEASE( depthTexture );
		return false;
	}

	RELEASE( depthTexture );

	context_->OMSetRenderTargets( 1, &backBufferView_, depthStencilView_ );

	screenViewport_.Width = (float)desc.BufferDesc.Width;
	screenViewport_.Height = (float)desc.BufferDesc.Height;
	screenViewport_.MinDepth = 0.0f;
	screenViewport_.MaxDepth = 1.0f;
	screenViewport_.TopLeftX = 0;
	screenViewport_.TopLeftY = 0;
	context_->RSSetViewports( 1, &screenViewport_ );

	D3D11_SUBRESOURCE_DATA cbInitData;
	GlobalCB cbData;
	MakeGlobalCBInitData( &cbData, &cbInitData, screenViewport_.Width, screenViewport_.Height );

	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create global constant buffer!" );
		return false;
	}

	if( globalConstantBuffer_ ) {
		context_->UpdateSubresource( globalConstantBuffer_, 0, NULL, &cbData, sizeof( GlobalCB ), 0 );
	}

	return true;
}

void Renderer::SetView( XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 up )
{
	viewPosition_ = pos;
	viewDirection_ = dir;
	viewUp_ = up;

	XMVECTOR vPos, vDir, vUp;
	vPos = XMLoadFloat3( &pos );
	vDir = XMLoadFloat3( &dir );
	vUp = XMLoadFloat3( &up );
	XMMATRIX view =  XMMatrixLookToLH( vPos, vDir, vUp );

	// XMStoreFloat4x4( &view_, view );

	XMMATRIX proj = XMLoadFloat4x4( &projection_ );
	XMMATRIX vp = XMMatrixTranspose( XMMatrixMultiply( view, proj ) );

	FrameCB frameCBData;
	XMStoreFloat4x4( &frameCBData.vp, vp );

	context_->UpdateSubresource( frameConstantBuffer_, 0, NULL, &frameCBData, sizeof( FrameCB ), 0 );
}

void Renderer::SetSampler( SAMPLER_TYPE st )
{
	uint slot = 0;
	uint numSamplers = 1;
	context_->PSSetSamplers( slot, numSamplers, &samplers_[ st ] );
}

void Renderer::SetBlendMode( BLEND_MODE bm )
{
	UINT sampleMask   = 0xffffffff;
	context_->OMSetBlendState( blendStates_[ bm ], NULL, sampleMask );
}

void Renderer::SetDepthBufferMode( DEPTH_BUFFER_MODE dbm )
{
	uint stencilRef = 0;
	context_->OMSetDepthStencilState( depthStencilStates_[ dbm ], stencilRef );
}

void Renderer::SetMesh( const Mesh& mesh )
{
	uint stride = sizeof( VertexPosNormalTexcoord );
	uint offset = 0;
	context_->IASetVertexBuffers( 0, 1, &mesh.vertexBuffer_, &stride, &offset );
}

void Renderer::SetShader( const Shader& shader )
{
	context_->IASetInputLayout( shader.inputLayout_ );
	context_->VSSetShader( shader.vertexShader_, NULL, 0 );
	context_->PSSetShader( shader.pixelShader_, NULL, 0 );
}

void Renderer::SetTexture( const Texture& texture, SHADER_TYPE shader, uint slot )
{
	if( ( shader & ST_VERTEX ) == ST_VERTEX ) {
		context_->VSSetShaderResources( slot, 1, &texture.textureView_ );	
	}
	if( ( shader & ST_GEOMETRY ) == ST_GEOMETRY ) {
		context_->GSSetShaderResources( slot, 1, &texture.textureView_ );
	}
	if( ( shader & ST_FRAGMENT ) == ST_FRAGMENT ) {
		context_->PSSetShaderResources( slot, 1, &texture.textureView_ );
	}
}

void MakeGlobalCBInitData( GlobalCB *cbData, D3D11_SUBRESOURCE_DATA *cbInitData, float screenWidth, float screenHeight )
{
	cbData->screenToNDC = XMFLOAT4X4(
		2.0f / screenWidth,	 0.0f,					0.0f,	-1.0f,
		0.0f,				-2.0f / screenHeight,	0.0f,	 1.0f,
		0.0f,				 0.0f,					1.0f,	 0.0f,
		0.0f,				 0.0f,					0.0f,	 1.0f
	);

	cbData->normals[0] = {  0.0f,  0.0f, -1.0f, 0.0f }; // -Z
	cbData->normals[1] = {  1.0f,  0.0f,  0.0f, 0.0f }; // +X
	cbData->normals[2] = {  0.0f,  0.0f,  1.0f, 0.0f }; // +Z
	cbData->normals[3] = { -1.0f,  0.0f,  0.0f, 0.0f }; // -X
	cbData->normals[4] = {  0.0f,  1.0f,  0.0f, 0.0f }; // +Y
	cbData->normals[5] = {  0.0f, -1.0f,  0.0f, 0.0f }; // -Y

	cbData->texcoords[0] = { 2.0f / 512.0f, 		2.0f / 512.0f, 0.0f, 0.0f }; // top left
	cbData->texcoords[1] = { 2.0f / 512.0f,		254.0f / 512.0f, 0.0f, 0.0f }; // bottom left
	cbData->texcoords[2] = { 254.0f / 512.0f, 	2.0f / 512.0f, 0.0f, 0.0f }; // top right
	cbData->texcoords[3] = { 254.0f / 512.0f, 	254.0f / 512.0f, 0.0f, 0.0f }; // bottom right

	cbData->texcoords[4] = { ( 2.0f + 254.0f ) / 512.0f, 	2.0f / 512.0f, 0.0f, 0.0f }; // top left
	cbData->texcoords[5] = { ( 2.0f + 254.0f ) / 512.0f,		254.0f / 512.0f, 0.0f, 0.0f }; // bottom left
	cbData->texcoords[6] = { ( 254.0f + 254.0f ) / 512.0f, 	2.0f / 512.0f, 0.0f, 0.0f }; // top right
	cbData->texcoords[7] = { ( 254.0f + 254.0f ) / 512.0f, 	254.0f / 512.0f, 0.0f, 0.0f }; // bottom right

	cbData->texcoords[8] = { 2.0f / 512.0f,	 	( 2.0f + 254.0f ) / 512.0f, 0.0f, 0.0f }; // top left
	cbData->texcoords[9] = { 2.0f / 512.0f,		( 254.0f + 254.0f ) / 512.0f, 0.0f, 0.0f }; // bottom left
	cbData->texcoords[10] = { 254.0f / 512.0f, 	( 2.0f + 254.0f ) / 512.0f, 0.0f, 0.0f }; // top right
	cbData->texcoords[11] = { 254.0f / 512.0f, 	( 254.0f + 254.0f ) / 512.0f, 0.0f, 0.0f }; // bottom right

	cbData->texcoords[12] = { ( 2.0f + 254.0f ) / 512.0f,	 	( 2.0f + 254.0f ) / 512.0f, 0.0f, 0.0f }; // top left
	cbData->texcoords[13] = { ( 2.0f + 254.0f ) / 512.0f,		( 254.0f + 254.0f ) / 512.0f, 0.0f, 0.0f }; // bottom left
	cbData->texcoords[14] = { ( 254.0f + 254.0f ) / 512.0f, 	( 2.0f + 254.0f ) / 512.0f, 0.0f, 0.0f }; // top right
	cbData->texcoords[15] = { ( 254.0f + 254.0f ) / 512.0f, 	( 254.0f + 254.0f ) / 512.0f, 0.0f, 0.0f }; // bottom right

	cbInitData->pSysMem = cbData;
	cbInitData->SysMemPitch = sizeof( GlobalCB );
	cbInitData->SysMemSlicePitch = 0;

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
textShader_(),
primitiveShader_(),
texture_()
{
	renderer_ = 0;
	textVB_ = 0;
	primitiveVB_ = 0;
//	textureView_ = 0;
//	sampler_ = 0;
	constantBuffer_ = 0;

	lineNumber_ = 0;
	lineOffset_ = 0;

	currentColor_ = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
}

Overlay::~Overlay()
{
	RELEASE( constantBuffer_ );
//	RELEASE( sampler_ );
//	RELEASE( textureView_ );
	RELEASE( textVB_ );
	RELEASE( primitiveVB_ );
}

bool Overlay::Start( Renderer *renderer )
{
	HRESULT hr;

	renderer_ = renderer;
	if( !textShader_.Load( L"assets/shaders/overlay.fx", renderer_->device_ ) )
		return false;
	if( !primitiveShader_.Load( L"assets/shaders/primitive.fx", renderer_->device_ ) )
		return false;

	// create vertex buffer for text drawing
	D3D11_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof( desc ) );
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = sizeof( OverlayVertex2D ) * MAX_OVERLAY_CHARS;

	hr = renderer_->device_->CreateBuffer( &desc, NULL, &textVB_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create overlay vertex buffer!" );
		return false;
	}

	// load text texture
	if( !texture_.Load( L"assets/textures/droidMono.dds", renderer_->device_ ) )
	{
		OutputDebugStringA( "Failed to load Droid Mono texture!" );
		return false;
	}

	// create vertex buffer for primitive drawing
	ZeroMemory( &desc, sizeof( desc ) );
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = sizeof( OverlayVertex3D ) * OVERLAY_3DVB_SIZE;

	hr = renderer_->device_->CreateBuffer( &desc, NULL, &primitiveVB_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create overlay vertex buffer!" );
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

void Overlay::Write( const char* fmt, ... )
{
	char buffer[ MAX_OVERLAY_CHARS ];
	va_list va;
	va_start(va, fmt);
	vsprintf(buffer, fmt, va);
	va_end(va);
	WriteUnformatted( buffer );
}

void Overlay::WriteLine( const char* fmt, ... )
{
	char buffer[ MAX_OVERLAY_CHARS ];
	va_list va;
	va_start(va, fmt);
	vsprintf(buffer, fmt, va);
	va_end(va);
	WriteLineUnformatted( buffer );
}

void Overlay::WriteLineUnformatted( const char* text )
{
	Write( text );
	lineOffset_ = 0;
	lineNumber_++;
}

void Overlay::WriteUnformatted( const char* text )
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

	OverlayVertex2D vertices[ MAX_OVERLAY_CHARS * 6 ]; // 6 vertices per sqaure/char
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
	HRESULT hr = renderer_->context_->Map( textVB_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapResource );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to map subresource!");
		return;
	}
	memcpy( mapResource.pData, vertices, sizeof( OverlayVertex2D ) * textLength * 6 );
	renderer_->context_->Unmap( textVB_, 0 );

	uint stride = sizeof( OverlayVertex2D );
	uint offset = 0;

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

	renderer_->SetDepthBufferMode( DB_DISABLED );
	renderer_->SetBlendMode( BM_ALPHA );
	renderer_->context_->IASetVertexBuffers( 0, 1, &textVB_, &stride, &offset );
	// renderer_->context_->PSSetShaderResources( 0, 1, &textureView_ );
	renderer_->SetTexture( texture_, ST_FRAGMENT );
	renderer_->SetSampler( SAMPLER_POINT );
	renderer_->context_->PSSetConstantBuffers( 1, 1, &constantBuffer_ );
	renderer_->SetShader( textShader_ );

	renderer_->context_->Draw( textLength * 6, 0 );

	renderer_->SetBlendMode( BM_DEFAULT );
	renderer_->SetDepthBufferMode( DB_ENABLED );
}

float Overlay::GetCharOffset( char c )
{
	std::string alpha = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890+-*/=><()_\"';:.,!?@#[] ";

	int index = (int)alpha.find_first_of( c );

	return (float)index * NORMALIZED_CHAR_WIDTH;
}

void Overlay::DrawLine( XMFLOAT3 A, XMFLOAT3 B, XMFLOAT4 color )
{
	OverlayVertex3D vertices[2];
	vertices[0].pos[0] = A.x;
	vertices[0].pos[1] = A.y;
	vertices[0].pos[2] = A.z;
	vertices[1].pos[0] = B.x;
	vertices[1].pos[1] = B.y;
	vertices[1].pos[2] = B.z;

	vertices[0].color[0] = color.x;
	vertices[0].color[1] = color.y;
	vertices[0].color[2] = color.z;
	vertices[0].color[3] = color.w;
	vertices[1].color[0] = color.x;
	vertices[1].color[1] = color.y;
	vertices[1].color[2] = color.z;
	vertices[1].color[3] = color.w;

	D3D11_MAPPED_SUBRESOURCE mapResource;
	HRESULT hr = renderer_->context_->Map( primitiveVB_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapResource );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to map subresource!");
		return;
	}
	memcpy( mapResource.pData, vertices, sizeof( OverlayVertex3D ) * 2 );
	renderer_->context_->Unmap( primitiveVB_, 0 );

	uint stride = sizeof( OverlayVertex3D );
	uint offset = 0;

	renderer_->SetDepthBufferMode( DB_READ );
	renderer_->SetBlendMode( BM_ALPHA );
	renderer_->context_->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
	renderer_->context_->IASetVertexBuffers( 0, 1, &primitiveVB_, &stride, &offset );
	renderer_->context_->VSSetConstantBuffers( 1, 1, &renderer_->frameConstantBuffer_ );
	renderer_->SetShader( primitiveShader_ );

	renderer_->context_->Draw( 2, 0 );

	renderer_->context_->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	renderer_->SetBlendMode( BM_DEFAULT );
	renderer_->SetDepthBufferMode( DB_ENABLED );
}

void Overlay::DrawLineDir( XMFLOAT3 start, XMFLOAT3 dir, DirectX::XMFLOAT4 color )
{
	XMFLOAT3 A = start;
	XMFLOAT3 B = {
		start.x + dir.x,
		start.y + dir.y,
		start.z + dir.z,
	};
	DrawLine( A, B, color );
}

void Overlay::DrawPoint( XMFLOAT3 P, XMFLOAT4 color )
{
	XMVECTOR vPoint = XMLoadFloat3( &P );
	XMVECTOR vView = XMLoadFloat3( &renderer_->viewPosition_ );

	float dist = XMVectorGetX( XMVector4Length( vPoint - vView ) );
	float offset = 0.05f * dist;

	DrawLine( { P.x - offset, P.y, P.z }, { P.x + offset, P.y, P.z }, color );
	DrawLine( { P.x, P.y - offset, P.z }, { P.x, P.y + offset, P.z }, color );
	DrawLine( { P.x, P.y, P.z - offset }, { P.x, P.y, P.z + offset }, color );
}

void Overlay::OulineBlock( int chunkX, int chunkZ, int x, int y, int z, XMFLOAT4 color )
{
	float negOffset = -0.00f;
	float posOffset =  1.00f;
	XMFLOAT3 min = { chunkX * CHUNK_WIDTH + x + negOffset, y + negOffset, chunkZ * CHUNK_WIDTH + z + negOffset };
	XMFLOAT3 max = { chunkX * CHUNK_WIDTH + x + posOffset, y + posOffset, chunkZ * CHUNK_WIDTH + z + posOffset };

	DrawLine( { min.x, min.y, min.z }, { max.x, min.y, min.z }, color );
	DrawLine( { min.x, min.y, min.z }, { min.x, max.y, min.z }, color );
	DrawLine( { min.x, min.y, min.z }, { min.x, min.y, max.z }, color );
	DrawLine( { max.x, max.y, max.z }, { min.x, max.y, max.z }, color );
	DrawLine( { max.x, max.y, max.z }, { max.x, min.y, max.z }, color );
	DrawLine( { max.x, max.y, max.z }, { max.x, max.y, min.z }, color );
	
	DrawLine( { min.x, min.y, max.z }, { max.x, min.y, max.z }, color );
	DrawLine( { min.x, min.y, max.z }, { min.x, max.y, max.z }, color );
	DrawLine( { max.x, max.y, min.z }, { min.x, max.y, min.z }, color );
	DrawLine( { max.x, max.y, min.z }, { max.x, min.y, min.z }, color );
	
	DrawLine( { min.x, max.y, min.z }, { min.x, max.y, max.z }, color );
	DrawLine( { max.x, min.y, max.z }, { max.x, min.y, min.z }, color );

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
	uint numInputParameters = shaderDesc.InputParameters;
	D3D11_INPUT_ELEMENT_DESC *inputLayoutDesc = new D3D11_INPUT_ELEMENT_DESC[ numInputParameters ];
	for( uint i = 0; i < numInputParameters; ++i )
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