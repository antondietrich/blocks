#define ASSET_DEF_IMPLEMENTATION

#include "renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "lib\stb_image.h"

namespace Blocks
{

using namespace DirectX;

/*** globals ***/
extern ConfigType Config;
extern int gChunkMeshCacheDim;

ResourceHandle gDefaultDepthStencilState;
ResourceHandle gDefaultRasterizerState;

TextureDefinition gTextureDefinitions[ TEXTURE::TEXTURE_COUNT ];
static Texture gTempTextureStorage[ TEXTURE::TEXTURE_COUNT ];
static Shader gTempShaderStorage[ SHADER::SHADER_COUNT ];
static Mesh gTempMeshStorage[ MESH::MESH_COUNT ];

static ResourceHandle nextFreeVBSlot = 0;

#define VB_STORAGE_SIZE 64
VertexBuffer * gTempVBStorage[ VB_STORAGE_SIZE ];

bool Renderer::isInstantiated_ = false;


/*** function declarations ***/
void MakeGlobalCBInitData( GlobalCB *cbData, D3D11_SUBRESOURCE_DATA *cbInitData, float screenWidth, float screenHeight, float viewDistance );
bool LoadTextures( ID3D11Device * device, ID3D11DeviceContext * context );
bool LoadShaders( ID3D11Device * device );
bool LoadMeshes( Renderer * renderer );

Mesh * GetMeshByID( MESH id )
{
	return &gTempMeshStorage[ id ];
}

Renderer::Renderer()
{
	assert( !isInstantiated_ );
	isInstantiated_ = true;
	vsync_ = 0;

	device_ = 0;
	context_ = 0;
	swapChain_ = 0;
	backBufferView_ = 0;
	depthStencilView_ = 0;
	globalConstantBuffer_ = 0;
	frameConstantBuffer_ = 0;
	lightConstantBuffer_ = 0;
	modelConstantBuffer_ = 0;
	gameObjectConstantBuffer_ = 0;

	nextFreeDepthStencilSlot = 0;
	nextFreeRasterizerSlot = 0;

	for( int i = 0; i < MAX_DEPTH_STENCIL_STATES; i++ ) {
		depthStencilStates_[i] = 0;
	}
	for( int i = 0; i < NUM_SAMPLER_TYPES; i++ ) {
		samplers_[i] = 0;
	}
	for( int i = 0; i < MAX_RASTERIZER_STATES; i++ ) {
		rasterizerStates_[i] = 0;
	}
	for( int i = 0; i < NUM_BLEND_MODES; i++ ) {
		blendStates_[i] = 0;
	}

//	for( int i = 0; i < MAX_SHADERS; i++ )
//	{
//		shaders_[i] = Shader();
//	}

//	for( int i = 0; i < MAX_MESHES; i++ )
//	{
//		meshes_[i] = Mesh();
//	}

	for( int i = 0; i < VB_STORAGE_SIZE; i++ )
	{
		gTempVBStorage[i] = 0;
	}

	viewPosition_ = { 0, 0, 0 };
	viewDirection_ = { 0, 0, 0 };
	viewUp_ = { 0, 0, 0 };

//	for( int i = 0; i < TEXTURE::COUNT; i++ )
//	{
//		gTempTextureStorage[i] = 0;
//	}
}

Renderer::~Renderer()
{
	swapChain_->SetFullscreenState( FALSE, 0 );

#ifdef _DEBUG_
	ID3D11Debug* DebugDevice = nullptr;
	device_->QueryInterface(__uuidof(ID3D11Debug), (void**)(&DebugDevice));
#endif

	FreeTextureDefinitions( gTextureDefinitions );

	for( int i = 0; i < VB_STORAGE_SIZE; i++ )
	{
		if( gTempVBStorage[i] )
			delete gTempVBStorage[i];
	}

	for( int i = 0; i < NUM_BLEND_MODES; i++ ) {
		RELEASE( blendStates_[i] );
	}
	for( int i = 0; i < MAX_RASTERIZER_STATES; i++ ) {
		RELEASE( rasterizerStates_[i] );
	}
	for( int i = 0; i < NUM_SAMPLER_TYPES; i++ ) {
		RELEASE( samplers_[i] );
	}
	for( int i = 0; i < MAX_DEPTH_STENCIL_STATES; i++ ) {
		RELEASE( depthStencilStates_[i] );
	}
	RELEASE( gameObjectConstantBuffer_ );
	RELEASE( modelConstantBuffer_ );
	RELEASE( lightConstantBuffer_ );
	RELEASE( frameConstantBuffer_ );
	RELEASE( globalConstantBuffer_ );
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


	// NOTE: as a side-effect, sets the back buffer and the depth buffer;
	//  updates the GlobalCB (consider making separate functions)
	ResizeBuffers();

	if( Config.fullscreen ) {
		ToggleFullscreen();
	}

	// depth/stencil
	DepthStateDesc depthStateDesc;
	ZERO_MEMORY( depthStateDesc );
	depthStateDesc.enabled = true;
	depthStateDesc.readonly = false;
	depthStateDesc.comparisonFunction = COMPARISON_FUNCTION::LESS_EQUAL;
	StencilStateDesc stencilStateDesc;
	stencilStateDesc.enabled = false;
	gDefaultDepthStencilState = CreateDepthStencilState( depthStateDesc, stencilStateDesc );
	if( gDefaultDepthStencilState == INVALID_HANDLE )
	{
		OutputDebugStringA( "Failed to create default depth/stencil state!" );
		return false;
	}

	// rasterizer
	RasterizerStateDesc rasterizerStateDesc;
	ZERO_MEMORY( rasterizerStateDesc );
	rasterizerStateDesc.fillMode = FILL_MODE::SOLID;
	rasterizerStateDesc.cullMode = CULL_MODE::BACK;
	rasterizerStateDesc.frontCCW = true;
	rasterizerStateDesc.depthClipEnabled = true;
	if( Config.multisampling == 2 || Config.multisampling == 4 || Config.multisampling == 8 || Config.multisampling == 16 )
	{
		rasterizerStateDesc.multisampleEnabled = true;
		rasterizerStateDesc.antialiasedLineEnabled = true;
	}

	gDefaultRasterizerState = CreateRasterizerState( rasterizerStateDesc );
	if( gDefaultRasterizerState == INVALID_HANDLE )
	{
		OutputDebugStringA( "Failed to create default rasterizer state!" );
		return false;
	}

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

	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_GREATER;
	hr = device_->CreateSamplerState( &samplerDesc, &samplers_[ SAMPLER_SHADOW ] );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create compare sampler state!" );
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

	MakeGlobalCBInitData( &cbData, &cbInitData, screenViewport_.Width, screenViewport_.Height, (float)(Config.viewDistanceChunks * CHUNK_WIDTH) );
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
	context_->PSSetConstantBuffers( 1, 1, &frameConstantBuffer_ );

	// light constant buffer
	ZeroMemory( &cbDesc, sizeof( cbDesc ) );
	cbDesc.ByteWidth = sizeof( LightCB );
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = device_->CreateBuffer( &cbDesc, NULL, &lightConstantBuffer_ );

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

	// game object constant buffer
	ZeroMemory( &cbDesc, sizeof( cbDesc ) );
	cbDesc.ByteWidth = sizeof( GameObjectCB );
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = device_->CreateBuffer( &cbDesc, NULL, &gameObjectConstantBuffer_ );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to create game object constant buffer!" );
		return false;
	}

