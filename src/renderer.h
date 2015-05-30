#ifndef __BLOCKS_RENDERER__
#define __BLOCKS_RENDERER__

#include <d3d11.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <assert.h>

#include "DDSTextureLoader.h"

#include "config.h"
#include "utils.h"

namespace Blocks
{

#define VERTEX_SHADER_ENTRY "VSMain"
#define PIXEL_SHADER_ENTRY "PSMain"

#define BACK_BUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

enum BLEND_MODE
{
	BM_DEFAULT,
	BM_ALPHA,
	NUM_BLEND_MODES
};

bool LoadShader( wchar_t *filename, const char *entry, const char *shaderModel, ID3DBlob **buffer );
/* Based on the implementation by @BobbyAnguelov. Thank you! */
HRESULT CreateInputLayoutFromShaderBytecode( ID3DBlob* shaderBytecode, ID3D11Device* device, ID3D11InputLayout** inputLayout );

struct VertexPosition
{
	float pos[3];
};

class Shader
{
public:
	Shader();
	~Shader();

	bool Load( wchar_t* filename, ID3D11Device *device );

	ID3D11VertexShader *vertexShader_;
	ID3D11PixelShader *pixelShader_;
	ID3D11InputLayout *inputLayout_;
private:
};

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Start( HWND wnd );
	void Begin();
	void End();
	void Present();

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

	void SetBlendMode( BLEND_MODE bm );
	void SetShader( const Shader& shader );
private:
	static bool isInstantiated_;
	unsigned int vsync_;

	ID3D11Device *device_;
	ID3D11DeviceContext *context_;
	IDXGISwapChain *swapChain_;
	ID3D11RenderTargetView *backBufferView_;
	ID3D11RasterizerState *defaultRasterizerState_;
	D3D11_VIEWPORT screenViewport_;

	ID3D11BlendState *blendStates_[ NUM_BLEND_MODES ];

	ID3D11Buffer *triangleVB_;

	Shader colorShader;

	friend class Overlay;
};

//******************************
// Debug overlay
//******************************

#define MAX_OVERLAY_CHARS 1024
// measured in pixels
#define LINE_HEIGHT 24
#define LINE_SPACING 4
#define CHAR_WIDTH 12.0f
#define FONT_BITMAP_WIDTH 1024.0f
#define FONT_NUM_CHARS 86.0f
#define FONT_CHAR_OFFSET ( CHAR_WIDTH / FONT_BITMAP_WIDTH )
// #define FONT_CHAR_OFFSET 0.01171875f

struct OverlayVertex
{
	float pos[2];
	float texcoord[2];
};

class Overlay
{
public:
	Overlay();
	~Overlay();
	bool Start( Renderer* renderer );

	void Print( const char* text );
	void DisplayText( int x, int y, const char* text, DirectX::XMFLOAT4 color );

	float GetCharOffset( char c );

private:
	Renderer *renderer_;
	Shader shader_;
	ID3D11Buffer *vb_;
	ID3D11ShaderResourceView *textureView_;
	ID3D11SamplerState *sampler_;
};

}

#endif // __BLOCKS_RENDERER__