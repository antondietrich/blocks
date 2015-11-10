#ifndef __BLOCKS_RENDERER__
#define __BLOCKS_RENDERER__

#include <d3d11.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <assert.h>

#include "renderer_def.h"
#include "DDSTextureLoader.h"
#include "types.h"
#include "world.h"
#include "config.h"
#include "utils.h"

namespace Blocks
{

bool LoadShader( wchar_t *filename, const char *entry, const char *shaderModel, ID3DBlob **buffer );
/* Based on the implementation by @BobbyAnguelov. Thank you! */
HRESULT CreateInputLayoutFromShaderBytecode( ID3DBlob* shaderBytecode, ID3D11Device* device, ID3D11InputLayout** inputLayout );

struct VertexPosition
{
	float pos[3];
};

struct VertexPosNormalTexcoord
{
	float pos[3];
	float normal[3];
	float texcoord[2];
};

//*************************
// constant buffers
//*************************
struct GlobalCB {
	DirectX::XMFLOAT4X4 screenToNDC;
	DirectX::XMFLOAT4 normals[6];
	DirectX::XMFLOAT4 texcoords[4];
	float viewDistance;
	float padding[3];
};

struct FrameCB {
	DirectX::XMFLOAT4X4 vp;
	DirectX::XMFLOAT4X4 lightVP;
	DirectX::XMFLOAT4 sunDirection;
	DirectX::XMFLOAT4 sunColor;
	DirectX::XMFLOAT4 ambientColor;
	float dayTimeNorm;
	float padding[3];
};

struct LightCB {
	DirectX::XMFLOAT4X4 vp;
};

struct ModelCB {
	DirectX::XMFLOAT4 translate;
};

class Shader
{
public:
	Shader();
	~Shader();

	bool Load( wchar_t* filename, ID3D11Device *device );

private:
	ID3D11VertexShader *vertexShader_;
	ID3D11PixelShader *pixelShader_;
	ID3D11InputLayout *inputLayout_;

	friend class Renderer;
};

class Texture
{
public:
	Texture();
	~Texture();

	bool LoadFile( wchar_t* filename, ID3D11Device *device );

private:
	ID3D11ShaderResourceView *textureView_;

	friend class Renderer;
};

class RenderTarget
{
public:
	RenderTarget();
	~RenderTarget();

	bool Init( uint width, uint height, DXGI_FORMAT format, bool shaderAccess, ID3D11Device *device );

	ID3D11RenderTargetView ** GetRTV() { return &renderTargetView_; };
	ID3D11ShaderResourceView ** GetSRV() { return &shaderResourceView_; };
private:
	ID3D11ShaderResourceView *shaderResourceView_;
	ID3D11RenderTargetView *renderTargetView_;
};

class DepthBuffer
{
public:
	DepthBuffer();
	~DepthBuffer();

	bool Init( uint width, uint height, DXGI_FORMAT format, uint msCount, uint msQuality, bool shaderAccess, ID3D11Device *device );

	ID3D11DepthStencilView * GetDSV() { return depthStencilView_; };
	ID3D11ShaderResourceView ** GetSRV() { return &shaderResourceView_; };
private:
	ID3D11ShaderResourceView *shaderResourceView_;
	ID3D11DepthStencilView *depthStencilView_;
};

class Mesh
{
public:
	Mesh();
	~Mesh();

	bool Load( const VertexPosNormalTexcoord *vertices, int numVertices, ID3D11Device *device );
private:
	ID3D11Buffer *vertexBuffer_;

	friend class Renderer;
};

struct Frustum
{
	DirectX::XMFLOAT3 corners[8];
};

//********************
// Renderer
//********************

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Start( HWND wnd );
	void Begin();
	void End();

	void SetChunkDrawingState();
	void DrawChunkMesh( int x, int z, BlockVertex *vertices, int numVertices );

	void ClearTexture( RenderTarget *rt, float r = 1.0f, float g = 0.0f, float b = 1.0f, float a = 1.0f );
	void ClearTexture( DepthBuffer *db, float d = 1.0f );

	/* Accessors */
	ID3D11Device * GetDevice() { return device_; };

	/* Window management */
	void ToggleFullscreen();
	bool Fullscreen();
	bool ResizeBuffers();
	int GetViewportWidth() {
		return (int)screenViewport_.Width;
	};
	int GetViewportHeight() {
		return (int)screenViewport_.Height;
	};

	/* render state */
	void SetRenderTarget( RenderTarget *rt, DepthBuffer *db );
	void SetRasterizer( RASTERIZER_STATE rs );
	void SetSampler( SAMPLER_TYPE st, SHADER_TYPE shader, uint slot = 0 );
	void SetBlendMode( BLEND_MODE bm );
	void SetDepthBufferMode( DEPTH_BUFFER_MODE bm );