	// Load textures
	LoadTextureDefinitions( gTextureDefinitions );
	if( !LoadTextures( device_, context_ ) )
	{
		OutputDebugStringA( "Failed to load textures!" );
		return false;
	}

	// Load shaders
	if( !LoadShaders( device_ ) )
	{
		OutputDebugStringA( "Failed to load shaders!" );
		return false;
	}

	// Load meshes
	if( !LoadMeshes( this ) )
	{
		OutputDebugStringA( "Failed to load meshes!" );
		return false;
	}

//	blockVB_ = new ID3D11BufferArray[ gChunkMeshCacheDim * gChunkMeshCacheDim ];
//	for( int i = 0; i < gChunkMeshCacheDim * gChunkMeshCacheDim; i++ )
//	{
//		blockVB_[i] = 0;
//	}
#if 0
	// vertex buffer to store chunk meshes
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
#endif
	context_->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	return true;
}

void Renderer::Begin()
{
	float clearColor[ 4 ] = { 0.5f, 0.5f, 0.5f, 1.0f };
	context_->ClearRenderTargetView( backBufferView_, clearColor );
	context_->ClearDepthStencilView( depthStencilView_, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	context_->VSSetConstantBuffers( 1, 1, &frameConstantBuffer_ );
	context_->VSSetConstantBuffers( 2, 1, &modelConstantBuffer_ );
	SetSampler( SAMPLER_ANISOTROPIC, ST_FRAGMENT );
	SetSampler( SAMPLER_SHADOW, ST_FRAGMENT, 2 );

	numBatches_ = 0;
//	numCachedVerts_ = 0;
}

void Renderer::SetChunkDrawingState()
{
	SetTexture( TEXTURE::BLOCKS_OPAQUE, ST_FRAGMENT, 0 );
	SetTexture( TEXTURE::LIGHT_COLOR, ST_FRAGMENT, 2 );

	SetShader( SHADER::SHADER_BLOCK );
	context_->PSSetConstantBuffers( 1, 1, &frameConstantBuffer_ );
}

void Renderer::DrawVertexBuffer( VertexBuffer * vBuffer, uint slot, int x, int z )
{
	uint stride = vBuffer->strides_[ slot ];
	uint numVertices = vBuffer->sizes_[ slot ];
	uint offset = 0;
	context_->IASetVertexBuffers( 0, 1, vBuffer->GetBuffer( slot ), &stride, &offset );

	ModelCB modelCBData;
	modelCBData.translate = XMFLOAT4( (float)x, 0.0f, (float)z, 0.0f );
	context_->UpdateSubresource( modelConstantBuffer_, 0, NULL, &modelCBData, sizeof( ModelCB ), 0 );

	context_->Draw( numVertices, 0 );
}

void Renderer::Draw( uint vertexCount )
{
	context_->Draw( vertexCount, 0 );
}

#if 0
void Renderer::DrawChunkMesh( int x, int z, BlockVertex *vertices, int numVertices )
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

	uint stride = sizeof( BlockVertex );
	uint offset = 0;
	context_->IASetVertexBuffers( 0, 1, &blockVB_, &stride, &offset );

	//SetShader( shaders_[0] );

	context_->Draw( numVertices, numCachedVerts_ );

	numBatches_++;

	ProfileStop();

	numCachedVerts_ += numVertices;
}
#endif

void Renderer::ClearTexture( RenderTarget *rt, float r, float g, float b, float a )
{
	float clearColor[ 4 ] = { r, g, b, a };
	context_->ClearRenderTargetView( *rt->GetRTV(), clearColor );
}
void Renderer::ClearTexture( DepthBuffer *db, float d )
{
	for( uint i = 0; i < db->GetArraySize(); ++i )
	{
		context_->ClearDepthStencilView( db->GetDSV( i ), D3D11_CLEAR_DEPTH, d, 0 );
	}
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

	SetRenderTarget( 0, 0 );

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
#if 1
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

#else
//	DepthBuffer db;
	db.Init( desc.BufferDesc.Width, desc.BufferDesc.Height,
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				desc.SampleDesc.Count, desc.SampleDesc.Quality,
				false,
				device_ );
#endif

	SetRenderTarget();

	screenViewport_.Width = (float)desc.BufferDesc.Width;
	screenViewport_.Height = (float)desc.BufferDesc.Height;
	screenViewport_.MinDepth = 0.0f;
	screenViewport_.MaxDepth = 1.0f;
	screenViewport_.TopLeftX = 0;
	screenViewport_.TopLeftY = 0;
	context_->RSSetViewports( 1, &screenViewport_ );

	D3D11_SUBRESOURCE_DATA cbInitData;
	GlobalCB cbData;
	MakeGlobalCBInitData( &cbData, &cbInitData, screenViewport_.Width, screenViewport_.Height, (float)(Config.viewDistanceChunks * CHUNK_WIDTH) );

	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create global constant buffer!" );
		return false;
	}

	if( globalConstantBuffer_ ) {
		context_->UpdateSubresource( globalConstantBuffer_, 0, NULL, &cbData, sizeof( GlobalCB ), 0 );
	}

	return true;
} // ResizeBuffers

