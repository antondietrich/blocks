#ifndef __BLOCKS_RENDERER__
#define __BLOCKS_RENDERER__

#include <d3d11.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <assert.h>

#include "DDSTextureLoader.h"
#include "types.h"
#include "world.h"
#include "config.h"
#include "utils.h"

namespace Blocks
{

#define VERTEX_SHADER_ENTRY "VSMain"
#define PIXEL_SHADER_ENTRY "PSMain"

#define BACK_BUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

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
};

struct FrameCB {
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

	bool Load( wchar_t* filename, ID3D11Device *device );

private:
	ID3D11ShaderResourceView *textureView_;

	friend class Renderer;
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

//********************
// Renderer
//********************

#define MAX_SHADERS 8
#define MAX_TEXTURES 8
#define MAX_MESHES 8

#define VERTS_PER_BLOCK 36
#define VERTS_PER_FACE 6
// #define MAX_VERTS_PER_BATCH 9216 * 8 // up to 1024 blocks
#define MAX_VERTS_PER_BATCH 50000 * 256 * 2
// #define MAX_VERTS_PER_BATCH MAX_VERTS_PER_CHUNK_MESH

enum SAMPLER_TYPE
{
	SAMPLER_POINT,
	SAMPLER_LINEAR,
	SAMPLER_ANISOTROPIC,
	NUM_SAMPLER_TYPES
};

enum BLEND_MODE
{
	BM_DEFAULT,
	BM_ALPHA,
	NUM_BLEND_MODES
};

enum DEPTH_BUFFER_MODE
{
	DB_ENABLED,
	DB_DISABLED,
	DB_READ,
	NUM_DEPTH_BUFFER_MODES
};

enum SHADER_TYPE
{
	ST_VERTEX = 1,
	ST_GEOMETRY = 2,
	ST_FRAGMENT = 4,
};

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Start( HWND wnd );
	void Begin();
	void End();
	void Draw( uint vertexCount, uint startVertexOffset = 0 );
	void DrawCube( DirectX::XMFLOAT3 offset );
	void DrawChunk( int x, int z, BlockVertex *vertices, int numVertices );

	void Flush();

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

	void SetSampler( SAMPLER_TYPE st );
	void SetBlendMode( BLEND_MODE bm );
	void SetDepthBufferMode( DEPTH_BUFFER_MODE bm );

	void SetView( DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, DirectX::XMFLOAT3 up );
	void SetMesh( const Mesh& mesh );
	void SetShader( const Shader& shader );
	void SetTexture( const Texture& texture, SHADER_TYPE shader, uint slot = 0 );

	unsigned int numBatches_;
private:
	static bool isInstantiated_;
	unsigned int vsync_;

	ID3D11Device *device_;
	ID3D11DeviceContext *context_;
	IDXGISwapChain *swapChain_;
	ID3D11RenderTargetView *backBufferView_;
	ID3D11DepthStencilView *depthStencilView_;
	ID3D11RasterizerState *defaultRasterizerState_;
	D3D11_VIEWPORT screenViewport_;
	ID3D11Buffer *globalConstantBuffer_;
	ID3D11Buffer *frameConstantBuffer_;
	ID3D11Buffer *modelConstantBuffer_;
	//ID3D11SamplerState *linearSampler_;

	ID3D11SamplerState *samplers_[ NUM_SAMPLER_TYPES ];
	ID3D11BlendState *blendStates_[ NUM_BLEND_MODES ];
	ID3D11DepthStencilState *depthStencilStates_[ NUM_DEPTH_BUFFER_MODES ];

	ID3D11Buffer *blockVB_;
//	BlockVertex block_[ VERTS_PER_BLOCK ];
	// TODO: move this out of stack!!
	BlockVertex *blockCache_;
	uint numCachedBlocks_;
	uint numCachedVerts_;
	
	Shader shaders_[ MAX_SHADERS ];
	Texture textures_[ MAX_TEXTURES ];
	Mesh meshes_[ MAX_MESHES ];

	DirectX::XMFLOAT4X4 view_;
	DirectX::XMFLOAT4X4 projection_;

	friend class Overlay;
};

//******************************
// Debug overlay
//******************************

#define MAX_OVERLAY_CHARS 1024
// measured in pixels
#define TEXT_PADDING 10
#define LINE_HEIGHT 24
#define LINE_SPACING 4
#define CHAR_WIDTH 12
#define FONT_BITMAP_WIDTH 1024.0f
#define FONT_NUM_CHARS 86.0f
#define NORMALIZED_CHAR_WIDTH ( CHAR_WIDTH / FONT_BITMAP_WIDTH )
#define MAX_CHARS_IN_LINE 32
// #define FONT_CHAR_OFFSET 0.01171875f

struct OverlayVertex
{
	float pos[2];
	float texcoord[2];
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

	float GetCharOffset( char c );

private:
	Renderer *renderer_;
	Shader shader_;
	ID3D11Buffer *vb_;
	//ID3D11ShaderResourceView *textureView_;
	Texture texture_;
//	ID3D11SamplerState *sampler_;
	ID3D11Buffer *constantBuffer_;

	uint lineNumber_;
	uint lineOffset_;
	DirectX::XMFLOAT4 currentColor_;
};

}

#endif // __BLOCKS_RENDERER__