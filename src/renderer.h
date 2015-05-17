#ifndef __BLOCKS_RENDERER__
#define __BLOCKS_RENDERER__

#include <d3d11.h>
#include <assert.h>

#include "config.h"
#include "utils.h"

namespace Blocks
{

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Start( HWND wnd );
	void Present();
private:
	static bool isInstantiated_;

	ID3D11Device *device_;
	ID3D11DeviceContext *context_;
	IDXGISwapChain *swapChain_;
	ID3D11RenderTargetView *backBufferView_;
	ID3D11RasterizerState *defaultRasterizerState_;
	D3D11_VIEWPORT screenViewport_;
};

}

#endif