void Renderer::SetView( XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 up )
{
	viewPosition_ = pos;
	viewDirection_ = dir;
	viewUp_ = up;
}

void Renderer::SetViewport( D3D11_VIEWPORT *viewport )
{
	if( !viewport )
	{
		context_->RSSetViewports( 1, &screenViewport_ );
	}
	else
	{
		context_->RSSetViewports( 1, viewport );
	}
}

void Renderer::SetFrameCBuffer( FrameCB data )
{
	context_->UpdateSubresource( frameConstantBuffer_, 0, NULL, &data, sizeof( FrameCB ), 0 );
	context_->VSSetConstantBuffers( 1, 1, &frameConstantBuffer_ );
}

void Renderer::SetLightCBuffer( LightCB data )
{
	context_->UpdateSubresource( lightConstantBuffer_, 0, NULL, &data, sizeof( LightCB ), 0 );
	context_->VSSetConstantBuffers( 1, 1, &lightConstantBuffer_ );
}

void Renderer::SetGameObjectCBuffer( GameObjectCB &data )
{
	context_->UpdateSubresource( gameObjectConstantBuffer_, 0, NULL, &data, sizeof( GameObjectCB ), 0 );
	context_->VSSetConstantBuffers( 2, 1, &gameObjectConstantBuffer_ );
}

void Renderer::SetVertexBuffer( ResourceHandle id, uint slot )
{
	VertexBuffer * vBuffer = gTempVBStorage[ id ];
	uint stride = vBuffer->strides_[ slot ];
	//uint numVertices = vBuffer->sizes_[ slot ];
	uint offset = 0;
	context_->IASetVertexBuffers( 0, 1, vBuffer->GetBuffer( slot ), &stride, &offset );
}

// Render state

ResourceHandle Renderer::CreateDepthStencilState( DepthStateDesc depthStateDesc, StencilStateDesc stencilStateDesc )
{
	assert( nextFreeDepthStencilSlot < MAX_DEPTH_STENCIL_STATES );
	ResourceHandle result = nextFreeDepthStencilSlot;
	++nextFreeDepthStencilSlot;

	D3D11_DEPTH_WRITE_MASK mask;
	mask = depthStateDesc.readonly ? D3D11_DEPTH_WRITE_MASK_ZERO : D3D11_DEPTH_WRITE_MASK_ALL;

	D3D11_DEPTH_STENCIL_DESC depthDesc;
	ZeroMemory( &depthDesc, sizeof( depthDesc ) );
	depthDesc.DepthEnable 		= depthStateDesc.enabled;
	depthDesc.DepthWriteMask 	= mask;
	depthDesc.DepthFunc 		= (D3D11_COMPARISON_FUNC)depthStateDesc.comparisonFunction;

	depthDesc.StencilEnable 	= stencilStateDesc.enabled;
	depthDesc.StencilReadMask 	= stencilStateDesc.readMask;
	depthDesc.StencilWriteMask 	= stencilStateDesc.writeMask;

	depthDesc.BackFace.StencilFunc 			= (D3D11_COMPARISON_FUNC)stencilStateDesc.backFace.comparisonFunction;
	depthDesc.BackFace.StencilFailOp 		= (D3D11_STENCIL_OP)stencilStateDesc.backFace.stencilFail;
	depthDesc.BackFace.StencilPassOp 		= (D3D11_STENCIL_OP)stencilStateDesc.backFace.bothPass;
	depthDesc.BackFace.StencilDepthFailOp 	= (D3D11_STENCIL_OP)stencilStateDesc.backFace.stencilPassDepthFail;

	depthDesc.FrontFace.StencilFunc 		= (D3D11_COMPARISON_FUNC)stencilStateDesc.frontFace.comparisonFunction;
	depthDesc.FrontFace.StencilFailOp 		= (D3D11_STENCIL_OP)stencilStateDesc.frontFace.stencilFail;
	depthDesc.FrontFace.StencilPassOp 		= (D3D11_STENCIL_OP)stencilStateDesc.frontFace.bothPass;
	depthDesc.FrontFace.StencilDepthFailOp 	= (D3D11_STENCIL_OP)stencilStateDesc.frontFace.stencilPassDepthFail;

	HRESULT hr = device_->CreateDepthStencilState( &depthDesc, &depthStencilStates_[ result ] );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create depth-stencil state!" );
		result = INVALID_HANDLE;
	}

	return result;
}

ResourceHandle Renderer::CreateRasterizerState( RasterizerStateDesc rasterizerStateDesc )
{
	assert( nextFreeRasterizerSlot < MAX_DEPTH_STENCIL_STATES );
	ResourceHandle result = nextFreeRasterizerSlot;
	++nextFreeRasterizerSlot;

	D3D11_RASTERIZER_DESC desc;
	ZeroMemory( &desc, sizeof( desc ) );

	desc.FillMode = (D3D11_FILL_MODE)rasterizerStateDesc.fillMode;
	desc.CullMode = (D3D11_CULL_MODE)rasterizerStateDesc.cullMode;
	desc.FrontCounterClockwise = rasterizerStateDesc.frontCCW;
	desc.DepthBias = rasterizerStateDesc.depthBias;
	desc.DepthBiasClamp = rasterizerStateDesc.depthBiasClamp;
	desc.SlopeScaledDepthBias = rasterizerStateDesc.slopeScaledDepthBias;
	desc.DepthClipEnable = rasterizerStateDesc.depthClipEnabled;
	desc.ScissorEnable = rasterizerStateDesc.scissorEnabled;
	desc.MultisampleEnable = rasterizerStateDesc.multisampleEnabled;
	desc.AntialiasedLineEnable = rasterizerStateDesc.antialiasedLineEnabled;

	HRESULT hr =  device_->CreateRasterizerState( &desc, &rasterizerStates_[ result ] );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create default rasterizer state!" );
		result = INVALID_HANDLE;
	}

	return result;
}

