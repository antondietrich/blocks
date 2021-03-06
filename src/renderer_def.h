#ifndef __BLOCKS_RENDERER_DEF__
#define __BLOCKS_RENDERER_DEF__

#include "types.h"

namespace Blocks
{

#define VERTEX_SHADER_ENTRY "VSMain"
#define PIXEL_SHADER_ENTRY "PSMain"

#define BACK_BUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

#define MAX_SHADERS 8
#define MAX_TEXTURES 8
#define MAX_MESHES 8

#define VERTS_PER_BLOCK 36
#define VERTS_PER_FACE 6
#define MAX_VERTS_PER_BATCH 50000 * 256 * 2 // 25 600 000

typedef ID3D11Buffer * ID3D11BufferArray;
typedef ID3D11DepthStencilView * ID3D11DepthStencilViewArray;

typedef uint8 ResourceHandle;
#define INVALID_HANDLE 0xff

/*** Render states ***/
#define MAX_DEPTH_STENCIL_STATES 8
#define MAX_RASTERIZER_STATES 8

#define ZERO_MEMORY( t ) ( ZeroMemory( &(t), sizeof(t) ) )

/* Depth / stencil state */
extern ResourceHandle gDefaultDepthStencilState;

enum class COMPARISON_FUNCTION
{
	NEVER = D3D11_COMPARISON_NEVER,
	LESS = D3D11_COMPARISON_LESS,
	EQUAL = D3D11_COMPARISON_EQUAL,
	GREATER = D3D11_COMPARISON_GREATER,
	LESS_EQUAL = D3D11_COMPARISON_LESS_EQUAL,
	GREATER_EQUAL = D3D11_COMPARISON_GREATER_EQUAL,
	NOT_EQUAL = D3D11_COMPARISON_NOT_EQUAL,
	ALWAYS = D3D11_COMPARISON_ALWAYS,
};

struct DepthStateDesc
{
	bool enabled;
	bool readonly;
	COMPARISON_FUNCTION comparisonFunction;
};

enum class STENCIL_OP
{
	KEEP = D3D11_STENCIL_OP_KEEP,
	ZERO = D3D11_STENCIL_OP_ZERO,
	REPLACE = D3D11_STENCIL_OP_REPLACE,
	INVERT = D3D11_STENCIL_OP_INVERT,
	INCR_CLAMP = D3D11_STENCIL_OP_INCR_SAT,
	DECR_CLAMP = D3D11_STENCIL_OP_DECR_SAT,
	INCR_WRAP = D3D11_STENCIL_OP_INCR,
	DECR_WRAP = D3D11_STENCIL_OP_DECR,
};

struct STENCIL_OP_DESC
{
	STENCIL_OP stencilFail;
	STENCIL_OP stencilPassDepthFail;
	STENCIL_OP bothPass;
	COMPARISON_FUNCTION comparisonFunction;
};

struct StencilStateDesc
{
	bool enabled;
	uint8 readMask;
	uint8 writeMask;
	STENCIL_OP_DESC frontFace;
	STENCIL_OP_DESC backFace;
};


/* Rasterizer state */
extern ResourceHandle gDefaultRasterizerState;

enum class FILL_MODE
{
	WIREFRAME = D3D11_FILL_WIREFRAME,
	SOLID = D3D11_FILL_SOLID,
};

enum class CULL_MODE
{
	NONE = D3D11_CULL_NONE,
	FRONT = D3D11_CULL_FRONT,
	BACK = D3D11_CULL_BACK,
};

struct RasterizerStateDesc
{
	FILL_MODE fillMode;
	CULL_MODE cullMode;
	bool frontCCW;
	int depthBias;
	float depthBiasClamp;
	float slopeScaledDepthBias;
	bool depthClipEnabled;
	bool scissorEnabled;
	bool multisampleEnabled;
	bool antialiasedLineEnabled;
};

enum RASTERIZER_STATE
{
	RS_DEFAULT,
	RS_SHADOWMAP,

	NUM_RASTERIZER_STATES
};

/* Blend state */
enum BLEND_MODE
{
	BM_DEFAULT,
	BM_ALPHA,

	NUM_BLEND_MODES
};
enum SAMPLER_TYPE
{
	SAMPLER_POINT,
	SAMPLER_LINEAR,
	SAMPLER_ANISOTROPIC,
	SAMPLER_SHADOW,

	NUM_SAMPLER_TYPES
};

enum SHADER_TYPE
{
	ST_VERTEX = 1,
	ST_GEOMETRY = 2,
	ST_FRAGMENT = 4,
};

enum RENDER_TARGET
{
	RT_FRAMEBUFFER,
	RT_DEPTHBUFFER,
	RT_FRAME_SHADER_ACCESS,
	RT_DEPTH_SHADER_ACCESS,
};


// Buffer related

enum class RESOURCE_USAGE
{
	DEFAULT = 0,
	IMMUTABLE = 1,
	DYNAMIC = 2,
	STAGING = 3
};

enum class CPU_ACCESS
{
	NONE = 0,
	WRITE = 0x10000L,
	READ = 0x20000L
};

} // namespace Blocks

#endif // __BLOCKS_RENDERER_DEF__
