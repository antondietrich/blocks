#ifndef __ASSET_DEF__
#define __ASSET_DEF__

/***	TEXTURE		***/

enum TEXTURE_TYPE
{
	SINGLE,
	ARRAY,
};

enum TEXTURE
{
	BLOCKS_OPAQUE,
	BLOCKS_TRANSPARENT,
	LIGHT_COLOR,
	FONT,
	CROSSHAIR,
	CAMPFIRE,
	AXE,

	TEXTURE_COUNT
};

enum BLOCK_TEXTURE
{
	GRASS,
	DIRT,
	ROCK,
	WOOD,
	TREE_BARK,
	TREE_TRUNK,
	LEAVES,

	BLOCK_TEXTURE_COUNT
};

struct TextureDefinition
{
	char ** filenames;
	TEXTURE_TYPE type;
	uint arraySize;
	uint width;
	uint height;
	bool generateMips;
};

void LoadTextureDefinitions( TextureDefinition * textureDefinitions );
void FreeTextureDefinitions( TextureDefinition * textureDefinitions );

/***	SHADER	***/

enum SHADER
{
	SHADER_STANDARD,
	SHADER_BLOCK,
	SHADER_SHADOWMAP,
	SHADER_TEXT,
	SHADER_PRIMITIVE,

	SHADER_COUNT
};

struct ShaderDefinition
{
	wchar_t * filename;
};

#ifdef ASSET_DEF_IMPLEMENTATION
ShaderDefinition gShaderDefinitions[ SHADER::SHADER_COUNT ] =
{
	{ L"assets/shaders/standard.fx" },
	{ L"assets/shaders/global.fx" },
	{ L"assets/shaders/shadowmap.fx" },
	{ L"assets/shaders/overlay.fx" },
	{ L"assets/shaders/primitive.fx" },
};
#endif

/***	MATERIAL	***/

enum MATERIAL
{
	MATERIAL_CAMPFIRE,
	MATERIAL_AXE,

	MATERIAL_COUNT
};

struct MaterialDefinition
{
	SHADER shaderID;
	TEXTURE diffuseTextureID;
};

#ifdef ASSET_DEF_IMPLEMENTATION
MaterialDefinition gMaterialDefinitions[ MATERIAL::MATERIAL_COUNT ] =
{
	{ SHADER::SHADER_STANDARD, TEXTURE::CAMPFIRE },
	{ SHADER::SHADER_STANDARD, TEXTURE::AXE },
};
#endif

/***	MESH	***/

#define MAX_VERTS_PER_MESH (4096 * 3)

enum MESH
{
	MESH_CAMPFIRE,
	MESH_AXE,

	MESH_COUNT
};

struct MeshDefinition
{
	const char * filename;
	uint material;
};

#ifdef ASSET_DEF_IMPLEMENTATION
MeshDefinition gMeshDefinitions[ MESH::MESH_COUNT ] =
{
	{ "assets/models/fire.obj", MATERIAL::MATERIAL_CAMPFIRE },
	{ "assets/models/axe.obj", MATERIAL::MATERIAL_AXE },
};
#endif

#ifdef ASSET_DEF_IMPLEMENTATION

void LoadTextureDefinitions( TextureDefinition * textureDefinitions )
{
	for( int i = 0; i < TEXTURE_COUNT; i++ )
	{
		textureDefinitions[ i ].type = (TEXTURE_TYPE)0;
		textureDefinitions[ i ].arraySize = 0;
		textureDefinitions[ i ].width = 0;
		textureDefinitions[ i ].height = 0;
		textureDefinitions[ i ].arraySize = 0;
		textureDefinitions[ i ].filenames = 0;
	}

	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].type = TEXTURE_TYPE::ARRAY;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].arraySize = BLOCK_TEXTURE_COUNT;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].width = 512;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].height = 512;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].generateMips = true;
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames = new char*[ BLOCK_TEXTURE_COUNT ];
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[0] = "assets/textures/grass.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[1] = "assets/textures/dirt.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[2] = "assets/textures/rock.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[3] = "assets/textures/wood.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[4] = "assets/textures/tree_bark.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[5] = "assets/textures/wood_trunk.png";
	textureDefinitions[ TEXTURE::BLOCKS_OPAQUE ].filenames[6] = "assets/textures/leaves.png";

	textureDefinitions[ TEXTURE::LIGHT_COLOR ].type = TEXTURE_TYPE::SINGLE;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].arraySize = 1;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].width = 256;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].height = 8;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].generateMips = false;
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].filenames = new char*[ 1 ];
	textureDefinitions[ TEXTURE::LIGHT_COLOR ].filenames[0] = "assets/textures/light_color.png";

	textureDefinitions[ TEXTURE::FONT ].type = TEXTURE_TYPE::SINGLE;
	textureDefinitions[ TEXTURE::FONT ].arraySize = 1;
	textureDefinitions[ TEXTURE::FONT ].width = 1024;
	textureDefinitions[ TEXTURE::FONT ].height = 24;
	textureDefinitions[ TEXTURE::FONT ].generateMips = false;
	textureDefinitions[ TEXTURE::FONT ].filenames = new char*[ 1 ];
	textureDefinitions[ TEXTURE::FONT ].filenames[0] = "assets/textures/droid_mono.png";

	textureDefinitions[ TEXTURE::CROSSHAIR ].type = TEXTURE_TYPE::SINGLE;
	textureDefinitions[ TEXTURE::CROSSHAIR ].arraySize = 1;
	textureDefinitions[ TEXTURE::CROSSHAIR ].width = 16;
	textureDefinitions[ TEXTURE::CROSSHAIR ].height = 8;
	textureDefinitions[ TEXTURE::CROSSHAIR ].generateMips = false;
	textureDefinitions[ TEXTURE::CROSSHAIR ].filenames = new char*[ 1 ];
	textureDefinitions[ TEXTURE::CROSSHAIR ].filenames[0] = "assets/textures/crosshair.png";

	textureDefinitions[ TEXTURE::CAMPFIRE ].type = TEXTURE_TYPE::SINGLE;
	textureDefinitions[ TEXTURE::CAMPFIRE ].arraySize = 1;
	textureDefinitions[ TEXTURE::CAMPFIRE ].width = 512;
	textureDefinitions[ TEXTURE::CAMPFIRE ].height = 512;
	textureDefinitions[ TEXTURE::CAMPFIRE ].generateMips = true;
	textureDefinitions[ TEXTURE::CAMPFIRE ].filenames = new char*[ 1 ];
	textureDefinitions[ TEXTURE::CAMPFIRE ].filenames[0] = "assets/textures/fire01.png";

	textureDefinitions[ TEXTURE::AXE ].type = TEXTURE_TYPE::SINGLE;
	textureDefinitions[ TEXTURE::AXE ].arraySize = 1;
	textureDefinitions[ TEXTURE::AXE ].width = 2048;
	textureDefinitions[ TEXTURE::AXE ].height = 2048;
	textureDefinitions[ TEXTURE::AXE ].generateMips = true;
	textureDefinitions[ TEXTURE::AXE ].filenames = new char*[ 1 ];
	textureDefinitions[ TEXTURE::AXE ].filenames[0] = "assets/textures/axe.png";
}

void FreeTextureDefinitions( TextureDefinition * textureDefinitions )
{
	for( int i = 0; i < TEXTURE::TEXTURE_COUNT; i++ )
	{
		if( textureDefinitions[ i ].filenames )
		{
			delete[] textureDefinitions[ i ].filenames;
		}
	}
}

#endif ASSET_DEF_IMPLEMENTATION

#endif // __ASSET_DEF__