ResourceHandle Renderer::CreateVertexBufferForMesh( Mesh * mesh )
{
	ResourceHandle result = nextFreeVBSlot;
	gTempVBStorage[ nextFreeVBSlot ] = new VertexBuffer();
	if( !gTempVBStorage[ nextFreeVBSlot ]->Create( 0, sizeof( VertexPosNormalTexcoord ), mesh->vertexCount, device_, mesh->vertices ) )
	{
		return INVALID_HANDLE;
	}

	nextFreeVBSlot++;

	return result;
}

void Renderer::SetDepthStencilState( ResourceHandle handle, uint stencilReference )
{
	context_->OMSetDepthStencilState( depthStencilStates_[ handle ], stencilReference );
}

void Renderer::SetRasterizerState( ResourceHandle handle )
{
	context_->RSSetState( rasterizerStates_[ handle ] );
}

void Renderer::SetRenderTarget( RenderTarget *rt, DepthBuffer *db, uint dbSlice )
{
	if( rt && db )
	{
		context_->OMSetRenderTargets( 1, rt->GetRTV(), db->GetDSV( dbSlice ) );
	}
	else if( rt )
	{
		context_->OMSetRenderTargets( 1, rt->GetRTV(), NULL );
	}
	else if( db )
	{
		context_->OMSetRenderTargets( 0, NULL, db->GetDSV( dbSlice ) );
	}
}

void Renderer::SetRenderTarget()
{
	context_->OMSetRenderTargets( 1, &backBufferView_, depthStencilView_ );
}

void Renderer::SetSampler( SAMPLER_TYPE st, SHADER_TYPE shader, uint slot )
{
	uint numSamplers = 1;

	if( ( shader & ST_VERTEX ) == ST_VERTEX ) {
		context_->VSSetSamplers( slot, numSamplers, &samplers_[ st ] );
	}
	if( ( shader & ST_GEOMETRY ) == ST_GEOMETRY ) {
		context_->GSSetSamplers( slot, numSamplers, &samplers_[ st ] );
	}
	if( ( shader & ST_FRAGMENT ) == ST_FRAGMENT ) {
		context_->PSSetSamplers( slot, numSamplers, &samplers_[ st ] );
	}
}

void Renderer::SetBlendMode( BLEND_MODE bm )
{
	UINT sampleMask   = 0xffffffff;
	context_->OMSetBlendState( blendStates_[ bm ], NULL, sampleMask );
}

#if 0
void Renderer::SetMesh( const Mesh& mesh )
{
	uint stride = sizeof( VertexPosNormalTexcoord );
	uint offset = 0;
	context_->IASetVertexBuffers( 0, 1, &mesh.vertexBuffer_, &stride, &offset );
}
#endif

#if 0
void Renderer::SetShader( const Shader& shader )
{
	context_->IASetInputLayout( shader.inputLayout_ );
	context_->VSSetShader( shader.vertexShader_, NULL, 0 );
	context_->PSSetShader( shader.pixelShader_, NULL, 0 );
}
#endif

void Renderer::SetShader( SHADER shaderID )
{
	context_->IASetInputLayout( gTempShaderStorage[ shaderID ].inputLayout_ );
	context_->VSSetShader( gTempShaderStorage[ shaderID ].vertexShader_, NULL, 0 );
	context_->PSSetShader( gTempShaderStorage[ shaderID ].pixelShader_, NULL, 0 );
}

void Renderer::SetTexture( TEXTURE id, SHADER_TYPE shader, uint slot )
{
	if( ( shader & ST_VERTEX ) == ST_VERTEX ) {
		context_->VSSetShaderResources( slot, 1, &gTempTextureStorage[id].textureView_ );
	}
	if( ( shader & ST_GEOMETRY ) == ST_GEOMETRY ) {
		context_->GSSetShaderResources( slot, 1, &gTempTextureStorage[id].textureView_ );
	}
	if( ( shader & ST_FRAGMENT ) == ST_FRAGMENT ) {
		context_->PSSetShaderResources( slot, 1, &gTempTextureStorage[id].textureView_ );
	}
}

void Renderer::SetTexture( DepthBuffer& texture, SHADER_TYPE shader, uint slot )
{
	if( ( shader & ST_VERTEX ) == ST_VERTEX ) {
		context_->VSSetShaderResources( slot, 1, texture.GetSRV() );
	}
	if( ( shader & ST_GEOMETRY ) == ST_GEOMETRY ) {
		context_->GSSetShaderResources( slot, 1, texture.GetSRV() );
	}
	if( ( shader & ST_FRAGMENT ) == ST_FRAGMENT ) {
		context_->PSSetShaderResources( slot, 1, texture.GetSRV() );
	}
}

void Renderer::SetTexture( RenderTarget& texture, SHADER_TYPE shader, uint slot )
{
	if( ( shader & ST_VERTEX ) == ST_VERTEX ) {
		context_->VSSetShaderResources( slot, 1, texture.GetSRV() );
	}
	if( ( shader & ST_GEOMETRY ) == ST_GEOMETRY ) {
		context_->GSSetShaderResources( slot, 1, texture.GetSRV() );
	}
	if( ( shader & ST_FRAGMENT ) == ST_FRAGMENT ) {
		context_->PSSetShaderResources( slot, 1, texture.GetSRV() );
	}
}