	void SetView( DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, DirectX::XMFLOAT3 up );
	void SetViewport( D3D11_VIEWPORT *viewport );
	void SetFrameCBuffer( FrameCB data );
	void SetLightCBuffer( LightCB data );
	void SetMesh( const Mesh& mesh );
	void SetShader( const Shader& shader );
	void SetShader( uint shaderID );
	void SetTexture( const Texture& texture, SHADER_TYPE shader, uint slot = 0 );
	void SetTexture( RenderTarget& texture, SHADER_TYPE shader, uint slot = 0 );
	void RemoveTexture( SHADER_TYPE shader, uint slot = 0 );

	unsigned int numBatches_;
private:
	static bool isInstantiated_;
	unsigned int vsync_;

	ID3D11Device *device_;
	ID3D11DeviceContext *context_;
	IDXGISwapChain *swapChain_;
	ID3D11RenderTargetView *backBufferView_;
	ID3D11DepthStencilView *depthStencilView_;
	// ID3D11RasterizerState *defaultRasterizerState_;
	D3D11_VIEWPORT screenViewport_;
	ID3D11Buffer *globalConstantBuffer_;
	ID3D11Buffer *frameConstantBuffer_;
	ID3D11Buffer *lightConstantBuffer_;
	ID3D11Buffer *modelConstantBuffer_;

	ID3D11RasterizerState *rasterizerStates_[ NUM_RASTERIZER_STATES ];
	ID3D11SamplerState *samplers_[ NUM_SAMPLER_TYPES ];
	ID3D11BlendState *blendStates_[ NUM_BLEND_MODES ];
	ID3D11DepthStencilState *depthStencilStates_[ NUM_DEPTH_BUFFER_MODES ];

	ID3D11Buffer *blockVB_;
	BlockVertex *blockCache_;
	uint numCachedVerts_;

	Shader shaders_[ MAX_SHADERS ];
	Texture textures_[ MAX_TEXTURES ];
	Mesh meshes_[ MAX_MESHES ];

	/* main view */
	DirectX::XMFLOAT3 viewPosition_;
	DirectX::XMFLOAT3 viewDirection_;
	DirectX::XMFLOAT3 viewUp_;
	DirectX::XMFLOAT4X4 view_;
	DirectX::XMFLOAT4X4 projection_;

	friend class Overlay;
};

//******************************
// Debug overlay
//******************************

#define MAX_OVERLAY_CHARS 1024
#define OVERLAY_3DVB_SIZE 1024

// text metrics in pixels
#define TEXT_PADDING 10
#define LINE_HEIGHT 24
#define LINE_SPACING 4
#define CHAR_WIDTH 12
#define FONT_BITMAP_WIDTH 1024.0f
#define FONT_NUM_CHARS 86.0f
#define NORMALIZED_CHAR_WIDTH ( CHAR_WIDTH / FONT_BITMAP_WIDTH )
#define MAX_CHARS_IN_LINE 32
// #define FONT_CHAR_OFFSET 0.01171875f

struct OverlayVertex2D
{
	float pos[2];
	float texcoord[2];
};

struct OverlayVertex3D
{
	float pos[3];
	float color[4];
};

struct OverlayShaderCB
{
	DirectX::XMFLOAT4 color;
};

class Overlay
{
public:
	Overlay();
	~Overlay();
	bool Start( Renderer* renderer );
	void Reset();

	void Write( const char* fmt, ... );
	void WriteLine( const char* fmt, ... );
	void WriteUnformatted( const char* text );
	void WriteLineUnformatted( const char* text );
	void DisplayText( int x, int y, const char* text, DirectX::XMFLOAT4 color );

	void DrawLine( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B, DirectX::XMFLOAT4 color );
	void DrawLineDir( DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 dir, DirectX::XMFLOAT4 color );
	void DrawPoint( DirectX::XMFLOAT3 P, DirectX::XMFLOAT4 color );
	void OulineBlock( int chunkX, int chunkZ, int x, int y, int z, DirectX::XMFLOAT4 color={ 1.0f, 1.0f, 1.0f, 1.0f });

	float GetCharOffset( char c );

private:
	Renderer *renderer_;
	Shader textShader_;
	Shader primitiveShader_;
	ID3D11Buffer *textVB_;
	ID3D11Buffer *primitiveVB_;
	Texture texture_;
	ID3D11Buffer *constantBuffer_;

	uint lineNumber_;
	uint lineOffset_;
	DirectX::XMFLOAT4 currentColor_;
};

}

#endif // __BLOCKS_RENDERER__
