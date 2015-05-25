#ifndef __BLOCKS_RENDERER__
#define __BLOCKS_RENDERER__

#include <d3d11.h>
#include <D3Dcompiler.h>
#include <string>
#include <assert.h>

#include "config.h"
#include "utils.h"

namespace Blocks
{

#define VERTEX_SHADER_ENTRY "VSMain"
#define PIXEL_SHADER_ENTRY "PSMain"

#define BACK_BUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

bool LoadShader( wchar_t *filename, const char *entry, const char *shaderModel, ID3DBlob **buffer );

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
	void Present();

	/* Window management */
	void ToggleFullscreen();
	bool Fullscreen();
	bool ResizeBuffers();

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

	ID3D11Buffer *triangleVB_;

	Shader colorShader;
};

}

#endif // __BLOCKS_RENDERER__