void Renderer::RemoveTexture( SHADER_TYPE shader, uint slot )
{
	ID3D11ShaderResourceView *nullSRV = { 0 };
	if( ( shader & ST_VERTEX ) == ST_VERTEX ) {
		context_->VSSetShaderResources( slot, 1, &nullSRV );
	}
	if( ( shader & ST_GEOMETRY ) == ST_GEOMETRY ) {
		context_->GSSetShaderResources( slot, 1, &nullSRV );
	}
	if( ( shader & ST_FRAGMENT ) == ST_FRAGMENT ) {
		context_->PSSetShaderResources( slot, 1, &nullSRV );
	}
}

void MakeGlobalCBInitData( GlobalCB *cbData, D3D11_SUBRESOURCE_DATA *cbInitData, float screenWidth, float screenHeight, float viewDistance )
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

	cbData->texcoords[0] = { 0.0f, 0.0f, 0.0f, 0.0f }; // top left
	cbData->texcoords[1] = { 0.0f, 1.0f, 0.0f, 0.0f }; // bottom left
	cbData->texcoords[2] = { 1.0f, 0.0f, 0.0f, 0.0f }; // top right
	cbData->texcoords[3] = { 1.0f, 1.0f, 0.0f, 0.0f }; // bottom right

	cbData->viewDistance = viewDistance;

	cbInitData->pSysMem = cbData;
	cbInitData->SysMemPitch = sizeof( GlobalCB );
	cbInitData->SysMemSlicePitch = 0;
}

bool LoadTextures( ID3D11Device * device, ID3D11DeviceContext * context )
{
	for( int i = 0; i < TEXTURE::TEXTURE_COUNT; i++ )
	{
		if( !gTextureDefinitions[i].width )
			continue;

		// gTempTextureStorage[i] = new Texture();
		gTempTextureStorage[i] = Texture();
		if( !gTempTextureStorage[i].Load( gTextureDefinitions[i], device, context ) )
		{
			return false;
		}
	}
	return true;
}

bool LoadShaders( ID3D11Device * device )
{
	for( int i = 0; i < SHADER::SHADER_COUNT; i++ )
	{
		gTempShaderStorage[i] = Shader();
		if( !gTempShaderStorage[i].Load( gShaderDefinitions[i].filename, device ) )
		{
			return false;
		}
	}
	return true;
}

bool LoadMeshes( Renderer * renderer )
{
	float positions[ MAX_VERTS_PER_MESH * 3 ];
	float normals[ MAX_VERTS_PER_MESH * 3 ];
	float texcoords[ MAX_VERTS_PER_MESH * 2 ];

	for( int i = 0; i < MESH::MESH_COUNT; i++ )
	{
		int vertexCount = LoadObjFromFile( gMeshDefinitions[i].filename, positions, normals, texcoords );
		gTempMeshStorage[i] = Mesh();
		gTempMeshStorage[i].Create( positions, normals, texcoords, vertexCount );

		ResourceHandle meshVB = renderer->CreateVertexBufferForMesh( &gTempMeshStorage[i] );
		if( meshVB == INVALID_HANDLE )
		{
			OutputDebugStringA( "Failed to create mesh vertex buffer" );
			return false;
		}

		gTempMeshStorage[i].vertexBufferID = meshVB;

		gTempMeshStorage[i].material.shaderID = gMaterialDefinitions[ gMeshDefinitions[i].material ].shaderID;
		gTempMeshStorage[i].material.textureID = gMaterialDefinitions[ gMeshDefinitions[i].material ].diffuseTextureID;
	}

	return true;
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
		RELEASE( vShaderBytecode );
		return false;
	}

	RELEASE( vShaderBytecode );

	ID3DBlob *pShaderBytecode = 0;
	if( !LoadShader( filename, PIXEL_SHADER_ENTRY, "ps_4_1", &pShaderBytecode ) )
	{
		OutputDebugStringA( "No pixel shader!" );
		RELEASE( pShaderBytecode );
		pixelShader_ = 0;
		return true;
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
	texture_ = 0;
	textureView_ = 0;
}

Texture::~Texture()
{
	RELEASE( texture_ );
	RELEASE( textureView_ );
}

bool Texture::LoadFile( wchar_t* filename, ID3D11Device *device )
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

bool Texture::Load( const TextureDefinition & textureDefinition,
				  	ID3D11Device * device, ID3D11DeviceContext * context )
{
	HRESULT hr;

	int arraySize = textureDefinition.arraySize;

	uint8 ** textureData = new uint8*[ arraySize ];

	int textureWidth = 0, textureHeight = 0, textureComponentsPerPixel = 0;
	for( int i = 0; i < arraySize; i++ )
	{
		textureData[i] = stbi_load( textureDefinition.filenames[i], &textureWidth, &textureHeight,
									&textureComponentsPerPixel, 4 );

		if( textureData[i] == 0 )
		{
			OutputDebugStringA( "Failed to load PNG image" );

			delete[] textureData;

			return false;
		}

		// sanity check
		if( (uint)textureWidth != textureDefinition.width ||
			(uint)textureHeight != textureDefinition.height )
		{
			OutputDebugStringA( "Incorrect image size, can't load!" );

			delete[] textureData;

			return false;
		}
	}

	DXGI_FORMAT format;
	switch( textureComponentsPerPixel )
	{
		case 1:
			format = DXGI_FORMAT_R8_UNORM;
			break;
		case 3:
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case 4:
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		default:
			OutputDebugStringA( "Unsupported number of channels per pixel!" );
			delete[] textureData;
			return false;
	}

	UINT mipLevels = 0;
	UINT bindFlags = 0;
	UINT miscFlags = 0;
	if( textureDefinition.generateMips )
	{
		mipLevels = 0; // storage will be allocated for a full mip chain
		bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		miscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	else
	{
		mipLevels = 1;
		bindFlags = D3D11_BIND_SHADER_RESOURCE;
	}

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory( &textureDesc, sizeof( textureDesc ) );

	textureDesc.Width = textureWidth;
	textureDesc.Height = textureHeight;
	textureDesc.MipLevels = mipLevels;
	textureDesc.ArraySize = arraySize;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = bindFlags;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = miscFlags;

	hr = device->CreateTexture2D( &textureDesc, NULL, &texture_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create block texture array" );
		delete[] textureData;
		return false;
	}

	// get the actual number of mip levels in a texture
	D3D11_TEXTURE2D_DESC createdDesc;
	texture_->GetDesc( &createdDesc );
	mipLevels = createdDesc.MipLevels;

	// update data for mip level 0 for every array slice
	for( int i = 0; i < arraySize; i++ )
	{
		UINT subresourceIndex =  D3D11CalcSubresource( 0, i, mipLevels );

		context->UpdateSubresource(
			texture_,
			subresourceIndex,
			NULL,
			textureData[i],
			textureWidth * 4,
			textureWidth * textureHeight * 4
		);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	ZeroMemory( &viewDesc, sizeof( viewDesc ) );

	viewDesc.Format = textureDesc.Format;
	switch( textureDefinition.type )
	{
		case TEXTURE_TYPE::SINGLE:
			viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MostDetailedMip = 0;
			viewDesc.Texture2D.MipLevels = (UINT)-1;
			break;
		case TEXTURE_TYPE::ARRAY:
			viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			viewDesc.Texture2DArray.MostDetailedMip = 0;
			viewDesc.Texture2DArray.MipLevels = (UINT)-1;
			viewDesc.Texture2DArray.FirstArraySlice = 0;
			viewDesc.Texture2DArray.ArraySize = textureDesc.ArraySize;
			break;
	}

	hr = device->CreateShaderResourceView( texture_, &viewDesc, &textureView_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to create block texture array view" );
		return false;
	}

	if( textureDefinition.generateMips )
	{
		context->GenerateMips( textureView_ );
	}

	for( int i = 0; i < arraySize; i++ )
		stbi_image_free( textureData[i] );
	delete[] textureData;

	return true;
}

//********************************
// Render target
//********************************

RenderTarget::RenderTarget()
{
	renderTargetView_ = 0;
	shaderResourceView_ = 0;
}

RenderTarget::~RenderTarget()
{
	RELEASE( shaderResourceView_ );
	RELEASE( renderTargetView_ );
}

bool RenderTarget::Init( uint width, uint height, DXGI_FORMAT format, bool shaderAccess, ID3D11Device *device )
{
	HRESULT hr;

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory( &texDesc, sizeof( texDesc ) );
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	if( shaderAccess )
	{
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	}
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D *texture = 0;
	hr = device->CreateTexture2D( &texDesc, NULL, &texture );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "RenderTarget::Init() failed to create RT texture!" );
		return false;
	}

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory( &rtvDesc, sizeof( rtvDesc ) );
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	hr = device->CreateRenderTargetView( texture, &rtvDesc, &renderTargetView_ );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "RenderTarget::Init() failed to create render target!" );
		return false;
	}

	if( shaderAccess )
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof( srvDesc ) );
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		hr = device->CreateShaderResourceView( texture, &srvDesc, &shaderResourceView_ );
		if( FAILED( hr ) )
		{
			OutputDebugStringA( "RenderTarget::Init() failed to create shader resource view!" );
			return false;
		}
	}

	return true;
}

DepthBuffer::DepthBuffer( uint arraySize )
{
	assert( "Array size cannot be zero or negative" && arraySize );
	arraySize_ = arraySize;
	shaderResourceView_ = 0;
	depthStencilViews_ = new ID3D11DepthStencilViewArray[ arraySize ];
}
DepthBuffer::~DepthBuffer()
{
	RELEASE( shaderResourceView_ );
	for( uint i = 0; i < arraySize_; ++i )
	{
		RELEASE( depthStencilViews_[i] );
	}
	delete[] depthStencilViews_;
}

bool DepthBuffer::Init( uint width, uint height, DXGI_FORMAT format, uint msCount, uint msQuality, bool shaderAccess, ID3D11Device *device )
{
	assert( "Texture width/height cannot be zero or negative" && width && height );
	HRESULT hr;

	DXGI_FORMAT formatTexture = (DXGI_FORMAT)0;
	DXGI_FORMAT formatSRV = (DXGI_FORMAT)0;
	switch( format )
	{
		case DXGI_FORMAT_D32_FLOAT:
			formatTexture = DXGI_FORMAT_R32_TYPELESS;
			formatSRV = DXGI_FORMAT_R32_FLOAT;
			break;

		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			formatTexture = DXGI_FORMAT_R24G8_TYPELESS;
			formatSRV = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;

		case DXGI_FORMAT_D16_UNORM:
			formatTexture = DXGI_FORMAT_R16_TYPELESS;
			formatSRV = DXGI_FORMAT_R16_UNORM;
			break;

		default:
			return false;
	}

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory( &texDesc, sizeof( texDesc ) );
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = arraySize_;
	texDesc.Format = formatTexture;
	texDesc.SampleDesc.Count = msCount;
	texDesc.SampleDesc.Quality = msQuality;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	if( shaderAccess )
	{
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	}
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D *texture = 0;
	hr = device->CreateTexture2D( &texDesc, NULL, &texture );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "DepthBuffer::Init() failed to create DB texture!" );
		return false;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory( &dsvDesc, sizeof( dsvDesc ) );
	dsvDesc.Format = format;

	if( arraySize_ == 1 )
	{
		if( msCount > 1 )
		{
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
		}
		hr = device->CreateDepthStencilView( texture, &dsvDesc, &depthStencilViews_[0] );
		if( FAILED( hr ) )
		{
			OutputDebugStringA( "DepthBuffer::Init() failed to create depth stencil view!" );
			return false;
		}
	}
	else
	{
		for( uint i = 0; i < arraySize_; ++i )
		{
			if( msCount > 1 )
			{
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
				dsvDesc.Texture2DMSArray.FirstArraySlice = i;
				dsvDesc.Texture2DMSArray.ArraySize = 1;
			}
			else
			{
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				dsvDesc.Texture2DArray.ArraySize = 1;
				dsvDesc.Texture2DArray.MipSlice = 0;
			}
			hr = device->CreateDepthStencilView( texture, &dsvDesc, &depthStencilViews_[i] );
			if( FAILED( hr ) )
			{
				OutputDebugStringA( "DepthBuffer::Init() failed to create depth stencil view!" );
				return false;
			}
		}
	}


	if( shaderAccess )
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof( srvDesc ) );
		srvDesc.Format = formatSRV;

		if( arraySize_ == 1 )
		{
			if( msCount > 1 )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = 1;
			}
		}
		else
		{
			if( msCount > 1 )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				srvDesc.Texture2DMSArray.FirstArraySlice = 0;
				srvDesc.Texture2DMSArray.ArraySize = arraySize_;
			}
			else
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.MipLevels = 1;
				srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize = arraySize_;
			}
		}

		hr = device->CreateShaderResourceView( texture, &srvDesc, &shaderResourceView_ );
		if( FAILED( hr ) )
		{
			OutputDebugStringA( "DepthBuffer::Init() failed to create shader resource view!" );
			return false;
		}
	}

	return true;
}

//********************************
// Mesh
//********************************
Mesh::Mesh()
{
	vertexCount = 0;
	vertices = 0;
	vertexBufferID = INVALID_HANDLE;
}

Mesh::~Mesh()
{
	if( vertices )
		delete[] vertices;
}

bool Mesh::Create( float * positions, float * normals, float * texcoords, uint numVertices )
{
	vertexCount = numVertices;
	vertices = new VertexPosNormalTexcoord[ numVertices ];

	for( uint i = 0; i < numVertices; i++ )
	{
		vertices[i].position[0] = positions[ i*3 + 0 ];
		vertices[i].position[1] = positions[ i*3 + 1 ];
		vertices[i].position[2] = positions[ i*3 + 2 ];

		vertices[i].normal[0] = normals[ i*3 + 0 ];
		vertices[i].normal[1] = normals[ i*3 + 1 ];
		vertices[i].normal[2] = normals[ i*3 + 2 ];

		vertices[i].texcoord[0] = texcoords[ i*2 + 0 ];
		vertices[i].texcoord[1] = texcoords[ i*2 + 1 ];
	}

	return true;
}

//**************************************************************
// Vertex Buffer
//**************************************************************
VertexBuffer::VertexBuffer( uint arraySize )
{
	numBuffers_ = arraySize;
	buffers_ = new ID3D11Buffer * [ arraySize ];
	for( uint i = 0; i < numBuffers_; i ++ )
	{
		buffers_[i] = 0;
	}
	sizes_ = new uint[ arraySize ];
	strides_ = new uint[ arraySize ];
}

VertexBuffer::~VertexBuffer()
{
	for( uint i = 0; i < numBuffers_; i ++ )
	{
		RELEASE( buffers_[i] );
	}
	delete[] buffers_;
	delete[] strides_;
	delete[] sizes_;
}

bool VertexBuffer::Create( uint slot,
						   uint vertexSize,
						   uint numVertices,
						   ID3D11Device * device,
						   void * vertices )
{
	return Create(	slot,
					vertexSize,
					numVertices,
					vertices,
					RESOURCE_USAGE::DEFAULT,
					CPU_ACCESS::NONE,
					device );
}

bool VertexBuffer::Create( 	uint slot,
							uint vertexSize,
							uint numVertices,
							void * vertices,
							RESOURCE_USAGE usage,
							CPU_ACCESS access,
							ID3D11Device * device )
{
	HRESULT hr;

	strides_[ slot ] = vertexSize;
	sizes_[ slot ] = numVertices;

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage            = (D3D11_USAGE)usage;
	bufferDesc.ByteWidth        = vertexSize * numVertices;
	bufferDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags   = (UINT)access;
	bufferDesc.MiscFlags        = 0;

	if( vertices )
	{
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = vertices;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;
		hr = device->CreateBuffer( &bufferDesc, &initData, &buffers_[ slot ] );
	}
	else
	{
		hr = device->CreateBuffer( &bufferDesc, NULL, &buffers_[ slot ] );
	}

	if( FAILED( hr ) )
	{
		return false;
	}
	return true;
}

void VertexBuffer::Release( uint slot )
{
	RELEASE( buffers_[ slot ] );
}

//********************************
// Debug overlay
//********************************
Overlay::Overlay()
{
	renderer_ = 0;
	textVB_ = 0;
	primitiveVB_ = 0;
	constantBuffer_ = 0;

	depthStateRead_ = 0;
	depthStateDisabled_ = 0;

	lineNumber_ = 0;
	lineOffset_ = 0;

	currentColor_ = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
}

Overlay::~Overlay()
{
	RELEASE( constantBuffer_ );
	RELEASE( textVB_ );
	RELEASE( primitiveVB_ );
}

bool Overlay::Start( Renderer *renderer )
{
	HRESULT hr;

	renderer_ = renderer;

	// Create depth states
	DepthStateDesc depthStateDesc = {0};
	depthStateDesc.enabled = true;
	depthStateDesc.readonly = true;
	depthStateDesc.comparisonFunction = COMPARISON_FUNCTION::LESS_EQUAL;
	StencilStateDesc stencilStateDesc = {0};
	stencilStateDesc.enabled = false;
	depthStateRead_ = renderer->CreateDepthStencilState( depthStateDesc, stencilStateDesc );
	if( depthStateRead_ == INVALID_HANDLE )
	{
		return false;
	}

	depthStateDesc.enabled = false;
	depthStateDisabled_ = renderer->CreateDepthStencilState( depthStateDesc, stencilStateDesc );
	if( depthStateRead_ == INVALID_HANDLE )
	{
		return false;
	}

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

	renderer_->SetDepthStencilState( depthStateDisabled_ );
	renderer_->SetRasterizerState( gDefaultRasterizerState );
	renderer_->SetBlendMode( BM_ALPHA );
	renderer_->context_->IASetVertexBuffers( 0, 1, &textVB_, &stride, &offset );
//	renderer_->SetTexture( texture_, ST_FRAGMENT );
	renderer_->SetTexture( TEXTURE::FONT, ST_FRAGMENT );
	renderer_->SetSampler( SAMPLER_POINT, ST_FRAGMENT );
	renderer_->context_->PSSetConstantBuffers( 1, 1, &constantBuffer_ );
	renderer_->SetShader( SHADER_TEXT );

	renderer_->context_->Draw( textLength * 6, 0 );

	renderer_->SetBlendMode( BM_DEFAULT );
}

float Overlay::GetCharOffset( char c )
{
	std::string alpha = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890+-*/=><()_\"';:.,!?@#[] ";

	int index = (int)alpha.find_first_of( c );

	return (float)index * NORMALIZED_CHAR_WIDTH;
}

void Overlay::DrawCrosshair( int x, int y, CROSSHAIR_STATE state )
{
	float textureOffset = 0.0f;
	switch( state )
	{
		case CROSSHAIR_STATE::INACTIVE:
			textureOffset = 0.0f / CROSSHAIR_STATE::CHS_COUNT;
			break;
		case CROSSHAIR_STATE::ACTIVE:
			textureOffset = 1.0f / CROSSHAIR_STATE::CHS_COUNT;
			break;
	}
	OverlayVertex2D vertices[6];
	vertices[0] = { { x - 4.0f, y - 4.0f }, { 0.0f + textureOffset, 0.0f } };
	vertices[1] = { { x + 4.0f, y + 4.0f }, { 0.5f + textureOffset, 1.0f } };
	vertices[2] = { { x + 4.0f, y - 4.0f }, { 0.5f + textureOffset, 0.0f } };
	vertices[3] = { { x - 4.0f, y - 4.0f }, { 0.0f + textureOffset, 0.0f } };
	vertices[4] = { { x - 4.0f, y + 4.0f }, { 0.0f + textureOffset, 1.0f } };
	vertices[5] = { { x + 4.0f, y + 4.0f }, { 0.5f + textureOffset, 1.0f } };

	D3D11_MAPPED_SUBRESOURCE mapResource;
	HRESULT hr = renderer_->context_->Map( textVB_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapResource );
	if( FAILED( hr ) )
	{
		OutputDebugStringA( "Failed to map subresource!");
		return;
	}
	memcpy( mapResource.pData, vertices, sizeof( OverlayVertex2D ) * 6 );
	renderer_->context_->Unmap( textVB_, 0 );

	OverlayShaderCB	cbData;
	cbData.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	currentColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	renderer_->context_->UpdateSubresource( constantBuffer_, 0, NULL, &cbData, sizeof( OverlayShaderCB ), 0 );

	uint stride = sizeof( OverlayVertex2D );
	uint offset = 0;

	renderer_->SetDepthStencilState( depthStateDisabled_ );
	renderer_->SetRasterizerState( gDefaultRasterizerState );
	renderer_->SetBlendMode( BM_ALPHA );
	renderer_->context_->IASetVertexBuffers( 0, 1, &textVB_, &stride, &offset );
	renderer_->SetTexture( TEXTURE::CROSSHAIR, ST_FRAGMENT );
	renderer_->SetSampler( SAMPLER_POINT, ST_FRAGMENT );
	renderer_->context_->PSSetConstantBuffers( 1, 1, &constantBuffer_ );
	renderer_->SetShader( SHADER_TEXT );


	renderer_->context_->Draw( 6, 0 );

	renderer_->SetBlendMode( BM_DEFAULT );
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

	renderer_->SetDepthStencilState( depthStateRead_ );
	renderer_->SetRasterizerState( gDefaultRasterizerState );
	renderer_->SetBlendMode( BM_ALPHA );
	renderer_->context_->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
	renderer_->context_->IASetVertexBuffers( 0, 1, &primitiveVB_, &stride, &offset );
	renderer_->context_->VSSetConstantBuffers( 1, 1, &renderer_->frameConstantBuffer_ );
	renderer_->SetShader( SHADER_PRIMITIVE );

	renderer_->context_->Draw( 2, 0 );

	renderer_->context_->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	renderer_->SetBlendMode( BM_DEFAULT );
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

void Overlay::DrawFrustum( const Frustum & frustum, DirectX::XMFLOAT4 color )
{
	DrawLine( frustum.corners[0], frustum.corners[1], color );
	DrawLine( frustum.corners[1], frustum.corners[2], color );
	DrawLine( frustum.corners[2], frustum.corners[3], color );
	DrawLine( frustum.corners[3], frustum.corners[0], color );

	DrawLine( frustum.corners[4], frustum.corners[5], color );
	DrawLine( frustum.corners[5], frustum.corners[6], color );
	DrawLine( frustum.corners[6], frustum.corners[7], color );
	DrawLine( frustum.corners[7], frustum.corners[4], color );

	DrawLine( frustum.corners[0], frustum.corners[4], color );
	DrawLine( frustum.corners[1], frustum.corners[5], color );
	DrawLine( frustum.corners[2], frustum.corners[6], color );
	DrawLine( frustum.corners[3], frustum.corners[7], color );
}

//********************************
// Function definitions
//********************************
bool LoadShader( wchar_t *filename, const char *entry, const char *shaderModel, ID3DBlob **buffer )
{
	HRESULT hr;
	ID3DBlob *errorBuffer = 0;

	hr = D3DCompileFromFile( filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE , entry, shaderModel, 0, 0, buffer, &errorBuffer );
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


