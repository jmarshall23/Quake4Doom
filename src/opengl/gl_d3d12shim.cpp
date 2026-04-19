// gl_d3d12shim.cpp
//

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <cmath>
#include <immintrin.h>
#include <stdint.h>
#include <vector>
#include <string.h>
#include <algorithm>
#include <memory>

#define QD3D12_ENABLE_STREAMLINE

#if defined(QD3D12_ENABLE_STREAMLINE)
#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_dlss_d.h>
#endif

#if defined(QD3D12_ENABLE_FFX)
#include <ffx_api/ffx_api.hpp>
#include <ffx_api/ffx_upscale.hpp>
#include <ffx_api/dx12/ffx_api_dx12.hpp>
#include <ffx_api/ffx_denoiser.hpp>
#endif

using Microsoft::WRL::ComPtr;

#include "opengl.h"
#include "gl_d3d12arb.h"


#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_COMPRESSED_ALPHA_ARB
#define GL_COMPRESSED_ALPHA_ARB 0x84E9
#endif
#ifndef GL_COMPRESSED_LUMINANCE_ARB
#define GL_COMPRESSED_LUMINANCE_ARB 0x84EA
#endif
#ifndef GL_COMPRESSED_LUMINANCE_ALPHA_ARB
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84EB
#endif
#ifndef GL_COMPRESSED_INTENSITY_ARB
#define GL_COMPRESSED_INTENSITY_ARB 0x84EC
#endif
#ifndef GL_COMPRESSED_RGB_ARB
#define GL_COMPRESSED_RGB_ARB 0x84ED
#endif
#ifndef GL_COMPRESSED_RGBA_ARB
#define GL_COMPRESSED_RGBA_ARB 0x84EE
#endif
#ifndef GL_TEXTURE_COMPRESSION_HINT_ARB
#define GL_TEXTURE_COMPRESSION_HINT_ARB 0x84EF
#endif
#ifndef GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB 0x86A0
#endif
#ifndef GL_TEXTURE_COMPRESSED_ARB
#define GL_TEXTURE_COMPRESSED_ARB 0x86A1
#endif
#ifndef GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#endif
#ifndef GL_COMPRESSED_TEXTURE_FORMATS_ARB
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A3
#endif
#ifndef GL_TEXTURE_WIDTH
#define GL_TEXTURE_WIDTH 0x1000
#endif
#ifndef GL_TEXTURE_HEIGHT
#define GL_TEXTURE_HEIGHT 0x1001
#endif
#ifndef GL_TEXTURE_INTERNAL_FORMAT
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#endif

#ifndef GL_RGB_S3TC
#define GL_RGB_S3TC 0x83A0
#endif
#ifndef GL_RGB4_S3TC
#define GL_RGB4_S3TC 0x83A1
#endif
#ifndef GL_RGBA_S3TC
#define GL_RGBA_S3TC 0x83A2
#endif
#ifndef GL_RGBA4_S3TC
#define GL_RGBA4_S3TC 0x83A3
#endif

static constexpr size_t GL_FRAME_VERTEX_CAPACITY = 2000000; // we make retro games, this comes out to about 100mb or so. 

struct QD3D12Window;
struct QD3D12GLContext;
struct QD3D12Pbuffer;

static HDC   g_qd3d12CurrentDC = nullptr;
static HGLRC g_qd3d12CurrentRC = nullptr;

static D3D12_GPU_DESCRIPTOR_HANDLE QD3D12_SrvGpu(UINT index);
static D3D12_CPU_DESCRIPTOR_HANDLE QD3D12_SrvCpu(UINT index);
static Mat4 CurrentModelMatrix();
static inline GLenum QD3D12_MapCompatTextureTarget(GLenum target);
static void QD3D12_CreateUploadRingForWindow(struct QD3D12Window& w);
static void QD3D12_DestroyUploadRingForWindow(struct QD3D12Window& w);
static void QD3D12_SubmitOpenFrameNoPresentAndWait();
static void QD3D12_EnsureFrameOpen();
static void QD3D12_RunUpscalerOrBlit(QD3D12Window& w);
static void QD3D12_ExecuteMainCommandListAndWait(QD3D12Window& w);
static void QD3D12_RunRayAIDenoiseIfEnabled(ID3D12GraphicsCommandList* cl, QD3D12Window& w, ID3D12Resource* lightingResource);
static void QD3D12_TransitionResource(ID3D12GraphicsCommandList* cl, ID3D12Resource* res, D3D12_RESOURCE_STATES& trackedState, D3D12_RESOURCE_STATES newState);
static UINT QD3D12_ActiveRasterWidth(const QD3D12Window& w);
static UINT QD3D12_ActiveRasterHeight(const QD3D12Window& w);
static void QD3D12_BindTargetsForCurrentPhase(QD3D12Window& w);
static void QD3D12_ResolveSceneToOutputAndEnterNativePhase(QD3D12Window& w);
static void QD3D12_RegisterWindowDC(QD3D12Window* w);
static void QD3D12_UnregisterWindowDC(HDC dc);
static HGLRC QD3D12_CreateGLContextHandle(HDC dc, QD3D12Window* window);

extern bool D3D12_SwapBufferMultWindows;

static void QD3D12_Log(const char* fmt, ...)
{
	char buffer[4096];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	OutputDebugStringA(buffer);
	OutputDebugStringA("\n");
}

static void QD3D12_Fatal(const char* fmt, ...)
{
	char buffer[4096];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	OutputDebugStringA(buffer);
	OutputDebugStringA("\n");
	MessageBoxA(nullptr, buffer, "QD3D12 Fatal", MB_OK | MB_ICONERROR);
	DebugBreak();
}

struct VertexArena
{
	GLVertex* buffer;
	size_t    capacity;
	size_t    cursor;
};

#define QD3D12_CHECK(x) do { HRESULT _hr = (x); if (FAILED(_hr)) { QD3D12_Fatal("HRESULT failed 0x%08X at %s:%d", (unsigned)_hr, __FILE__, __LINE__); } } while(0)

struct GLOcclusionQuery
{
	GLuint id = 0;
	bool   active = false;
	bool   pending = false;
	bool   resultReady = false;

	UINT   heapIndex = UINT_MAX;
	UINT64 result = 0;
	UINT64 submittedFence = 0;
};

struct QueryMarker
{
	enum Type
	{
		Begin,
		End
	};

	Type   type = Begin;
	GLuint id = 0;
};

static const UINT QD3D12_MaxQueries = 2048;

// ============================================================
// SECTION 3: D3D12 renderer structs
// ============================================================

static const UINT QD3D12_MaxTextureUnits = 8;
static const UINT QD3D12_FrameCount = 2;
static const UINT QD3D12_MaxTextures = 4096;
static const UINT QD3D12_UploadBufferSize = 128 * 1024 * 1024;

static const DXGI_FORMAT QD3D12_SceneColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
static const DXGI_FORMAT QD3D12_VelocityFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
static const DXGI_FORMAT QD3D12_DepthFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
static const DXGI_FORMAT QD3D12_DepthResourceFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
static const DXGI_FORMAT QD3D12_DepthSrvFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
static const DXGI_FORMAT QD3D12_DepthDsvFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

enum QD3D12UpscalerBackend
{
	QD3D12_UPSCALER_NONE = 0,
	QD3D12_UPSCALER_DLSS = 1,
	QD3D12_UPSCALER_FSR = 2
};

enum QD3D12UpscalerQuality
{
	QD3D12_QUALITY_NATIVE = 0,
	QD3D12_QUALITY_QUALITY,
	QD3D12_QUALITY_BALANCED,
	QD3D12_QUALITY_PERFORMANCE,
	QD3D12_QUALITY_ULTRA_PERFORMANCE
};

enum QD3D12FramePhase
{
	QD3D12_FRAME_LOW_RES = 0,
	QD3D12_FRAME_NATIVE_POST_UPSCALE
};

struct QD3D12CameraState
{
	Mat4 viewToClip = Mat4::Identity();
	Mat4 clipToView = Mat4::Identity();
	Mat4 clipToPrevClip = Mat4::Identity();
	Mat4 prevClipToClip = Mat4::Identity();
	float cameraPos[3] = { 0.0f, 0.0f, 0.0f };
	float cameraRight[3] = { 1.0f, 0.0f, 0.0f };
	float cameraUp[3] = { 0.0f, 1.0f, 0.0f };
	float cameraForward[3] = { 0.0f, 0.0f, 1.0f };
	float nearPlane = 0.01f;
	float farPlane = 4096.0f;
	float verticalFovRadians = 1.0471975512f;
	float aspectRatio = 1.0f;
	bool valid = false;
};

enum PipelineMode
{
	PIPE_OPAQUE_TEX = 0,
	PIPE_ALPHA_TEST_TEX,
	PIPE_BLEND_TEX,
	PIPE_OPAQUE_UNTEX,
	PIPE_BLEND_UNTEX,
	PIPE_COUNT
};

static void QD3D12_FlushQueuedBatches();
static PipelineMode PickPipeline(bool useTex0, bool useTex1);
static Mat4 CurrentMVP();

struct RetiredResource
{
	UINT64 fenceValue = 0;
	ComPtr<ID3D12Resource> resource;
};

static std::vector<RetiredResource> g_retiredResources;

struct FrameResources
{
	ComPtr<ID3D12CommandAllocator> cmdAlloc;
	UINT64 fenceValue = 0;
};

struct UploadRing
{
	ComPtr<ID3D12Resource> resource[QD3D12_FrameCount];
	uint8_t* cpuBase[QD3D12_FrameCount] = {};
	D3D12_GPU_VIRTUAL_ADDRESS gpuBase[QD3D12_FrameCount] = {};
	UINT size = 0;
	UINT offset = 0;
};

struct TextureResource
{
	GLuint glId = 0;
	int width = 0;
	int height = 0;
	GLenum format = GL_RGBA;
	GLenum minFilter = GL_LINEAR;
	GLenum magFilter = GL_LINEAR;
	GLenum wrapS = GL_REPEAT;
	GLenum wrapT = GL_REPEAT;

	std::vector<uint8_t> sysmem;

	bool compressed = false;
	GLenum compressedInternalFormat = 0;
	UINT compressedBlockBytes = 0;
	UINT compressedImageSize = 0;
	bool forceOpaqueAlpha = false;

	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	ComPtr<ID3D12Resource> texture;
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpu{};
	UINT srvIndex = UINT_MAX;
	bool gpuValid = false;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
};

struct DrawConstants
{
	Mat4 mvp;
	Mat4 prevMvp;
	Mat4 modelMatrix;

	float alphaRef;

	float useTex0;
	float useTex1;
	float tex1IsLightmap;

	float texEnvMode0;
	float texEnvMode1;
	float geometryFlag;
	float roughness;

	float fogEnabled;
	float fogMode;
	float fogDensity;
	float fogStart;

	float fogEnd;
	float PointSize;
	float materialType;
	float alphaFunc;

	float fogColor[4];

	float renderSize[2];
	float invRenderSize[2];
	float jitterPixels[2];
	float prevJitterPixels[2];
	float _motionPad[4];

	float texComb0RGB[4];
	float texComb0Alpha[4];
	float texComb0Operand[4];
	float texEnvColor0[4];
	float texComb1RGB[4];
	float texComb1Alpha[4];
	float texComb1Operand[4];
	float texEnvColor1[4];

	// Appended for translated ARBvp/ARBfp programs.  Fixed-function HLSL ignores
	// these fields; ARB-generated HLSL uses them as program.env/local storage.
	float arbEnv[QD3D12_ARB_MAX_PROGRAM_PARAMETERS][4];
	float arbLocalVP[QD3D12_ARB_MAX_PROGRAM_PARAMETERS][4];
	float arbLocalFP[QD3D12_ARB_MAX_PROGRAM_PARAMETERS][4];
};

struct GLBufferObject
{
	GLuint               id = 0;
	GLenum               target = 0;
	GLbitfield           storageFlags = 0;
	std::vector<uint8_t> data;

	bool       mapped = false;
	GLintptr   mappedOffset = 0;
	GLsizeiptr mappedLength = 0;
	GLbitfield mappedAccess = 0;
};

const char* vendor = "Justin Marshall";
const char* renderer = "Quake D3D12 Wrapper";
const char* version = "1.1-quake-d3d12";
const char* extensions = "GL_SGIS_multitexture GL_ARB_multitexture GL_EXT_texture_env_add GL_ARB_texture_env_combine GL_ARB_texture_compression GL_EXT_texture_compression_s3tc GL_ARB_vertex_program GL_ARB_fragment_program GL_EXT_texture_cube_map GL_EXT_depth_bounds_test GL_EXT_stencil_two_side GL_ATI_separate_stencil";

enum TexEnvModeShader
{
	TEXENV_MODULATE = 0,
	TEXENV_REPLACE = 1,
	TEXENV_DECAL = 2,
	TEXENV_BLEND = 3,
	TEXENV_ADD = 4,
	TEXENV_COMBINE = 5
};

static float MapTexEnvMode(GLenum mode)
{
	switch (mode)
	{
	case GL_REPLACE:  return (float)TEXENV_REPLACE;
	case GL_BLEND:    return (float)TEXENV_BLEND;
#ifdef GL_COMBINE_ARB
	case GL_COMBINE_ARB: return (float)TEXENV_COMBINE;
#endif
#ifdef GL_ADD
	case GL_ADD:      return (float)TEXENV_ADD;
#endif
	case GL_MODULATE:
	default:          return (float)TEXENV_MODULATE;
	}
}


struct BatchKey
{
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor;

	PipelineMode pipeline = PIPE_OPAQUE_UNTEX;
	D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT tex0SrvIndex = 0;
	UINT tex1SrvIndex = 0;
	UINT textureSrvIndex[QD3D12_MaxTextureUnits] = {};

	bool useARBPrograms = false;
	GLuint arbVertexProgram = 0;
	GLuint arbFragmentProgram = 0;
	uint32_t arbVertexRevision = 0;
	uint32_t arbFragmentRevision = 0;
	ID3DBlob* arbVertexBlob = nullptr;
	ID3DBlob* arbFragmentBlob = nullptr;
	QD3D12ARBDrawConstantArrays arbConstants{};

	float texComb0RGB[4] = {};
	float texComb0Alpha[4] = {};
	float texComb0Operand[4] = {};
	float texEnvColor0[4] = {};
	float texComb1RGB[4] = {};
	float texComb1Alpha[4] = {};
	float texComb1Operand[4] = {};
	float texEnvColor1[4] = {};

	UINT8 colorWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	bool cullFaceEnabled = false;
	GLenum cullMode = GL_BACK;
	GLenum frontFace = GL_CCW;

	bool stencilTest = false;
	UINT8 stencilReadMask = 0xFF;
	UINT8 stencilWriteMask = 0xFF;
	UINT8 stencilRef = 0;
	GLenum stencilFrontFunc = GL_ALWAYS;
	GLenum stencilFrontSFail = GL_KEEP;
	GLenum stencilFrontDPFail = GL_KEEP;
	GLenum stencilFrontDPPass = GL_KEEP;
	GLenum stencilBackFunc = GL_ALWAYS;
	GLenum stencilBackSFail = GL_KEEP;
	GLenum stencilBackDPFail = GL_KEEP;
	GLenum stencilBackDPPass = GL_KEEP;

	bool depthBoundsTest = false;
	float depthBoundsMin = 0.0f;
	float depthBoundsMax = 1.0f;

	float alphaRef = 0.0f;
	float alphaFunc = 4.0f;
	float useTex0 = 0.0f;
	float useTex1 = 0.0f;
	float tex1IsLightmap = 0.0f;
	float texEnvMode0 = 0.0f;
	float texEnvMode1 = 0.0f;

	GLenum blendSrc = GL_ONE;
	GLenum blendDst = GL_ZERO;

	bool depthTest = true;
	bool depthWrite = true;
	GLenum depthFunc = GL_LEQUAL;

	Mat4 mvp;
	Mat4 prevMvp;
	Mat4 modelMatrix;

	float geometryFlag = 0.0f;
	float roughness = 0.5f;
	float materialType = 0.0f;
	GLuint motionObjectId = 0;
};


static bool ViewportEquals(const D3D12_VIEWPORT& a, const D3D12_VIEWPORT& b)
{
	return a.TopLeftX == b.TopLeftX &&
		a.TopLeftY == b.TopLeftY &&
		a.Width == b.Width &&
		a.Height == b.Height &&
		a.MinDepth == b.MinDepth &&
		a.MaxDepth == b.MaxDepth;
}

static bool RectEquals(const D3D12_RECT& a, const D3D12_RECT& b)
{
	return a.left == b.left &&
		a.top == b.top &&
		a.right == b.right &&
		a.bottom == b.bottom;
}

static bool TextureSrvArrayEquals(const UINT* a, const UINT* b)
{
	for (UINT i = 0; i < QD3D12_MaxTextureUnits; ++i)
	{
		if (a[i] != b[i])
			return false;
	}
	return true;
}

static bool BatchKeyEquals(const BatchKey& a, const BatchKey& b)
{
	return
		ViewportEquals(a.viewport, b.viewport) &&
		RectEquals(a.scissor, b.scissor) &&
		a.pipeline == b.pipeline &&
		a.topology == b.topology &&
		a.tex0SrvIndex == b.tex0SrvIndex &&
		a.tex1SrvIndex == b.tex1SrvIndex &&
		a.alphaRef == b.alphaRef &&
		a.alphaFunc == b.alphaFunc &&
		a.useTex0 == b.useTex0 &&
		a.useTex1 == b.useTex1 &&
		a.tex1IsLightmap == b.tex1IsLightmap &&
		a.texEnvMode0 == b.texEnvMode0 &&
		a.texEnvMode1 == b.texEnvMode1 &&
		a.colorWriteMask == b.colorWriteMask &&
		a.cullFaceEnabled == b.cullFaceEnabled &&
		a.cullMode == b.cullMode &&
		a.frontFace == b.frontFace &&
		a.stencilTest == b.stencilTest &&
		a.stencilReadMask == b.stencilReadMask &&
		a.stencilWriteMask == b.stencilWriteMask &&
		a.stencilRef == b.stencilRef &&
		a.stencilFrontFunc == b.stencilFrontFunc &&
		a.stencilFrontSFail == b.stencilFrontSFail &&
		a.stencilFrontDPFail == b.stencilFrontDPFail &&
		a.stencilFrontDPPass == b.stencilFrontDPPass &&
		a.stencilBackFunc == b.stencilBackFunc &&
		a.stencilBackSFail == b.stencilBackSFail &&
		a.stencilBackDPFail == b.stencilBackDPFail &&
		a.stencilBackDPPass == b.stencilBackDPPass &&
		a.depthBoundsTest == b.depthBoundsTest &&
		a.depthBoundsMin == b.depthBoundsMin &&
		a.depthBoundsMax == b.depthBoundsMax &&
		memcmp(a.texComb0RGB, b.texComb0RGB, sizeof(a.texComb0RGB)) == 0 &&
		memcmp(a.texComb0Alpha, b.texComb0Alpha, sizeof(a.texComb0Alpha)) == 0 &&
		memcmp(a.texComb0Operand, b.texComb0Operand, sizeof(a.texComb0Operand)) == 0 &&
		memcmp(a.texEnvColor0, b.texEnvColor0, sizeof(a.texEnvColor0)) == 0 &&
		memcmp(a.texComb1RGB, b.texComb1RGB, sizeof(a.texComb1RGB)) == 0 &&
		memcmp(a.texComb1Alpha, b.texComb1Alpha, sizeof(a.texComb1Alpha)) == 0 &&
		memcmp(a.texComb1Operand, b.texComb1Operand, sizeof(a.texComb1Operand)) == 0 &&
		memcmp(a.texEnvColor1, b.texEnvColor1, sizeof(a.texEnvColor1)) == 0 &&
		a.blendSrc == b.blendSrc &&
		a.blendDst == b.blendDst &&
		a.depthTest == b.depthTest &&
		a.depthWrite == b.depthWrite &&
		a.depthFunc == b.depthFunc &&
		a.geometryFlag == b.geometryFlag &&
		a.roughness == b.roughness &&
		a.materialType == b.materialType &&
		a.motionObjectId == b.motionObjectId &&
		TextureSrvArrayEquals(a.textureSrvIndex, b.textureSrvIndex) &&
		a.useARBPrograms == b.useARBPrograms &&
		a.arbVertexProgram == b.arbVertexProgram &&
		a.arbFragmentProgram == b.arbFragmentProgram &&
		a.arbVertexRevision == b.arbVertexRevision &&
		a.arbFragmentRevision == b.arbFragmentRevision &&
		a.arbVertexBlob == b.arbVertexBlob &&
		a.arbFragmentBlob == b.arbFragmentBlob &&
		(!a.useARBPrograms || memcmp(&a.arbConstants, &b.arbConstants, sizeof(a.arbConstants)) == 0) &&
		memcmp(a.mvp.m, b.mvp.m, sizeof(a.mvp.m)) == 0 &&
		memcmp(a.prevMvp.m, b.prevMvp.m, sizeof(a.prevMvp.m)) == 0 &&
		memcmp(a.modelMatrix.m, b.modelMatrix.m, sizeof(a.modelMatrix.m)) == 0;
}

struct QueuedBatch
{
	BatchKey key;
	size_t markerBegin = 0;
	size_t markerEnd = 0;

	size_t firstVertex;
	size_t vertexCount;
};

struct QD3D12Window
{
	HWND hwnd = nullptr;
	HDC  hdc = nullptr;
	bool ownsHdc = false;
	bool isPbuffer = false;

	UINT width = 640;          // display/output size
	UINT height = 480;
	UINT renderWidth = 640;    // internal render size (may be lower for upscalers)
	UINT renderHeight = 480;

	ComPtr<IDXGISwapChain3> swapChain;
	UINT frameIndex = 0;

	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	UINT rtvStride = 0;

	// 0 .. FrameCount-1            : scene color (render resolution)
	// FrameCount .. 2*FrameCount-1 : normals/roughness
	// 2*FrameCount .. 3*FrameCount-1 : world position
	// 3*FrameCount .. 4*FrameCount-1 : velocity
	// 4*FrameCount .. 5*FrameCount-1 : swap-chain backbuffer
	std::array<ComPtr<ID3D12Resource>, QD3D12_FrameCount> sceneColorBuffers;
	D3D12_RESOURCE_STATES sceneColorState[QD3D12_FrameCount] = {};

	std::array<ComPtr<ID3D12Resource>, QD3D12_FrameCount> normalBuffers;
	D3D12_RESOURCE_STATES normalBufferState[QD3D12_FrameCount] = {};

	std::array<ComPtr<ID3D12Resource>, QD3D12_FrameCount> positionBuffers;
	D3D12_RESOURCE_STATES positionBufferState[QD3D12_FrameCount] = {};

	std::array<ComPtr<ID3D12Resource>, QD3D12_FrameCount> velocityBuffers;
	D3D12_RESOURCE_STATES velocityBufferState[QD3D12_FrameCount] = {};

	std::array<ComPtr<ID3D12Resource>, QD3D12_FrameCount> backBuffers;
	D3D12_RESOURCE_STATES backBufferState[QD3D12_FrameCount] = {};

	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12Resource> depthBuffer;
	D3D12_RESOURCE_STATES depthState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ComPtr<ID3D12Resource> nativeDepthBuffer;
	D3D12_RESOURCE_STATES nativeDepthState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	UINT sceneColorSrvIndex[QD3D12_FrameCount] = { UINT_MAX, UINT_MAX };
	D3D12_CPU_DESCRIPTOR_HANDLE sceneColorSrvCpu[QD3D12_FrameCount]{};
	D3D12_GPU_DESCRIPTOR_HANDLE sceneColorSrvGpu[QD3D12_FrameCount]{};

	UINT normalSrvIndex[QD3D12_FrameCount] = { UINT_MAX, UINT_MAX };
	D3D12_CPU_DESCRIPTOR_HANDLE normalSrvCpu[QD3D12_FrameCount]{};
	D3D12_GPU_DESCRIPTOR_HANDLE normalSrvGpu[QD3D12_FrameCount]{};

	UINT positionSrvIndex[QD3D12_FrameCount] = { UINT_MAX, UINT_MAX };
	D3D12_CPU_DESCRIPTOR_HANDLE positionSrvCpu[QD3D12_FrameCount]{};
	D3D12_GPU_DESCRIPTOR_HANDLE positionSrvGpu[QD3D12_FrameCount]{};

	UINT velocitySrvIndex[QD3D12_FrameCount] = { UINT_MAX, UINT_MAX };
	D3D12_CPU_DESCRIPTOR_HANDLE velocitySrvCpu[QD3D12_FrameCount]{};
	D3D12_GPU_DESCRIPTOR_HANDLE velocitySrvGpu[QD3D12_FrameCount]{};

	UINT depthSrvIndex = UINT_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE depthSrvGpu{};

	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissor{};

	UploadRing upload;
	FrameResources frames[QD3D12_FrameCount];
};

struct QD3D12Window* AllocD3D12Window() {
	return new QD3D12Window();
}

void FreeD3D12Window(struct QD3D12Window* wnd) {
	if (wnd)
	{
		QD3D12_DestroyUploadRingForWindow(*wnd);
		QD3D12_UnregisterWindowDC(wnd->hdc);
		if (wnd->ownsHdc && wnd->hwnd && wnd->hdc)
			ReleaseDC(wnd->hwnd, wnd->hdc);
	}
	delete wnd;
}

struct ImmediateVertexBuffer
{
	std::vector<GLVertex> storage;
	size_t count = 0;

	void Init(size_t initialCapacity = 1024)
	{
		storage.resize(initialCapacity);
		count = 0;
	}

	void Clear()
	{
		count = 0;
	}

	GLVertex& Push()
	{
		if (count >= storage.size())
		{
			const size_t newSize = storage.empty() ? 1024 : storage.size() * 2;
			storage.resize(newSize);
		}

		return storage[count++];
	}

	GLVertex* Data()
	{
		return storage.data();
	}

	const GLVertex* Data() const
	{
		return storage.data();
	}

	size_t Size() const
	{
		return count;
	}
};

struct GLState
{
	VertexArena frameVerts;

	GLfloat pointSize = 1.0f;
	GLfloat pointSizeMin = 1.0f;
	GLfloat pointSizeMax = 64.0f;
	GLfloat pointFadeThresholdSize = 1.0f;
	GLfloat pointDistanceAttenuation[3] = { 1.0f, 0.0f, 0.0f };

	std::unordered_map<GLuint, GLOcclusionQuery> queries;
	GLuint                                       nextQueryId = 1;
	GLuint                                       currentQuery = 0;

	std::vector<QueryMarker> queryMarkers;

	ComPtr<ID3D12QueryHeap> occlusionQueryHeap;
	ComPtr<ID3D12Resource>  occlusionReadback;
	uint64_t* occlusionReadbackCpu = nullptr;

	std::unordered_map<GLuint, GLBufferObject> buffers;
	GLuint                                     nextBufferId = 1;
	GLuint                                     boundArrayBuffer = 0;
	GLuint                                     boundElementArrayBuffer = 0;

	Mat4 modelMatrix = Mat4::Identity();

	float currentGeometryFlag = 0.0f;

	bool fog = false;
	GLenum fogMode = GL_EXP;
	GLfloat fogDensity = 1.0f;
	GLfloat fogStart = 0.0f;
	GLfloat fogEnd = 1.0f;
	GLfloat fogColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLenum fogHint = GL_DONT_CARE;

	GLclampd depthRangeNear = 0.0;
	GLclampd depthRangeFar = 1.0;

	std::vector<QueuedBatch> queuedBatches;
	bool frameOpen = false;
	QD3D12Window* frameOwner = nullptr;
	QD3D12FramePhase framePhase = QD3D12_FRAME_LOW_RES;
	bool sceneResolvedThisFrame = false;
	bool raytracedLightingReadyThisFrame = false;
	float upscalerSharpness = 1.0f;

	GLenum depthFunc = GL_LEQUAL;
	UINT64 nextFenceValue = 1;

	GLuint currentMotionObjectId = 0;
	float currentSurfaceRoughness = 0.5f;
	float currentMaterialType = 0.0f;
	std::unordered_map<GLuint, Mat4> prevObjectMVPs;
	std::unordered_map<GLuint, Mat4> currObjectMVPs;
	uint64_t frameSerial = 0;
	bool motionHistoryReset = true;

	QD3D12UpscalerBackend upscalerBackend = QD3D12_UPSCALER_DLSS;
	QD3D12UpscalerQuality upscalerQuality = QD3D12_QUALITY_QUALITY;
	bool enableRayAIDenoise = false;
	bool enableDLSSRayReconstruction = true;
	bool enableFSRRayRegeneration = false;

	float jitterX = 0.0f;
	float jitterY = 0.0f;
	float prevJitterX = 0.0f;
	float prevJitterY = 0.0f;
	QD3D12CameraState cameraState{};

	struct ClientArrayState {
		GLint size = 4;
		GLenum type = GL_FLOAT;
		GLsizei stride = 0;
		const uint8_t* ptr = nullptr;
		bool enabled = false;
	};

	ClientArrayState vertexArray;
	ClientArrayState normalArray;
	ClientArrayState tangentArray;
	ClientArrayState bitangentArray;
	ClientArrayState colorArray;
	ClientArrayState texCoordArray[QD3D12_MaxTextureUnits];
	GLuint clientActiveTextureUnit = 0;

	bool scissorTest = false;
	GLint scissorX = 0;
	GLint scissorY = 0;
	GLsizei scissorW = 640;
	GLsizei scissorH = 480;

	GLenum lastError = GL_NO_ERROR;

	GLuint stencilMask = ~0u;
	GLint clearStencilValue = 0;
	bool stencilTest = false;
	bool stencilTwoSide = false;
	GLenum activeStencilFace = GL_FRONT;
	GLenum stencilFunc = GL_ALWAYS;
	GLint stencilRef = 0;
	GLuint stencilFuncMask = ~0u;
	GLenum stencilSFail = GL_KEEP;
	GLenum stencilDPFail = GL_KEEP;
	GLenum stencilDPPass = GL_KEEP;
	GLuint stencilFrontMask = ~0u;
	GLenum stencilFrontFunc = GL_ALWAYS;
	GLint stencilFrontRef = 0;
	GLuint stencilFrontFuncMask = ~0u;
	GLenum stencilFrontSFail = GL_KEEP;
	GLenum stencilFrontDPFail = GL_KEEP;
	GLenum stencilFrontDPPass = GL_KEEP;
	GLuint stencilBackMask = ~0u;
	GLenum stencilBackFunc = GL_ALWAYS;
	GLint stencilBackRef = 0;
	GLuint stencilBackFuncMask = ~0u;
	GLenum stencilBackSFail = GL_KEEP;
	GLenum stencilBackDPFail = GL_KEEP;
	GLenum stencilBackDPPass = GL_KEEP;
	bool depthBoundsTest = false;
	GLclampd depthBoundsMin = 0.0;
	GLclampd depthBoundsMax = 1.0;

	GLdouble clipPlane0[4] = { 0.0, 0.0, 0.0, 0.0 };
	bool clipPlane0Enabled = false;

	GLfloat polygonOffsetFactor = 0.0f;
	GLfloat polygonOffsetUnits = 0.0f;

	GLclampd clearDepthValue = 1.0;
	GLboolean colorMaskR = GL_TRUE;
	GLboolean colorMaskG = GL_TRUE;
	GLboolean colorMaskB = GL_TRUE;
	GLboolean colorMaskA = GL_TRUE;

	float clearColor[4] = { 0, 0, 0, 1 };
	bool blend = false;
	bool alphaTest = false;
	bool depthTest = true;
	bool cullFace = false;
	bool texture2D[QD3D12_MaxTextureUnits] = { true, false };
	bool depthWrite = true;
	GLenum blendSrc = GL_SRC_ALPHA;
	GLenum blendDst = GL_ONE_MINUS_SRC_ALPHA;
	GLenum alphaFunc = GL_GREATER;
	float alphaRef = 0.666f;
	GLenum cullMode = GL_BACK;
	GLenum frontFace = GL_CCW;
	GLenum shadeModel = GL_FLAT;
	GLenum drawBuffer = GL_BACK;
	GLenum readBuffer = GL_BACK;

	GLenum texEnvMode[QD3D12_MaxTextureUnits] = {};
	GLenum texCombineRGB[QD3D12_MaxTextureUnits] = {};
	GLenum texCombineAlpha[QD3D12_MaxTextureUnits] = {};
	GLenum texSource0RGB[QD3D12_MaxTextureUnits] = {};
	GLenum texSource1RGB[QD3D12_MaxTextureUnits] = {};
	GLenum texSource0Alpha[QD3D12_MaxTextureUnits] = {};
	GLenum texSource1Alpha[QD3D12_MaxTextureUnits] = {};
	GLenum texOperand0RGB[QD3D12_MaxTextureUnits] = {};
	GLenum texOperand1RGB[QD3D12_MaxTextureUnits] = {};
	GLenum texOperand0Alpha[QD3D12_MaxTextureUnits] = {};
	GLenum texOperand1Alpha[QD3D12_MaxTextureUnits] = {};
	GLfloat texRGBScale[QD3D12_MaxTextureUnits] = {};
	GLfloat texAlphaScale[QD3D12_MaxTextureUnits] = {};
	GLfloat texEnvColor[QD3D12_MaxTextureUnits][4] = {};

	GLint viewportX = 0;
	GLint viewportY = 0;
	GLsizei viewportW = 640;
	GLsizei viewportH = 480;

	GLuint boundTexture[QD3D12_MaxTextureUnits] = {};
	GLuint activeTextureUnit = 0;

	GLenum currentPrim = 0;
	bool inBeginEnd = false;
	float curU[QD3D12_MaxTextureUnits] = {};
	float curV[QD3D12_MaxTextureUnits] = {};
	float curColor[4] = { 1, 1, 1, 1 };
	ImmediateVertexBuffer immediateVerts;

	GLenum matrixMode = GL_MODELVIEW;
	std::vector<Mat4> modelStack{ Mat4::Identity() };
	std::vector<Mat4> projStack{ Mat4::Identity() };
	std::vector<Mat4> texStack[QD3D12_MaxTextureUnits] = { { Mat4::Identity() }, { Mat4::Identity() } };

	std::unordered_map<GLuint, TextureResource> textures;
	GLuint nextTextureId = 1;
	UINT nextSrvIndex = 1;

	ComPtr<IDXGIFactory4> factory;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> queue;

	ComPtr<ID3D12DescriptorHeap> srvHeap;
	UINT srvStride = 0;

	ComPtr<ID3D12GraphicsCommandList> cmdList;

	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent = nullptr;

	ComPtr<ID3D12RootSignature> rootSig;
	ComPtr<ID3D12RootSignature> postRootSig;
	ComPtr<ID3D12PipelineState> postCopyPSO;
	ComPtr<ID3D12PipelineState> postAdditivePSO;
	ComPtr<ID3D12PipelineState> postDepthCopyPSO;

	ComPtr<ID3DBlob> vsMainBlob;
	ComPtr<ID3DBlob> psMainBlob;
	ComPtr<ID3DBlob> psAlphaBlob;
	ComPtr<ID3DBlob> psUntexturedBlob;
	ComPtr<ID3DBlob> psMainColorOnlyBlob;
	ComPtr<ID3DBlob> psAlphaColorOnlyBlob;
	ComPtr<ID3DBlob> psUntexturedColorOnlyBlob;
	ComPtr<ID3DBlob> postVsBlob;
	ComPtr<ID3DBlob> postPsCopyBlob;
	ComPtr<ID3DBlob> postPsAddBlob;
	ComPtr<ID3DBlob> postPsDepthCopyBlob;

	TextureResource whiteTexture;

	GLenum defaultMinFilter = GL_LINEAR;
	GLenum defaultMagFilter = GL_LINEAR;
	GLenum defaultWrapS = GL_REPEAT;
	GLenum defaultWrapT = GL_REPEAT;

	GLState()
	{
		for (UINT i = 0; i < QD3D12_MaxTextureUnits; ++i)
		{
			texture2D[i] = (i == 0);
			if (texStack[i].empty())
				texStack[i].push_back(Mat4::Identity());
			texEnvMode[i] = GL_MODULATE;
			texCombineRGB[i] = GL_MODULATE;
			texCombineAlpha[i] = GL_MODULATE;
			texSource0RGB[i] = GL_TEXTURE;
			texSource1RGB[i] = GL_PREVIOUS_ARB;
			texSource0Alpha[i] = GL_TEXTURE;
			texSource1Alpha[i] = GL_PREVIOUS_ARB;
			texOperand0RGB[i] = GL_SRC_COLOR;
			texOperand1RGB[i] = GL_SRC_COLOR;
			texOperand0Alpha[i] = GL_SRC_ALPHA;
			texOperand1Alpha[i] = GL_SRC_ALPHA;
			texRGBScale[i] = 1.0f;
			texAlphaScale[i] = 1.0f;
			texEnvColor[i][0] = 0.0f;
			texEnvColor[i][1] = 0.0f;
			texEnvColor[i][2] = 0.0f;
			texEnvColor[i][3] = 0.0f;
		}
	}
};

static GLState g_gl;
static std::unordered_map<HWND, QD3D12Window> g_windows;
QD3D12Window* g_currentWindow = nullptr;

void QD3D12_SetCurrentWindow(struct QD3D12Window* window) {
	if (window != g_currentWindow &&
		g_gl.frameOpen &&
		g_gl.frameOwner &&
		g_gl.frameOwner != window)
	{
		QD3D12_SubmitOpenFrameNoPresentAndWait();
	}

	g_currentWindow = window;
	if (window && window->hdc)
		g_qd3d12CurrentDC = window->hdc;
}

static GLuint g_lightingTextureId = 0;
static TextureResource* g_lightingTexture = nullptr;
static D3D12_RESOURCE_STATES  g_lightingTextureState = D3D12_RESOURCE_STATE_COMMON;

static inline const GLVertex* QD3D12_GetBatchVertices(const QueuedBatch& batch)
{
	return g_gl.frameVerts.buffer + batch.firstVertex;
}

static void QD3D12_InitFrameVertexArena()
{
	g_gl.frameVerts.capacity = GL_FRAME_VERTEX_CAPACITY;
	g_gl.frameVerts.cursor = 0;
	g_gl.frameVerts.buffer = (GLVertex*)_aligned_malloc(sizeof(GLVertex) * g_gl.frameVerts.capacity, 64);
	assert(g_gl.frameVerts.buffer);
}

static void QD3D12_ShutdownFrameVertexArena()
{
	if (g_gl.frameVerts.buffer)
	{
		_aligned_free(g_gl.frameVerts.buffer);
		g_gl.frameVerts.buffer = nullptr;
	}

	g_gl.frameVerts.capacity = 0;
	g_gl.frameVerts.cursor = 0;
}

static inline void QD3D12_ResetFrameVertexArena()
{
	g_gl.frameVerts.cursor = 0;
}

static inline GLVertex* QD3D12_AllocFrameVertices(size_t count, size_t* outFirstVertex)
{
	if (count == 0)
	{
		if (outFirstVertex)
			*outFirstVertex = 0;
		return nullptr;
	}

	const size_t start = g_gl.frameVerts.cursor;
	const size_t end = start + count;

	if (end > g_gl.frameVerts.capacity)
	{
		assert(!"Frame vertex arena overflow");
		return nullptr;
	}

	g_gl.frameVerts.cursor = end;

	if (outFirstVertex)
		*outFirstVertex = start;

	return g_gl.frameVerts.buffer + start;
}

static D3D12_CPU_DESCRIPTOR_HANDLE CurrentPositionRTV()
{
	QD3D12Window& w = *g_currentWindow;
	D3D12_CPU_DESCRIPTOR_HANDLE h = w.rtvHeap->GetCPUDescriptorHandleForHeapStart();
	h.ptr += SIZE_T((QD3D12_FrameCount * 2) + w.frameIndex) * SIZE_T(w.rtvStride);
	return h;
}

static D3D12_CPU_DESCRIPTOR_HANDLE CurrentVelocityRTV()
{
	QD3D12Window& w = *g_currentWindow;
	D3D12_CPU_DESCRIPTOR_HANDLE h = w.rtvHeap->GetCPUDescriptorHandleForHeapStart();
	h.ptr += SIZE_T((QD3D12_FrameCount * 3) + w.frameIndex) * SIZE_T(w.rtvStride);
	return h;
}

static D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferRTV()
{
	QD3D12Window& w = *g_currentWindow;
	D3D12_CPU_DESCRIPTOR_HANDLE h = w.rtvHeap->GetCPUDescriptorHandleForHeapStart();
	h.ptr += SIZE_T((QD3D12_FrameCount * 4) + w.frameIndex) * SIZE_T(w.rtvStride);
	return h;
}

static D3D12_CPU_DESCRIPTOR_HANDLE CurrentSceneDepthDSV()
{
	return g_currentWindow->dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

static D3D12_CPU_DESCRIPTOR_HANDLE CurrentNativeDepthDSV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE h = g_currentWindow->dsvHeap->GetCPUDescriptorHandleForHeapStart();
	const UINT dsvStride = g_gl.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	h.ptr += SIZE_T(dsvStride);
	return h;
}

static UINT QD3D12_ActiveRasterWidth(const QD3D12Window& w)
{
	return (g_gl.framePhase == QD3D12_FRAME_NATIVE_POST_UPSCALE) ? w.width : w.renderWidth;
}

static UINT QD3D12_ActiveRasterHeight(const QD3D12Window& w)
{
	return (g_gl.framePhase == QD3D12_FRAME_NATIVE_POST_UPSCALE) ? w.height : w.renderHeight;
}

static GLBufferObject* QD3D12_GetBuffer(GLuint id)
{
	if (id == 0)
		return nullptr;

	auto it = g_gl.buffers.find(id);
	if (it == g_gl.buffers.end())
		return nullptr;

	return &it->second;
}

static const uint8_t* QD3D12_ResolveArrayPointer(const void* ptr)
{
	if (g_gl.boundArrayBuffer != 0)
	{
		GLBufferObject* bo = QD3D12_GetBuffer(g_gl.boundArrayBuffer);
		if (!bo)
			return nullptr;

		const size_t offset = (size_t)ptr;
		if (offset > bo->data.size())
			return nullptr;

		return bo->data.data() + offset;
	}

	return reinterpret_cast<const uint8_t*>(ptr);
}

static const void* QD3D12_ResolveElementPointer(const void* ptr, GLenum indexType, GLsizei count)
{
	if (g_gl.boundElementArrayBuffer != 0)
	{
		GLBufferObject* bo = QD3D12_GetBuffer(g_gl.boundElementArrayBuffer);
		if (!bo)
			return nullptr;

		size_t indexSize = 0;
		switch (indexType)
		{
		case GL_UNSIGNED_INT:
			indexSize = sizeof(GLuint);
			break;
		case GL_UNSIGNED_SHORT:
			indexSize = sizeof(GLushort);
			break;
		case GL_UNSIGNED_BYTE:
			indexSize = sizeof(GLubyte);
			break;
		default:
			return nullptr;
		}

		const size_t offset = (size_t)ptr;
		const size_t bytesNeeded = (size_t)count * indexSize;
		if (offset > bo->data.size() || bytesNeeded > (bo->data.size() - offset))
			return nullptr;

		return bo->data.data() + offset;
	}

	return ptr;
}

static void QD3D12_RetireResource(ComPtr<ID3D12Resource>& res)
{
	if (!res)
		return;

	RetiredResource rr{};
	rr.resource = res;

	// If no frame has been submitted yet, force it to live until next GPU idle.
	UINT64 fenceValue = 0;

	if (g_gl.frameOpen && g_gl.frameOwner)
	{
		fenceValue = g_gl.frameOwner->frames[g_gl.frameOwner->frameIndex].fenceValue;
		if (fenceValue == 0)
			fenceValue = g_gl.nextFenceValue;
	}
	else
	{
		fenceValue = g_gl.nextFenceValue;
	}

	rr.fenceValue = fenceValue;
	g_retiredResources.push_back(std::move(rr));

	res.Reset();
}

static UINT64 QD3D12_CurrentSubmissionFenceValue()
{
	return g_gl.nextFenceValue;
}

void QD3D12_CollectRetiredResources()
{
	const UINT64 completed = g_gl.fence ? g_gl.fence->GetCompletedValue() : 0;

	size_t write = 0;
	for (size_t read = 0; read < g_retiredResources.size(); ++read)
	{
		if (g_retiredResources[read].fenceValue <= completed)
		{
			// let ComPtr drop here
		}
		else
		{
			if (write != read)
				g_retiredResources[write] = std::move(g_retiredResources[read]);
			++write;
		}
	}
	g_retiredResources.resize(write);
}

// ============================================================
// SECTION 4: shaders
// ============================================================

static const char* kQuakeWrapperHLSL = R"HLSL(
cbuffer DrawCB : register(b0)
{
    float4x4 gMVP;
    float4x4 gPrevMVP;
    float4x4 gModelMatrix;

    float gAlphaRef;
    float gUseTex0;
    float gUseTex1;
    float gTex1IsLightmap;
    float gTexEnvMode0;
    float gTexEnvMode1;
    float geometryFlag;
    float gRoughness;

    float gFogEnabled;
    float gFogMode;
    float gFogDensity;
    float gFogStart;

    float gFogEnd;
    float gPointSize;
    float gMaterialType;
    float gAlphaFunc;

    float4 gFogColor;

    float2 gRenderSize;
    float2 gInvRenderSize;
    float2 gJitterPixels;
    float2 gPrevJitterPixels;
    float4 gMotionPad;

    float4 gTexComb0RGB;
    float4 gTexComb0Alpha;
    float4 gTexComb0Operand;
    float4 gTexEnvColor0;
    float4 gTexComb1RGB;
    float4 gTexComb1Alpha;
    float4 gTexComb1Operand;
    float4 gTexEnvColor1;
};

Texture2D gTex0 : register(t0);
Texture2D gTex1 : register(t1);
SamplerState gSamp0 : register(s0);
SamplerState gSamp1 : register(s1);

struct VSIn
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 col : COLOR0;
};

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 col : COLOR0;
    float fogCoord : TEXCOORD2;
    float3 worldPos : TEXCOORD3;
    float3 normal : TEXCOORD4;
    float4 attr : TEXCOORD5;
    float4 prevClip : TEXCOORD6;
    float4 currClip : TEXCOORD7;
    float psize : PSIZE;
};

struct PSOut
{
    float4 color    : SV_Target0;
    float4 normal   : SV_Target1;
    float4 position : SV_Target2;
    float4 velocity : SV_Target3;
};

float4 QD3D12_GetTexEnvSource(float source, float4 texel, float4 primary, float4 previous, float4 constantColor)
{
    if (source < 0.5)       return texel;
    else if (source < 1.5)  return primary;
    else if (source < 2.5)  return previous;
    else                    return constantColor;
}

float3 QD3D12_ApplyRgbOperand(float4 v, float operand)
{
    if (operand < 0.5)       return v.rgb;          // SRC_COLOR
    else if (operand < 1.5)  return float3(1.0, 1.0, 1.0) - v.rgb; // ONE_MINUS_SRC_COLOR
    else if (operand < 2.5)  return v.aaa;          // SRC_ALPHA
    else                     return float3(1.0, 1.0, 1.0) - v.aaa; // ONE_MINUS_SRC_ALPHA
}

float QD3D12_ApplyAlphaOperand(float4 v, float operand)
{
    if (operand < 0.5)       return v.a;      // SRC_ALPHA
    else if (operand < 1.5)  return 1.0 - v.a; // ONE_MINUS_SRC_ALPHA
    else if (operand < 2.5)  return v.r;      // SRC_COLOR fallback
    else                     return 1.0 - v.r;
}

float4 ApplyTexCombine(
    float4 previous,
    float4 texel,
    float4 primary,
    float mode,
    float4 rgbDesc,
    float4 alphaDesc,
    float4 operandDesc,
    float4 constantColor)
{
    // Packed C++ descriptors:
    // rgbDesc   = { combineRGB, source0RGB, source1RGB, rgbScale }
    // alphaDesc = { combineAlpha, source0Alpha, source1Alpha, alphaScale }
    // operand   = { operand0RGB, operand1RGB, operand0Alpha, operand1Alpha }
    if (mode < 4.5)
    {
        if (mode < 0.5)
            return previous * texel;
        else if (mode < 1.5)
            return texel;
        else if (mode < 2.5)
        {
            float3 rgb = lerp(previous.rgb, texel.rgb, texel.a);
            return float4(rgb, previous.a);
        }
        else if (mode < 3.5)
        {
            // GL_BLEND texture-env:
            // RGB = previous * (1 - texel.rgb) + envColor * texel.rgb
            // A   = previous.a * texel.a
            float3 rgb = lerp(previous.rgb, constantColor.rgb, texel.rgb);
            return float4(rgb, previous.a * texel.a);
        }
        else
        {
            return float4(previous.rgb + texel.rgb, previous.a * texel.a);
        }
    }

    float4 s0rgb = QD3D12_GetTexEnvSource(rgbDesc.y, texel, primary, previous, constantColor);
    float4 s1rgb = QD3D12_GetTexEnvSource(rgbDesc.z, texel, primary, previous, constantColor);
    float3 a0rgb = QD3D12_ApplyRgbOperand(s0rgb, operandDesc.x);
    float3 a1rgb = QD3D12_ApplyRgbOperand(s1rgb, operandDesc.y);

    float3 outRgb;
    if (rgbDesc.x < 1.5)         outRgb = a0rgb;          // REPLACE
    else if (rgbDesc.x < 2.5)    outRgb = a0rgb * a1rgb;  // MODULATE
    else if (rgbDesc.x < 3.5)    outRgb = a0rgb + a1rgb;  // ADD
    else if (rgbDesc.x < 4.5)    outRgb = a0rgb + a1rgb - float3(0.5, 0.5, 0.5); // ADD_SIGNED
    else                         outRgb = a0rgb * a1rgb;  // fallback
    outRgb *= max(rgbDesc.w, 1.0);

    float4 s0a = QD3D12_GetTexEnvSource(alphaDesc.y, texel, primary, previous, constantColor);
    float4 s1a = QD3D12_GetTexEnvSource(alphaDesc.z, texel, primary, previous, constantColor);
    float a0 = QD3D12_ApplyAlphaOperand(s0a, operandDesc.z);
    float a1 = QD3D12_ApplyAlphaOperand(s1a, operandDesc.w);

    float outA;
    if (alphaDesc.x < 1.5)       outA = a0;          // REPLACE
    else if (alphaDesc.x < 2.5)  outA = a0 * a1;     // MODULATE
    else if (alphaDesc.x < 3.5)  outA = a0 + a1;     // ADD
    else if (alphaDesc.x < 4.5)  outA = a0 + a1 - 0.5; // ADD_SIGNED
    else                         outA = a0 * a1;
    outA *= max(alphaDesc.w, 1.0);

    return saturate(float4(outRgb, outA));
}

float TinyNoise(int2 p)
{
    uint n = (uint(p.x) * 1973u) ^ (uint(p.y) * 9277u) ^ 0x68bc21ebu;
    n = (n << 13u) ^ n;
    return 1.0 - float((n * (n * n * 15731u + 789221u) + 1376312589u) & 0x7fffffffu) / 1073741824.0;
}

float3 ApplySoftwareRendererLook(float3 c)
{
    c = saturate(c);

    float luma0 = dot(c, float3(0.299, 0.587, 0.114));

    float3 tinted = c * float3(1.01, 1.00, 0.995);

    float luma1 = dot(tinted, float3(0.299, 0.587, 0.114));
    tinted = lerp(luma1.xxx, tinted, 1.10);

    float luma2 = dot(tinted, float3(0.299, 0.587, 0.114));
    if (luma2 > 0.0001)
        tinted *= (luma0 / luma2);

    return saturate(tinted);
}

float ComputeFogFactor(float fogCoord)
{
    if (gFogEnabled < 0.5)
        return 1.0;

    float f = 1.0;

    if (gFogMode < 0.5)
    {
        float denom = max(gFogEnd - gFogStart, 0.00001);
        f = (gFogEnd - fogCoord) / denom;
    }
    else if (gFogMode < 1.5)
    {
        f = exp(-gFogDensity * fogCoord);
    }
    else
    {
        float d = gFogDensity * fogCoord;
        f = exp(-(d * d));
    }

    return saturate(f);
}

float4 ApplyFog(float4 color, float fogCoord)
{
    float fogFactor = ComputeFogFactor(fogCoord);
    color.rgb = lerp(gFogColor.rgb, color.rgb, fogFactor);
    return color;
}

void QD3D12_AlphaTest(float alpha)
{
    // Encoded by MapAlphaFunc() in C++:
    // 0 NEVER, 1 LESS, 2 EQUAL, 3 LEQUAL, 4 GREATER, 5 NOTEQUAL, 6 GEQUAL, 7 ALWAYS.
    const float eps = 0.00001;

    if (gAlphaFunc < 0.5)
        clip(-1.0);                              // GL_NEVER
    else if (gAlphaFunc < 1.5)
        clip(gAlphaRef - alpha - eps);           // GL_LESS
    else if (gAlphaFunc < 2.5)
        clip(eps - abs(alpha - gAlphaRef));      // GL_EQUAL
    else if (gAlphaFunc < 3.5)
        clip(gAlphaRef - alpha + eps);           // GL_LEQUAL
    else if (gAlphaFunc < 4.5)
        clip(alpha - gAlphaRef - eps);           // GL_GREATER
    else if (gAlphaFunc < 5.5)
        clip(abs(alpha - gAlphaRef) - eps);      // GL_NOTEQUAL
    else if (gAlphaFunc < 6.5)
        clip(alpha - gAlphaRef + eps);           // GL_GEQUAL
    // else GL_ALWAYS: no clipping.
}

float4 BuildTexturedColor(VSOut i)
{
    float4 primary = i.col;
    float4 outColor = primary;

    float2 uv0 = i.uv0;
    float2 uv1 = i.uv1;

    float n = TinyNoise(int2(i.pos.xy)) * 0.0005;
    uv0 += float2(n, -n);
    uv1 += float2(-n, n);

    if (gUseTex0 > 0.5)
    {
        float4 tex0 = gTex0.Sample(gSamp0, uv0);
        outColor = ApplyTexCombine(outColor, tex0, primary, gTexEnvMode0,
                                   gTexComb0RGB, gTexComb0Alpha, gTexComb0Operand, gTexEnvColor0);
    }

    if (gUseTex1 > 0.5)
    {
        float4 tex1 = gTex1.Sample(gSamp1, uv1);
        outColor = ApplyTexCombine(outColor, tex1, primary, gTexEnvMode1,
                                   gTexComb1RGB, gTexComb1Alpha, gTexComb1Operand, gTexEnvColor1);
    }

    outColor.xyz = ApplySoftwareRendererLook(outColor.xyz);
    outColor = ApplyFog(outColor, i.fogCoord);

    return outColor;
}

float2 ClipToUv(float4 clipPos)
{
    float2 ndc = clipPos.xy / max(abs(clipPos.w), 1e-6);
    return float2(ndc.x * 0.5 + 0.5, -ndc.y * 0.5 + 0.5);
}

float4 BuildVelocity(VSOut i)
{
    float2 currUv = ClipToUv(i.currClip);
    float2 prevUv = ClipToUv(i.prevClip);

    // Remove camera jitter from the motion vectors so DLSS / FSR get stable history.
    currUv -= gJitterPixels * gInvRenderSize;
    prevUv -= gPrevJitterPixels * gInvRenderSize;

    float currDepth = i.currClip.z / max(abs(i.currClip.w), 1e-6);
    float prevDepth = i.prevClip.z / max(abs(i.prevClip.w), 1e-6);

    return float4(prevUv - currUv, prevDepth - currDepth, gMaterialType);
}

VSOut VSMain(VSIn i)
{
    VSOut o;

    float4 worldPos = mul(gModelMatrix, float4(i.pos, 1.0));
    float4 currClip = mul(gMVP, float4(i.pos, 1.0));
    float4 prevClip = mul(gPrevMVP, float4(i.pos, 1.0));
    float3 worldNormal = mul((float3x3)gModelMatrix, i.normal);

	currClip.z = 0.5 * (currClip.z + currClip.w);

    o.pos = currClip;
    o.currClip = currClip;
    o.prevClip = prevClip;
    o.fogCoord = abs(currClip.z / max(abs(currClip.w), 0.00001));
    o.uv0 = i.uv0;
    o.uv1 = i.uv1;
    o.col = i.col;
    o.worldPos = worldPos.xyz;
    o.normal = normalize(worldNormal);
    o.attr = float4(geometryFlag, gRoughness, gMaterialType, 0.0);
    o.psize = gPointSize;
    return o;
}

PSOut PSMain(VSOut i)
{
    PSOut o;
    o.color = BuildTexturedColor(i);
    o.normal = float4(i.normal, i.attr.y);
    o.position = float4(i.worldPos, i.attr.x);
    o.velocity = BuildVelocity(i);
    return o;
}

PSOut PSMainAlphaTest(VSOut i)
{
    PSOut o;
    o.color = BuildTexturedColor(i);
    QD3D12_AlphaTest(o.color.a);
    o.normal = float4(i.normal, i.attr.y);
    o.position = float4(i.worldPos, i.attr.x);
    o.velocity = BuildVelocity(i);
    return o;
}

PSOut PSMainUntextured(VSOut i)
{
    PSOut o;
    o.color = ApplyFog(i.col, i.fogCoord);
    o.normal = float4(i.normal, i.attr.y);
    o.position = float4(i.worldPos, i.attr.x);
    o.velocity = BuildVelocity(i);
    return o;
}

float4 PSMainColorOnly(VSOut i) : SV_Target0
{
    return BuildTexturedColor(i);
}

float4 PSMainAlphaTestColorOnly(VSOut i) : SV_Target0
{
    float4 c = BuildTexturedColor(i);
    QD3D12_AlphaTest(c.a);
    return c;
}

float4 PSMainUntexturedColorOnly(VSOut i) : SV_Target0
{
    return ApplyFog(i.col, i.fogCoord);
}
)HLSL";

static const char* kQD3D12PostHLSL = R"HLSL(
Texture2D gTex0 : register(t0);
SamplerState gSamp0 : register(s0);

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

VSOut VSMain(uint vid : SV_VertexID)
{
    VSOut o;
    float2 pos;
    if (vid == 0) pos = float2(-1.0, -1.0);
    else if (vid == 1) pos = float2(-1.0, 3.0);
    else pos = float2(3.0, -1.0);

    o.pos = float4(pos, 0.0, 1.0);
    o.uv = float2(pos.x * 0.5 + 0.5, -pos.y * 0.5 + 0.5);
    return o;
}

float4 PSCopy(VSOut i) : SV_Target0
{
    return gTex0.Sample(gSamp0, i.uv);
}

float4 PSAdd(VSOut i) : SV_Target0
{
    return gTex0.Sample(gSamp0, i.uv);
}

float PSDepthCopy(VSOut i) : SV_Depth
{
    uint srcW, srcH;
    gTex0.GetDimensions(srcW, srcH);

    float2 uv = saturate(i.uv);
    uint2 srcPixel = min(
        (uint2)(uv * float2(srcW, srcH)),
        uint2(max(srcW, 1u) - 1u, max(srcH, 1u) - 1u));

    return gTex0.Load(int3(int2(srcPixel), 0)).r;
}
)HLSL";

static size_t QD3D12_TypeSize(GLenum type) {
	switch (type) {
	case GL_BYTE: return sizeof(GLbyte);
	case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
	case GL_SHORT: return sizeof(GLshort);
	case GL_UNSIGNED_SHORT: return sizeof(GLushort);
	case GL_INT: return sizeof(GLint);
	case GL_UNSIGNED_INT: return sizeof(GLuint);
	case GL_FLOAT: return sizeof(GLfloat);
	case GL_DOUBLE: return sizeof(GLdouble);
	default: return 0;
	}
}

static inline float QD3D12_ReadScalarFast(const uint8_t* p, GLenum type)
{
	switch (type)
	{
	case GL_FLOAT:           return *(const float*)p;
	case GL_DOUBLE:          return (float)(*(const double*)p);
	case GL_INT:             return (float)(*(const GLint*)p);
	case GL_UNSIGNED_INT:    return (float)(*(const GLuint*)p);
	case GL_SHORT:           return (float)(*(const GLshort*)p);
	case GL_UNSIGNED_SHORT:  return (float)(*(const GLushort*)p);
	case GL_BYTE:            return (float)(*(const GLbyte*)p);
	case GL_UNSIGNED_BYTE:   return (float)(*(const GLubyte*)p);
	default:                 return 0.0f;
	}
}

static void QD3D12_FetchArrayVertex(GLint idx, GLVertex& out)
{
	// Direct init is much cheaper than memset + patching fields.
	out.px = 0.0f; out.py = 0.0f; out.pz = 0.0f;
	out.nx = 0.0f; out.ny = 0.0f; out.nz = 1.0f;
	out.r = 1.0f; out.g = 1.0f; out.b = 1.0f; out.a = 1.0f;
	out.u0 = 0.0f; out.v0 = 0.0f;
	out.u1 = 0.0f; out.v1 = 0.0f;
	out.tx = 1.0f; out.ty = 0.0f; out.tz = 0.0f;
	out.bx = 0.0f; out.by = 1.0f; out.bz = 0.0f;

	//
	// Position
	//
	const auto& va = g_gl.vertexArray;
	if (va.enabled && va.ptr)
	{
		const size_t typeSize = (size_t)QD3D12_TypeSize(va.type);
		const size_t elemSize = (size_t)va.size * typeSize;
		const size_t stride = va.stride ? (size_t)va.stride : elemSize;
		const uint8_t* p = va.ptr + stride * (size_t)idx;

		switch (va.type)
		{
		case GL_FLOAT:
		{
			const float* f = (const float*)p;
			if (va.size > 0) out.px = f[0];
			if (va.size > 1) out.py = f[1];
			if (va.size > 2) out.pz = f[2];
			break;
		}
		case GL_DOUBLE:
		{
			const double* f = (const double*)p;
			if (va.size > 0) out.px = (float)f[0];
			if (va.size > 1) out.py = (float)f[1];
			if (va.size > 2) out.pz = (float)f[2];
			break;
		}
		default:
			if (va.size > 0) out.px = QD3D12_ReadScalarFast(p + 0 * typeSize, va.type);
			if (va.size > 1) out.py = QD3D12_ReadScalarFast(p + 1 * typeSize, va.type);
			if (va.size > 2) out.pz = QD3D12_ReadScalarFast(p + 2 * typeSize, va.type);
			break;
		}
	}

	//
	// Normal
	//
	const auto& na = g_gl.normalArray;
	if (na.enabled && na.ptr)
	{
		const size_t typeSize = (size_t)QD3D12_TypeSize(na.type);
		const size_t stride = na.stride ? (size_t)na.stride : (3 * typeSize);
		const uint8_t* p = na.ptr + stride * (size_t)idx;

		switch (na.type)
		{
		case GL_FLOAT:
		{
			const float* f = (const float*)p;
			out.nx = f[0];
			out.ny = f[1];
			out.nz = f[2];
			break;
		}
		case GL_DOUBLE:
		{
			const double* f = (const double*)p;
			out.nx = (float)f[0];
			out.ny = (float)f[1];
			out.nz = (float)f[2];
			break;
		}
		default:
			out.nx = QD3D12_ReadScalarFast(p + 0 * typeSize, na.type);
			out.ny = QD3D12_ReadScalarFast(p + 1 * typeSize, na.type);
			out.nz = QD3D12_ReadScalarFast(p + 2 * typeSize, na.type);
			break;
		}
	}

	//
	// Tangent
	//
	const auto& ta = g_gl.tangentArray;
	if (ta.enabled && ta.ptr)
	{
		const size_t typeSize = (size_t)QD3D12_TypeSize(ta.type);
		const size_t stride = ta.stride ? (size_t)ta.stride : (3 * typeSize);
		const uint8_t* p = ta.ptr + stride * (size_t)idx;

		switch (ta.type)
		{
		case GL_FLOAT:
		{
			const float* f = (const float*)p;
			out.tx = f[0];
			out.ty = f[1];
			out.tz = f[2];
			break;
		}
		case GL_DOUBLE:
		{
			const double* f = (const double*)p;
			out.tx = (float)f[0];
			out.ty = (float)f[1];
			out.tz = (float)f[2];
			break;
		}
		default:
			out.tx = QD3D12_ReadScalarFast(p + 0 * typeSize, ta.type);
			out.ty = QD3D12_ReadScalarFast(p + 1 * typeSize, ta.type);
			out.tz = QD3D12_ReadScalarFast(p + 2 * typeSize, ta.type);
			break;
		}
	}

	//
	// Bitangent
	//
	const auto& ba = g_gl.bitangentArray;
	if (ba.enabled && ba.ptr)
	{
		const size_t typeSize = (size_t)QD3D12_TypeSize(ba.type);
		const size_t stride = ba.stride ? (size_t)ba.stride : (3 * typeSize);
		const uint8_t* p = ba.ptr + stride * (size_t)idx;

		switch (ba.type)
		{
		case GL_FLOAT:
		{
			const float* f = (const float*)p;
			out.bx = f[0];
			out.by = f[1];
			out.bz = f[2];
			break;
		}
		case GL_DOUBLE:
		{
			const double* f = (const double*)p;
			out.bx = (float)f[0];
			out.by = (float)f[1];
			out.bz = (float)f[2];
			break;
		}
		default:
			out.bx = QD3D12_ReadScalarFast(p + 0 * typeSize, ba.type);
			out.by = QD3D12_ReadScalarFast(p + 1 * typeSize, ba.type);
			out.bz = QD3D12_ReadScalarFast(p + 2 * typeSize, ba.type);
			break;
		}
	}

	//
	// Color
	//
	const auto& ca = g_gl.colorArray;
	if (ca.enabled && ca.ptr)
	{
		const size_t typeSize = (size_t)QD3D12_TypeSize(ca.type);
		const size_t elemSize = (size_t)ca.size * typeSize;
		const size_t stride = ca.stride ? (size_t)ca.stride : elemSize;
		const uint8_t* p = ca.ptr + stride * (size_t)idx;

		if (ca.type == GL_UNSIGNED_BYTE)
		{
			static const float kInv255 = 1.0f / 255.0f;
			if (ca.size > 0) out.r = p[0] * kInv255;
			if (ca.size > 1) out.g = p[1] * kInv255;
			if (ca.size > 2) out.b = p[2] * kInv255;
			if (ca.size > 3) out.a = p[3] * kInv255;
		}
		else if (ca.type == GL_FLOAT)
		{
			const float* f = (const float*)p;
			if (ca.size > 0) out.r = f[0];
			if (ca.size > 1) out.g = f[1];
			if (ca.size > 2) out.b = f[2];
			if (ca.size > 3) out.a = f[3];
		}
		else if (ca.type == GL_DOUBLE)
		{
			const double* f = (const double*)p;
			if (ca.size > 0) out.r = (float)f[0];
			if (ca.size > 1) out.g = (float)f[1];
			if (ca.size > 2) out.b = (float)f[2];
			if (ca.size > 3) out.a = (float)f[3];
		}
		else
		{
			if (ca.size > 0) out.r = QD3D12_ReadScalarFast(p + 0 * typeSize, ca.type);
			if (ca.size > 1) out.g = QD3D12_ReadScalarFast(p + 1 * typeSize, ca.type);
			if (ca.size > 2) out.b = QD3D12_ReadScalarFast(p + 2 * typeSize, ca.type);
			if (ca.size > 3) out.a = QD3D12_ReadScalarFast(p + 3 * typeSize, ca.type);
		}
	}

	//
	// Texcoord 0
	//
	{
		const auto& tc = g_gl.texCoordArray[0];
		if (tc.enabled && tc.ptr)
		{
			const size_t typeSize = (size_t)QD3D12_TypeSize(tc.type);
			const size_t elemSize = (size_t)tc.size * typeSize;
			const size_t stride = tc.stride ? (size_t)tc.stride : elemSize;
			const uint8_t* p = tc.ptr + stride * (size_t)idx;

			switch (tc.type)
			{
			case GL_FLOAT:
			{
				const float* f = (const float*)p;
				if (tc.size > 0) out.u0 = f[0];
				if (tc.size > 1) out.v0 = f[1];
				break;
			}
			case GL_DOUBLE:
			{
				const double* f = (const double*)p;
				if (tc.size > 0) out.u0 = (float)f[0];
				if (tc.size > 1) out.v0 = (float)f[1];
				break;
			}
			default:
				if (tc.size > 0) out.u0 = QD3D12_ReadScalarFast(p + 0 * typeSize, tc.type);
				if (tc.size > 1) out.v0 = QD3D12_ReadScalarFast(p + 1 * typeSize, tc.type);
				break;
			}
		}
	}

	//
	// Texcoord 1
	//
	{
		const auto& tc = g_gl.texCoordArray[1];
		if (tc.enabled && tc.ptr)
		{
			const size_t typeSize = (size_t)QD3D12_TypeSize(tc.type);
			const size_t elemSize = (size_t)tc.size * typeSize;
			const size_t stride = tc.stride ? (size_t)tc.stride : elemSize;
			const uint8_t* p = tc.ptr + stride * (size_t)idx;

			switch (tc.type)
			{
			case GL_FLOAT:
			{
				const float* f = (const float*)p;
				if (tc.size > 0) out.u1 = f[0];
				if (tc.size > 1) out.v1 = f[1];
				break;
			}
			case GL_DOUBLE:
			{
				const double* f = (const double*)p;
				if (tc.size > 0) out.u1 = (float)f[0];
				if (tc.size > 1) out.v1 = (float)f[1];
				break;
			}
			default:
				if (tc.size > 0) out.u1 = QD3D12_ReadScalarFast(p + 0 * typeSize, tc.type);
				if (tc.size > 1) out.v1 = QD3D12_ReadScalarFast(p + 1 * typeSize, tc.type);
				break;
			}
		}
	}
}

static D3D12_PRIMITIVE_TOPOLOGY GetDrawTopology(GLenum originalMode)
{
	switch (originalMode)
	{
	case GL_POINTS:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	case GL_LINES:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;

	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST; // we manually convert this, so im adjusting to be a linelist. 

	case GL_TRIANGLE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	case GL_TRIANGLES:
	default:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

static Mat4 QD3D12_GetPreviousMVPForObject(GLuint objectId, const Mat4& currentMvp);


static float MapAlphaFunc(GLenum func);
static float MapTexCombineMode(GLenum mode);
static float MapTexCombineSource(GLenum source);
static float MapTexCombineOperandRGB(GLenum operand);
static float MapTexCombineOperandAlpha(GLenum operand);

static UINT8 QD3D12_CurrentColorWriteMask()
{
	UINT8 mask = 0;
	if (g_gl.colorMaskR) mask |= D3D12_COLOR_WRITE_ENABLE_RED;
	if (g_gl.colorMaskG) mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
	if (g_gl.colorMaskB) mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
	if (g_gl.colorMaskA) mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
	return mask;
}

static UINT8 QD3D12_ClampStencilMask(GLuint mask)
{
	return (UINT8)(mask & 0xFFu);
}

static UINT8 QD3D12_ClampStencilRef(GLint ref)
{
	return (UINT8)(ClampValue<GLint>(ref, 0, 255) & 0xFF);
}

static void QD3D12_FillTexCombineKey(BatchKey& key, UINT unit)
{
	float* rgb = (unit == 0) ? key.texComb0RGB : key.texComb1RGB;
	float* alpha = (unit == 0) ? key.texComb0Alpha : key.texComb1Alpha;
	float* operand = (unit == 0) ? key.texComb0Operand : key.texComb1Operand;
	float* color = (unit == 0) ? key.texEnvColor0 : key.texEnvColor1;

	rgb[0] = MapTexCombineMode(g_gl.texCombineRGB[unit]);
	rgb[1] = MapTexCombineSource(g_gl.texSource0RGB[unit]);
	rgb[2] = MapTexCombineSource(g_gl.texSource1RGB[unit]);
	rgb[3] = g_gl.texRGBScale[unit];

	alpha[0] = MapTexCombineMode(g_gl.texCombineAlpha[unit]);
	alpha[1] = MapTexCombineSource(g_gl.texSource0Alpha[unit]);
	alpha[2] = MapTexCombineSource(g_gl.texSource1Alpha[unit]);
	alpha[3] = g_gl.texAlphaScale[unit];

	operand[0] = MapTexCombineOperandRGB(g_gl.texOperand0RGB[unit]);
	operand[1] = MapTexCombineOperandRGB(g_gl.texOperand1RGB[unit]);
	operand[2] = MapTexCombineOperandAlpha(g_gl.texOperand0Alpha[unit]);
	operand[3] = MapTexCombineOperandAlpha(g_gl.texOperand1Alpha[unit]);

	color[0] = g_gl.texEnvColor[unit][0];
	color[1] = g_gl.texEnvColor[unit][1];
	color[2] = g_gl.texEnvColor[unit][2];
	color[3] = g_gl.texEnvColor[unit][3];
}

#ifdef _DEBUG
#pragma optimize off
#endif
static BatchKey BuildCurrentBatchKey(GLenum originalMode, const TextureResource* tex0, const TextureResource* tex1, TextureResource* const* allTextures)
{
	const bool useTex0 = g_gl.texture2D[0];
	const bool useTex1 = g_gl.texture2D[1];

	BatchKey key{};
	key.pipeline = PickPipeline(useTex0, useTex1);
	key.topology = GetDrawTopology(originalMode);
	key.tex0SrvIndex = tex0 ? tex0->srvIndex : 0;
	key.tex1SrvIndex = tex1 ? tex1->srvIndex : 0;
	for (UINT i = 0; i < QD3D12_MaxTextureUnits; ++i)
		key.textureSrvIndex[i] = (allTextures && allTextures[i]) ? allTextures[i]->srvIndex : 0;

	key.useARBPrograms = QD3D12ARB_IsActive();
	if (key.useARBPrograms)
	{
		key.arbVertexProgram = QD3D12ARB_GetBoundVertexProgram();
		key.arbFragmentProgram = QD3D12ARB_GetBoundFragmentProgram();
		key.arbVertexRevision = QD3D12ARB_GetBoundVertexRevision();
		key.arbFragmentRevision = QD3D12ARB_GetBoundFragmentRevision();
		key.arbVertexBlob = QD3D12ARB_GetVertexShaderBlob();
		key.arbFragmentBlob = QD3D12ARB_GetFragmentShaderBlob();
		QD3D12ARB_FillDrawConstantArrays(&key.arbConstants);
		if (!key.arbVertexBlob || !key.arbFragmentBlob)
			key.useARBPrograms = false;
	}

	key.alphaRef = g_gl.alphaRef;
	key.alphaFunc = MapAlphaFunc(g_gl.alphaFunc);
	key.useTex0 = useTex0 ? 1.0f : 0.0f;
	key.useTex1 = useTex1 ? 1.0f : 0.0f;
	key.viewport = g_currentWindow->viewport;
	key.scissor = g_currentWindow->scissor;
	key.tex1IsLightmap = useTex1 ? 1.0f : 0.0f;
	key.texEnvMode0 = MapTexEnvMode(g_gl.texEnvMode[0]);
	key.texEnvMode1 = MapTexEnvMode(g_gl.texEnvMode[1]);
	QD3D12_FillTexCombineKey(key, 0);
	QD3D12_FillTexCombineKey(key, 1);
	key.colorWriteMask = QD3D12_CurrentColorWriteMask();
	key.cullFaceEnabled = g_gl.cullFace;
	key.cullMode = g_gl.cullMode;
	key.frontFace = g_gl.frontFace;
	key.stencilTest = g_gl.stencilTest;
	key.stencilReadMask = QD3D12_ClampStencilMask(g_gl.stencilFrontFuncMask & g_gl.stencilBackFuncMask);
	key.stencilWriteMask = QD3D12_ClampStencilMask(g_gl.stencilFrontMask | g_gl.stencilBackMask);
	key.stencilRef = QD3D12_ClampStencilRef(g_gl.stencilFrontRef);
	key.stencilFrontFunc = g_gl.stencilFrontFunc;
	key.stencilFrontSFail = g_gl.stencilFrontSFail;
	key.stencilFrontDPFail = g_gl.stencilFrontDPFail;
	key.stencilFrontDPPass = g_gl.stencilFrontDPPass;
	key.stencilBackFunc = g_gl.stencilBackFunc;
	key.stencilBackSFail = g_gl.stencilBackSFail;
	key.stencilBackDPFail = g_gl.stencilBackDPFail;
	key.stencilBackDPPass = g_gl.stencilBackDPPass;
	key.depthBoundsTest = g_gl.depthBoundsTest;
	key.depthBoundsMin = (float)g_gl.depthBoundsMin;
	key.depthBoundsMax = (float)g_gl.depthBoundsMax;
	key.blendSrc = g_gl.blendSrc;
	key.blendDst = g_gl.blendDst;
	key.depthTest = g_gl.depthTest;
	key.depthWrite = g_gl.depthWrite;
	key.depthFunc = g_gl.depthFunc;
	key.mvp = CurrentMVP();
	key.motionObjectId = g_gl.currentMotionObjectId;
	key.prevMvp = QD3D12_GetPreviousMVPForObject(key.motionObjectId, key.mvp);
	key.modelMatrix = CurrentModelMatrix();
	key.geometryFlag = g_gl.currentGeometryFlag;
	key.roughness = g_gl.currentSurfaceRoughness;
	key.materialType = g_gl.currentMaterialType;
	g_gl.currObjectMVPs[key.motionObjectId] = key.mvp;
	return key;
}

#ifdef _DEBUG
#pragma optimize on
#endif


static Mat4 QD3D12_GetPreviousMVPForObject(GLuint objectId, const Mat4& currentMvp)
{
	auto it = g_gl.prevObjectMVPs.find(objectId);
	if (it != g_gl.prevObjectMVPs.end())
		return it->second;

	return currentMvp;
}

static void QD3D12_CommitMotionHistoryForFrame()
{
	g_gl.prevObjectMVPs = g_gl.currObjectMVPs;
	g_gl.currObjectMVPs.clear();
	g_gl.motionHistoryReset = false;
	g_gl.prevJitterX = g_gl.jitterX;
	g_gl.prevJitterY = g_gl.jitterY;
	++g_gl.frameSerial;
}

void APIENTRY glSelectTextureSGIS(GLenum texture)
{
	switch (texture)
	{
	case GL_TEXTURE0_SGIS: g_gl.activeTextureUnit = 0; break;
	case GL_TEXTURE1_SGIS: g_gl.activeTextureUnit = 1; break;
	default: g_gl.activeTextureUnit = 0; break;
	}
}

void APIENTRY glMTexCoord2fSGIS(GLenum texture, GLfloat s, GLfloat t)
{
	GLuint oldUnit = g_gl.activeTextureUnit;

	switch (texture)
	{
	case GL_TEXTURE0_SGIS: g_gl.activeTextureUnit = 0; break;
	case GL_TEXTURE1_SGIS: g_gl.activeTextureUnit = 1; break;
	default: g_gl.activeTextureUnit = 0; break;
	}

	g_gl.curU[g_gl.activeTextureUnit] = s;
	g_gl.curV[g_gl.activeTextureUnit] = t;

	g_gl.activeTextureUnit = oldUnit;
}

void APIENTRY glActiveTextureARB(GLenum texture)
{
	if (texture >= GL_TEXTURE0_ARB)
		g_gl.activeTextureUnit = ClampValue<GLuint>((GLuint)(texture - GL_TEXTURE0_ARB), 0, QD3D12_MaxTextureUnits - 1);
	else
		g_gl.activeTextureUnit = 0;
}

void APIENTRY glMultiTexCoord2fARB(GLenum texture, GLfloat s, GLfloat t)
{
	GLuint unit = 0;
	if (texture >= GL_TEXTURE0_ARB)
		unit = ClampValue<GLuint>((GLuint)(texture - GL_TEXTURE0_ARB), 0, QD3D12_MaxTextureUnits - 1);

	g_gl.curU[unit] = s;
	g_gl.curV[unit] = t;
}


// ============================================================
// SECTION 5: utility mapping
// ============================================================

static std::vector<Mat4>& QD3D12_CurrentMatrixStack()
{
	switch (g_gl.matrixMode)
	{
	case GL_PROJECTION:
		return g_gl.projStack;

	case GL_TEXTURE:
		return g_gl.texStack[g_gl.activeTextureUnit];

	case GL_MODELVIEW:
	default:
		return g_gl.modelStack;
	}
}

static int BytesPerPixel(GLenum format, GLenum type)
{
	if (type != GL_UNSIGNED_BYTE)
		return 4;

	switch (format)
	{
	case GL_ALPHA:
	case GL_LUMINANCE:
	case GL_INTENSITY:
		return 1;
	case GL_RGB:
		return 3;
	case GL_RGBA:
	default:
		return 4;
	}
}

static DXGI_FORMAT MapTextureFormat(GLenum format)
{
	(void)format;

	// The fixed-function shader path samples RGBA for all legacy uncompressed
	// texture formats. Alpha, luminance, and intensity are expanded to RGBA8
	// in UploadTexture(), so the D3D12 resource must also be RGBA8.
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

static bool QD3D12_IsCompressedTextureFormat(GLenum internalFormat)
{
	switch (internalFormat)
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return true;
	default:
		return false;
	}
}

static DXGI_FORMAT QD3D12_MapCompressedTextureFormat(GLenum internalFormat)
{
	switch (internalFormat)
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return DXGI_FORMAT_BC1_UNORM;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		return DXGI_FORMAT_BC2_UNORM;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return DXGI_FORMAT_BC3_UNORM;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

static UINT QD3D12_CompressedTextureBlockBytes(GLenum internalFormat)
{
	switch (internalFormat)
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return 8;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return 16;
	default:
		return 0;
	}
}

static UINT QD3D12_CompressedTextureBlocksWide(GLsizei width)
{
	return std::max<UINT>(1u, ((UINT)width + 3u) / 4u);
}

static UINT QD3D12_CompressedTextureBlocksHigh(GLsizei height)
{
	return std::max<UINT>(1u, ((UINT)height + 3u) / 4u);
}

static UINT QD3D12_CompressedTextureImageSize(GLsizei width, GLsizei height, GLenum internalFormat)
{
	const UINT blockBytes = QD3D12_CompressedTextureBlockBytes(internalFormat);
	if (width <= 0 || height <= 0 || blockBytes == 0)
		return 0;

	const UINT blocksWide = QD3D12_CompressedTextureBlocksWide(width);
	const UINT blocksHigh = QD3D12_CompressedTextureBlocksHigh(height);
	return blocksWide * blocksHigh * blockBytes;
}


static UINT QD3D12_TextureShaderComponentMapping(const TextureResource& tex)
{
	if (!tex.forceOpaqueAlpha)
		return D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
		D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
		D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
		D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
		D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
}


static float MapTexCombineMode(GLenum mode)
{
	switch (mode)
	{
	case GL_REPLACE: return 1.0f;
	case GL_MODULATE: return 2.0f;
#ifdef GL_ADD
	case GL_ADD: return 3.0f;
#endif
#ifdef GL_ADD_SIGNED_ARB
	case GL_ADD_SIGNED_ARB: return 4.0f;
#endif
	default: return 2.0f;
	}
}

static float MapTexCombineSource(GLenum source)
{
	switch (source)
	{
	case GL_TEXTURE: return 0.0f;
#ifdef GL_PRIMARY_COLOR_ARB
	case GL_PRIMARY_COLOR_ARB: return 1.0f;
#endif
#ifdef GL_PREVIOUS_ARB
	case GL_PREVIOUS_ARB: return 2.0f;
#endif
#ifdef GL_CONSTANT_ARB
	case GL_CONSTANT_ARB: return 3.0f;
#endif
	default: return 0.0f;
	}
}

static float MapTexCombineOperandRGB(GLenum operand)
{
	switch (operand)
	{
	case GL_SRC_COLOR: return 0.0f;
	case GL_ONE_MINUS_SRC_COLOR: return 1.0f;
	case GL_SRC_ALPHA: return 2.0f;
	case GL_ONE_MINUS_SRC_ALPHA: return 3.0f;
	default: return 0.0f;
	}
}

static float MapTexCombineOperandAlpha(GLenum operand)
{
	switch (operand)
	{
	case GL_SRC_ALPHA: return 0.0f;
	case GL_ONE_MINUS_SRC_ALPHA: return 1.0f;
	case GL_SRC_COLOR: return 2.0f;
	case GL_ONE_MINUS_SRC_COLOR: return 3.0f;
	default: return 0.0f;
	}
}

static D3D12_BLEND MapBlendAlpha(GLenum v) {
	switch (v) {
	case GL_ZERO:                  return D3D12_BLEND_ZERO;
	case GL_ONE:                   return D3D12_BLEND_ONE;

	case GL_SRC_ALPHA:             return D3D12_BLEND_SRC_ALPHA;
	case GL_ONE_MINUS_SRC_ALPHA:   return D3D12_BLEND_INV_SRC_ALPHA;
	case GL_DST_ALPHA:             return D3D12_BLEND_DEST_ALPHA;
	case GL_ONE_MINUS_DST_ALPHA:   return D3D12_BLEND_INV_DEST_ALPHA;

		// Color factors are illegal in alpha slots.
		// Fold them to something reasonable.
	case GL_SRC_COLOR:             return D3D12_BLEND_SRC_ALPHA;
	case GL_ONE_MINUS_SRC_COLOR:   return D3D12_BLEND_INV_SRC_ALPHA;
	case GL_DST_COLOR:             return D3D12_BLEND_DEST_ALPHA;
	case GL_ONE_MINUS_DST_COLOR:   return D3D12_BLEND_INV_DEST_ALPHA;

	case GL_SRC_ALPHA_SATURATE:    return D3D12_BLEND_ONE;

	default:                       return D3D12_BLEND_ONE;
	}
}

static D3D12_BLEND MapBlend(GLenum v)
{
	switch (v)
	{
	case GL_ZERO:                  return D3D12_BLEND_ZERO;
	case GL_ONE:                   return D3D12_BLEND_ONE;
	case GL_SRC_COLOR:             return D3D12_BLEND_SRC_COLOR;
	case GL_ONE_MINUS_SRC_COLOR:   return D3D12_BLEND_INV_SRC_COLOR;
	case GL_DST_COLOR:             return D3D12_BLEND_DEST_COLOR;
	case GL_ONE_MINUS_DST_COLOR:   return D3D12_BLEND_INV_DEST_COLOR;
	case GL_SRC_ALPHA:             return D3D12_BLEND_SRC_ALPHA;
	case GL_ONE_MINUS_SRC_ALPHA:   return D3D12_BLEND_INV_SRC_ALPHA;
	case GL_DST_ALPHA:             return D3D12_BLEND_DEST_ALPHA;
	case GL_ONE_MINUS_DST_ALPHA:   return D3D12_BLEND_INV_DEST_ALPHA;
	case GL_SRC_ALPHA_SATURATE:    return D3D12_BLEND_SRC_ALPHA_SAT;
	default:                       return D3D12_BLEND_ONE;
	}
}

static float MapAlphaFunc(GLenum func)
{
	switch (func)
	{
	case GL_NEVER:    return 0.0f;
	case GL_LESS:     return 1.0f;
	case GL_EQUAL:    return 2.0f;
	case GL_LEQUAL:   return 3.0f;
	case GL_GREATER:  return 4.0f;
	case GL_NOTEQUAL: return 5.0f;
	case GL_GEQUAL:   return 6.0f;
	case GL_ALWAYS:   return 7.0f;
	default:          return 4.0f; // Match legacy default: GL_GREATER.
	}
}

static D3D12_COMPARISON_FUNC MapCompare(GLenum f)
{
	switch (f)
	{
	case GL_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
	case GL_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case GL_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case GL_LEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case GL_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case GL_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case GL_GEQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case GL_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	default: return D3D12_COMPARISON_FUNC_ALWAYS;
	}
}


static D3D12_CULL_MODE MapCull(GLenum m);

static D3D12_STENCIL_OP MapStencilOp(GLenum op)
{
	switch (op)
	{
	case GL_KEEP: return D3D12_STENCIL_OP_KEEP;
	case GL_ZERO: return D3D12_STENCIL_OP_ZERO;
	case GL_REPLACE: return D3D12_STENCIL_OP_REPLACE;
	case GL_INCR: return D3D12_STENCIL_OP_INCR_SAT;
	case GL_DECR: return D3D12_STENCIL_OP_DECR_SAT;
#ifdef GL_INCR_WRAP
	case GL_INCR_WRAP: return D3D12_STENCIL_OP_INCR;
#endif
#ifdef GL_DECR_WRAP
	case GL_DECR_WRAP: return D3D12_STENCIL_OP_DECR;
#endif
	case GL_INVERT: return D3D12_STENCIL_OP_INVERT;
	default: return D3D12_STENCIL_OP_KEEP;
	}
}

static D3D12_DEPTH_STENCILOP_DESC BuildStencilFaceDesc(GLenum func, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	D3D12_DEPTH_STENCILOP_DESC d{};
	d.StencilFailOp = MapStencilOp(sfail);
	d.StencilDepthFailOp = MapStencilOp(dpfail);
	d.StencilPassOp = MapStencilOp(dppass);
	d.StencilFunc = MapCompare(func);
	return d;
}

static void ApplyRasterDepthStencilState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& d, const BatchKey& key)
{
	d.RasterizerState.CullMode = key.cullFaceEnabled ? MapCull(key.cullMode) : D3D12_CULL_MODE_NONE;
	d.RasterizerState.FrontCounterClockwise = (key.frontFace == GL_CCW) ? TRUE : FALSE;

	d.DepthStencilState.DepthEnable = key.depthTest ? TRUE : FALSE;
	d.DepthStencilState.DepthWriteMask = key.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	d.DepthStencilState.DepthFunc = MapCompare(key.depthFunc);
	d.DepthStencilState.StencilEnable = key.stencilTest ? TRUE : FALSE;
	d.DepthStencilState.StencilReadMask = key.stencilReadMask;
	d.DepthStencilState.StencilWriteMask = key.stencilWriteMask;
	d.DepthStencilState.FrontFace = BuildStencilFaceDesc(key.stencilFrontFunc, key.stencilFrontSFail, key.stencilFrontDPFail, key.stencilFrontDPPass);
	d.DepthStencilState.BackFace = BuildStencilFaceDesc(key.stencilBackFunc, key.stencilBackSFail, key.stencilBackDPFail, key.stencilBackDPPass);
}

static D3D12_CULL_MODE MapCull(GLenum m)
{
	switch (m)
	{
	case GL_FRONT: return D3D12_CULL_MODE_FRONT;
	case GL_BACK: return D3D12_CULL_MODE_BACK;
	default: return D3D12_CULL_MODE_NONE;
	}
}

static D3D12_FILTER MapFilter(GLenum minFilter, GLenum magFilter)
{
	const bool linearMin = (minFilter == GL_LINEAR || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_LINEAR);
	const bool linearMag = (magFilter == GL_LINEAR);

	if (linearMin && linearMag)
		return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	if (!linearMin && !linearMag)
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	if (linearMin && !linearMag)
		return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
}

static D3D12_TEXTURE_ADDRESS_MODE MapAddress(GLenum wrap)
{
	switch (wrap)
	{
	case GL_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case GL_REPEAT:
	default: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
}

static D3D12_PRIMITIVE_TOPOLOGY MapPrimitive(GLenum mode)
{
	switch (mode)
	{
	case GL_POINTS:         return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case GL_LINES:          return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case GL_LINE_STRIP:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case GL_TRIANGLES:      return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case GL_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	default:                return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyTypeFromTopology(D3D12_PRIMITIVE_TOPOLOGY topo)
{
	switch (topo)
	{
	case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

	case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
	case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
	default:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
}

// ============================================================
// SECTION 6: upload helpers
// ============================================================

static void QD3D12_WaitForGPU()
{
	const UINT64 signalValue = g_gl.nextFenceValue++;
	QD3D12_CHECK(g_gl.queue->Signal(g_gl.fence.Get(), signalValue));

	if (g_gl.fence->GetCompletedValue() < signalValue)
	{
		QD3D12_CHECK(g_gl.fence->SetEventOnCompletion(signalValue, g_gl.fenceEvent));
		WaitForSingleObject(g_gl.fenceEvent, INFINITE);
	}
}

static void QD3D12_WaitForFrame(UINT frameIndex)
{
	FrameResources& fr = g_currentWindow->frames[frameIndex];
	if (fr.fenceValue != 0 && g_gl.fence->GetCompletedValue() < fr.fenceValue)
	{
		QD3D12_CHECK(g_gl.fence->SetEventOnCompletion(fr.fenceValue, g_gl.fenceEvent));
		WaitForSingleObject(g_gl.fenceEvent, INFINITE);
	}
}

static void QD3D12_ResetUploadRing()
{
	if (!g_currentWindow)
		return;

	g_currentWindow->upload.offset = 0;
}

struct UploadAlloc
{
	void* cpu = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
	UINT offset = 0;
};

static inline UINT QD3D12_AlignUpUINT(UINT value, UINT alignment)
{
	if (alignment == 0)
		alignment = 1;

	return (value + (alignment - 1)) & ~(alignment - 1);
}

static inline UINT64 QD3D12_AlignUpUINT64(UINT64 value, UINT64 alignment)
{
	if (alignment == 0)
		alignment = 1;

	return (value + (alignment - 1)) & ~(alignment - 1);
}

static bool QD3D12_DrainUploadRingAndContinue()
{
	if (!g_currentWindow || !g_gl.cmdList || !g_gl.queue || !g_gl.fence)
		return false;

	QD3D12Window& w = *g_currentWindow;

	// Submit the work recorded so far, wait for it, then reuse this frame's
	// upload resource from offset 0. This keeps texture streaming / huge UI
	// uploads from hard-failing when a frame exceeds the ring size.
	QD3D12_ExecuteMainCommandListAndWait(w);
	QD3D12_ResetUploadRing();

	// Command-list state is reset by ExecuteMainCommandListAndWait().
	// Re-bind the current phase targets so subsequent copies/draws continue.
	QD3D12_BindTargetsForCurrentPhase(w);
	return true;
}

static bool QD3D12_EnsureUploadSpace(UINT bytes, UINT alignment)
{
	if (!g_currentWindow)
		return false;

	UploadRing& upload = g_currentWindow->upload;

	if (bytes > upload.size)
	{
		QD3D12_Fatal(
			"Single upload allocation too large: need %u bytes, upload ring size %u",
			bytes,
			upload.size);
		return false;
	}

	const UINT alignedOffset = QD3D12_AlignUpUINT(upload.offset, alignment);
	if ((UINT64)alignedOffset + (UINT64)bytes <= (UINT64)upload.size)
		return false;

	if (!QD3D12_DrainUploadRingAndContinue())
	{
		QD3D12_Fatal(
			"Per-frame upload buffer overflow: need %u bytes, aligned offset %u, size %u",
			bytes,
			alignedOffset,
			upload.size);
		return false;
	}

	const UINT retryOffset = QD3D12_AlignUpUINT(upload.offset, alignment);
	if ((UINT64)retryOffset + (UINT64)bytes > (UINT64)upload.size)
	{
		QD3D12_Fatal(
			"Upload allocation still does not fit after drain: need %u bytes, aligned offset %u, size %u",
			bytes,
			retryOffset,
			upload.size);
	}

	return true;
}

static bool QD3D12_EnsureUploadSpaceForDraw(UINT vbBytes, UINT cbBytes)
{
	if (!g_currentWindow)
		return false;

	UploadRing& upload = g_currentWindow->upload;

	if (vbBytes > upload.size || cbBytes > upload.size)
	{
		QD3D12_Fatal(
			"Single draw upload too large: vb=%u cb=%u upload ring size=%u",
			vbBytes,
			cbBytes,
			upload.size);
		return false;
	}

	UINT64 cursor = upload.offset;
	const UINT64 vbOffset = QD3D12_AlignUpUINT64(cursor, 256);
	cursor = vbOffset + (UINT64)vbBytes;

	const UINT64 cbOffset = QD3D12_AlignUpUINT64(cursor, 256);
	cursor = cbOffset + (UINT64)cbBytes;

	if (cursor <= (UINT64)upload.size)
		return false;

	if (!QD3D12_DrainUploadRingAndContinue())
	{
		QD3D12_Fatal(
			"Per-frame upload buffer overflow during draw upload: vb=%u cb=%u offset=%u size=%u",
			vbBytes,
			cbBytes,
			upload.offset,
			upload.size);
		return false;
	}

	cursor = upload.offset;
	const UINT64 retryVbOffset = QD3D12_AlignUpUINT64(cursor, 256);
	cursor = retryVbOffset + (UINT64)vbBytes;

	const UINT64 retryCbOffset = QD3D12_AlignUpUINT64(cursor, 256);
	cursor = retryCbOffset + (UINT64)cbBytes;

	if (cursor > (UINT64)upload.size)
	{
		QD3D12_Fatal(
			"Draw upload still does not fit after drain: vb=%u cb=%u upload ring size=%u",
			vbBytes,
			cbBytes,
			upload.size);
	}

	return true;
}

static UploadAlloc QD3D12_AllocUpload(UINT bytes, UINT alignment)
{
	if (alignment == 0)
		alignment = 1;

	QD3D12_EnsureUploadSpace(bytes, alignment);

	UploadRing& upload = g_currentWindow->upload;
	UINT alignedOffset = QD3D12_AlignUpUINT(upload.offset, alignment);

	if ((UINT64)alignedOffset + (UINT64)bytes > (UINT64)upload.size)
	{
		QD3D12_Fatal(
			"Per-frame upload buffer overflow: need %u bytes, aligned offset %u, size %u",
			bytes,
			alignedOffset,
			upload.size);
	}

	UploadAlloc out;
	out.offset = alignedOffset;
	out.cpu = upload.cpuBase[g_currentWindow->frameIndex] + alignedOffset;
	out.gpu = upload.gpuBase[g_currentWindow->frameIndex] + alignedOffset;

	upload.offset = alignedOffset + bytes;
	return out;
}

// ============================================================
// SECTION 7: D3D12 initialization
// ============================================================

static void QD3D12_CreateOcclusionQueryObjects()
{
	D3D12_QUERY_HEAP_DESC qh{};
	qh.Count = QD3D12_MaxQueries;
	qh.NodeMask = 0;
	qh.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
	QD3D12_CHECK(g_gl.device->CreateQueryHeap(&qh, IID_PPV_ARGS(&g_gl.occlusionQueryHeap)));

	D3D12_HEAP_PROPERTIES hp{};
	hp.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC rd{};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = sizeof(UINT64) * QD3D12_MaxQueries;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	QD3D12_CHECK(
		g_gl.device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&g_gl.occlusionReadback)));

	QD3D12_CHECK(g_gl.occlusionReadback->Map(0, nullptr, (void**)&g_gl.occlusionReadbackCpu));
}


// ============================================================
// SECTION 6.5: temporal upscalers / AI denoise helpers
// ============================================================

static float QD3D12_QualityRatio(QD3D12UpscalerQuality quality)
{
	switch (quality)
	{
	case QD3D12_QUALITY_QUALITY:            return 0.6666667f;
	case QD3D12_QUALITY_BALANCED:          return 0.58f;
	case QD3D12_QUALITY_PERFORMANCE:       return 0.5f;
	case QD3D12_QUALITY_ULTRA_PERFORMANCE: return 0.3333333f;
	case QD3D12_QUALITY_NATIVE:
	default:                               return 1.0f;
	}
}

#if defined(QD3D12_ENABLE_STREAMLINE)
struct QD3D12StreamlineState
{
	bool initialized = false;
	bool deviceBound = false;
	sl::ViewportHandle viewport = { 1 };
};
static QD3D12StreamlineState g_qd3d12Sl;

static sl::DLSSMode QD3D12_MapDLSSMode(QD3D12UpscalerQuality quality)
{
	switch (quality)
	{
	case QD3D12_QUALITY_QUALITY:            return sl::DLSSMode::eUltraQuality;
	case QD3D12_QUALITY_BALANCED:          return sl::DLSSMode::eBalanced;
	case QD3D12_QUALITY_PERFORMANCE:       return sl::DLSSMode::eMaxPerformance;
	case QD3D12_QUALITY_ULTRA_PERFORMANCE: return sl::DLSSMode::eUltraPerformance;
	case QD3D12_QUALITY_NATIVE:
	default:                               return sl::DLSSMode::eOff;
	}
}

static void QD3D12_InitStreamlineEarly()
{
	if (g_qd3d12Sl.initialized)
		return;

	sl::Preferences pref{};
	pref.showConsole = true;
	pref.applicationId = 231313132; // Replace with your NVIDIA-assigned application ID for NGX-backed features like DLSS.
	pref.engine = sl::EngineType::eCustom;
	pref.engineVersion = "IceBridge 1.0";
	pref.flags |= sl::PreferenceFlags::eUseFrameBasedResourceTagging;
	pref.flags |= sl::PreferenceFlags::eAllowOTA | sl::PreferenceFlags::eLoadDownloadedPlugins;

	sl::Feature features[] =
	{
		sl::kFeatureDLSS,
		sl::kFeatureDLSS_RR
	};

	pref.featuresToLoad = features;
	pref.numFeaturesToLoad = _countof(features);

	auto result = slInit(pref);
	if (result != sl::Result::eOk)
	{
		QD3D12_Log("Streamline init failed (%d), DLSS disabled.", int(result));
		return;
	}

	g_qd3d12Sl.initialized = true;
}

static void QD3D12_StreamlineOnDeviceCreated()
{
	if (!g_qd3d12Sl.initialized || g_qd3d12Sl.deviceBound || !g_gl.device)
		return;

	auto result = slSetD3DDevice(g_gl.device.Get());
	if (result != sl::Result::eOk)
	{
		QD3D12_Log("slSetD3DDevice failed (%d), DLSS disabled.", int(result));
		return;
	}

	g_qd3d12Sl.deviceBound = true;
}

static bool QD3D12_WantsDLSSRayReconstruction()
{
	return g_gl.enableDLSSRayReconstruction &&
		g_gl.upscalerBackend == QD3D12_UPSCALER_DLSS &&
		g_qd3d12Sl.deviceBound;
}

static bool QD3D12_UseLightingTextureAsUpscaleInput(const QD3D12Window&)
{
	return QD3D12_WantsDLSSRayReconstruction() &&
		g_gl.raytracedLightingReadyThisFrame &&
		g_lightingTexture &&
		g_lightingTexture->texture;
}

static sl::Result QD3D12_SetStreamlineCommonConstants(sl::FrameToken& frameToken)
{
	sl::Constants consts{};
	memcpy(&consts.cameraViewToClip, g_gl.cameraState.viewToClip.m, sizeof(float) * 16);
	memcpy(&consts.clipToCameraView, g_gl.cameraState.clipToView.m, sizeof(float) * 16);
	memcpy(&consts.clipToPrevClip, g_gl.cameraState.clipToPrevClip.m, sizeof(float) * 16);
	memcpy(&consts.prevClipToClip, g_gl.cameraState.prevClipToClip.m, sizeof(float) * 16);
	consts.jitterOffset = { g_gl.jitterX, g_gl.jitterY };
	consts.mvecScale = { 1.0f, 1.0f }; // motion vectors are written in PreviousUV - CurrentUV
	memcpy(&consts.cameraPos, g_gl.cameraState.cameraPos, sizeof(float) * 3);
	memcpy(&consts.cameraRight, g_gl.cameraState.cameraRight, sizeof(float) * 3);
	memcpy(&consts.cameraUp, g_gl.cameraState.cameraUp, sizeof(float) * 3);
	memcpy(&consts.cameraFwd, g_gl.cameraState.cameraForward, sizeof(float) * 3);
	consts.cameraNear = g_gl.cameraState.nearPlane;
	consts.cameraFar = g_gl.cameraState.farPlane;
	consts.cameraFOV = g_gl.cameraState.verticalFovRadians;
	consts.cameraAspectRatio = g_gl.cameraState.aspectRatio;
	consts.reset = g_gl.motionHistoryReset ? sl::Boolean::eTrue : sl::Boolean::eFalse;
	consts.depthInverted = sl::Boolean::eFalse;
	consts.cameraMotionIncluded = sl::Boolean::eTrue;
	consts.motionVectors3D = sl::Boolean::eFalse;
	consts.motionVectorsDilated = sl::Boolean::eFalse;
	consts.motionVectorsJittered = sl::Boolean::eFalse;
	return slSetConstants(consts, frameToken, g_qd3d12Sl.viewport);
}
#else
static void QD3D12_InitStreamlineEarly() {}
static void QD3D12_StreamlineOnDeviceCreated() {}
static bool QD3D12_WantsDLSSRayReconstruction() { return false; }
static bool QD3D12_UseLightingTextureAsUpscaleInput(const QD3D12Window&) { return false; }
#endif

#if defined(QD3D12_ENABLE_FFX)
struct QD3D12FfxState
{
	bool upscaleContextValid = false;
	UINT maxRenderWidth = 0;
	UINT maxRenderHeight = 0;
	UINT maxOutputWidth = 0;
	UINT maxOutputHeight = 0;
	ffx::Context upscaleContext;
};
static QD3D12FfxState g_qd3d12Ffx;

static void QD3D12_DestroyFfxUpscaleContext()
{
	if (g_qd3d12Ffx.upscaleContextValid)
	{
		ffx::DestroyContext(g_qd3d12Ffx.upscaleContext);
		g_qd3d12Ffx.upscaleContextValid = false;
	}
}

static void QD3D12_EnsureFfxUpscaleContext(QD3D12Window& w)
{
	if (!g_gl.device)
		return;

	if (g_qd3d12Ffx.upscaleContextValid &&
		g_qd3d12Ffx.maxRenderWidth == w.renderWidth &&
		g_qd3d12Ffx.maxRenderHeight == w.renderHeight &&
		g_qd3d12Ffx.maxOutputWidth == w.width &&
		g_qd3d12Ffx.maxOutputHeight == w.height)
	{
		return;
	}

	QD3D12_DestroyFfxUpscaleContext();

	ffx::CreateBackendDX12Desc backendDesc{};
	backendDesc.device = g_gl.device.Get();

	ffx::CreateContextDescUpscale createDesc{};
	createDesc.maxUpscaleSize = { w.width, w.height };
	createDesc.maxRenderSize = { w.renderWidth, w.renderHeight };
	createDesc.flags = 0;
#ifdef FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE
	createDesc.flags |= FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
#endif
#ifdef FFX_UPSCALE_ENABLE_AUTO_EXPOSURE
	createDesc.flags |= FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;
#endif

	ffx::ReturnCode rc = ffx::CreateContext(g_qd3d12Ffx.upscaleContext, nullptr, createDesc, backendDesc);
	if (rc != ffx::ReturnCode::Ok)
	{
		QD3D12_Log("ffx::CreateContext(Upscale) failed (%d), FSR disabled.", int(rc));
		return;
	}

	g_qd3d12Ffx.upscaleContextValid = true;
	g_qd3d12Ffx.maxRenderWidth = w.renderWidth;
	g_qd3d12Ffx.maxRenderHeight = w.renderHeight;
	g_qd3d12Ffx.maxOutputWidth = w.width;
	g_qd3d12Ffx.maxOutputHeight = w.height;
}
#else
static void QD3D12_DestroyFfxUpscaleContext() {}
static void QD3D12_EnsureFfxUpscaleContext(QD3D12Window&) {}
#endif

static void QD3D12_SelectRenderResolution(QD3D12Window& w, UINT outputWidth, UINT outputHeight)
{
	w.width = outputWidth;
	w.height = outputHeight;
	w.renderWidth = outputWidth;
	w.renderHeight = outputHeight;

	if (g_gl.upscalerBackend == QD3D12_UPSCALER_NONE || g_gl.upscalerQuality == QD3D12_QUALITY_NATIVE)
		return;

#if defined(QD3D12_ENABLE_STREAMLINE)
	if (g_gl.upscalerBackend == QD3D12_UPSCALER_DLSS && g_qd3d12Sl.deviceBound)
	{
		if (QD3D12_WantsDLSSRayReconstruction())
		{
			sl::DLSSDOptions rrOptions{};
			rrOptions.mode = QD3D12_MapDLSSMode(g_gl.upscalerQuality);
			rrOptions.outputWidth = outputWidth;
			rrOptions.outputHeight = outputHeight;
			rrOptions.sharpness = g_gl.upscalerSharpness;
			rrOptions.preExposure = 1.0f;
			rrOptions.exposureScale = 1.0f;
			rrOptions.colorBuffersHDR = sl::Boolean::eTrue;
			rrOptions.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::ePacked;

			sl::DLSSDOptimalSettings rrOptimal{};
			const sl::Result rrOptimalResult = slDLSSDGetOptimalSettings(rrOptions, rrOptimal);
			if (rrOptimalResult == sl::Result::eOk)
			{
				w.renderWidth = rrOptimal.optimalRenderWidth;
				w.renderHeight = rrOptimal.optimalRenderHeight;
				return;
			}

			QD3D12_Log("slDLSSDGetOptimalSettings failed (%d), falling back to DLSS SR sizing.", int(rrOptimalResult));
		}

		sl::DLSSOptions options{};
		options.mode = QD3D12_MapDLSSMode(g_gl.upscalerQuality);
		options.outputWidth = outputWidth;
		options.outputHeight = outputHeight;

		sl::DLSSOptimalSettings optimal{};
		const sl::Result optimalResult = slDLSSGetOptimalSettings(options, optimal);
		if (optimalResult == sl::Result::eOk)
		{
			w.renderWidth = optimal.optimalRenderWidth;
			w.renderHeight = optimal.optimalRenderHeight;
			return;
		}
	}
#endif

	const float ratio = QD3D12_QualityRatio(g_gl.upscalerQuality);
	w.renderWidth = std::max<UINT>(1, (UINT)std::lround((double)outputWidth * ratio));
	w.renderHeight = std::max<UINT>(1, (UINT)std::lround((double)outputHeight * ratio));
}

static void QD3D12_PostFullscreenPass(ID3D12GraphicsCommandList* cl,
	ID3D12PipelineState* pso,
	D3D12_GPU_DESCRIPTOR_HANDLE inputSrv,
	D3D12_CPU_DESCRIPTOR_HANDLE outputRtv,
	const D3D12_VIEWPORT& viewport,
	const D3D12_RECT& scissor)
{
	if (!cl || !pso)
		return;

	ID3D12DescriptorHeap* heaps[] = { g_gl.srvHeap.Get() };
	cl->SetDescriptorHeaps(_countof(heaps), heaps);
	cl->SetGraphicsRootSignature(g_gl.postRootSig.Get());
	cl->SetPipelineState(pso);
	cl->OMSetRenderTargets(1, &outputRtv, FALSE, nullptr);
	cl->RSSetViewports(1, &viewport);
	cl->RSSetScissorRects(1, &scissor);
	cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cl->SetGraphicsRootDescriptorTable(0, inputSrv);
	cl->DrawInstanced(3, 1, 0, 0);
}

static void QD3D12_ExecuteMainCommandListAndWait(QD3D12Window& w)
{
	QD3D12_CHECK(g_gl.cmdList->Close());

	ID3D12CommandList* lists[] = { g_gl.cmdList.Get() };
	g_gl.queue->ExecuteCommandLists(1, lists);

	const UINT64 signalValue = g_gl.nextFenceValue++;
	QD3D12_CHECK(g_gl.queue->Signal(g_gl.fence.Get(), signalValue));
	w.frames[w.frameIndex].fenceValue = signalValue;

	if (g_gl.fence->GetCompletedValue() < signalValue)
	{
		QD3D12_CHECK(g_gl.fence->SetEventOnCompletion(signalValue, g_gl.fenceEvent));
		WaitForSingleObject(g_gl.fenceEvent, INFINITE);
	}

	QD3D12_CHECK(w.frames[w.frameIndex].cmdAlloc->Reset());
	QD3D12_CHECK(g_gl.cmdList->Reset(w.frames[w.frameIndex].cmdAlloc.Get(), nullptr));

	g_gl.frameOpen = true;
	g_gl.frameOwner = &w;
}

static void QD3D12_RunRayAIDenoiseIfEnabled(ID3D12GraphicsCommandList* cl, QD3D12Window& w, ID3D12Resource* lightingResource)
{
	(void)cl;
	(void)w;
	(void)lightingResource;

	if (!g_gl.enableRayAIDenoise)
		return;

	if (QD3D12_WantsDLSSRayReconstruction())
		return;

}

static void QD3D12_RunUpscalerOrBlit(QD3D12Window& w)
{
	ID3D12GraphicsCommandList* cl = g_gl.cmdList.Get();
	if (!cl)
		return;

	const bool haveTemporalCameraInputs = (!w.isPbuffer) && g_gl.cameraState.valid;
	const bool useLightingUpscaleInput = (!w.isPbuffer) && QD3D12_UseLightingTextureAsUpscaleInput(w);

	ID3D12Resource* upscaleInputResource = w.sceneColorBuffers[w.frameIndex].Get();
	D3D12_GPU_DESCRIPTOR_HANDLE upscaleInputSrv = w.sceneColorSrvGpu[w.frameIndex];

	if (useLightingUpscaleInput && g_lightingTexture && g_lightingTexture->texture)
	{
		upscaleInputResource = g_lightingTexture->texture.Get();
		upscaleInputSrv = g_lightingTexture->srvGpu;
	}

	D3D12_VIEWPORT outputViewport{};
	outputViewport.TopLeftX = 0.0f;
	outputViewport.TopLeftY = 0.0f;
	outputViewport.Width = (float)w.width;
	outputViewport.Height = (float)w.height;
	outputViewport.MinDepth = 0.0f;
	outputViewport.MaxDepth = 1.0f;

	D3D12_RECT outputScissor{};
	outputScissor.left = 0;
	outputScissor.top = 0;
	outputScissor.right = (LONG)w.width;
	outputScissor.bottom = (LONG)w.height;

#if defined(QD3D12_ENABLE_STREAMLINE)
	if (haveTemporalCameraInputs && useLightingUpscaleInput && g_gl.upscalerBackend == QD3D12_UPSCALER_DLSS && g_qd3d12Sl.deviceBound)
	{
		sl::FrameToken* frameToken = nullptr;
		uint32_t frameIndex = (uint32_t)g_gl.frameSerial;
		const sl::Result frameResult = slGetNewFrameToken(frameToken, &frameIndex);
		if (frameResult == sl::Result::eOk && frameToken)
		{
			sl::Extent renderExtent{};
			renderExtent.left = 0;
			renderExtent.top = 0;
			renderExtent.width = w.renderWidth;
			renderExtent.height = w.renderHeight;

			sl::Extent outputExtent{};
			outputExtent.left = 0;
			outputExtent.top = 0;
			outputExtent.width = w.width;
			outputExtent.height = w.height;

			sl::Resource colorIn = { sl::ResourceType::eTex2d, upscaleInputResource, nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
			sl::Resource colorOut = { sl::ResourceType::eTex2d, w.backBuffers[w.frameIndex].Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_RENDER_TARGET) };
			sl::Resource depth = { sl::ResourceType::eTex2d, w.depthBuffer.Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
			sl::Resource mvec = { sl::ResourceType::eTex2d, w.velocityBuffers[w.frameIndex].Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
			sl::Resource specularMvec = { sl::ResourceType::eTex2d, w.velocityBuffers[w.frameIndex].Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
			sl::Resource diffuseAlbedo = { sl::ResourceType::eTex2d, w.sceneColorBuffers[w.frameIndex].Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
			sl::Resource specularAlbedo = { sl::ResourceType::eTex2d, w.sceneColorBuffers[w.frameIndex].Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
			sl::Resource normalRoughness = { sl::ResourceType::eTex2d, w.normalBuffers[w.frameIndex].Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };

			sl::ResourceTag tags[] =
			{
				sl::ResourceTag{ &colorIn, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent },
				sl::ResourceTag{ &colorOut, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eOnlyValidNow, &outputExtent },
				sl::ResourceTag{ &depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent },
				sl::ResourceTag{ &mvec, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent },
				sl::ResourceTag{ &specularMvec, sl::kBufferTypeSpecularMotionVectors, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent },
				sl::ResourceTag{ &diffuseAlbedo, sl::kBufferTypeAlbedo, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent },
				sl::ResourceTag{ &specularAlbedo, sl::kBufferTypeSpecularAlbedo, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent },
				sl::ResourceTag{ &normalRoughness, sl::kBufferTypeNormalRoughness, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent }
			};

			slSetTagForFrame(*frameToken, g_qd3d12Sl.viewport, tags, _countof(tags), cl);

			sl::DLSSOptions dlssOptions{};
			dlssOptions.mode = QD3D12_MapDLSSMode(g_gl.upscalerQuality);
			dlssOptions.outputWidth = w.width;
			dlssOptions.outputHeight = w.height;
			dlssOptions.sharpness = g_gl.upscalerSharpness;
			dlssOptions.colorBuffersHDR = sl::Boolean::eTrue;
			dlssOptions.useAutoExposure = sl::Boolean::eFalse;
			dlssOptions.alphaUpscalingEnabled = sl::Boolean::eFalse;

			const sl::Result dlssOptionsResult = slDLSSSetOptions(g_qd3d12Sl.viewport, dlssOptions);
			if (dlssOptionsResult != sl::Result::eOk)
			{
				QD3D12_Log("slDLSSSetOptions for DLSS_RR failed (%d), falling back.", int(dlssOptionsResult));
			}
			else
			{
				sl::DLSSDOptions rrOptions{};
				rrOptions.mode = QD3D12_MapDLSSMode(g_gl.upscalerQuality);
				rrOptions.outputWidth = w.width;
				rrOptions.outputHeight = w.height;
				rrOptions.sharpness = g_gl.upscalerSharpness;
				rrOptions.preExposure = 1.0f;
				rrOptions.exposureScale = 1.0f;
				rrOptions.colorBuffersHDR = sl::Boolean::eTrue;
				rrOptions.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::ePacked;
				rrOptions.alphaUpscalingEnabled = sl::Boolean::eFalse;

				Mat4 identity = Mat4::Identity();
				memcpy(&rrOptions.worldToCameraView, identity.m, sizeof(float) * 16);
				memcpy(&rrOptions.cameraViewToWorld, identity.m, sizeof(float) * 16);

				const sl::Result rrOptionsResult = slDLSSDSetOptions(g_qd3d12Sl.viewport, rrOptions);
				if (rrOptionsResult != sl::Result::eOk)
				{
					QD3D12_Log("slDLSSDSetOptions failed (%d), falling back.", int(rrOptionsResult));
				}
				else
				{
					const sl::Result constResult = QD3D12_SetStreamlineCommonConstants(*frameToken);
					if (constResult != sl::Result::eOk)
					{
						QD3D12_Log("slSetConstants for DLSS_RR failed (%d), falling back.", int(constResult));
					}
					else
					{
						const sl::BaseStructure* inputs[] = { &g_qd3d12Sl.viewport };
						const sl::Result evalResult = slEvaluateFeature(sl::kFeatureDLSS_RR, *frameToken, inputs, _countof(inputs), cl);
						if (evalResult == sl::Result::eOk)
						{
							return;
						}

						QD3D12_Log("slEvaluateFeature(DLSS_RR) failed (%d), falling back.", int(evalResult));
					}
				}
			}
		}
		else if (frameResult != sl::Result::eOk)
		{
			QD3D12_Log("slGetNewFrameToken for DLSS_RR failed (%d), falling back.", int(frameResult));
		}
	}

	if (haveTemporalCameraInputs && g_gl.upscalerBackend == QD3D12_UPSCALER_DLSS && g_qd3d12Sl.deviceBound)
	{
		sl::FrameToken* frameToken = nullptr;
		uint32_t frameIndex = (uint32_t)g_gl.frameSerial;
		const sl::Result frameResult = slGetNewFrameToken(frameToken, &frameIndex);
		if (frameResult == sl::Result::eOk && frameToken)
		{
			sl::Extent renderExtent{};
			renderExtent.left = 0;
			renderExtent.top = 0;
			renderExtent.width = w.renderWidth;
			renderExtent.height = w.renderHeight;

			sl::Extent outputExtent{};
			outputExtent.left = 0;
			outputExtent.top = 0;
			outputExtent.width = w.width;
			outputExtent.height = w.height;

			sl::Resource colorIn = { sl::ResourceType::eTex2d, upscaleInputResource, nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
			sl::Resource colorOut = { sl::ResourceType::eTex2d, w.backBuffers[w.frameIndex].Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_RENDER_TARGET) };
			sl::Resource depth = { sl::ResourceType::eTex2d, w.depthBuffer.Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };
			sl::Resource mvec = { sl::ResourceType::eTex2d, w.velocityBuffers[w.frameIndex].Get(), nullptr, nullptr, uint32_t(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) };

			sl::ResourceTag tags[] =
			{
				sl::ResourceTag{ &colorIn, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent },
				sl::ResourceTag{ &colorOut, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eOnlyValidNow, &outputExtent },
				sl::ResourceTag{ &depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent },
				sl::ResourceTag{ &mvec, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent }
			};

			slSetTagForFrame(*frameToken, g_qd3d12Sl.viewport, tags, _countof(tags), cl);

			sl::DLSSOptions options{};
			options.mode = QD3D12_MapDLSSMode(g_gl.upscalerQuality);
			options.outputWidth = w.width;
			options.outputHeight = w.height;
			options.sharpness = g_gl.upscalerSharpness;
			options.colorBuffersHDR = useLightingUpscaleInput ? sl::Boolean::eTrue : sl::Boolean::eFalse;
			options.useAutoExposure = useLightingUpscaleInput ? sl::Boolean::eFalse : sl::Boolean::eTrue;
			options.alphaUpscalingEnabled = sl::Boolean::eFalse;
			slDLSSSetOptions(g_qd3d12Sl.viewport, options);

			const sl::Result constResult = QD3D12_SetStreamlineCommonConstants(*frameToken);
			if (constResult != sl::Result::eOk)
			{
				QD3D12_Log("slSetConstants for DLSS failed (%d), falling back.", int(constResult));
			}
			else
			{
				const sl::BaseStructure* inputs[] = { &g_qd3d12Sl.viewport };
				const sl::Result evalResult = slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, _countof(inputs), cl);
				if (evalResult == sl::Result::eOk)
				{
					return;
				}

				QD3D12_Log("slEvaluateFeature(DLSS) failed (%d), falling back.", int(evalResult));
			}
		}
	}
#endif

#if defined(QD3D12_ENABLE_FFX)
	if (haveTemporalCameraInputs && g_gl.upscalerBackend == QD3D12_UPSCALER_FSR)
	{
		QD3D12_EnsureFfxUpscaleContext(w);

		if (g_qd3d12Ffx.upscaleContextValid)
		{
			ffx::DispatchDescUpscale dispatch{};
			dispatch.commandList = cl;
			dispatch.color = ffxApiGetResourceDX12(w.sceneColorBuffers[w.frameIndex].Get(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
			dispatch.depth = ffxApiGetResourceDX12(w.depthBuffer.Get(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
			dispatch.motionVectors = ffxApiGetResourceDX12(w.velocityBuffers[w.frameIndex].Get(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
			dispatch.output = ffxApiGetResourceDX12(w.backBuffers[w.frameIndex].Get(), FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
			dispatch.jitterOffset.x = -g_gl.jitterX;
			dispatch.jitterOffset.y = -g_gl.jitterY;
			dispatch.motionVectorScale.x = (float)w.renderWidth;
			dispatch.motionVectorScale.y = (float)w.renderHeight;
			dispatch.renderSize.width = w.renderWidth;
			dispatch.renderSize.height = w.renderHeight;
			dispatch.upscaleSize.width = w.width;
			dispatch.upscaleSize.height = w.height;
			dispatch.enableSharpening = true;
			dispatch.sharpness = g_gl.upscalerSharpness;
			dispatch.frameTimeDelta = 16.6667f;
			dispatch.preExposure = 1.0f;
			dispatch.reset = g_gl.motionHistoryReset;
			dispatch.cameraNear = g_gl.cameraState.nearPlane;
			dispatch.cameraFar = g_gl.cameraState.farPlane;
			dispatch.cameraFovAngleVertical = g_gl.cameraState.verticalFovRadians;
			dispatch.viewSpaceToMetersFactor = 1.0f;
			dispatch.flags = 0;

			if (ffx::Dispatch(g_qd3d12Ffx.upscaleContext, dispatch) == ffx::ReturnCode::Ok)
				return;
		}
	}
#endif

	const float clearColor[4] = { 0, 0, 0, 1 };
	cl->ClearRenderTargetView(CurrentBackBufferRTV(), clearColor, 0, nullptr);
	QD3D12_PostFullscreenPass(cl, g_gl.postCopyPSO.Get(), upscaleInputSrv, CurrentBackBufferRTV(), outputViewport, outputScissor);
}

static void QD3D12_CreateDevice()
{
	QD3D12_InitStreamlineEarly();
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
			debug->EnableDebugLayer();
	}
#endif

	QD3D12_CHECK(CreateDXGIFactory1(IID_PPV_ARGS(&g_gl.factory)));
	QD3D12_CHECK(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_gl.device)));

	D3D12_COMMAND_QUEUE_DESC qd{};
	qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	QD3D12_CHECK(g_gl.device->CreateCommandQueue(&qd, IID_PPV_ARGS(&g_gl.queue)));

	QD3D12_StreamlineOnDeviceCreated();
}

static void QD3D12_CreateSwapChainForWindow(QD3D12Window& w)
{
	DXGI_SWAP_CHAIN_DESC1 sd{};
	sd.BufferCount = QD3D12_FrameCount;
	sd.Width = w.width;
	sd.Height = w.height;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.SampleDesc.Count = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain1> sc1;
	QD3D12_CHECK(g_gl.factory->CreateSwapChainForHwnd(
		g_gl.queue.Get(),
		w.hwnd,
		&sd,
		nullptr,
		nullptr,
		&sc1));

	QD3D12_CHECK(sc1.As(&w.swapChain));
	w.frameIndex = w.swapChain->GetCurrentBackBufferIndex();
}

static void QD3D12_CreateRTVsForWindow(QD3D12Window& w)
{
	D3D12_DESCRIPTOR_HEAP_DESC hd{};
	hd.NumDescriptors = QD3D12_FrameCount * 5; // scene color + normal + position + velocity + swap-chain backbuffer
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	QD3D12_CHECK(g_gl.device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&w.rtvHeap)));

	w.rtvStride = g_gl.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE h = w.rtvHeap->GetCPUDescriptorHandleForHeapStart();

	auto CreateRenderTexture = [&](ComPtr<ID3D12Resource>& outRes,
		D3D12_RESOURCE_STATES& outState,
		DXGI_FORMAT format,
		const float clearColor[4],
		bool useOptimizedClear,
		UINT texWidth,
		UINT texHeight)
		{
			D3D12_RESOURCE_DESC rd{};
			rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			rd.Width = texWidth;
			rd.Height = texHeight;
			rd.DepthOrArraySize = 1;
			rd.MipLevels = 1;
			rd.Format = format;
			rd.SampleDesc.Count = 1;
			rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			D3D12_HEAP_PROPERTIES hp{};
			hp.Type = D3D12_HEAP_TYPE_DEFAULT;

			D3D12_CLEAR_VALUE clear{};
			const D3D12_CLEAR_VALUE* clearPtr = nullptr;
			if (useOptimizedClear)
			{
				clear.Format = format;
				clear.Color[0] = clearColor[0];
				clear.Color[1] = clearColor[1];
				clear.Color[2] = clearColor[2];
				clear.Color[3] = clearColor[3];
				clearPtr = &clear;
			}

			QD3D12_CHECK(g_gl.device->CreateCommittedResource(
				&hp,
				D3D12_HEAP_FLAG_NONE,
				&rd,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				clearPtr,
				IID_PPV_ARGS(&outRes)));

			g_gl.device->CreateRenderTargetView(outRes.Get(), nullptr, h);
			outState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			h.ptr += w.rtvStride;
		};

	const float colorClear[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const float normalClear[4] = { 0.0f, 0.0f, 1.0f, 0.5f };
	const float positionClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float velocityClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		CreateRenderTexture(w.sceneColorBuffers[i], w.sceneColorState[i], QD3D12_SceneColorFormat, colorClear, false, w.renderWidth, w.renderHeight);

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		CreateRenderTexture(w.normalBuffers[i], w.normalBufferState[i], DXGI_FORMAT_R16G16B16A16_FLOAT, normalClear, true, w.renderWidth, w.renderHeight);

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		CreateRenderTexture(w.positionBuffers[i], w.positionBufferState[i], DXGI_FORMAT_R16G16B16A16_FLOAT, positionClear, true, w.renderWidth, w.renderHeight);

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		CreateRenderTexture(w.velocityBuffers[i], w.velocityBufferState[i], QD3D12_VelocityFormat, velocityClear, true, w.renderWidth, w.renderHeight);

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
	{
		if (w.swapChain)
		{
			QD3D12_CHECK(w.swapChain->GetBuffer(i, IID_PPV_ARGS(&w.backBuffers[i])));
			g_gl.device->CreateRenderTargetView(w.backBuffers[i].Get(), nullptr, h);
			w.backBufferState[i] = D3D12_RESOURCE_STATE_PRESENT;
			h.ptr += w.rtvStride;
		}
		else
		{
			// Offscreen pbuffer backbuffers are ordinary render-target textures.
			// CreateRenderTexture leaves the tracked state as RENDER_TARGET; EndFrame
			// will transition them to PRESENT/COMMON after the resolve/blit.
			CreateRenderTexture(w.backBuffers[i], w.backBufferState[i], QD3D12_SceneColorFormat, colorClear, false, w.width, w.height);
		}
	}

	auto CreateTextureSrv = [&](ID3D12Resource* res, DXGI_FORMAT format, UINT& srvIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE& cpu, D3D12_GPU_DESCRIPTOR_HANDLE& gpu)
		{
			if (srvIndex == UINT_MAX)
			{
				srvIndex = g_gl.nextSrvIndex++;
				cpu = QD3D12_SrvCpu(srvIndex);
				gpu = QD3D12_SrvGpu(srvIndex);
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
			sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			sd.Format = format;
			sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			sd.Texture2D.MipLevels = 1;
			g_gl.device->CreateShaderResourceView(res, &sd, cpu);
		};

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		CreateTextureSrv(w.sceneColorBuffers[i].Get(), QD3D12_SceneColorFormat, w.sceneColorSrvIndex[i], w.sceneColorSrvCpu[i], w.sceneColorSrvGpu[i]);

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		CreateTextureSrv(w.normalBuffers[i].Get(), DXGI_FORMAT_R16G16B16A16_FLOAT, w.normalSrvIndex[i], w.normalSrvCpu[i], w.normalSrvGpu[i]);

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		CreateTextureSrv(w.positionBuffers[i].Get(), DXGI_FORMAT_R16G16B16A16_FLOAT, w.positionSrvIndex[i], w.positionSrvCpu[i], w.positionSrvGpu[i]);

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		CreateTextureSrv(w.velocityBuffers[i].Get(), QD3D12_VelocityFormat, w.velocitySrvIndex[i], w.velocitySrvCpu[i], w.velocitySrvGpu[i]);
}

void QD3D12_CreateDSVForWindow(QD3D12Window& w)
{
	D3D12_DESCRIPTOR_HEAP_DESC hd{};
	hd.NumDescriptors = 2;
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	QD3D12_CHECK(g_gl.device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&w.dsvHeap)));

	const UINT dsvStride = g_gl.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	D3D12_CPU_DESCRIPTOR_HANDLE lowResDsv = w.dsvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = lowResDsv;
	nativeDsv.ptr += SIZE_T(dsvStride);

	auto CreateDepthTexture = [&](UINT texWidth, UINT texHeight, ComPtr<ID3D12Resource>& outRes)
		{
			D3D12_RESOURCE_DESC rd{};
			rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			rd.Width = texWidth;
			rd.Height = texHeight;
			rd.DepthOrArraySize = 1;
			rd.MipLevels = 1;
			rd.Format = QD3D12_DepthResourceFormat;
			rd.SampleDesc.Count = 1;
			rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_CLEAR_VALUE clear{};
			clear.Format = QD3D12_DepthDsvFormat;
			clear.DepthStencil.Depth = 1.0f;
			clear.DepthStencil.Stencil = 0;

			D3D12_HEAP_PROPERTIES hp{};
			hp.Type = D3D12_HEAP_TYPE_DEFAULT;

			QD3D12_CHECK(g_gl.device->CreateCommittedResource(
				&hp,
				D3D12_HEAP_FLAG_NONE,
				&rd,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clear,
				IID_PPV_ARGS(&outRes)));
		};

	CreateDepthTexture(w.renderWidth, w.renderHeight, w.depthBuffer);
	CreateDepthTexture(w.width, w.height, w.nativeDepthBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = QD3D12_DepthDsvFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	g_gl.device->CreateDepthStencilView(w.depthBuffer.Get(), &dsvDesc, lowResDsv);
	g_gl.device->CreateDepthStencilView(w.nativeDepthBuffer.Get(), &dsvDesc, nativeDsv);
	w.depthState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	w.nativeDepthState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	if (w.depthSrvIndex == UINT_MAX)
	{
		w.depthSrvIndex = g_gl.nextSrvIndex++;
		w.depthSrvCpu = QD3D12_SrvCpu(w.depthSrvIndex);
		w.depthSrvGpu = QD3D12_SrvGpu(w.depthSrvIndex);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = QD3D12_DepthSrvFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	g_gl.device->CreateShaderResourceView(w.depthBuffer.Get(), &srvDesc, w.depthSrvCpu);
}

static void QD3D12_CreateSrvHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC hd{};
	hd.NumDescriptors = QD3D12_MaxTextures;
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	QD3D12_CHECK(g_gl.device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&g_gl.srvHeap)));
	g_gl.srvStride = g_gl.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

static D3D12_CPU_DESCRIPTOR_HANDLE QD3D12_SrvCpu(UINT index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE h = g_gl.srvHeap->GetCPUDescriptorHandleForHeapStart();
	h.ptr += SIZE_T(index) * SIZE_T(g_gl.srvStride);
	return h;
}

static D3D12_GPU_DESCRIPTOR_HANDLE QD3D12_SrvGpu(UINT index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE h = g_gl.srvHeap->GetGPUDescriptorHandleForHeapStart();
	h.ptr += UINT64(index) * UINT64(g_gl.srvStride);
	return h;
}

static void QD3D12_CreateCommandObjects()
{
	QD3D12_CHECK(g_gl.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_currentWindow->frames[0].cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&g_gl.cmdList)));
	QD3D12_CHECK(g_gl.cmdList->Close());

	QD3D12_CHECK(g_gl.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_gl.fence)));
	g_gl.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

static void QD3D12_CreateUploadRingForWindow(QD3D12Window& w)
{
	D3D12_HEAP_PROPERTIES hp{};
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC rd{};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = QD3D12_UploadBufferSize;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	w.upload.size = QD3D12_UploadBufferSize;
	w.upload.offset = 0;

	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
	{
		if (w.upload.resource[i])
			continue;

		QD3D12_CHECK(g_gl.device->CreateCommittedResource(
			&hp,
			D3D12_HEAP_FLAG_NONE,
			&rd,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&w.upload.resource[i])));

		w.upload.gpuBase[i] = w.upload.resource[i]->GetGPUVirtualAddress();

		QD3D12_CHECK(w.upload.resource[i]->Map(
			0,
			nullptr,
			reinterpret_cast<void**>(&w.upload.cpuBase[i])));
	}
}

static void QD3D12_DestroyUploadRingForWindow(QD3D12Window& w)
{
	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
	{
		if (w.upload.resource[i] && w.upload.cpuBase[i])
			w.upload.resource[i]->Unmap(0, nullptr);

		w.upload.cpuBase[i] = nullptr;
		w.upload.gpuBase[i] = 0;
		w.upload.resource[i].Reset();
	}

	w.upload.size = 0;
	w.upload.offset = 0;
}

static ComPtr<ID3DBlob> CompileShaderSourceVariant(const char* source, const char* entry, const char* target)
{
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> err;
	HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr,
		entry, target, flags, 0, &blob, &err);
	if (FAILED(hr))
		QD3D12_Fatal("Shader compile failed for %s: %s", entry, err ? (const char*)err->GetBufferPointer() : "unknown");
	return blob;
}

static ComPtr<ID3DBlob> CompileShaderVariant(const char* entry, const char* target)
{
	return CompileShaderSourceVariant(kQuakeWrapperHLSL, entry, target);
}

static void QD3D12_CreatePostRootSignature()
{
	D3D12_DESCRIPTOR_RANGE range{};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0;
	range.RegisterSpace = 0;
	range.OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER param{};
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param.DescriptorTable.NumDescriptorRanges = 1;
	param.DescriptorTable.pDescriptorRanges = &range;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samp{};
	samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samp.ShaderRegister = 0;
	samp.RegisterSpace = 0;
	samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samp.MaxLOD = D3D12_FLOAT32_MAX;

	D3D12_ROOT_SIGNATURE_DESC rsd{};
	rsd.NumParameters = 1;
	rsd.pParameters = &param;
	rsd.NumStaticSamplers = 1;
	rsd.pStaticSamplers = &samp;
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> sig;
	ComPtr<ID3DBlob> err;
	QD3D12_CHECK(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err));
	QD3D12_CHECK(g_gl.device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&g_gl.postRootSig)));
}

static void QD3D12_CreateRootSignature()
{
	D3D12_DESCRIPTOR_RANGE ranges[QD3D12_MaxTextureUnits] = {};
	D3D12_ROOT_PARAMETER params[1 + QD3D12_MaxTextureUnits] = {};

	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor.ShaderRegister = 0;
	params[0].Descriptor.RegisterSpace = 0;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	for (UINT i = 0; i < QD3D12_MaxTextureUnits; ++i)
	{
		ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[i].NumDescriptors = 1;
		ranges[i].BaseShaderRegister = i;
		ranges[i].RegisterSpace = 0;
		ranges[i].OffsetInDescriptorsFromTableStart = 0;

		params[1 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[1 + i].DescriptorTable.NumDescriptorRanges = 1;
		params[1 + i].DescriptorTable.pDescriptorRanges = &ranges[i];
		params[1 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	D3D12_STATIC_SAMPLER_DESC samps[QD3D12_MaxTextureUnits] = {};
	for (UINT i = 0; i < QD3D12_MaxTextureUnits; ++i)
	{
		samps[i].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samps[i].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samps[i].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samps[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samps[i].ShaderRegister = i;
		samps[i].RegisterSpace = 0;
		samps[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		samps[i].MaxLOD = D3D12_FLOAT32_MAX;
	}

	D3D12_ROOT_SIGNATURE_DESC rsd{};
	rsd.NumParameters = _countof(params);
	rsd.pParameters = params;
	rsd.NumStaticSamplers = _countof(samps);
	rsd.pStaticSamplers = samps;
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> sig;
	ComPtr<ID3DBlob> err;
	QD3D12_CHECK(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err));
	QD3D12_CHECK(g_gl.device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&g_gl.rootSig)));
	QD3D12_CreatePostRootSignature();
}


static void QD3D12_CompileShaders()
{
	g_gl.vsMainBlob = CompileShaderVariant("VSMain", "vs_5_0");
	g_gl.psMainBlob = CompileShaderVariant("PSMain", "ps_5_0");
	g_gl.psAlphaBlob = CompileShaderVariant("PSMainAlphaTest", "ps_5_0");
	g_gl.psUntexturedBlob = CompileShaderVariant("PSMainUntextured", "ps_5_0");
	g_gl.psMainColorOnlyBlob = CompileShaderVariant("PSMainColorOnly", "ps_5_0");
	g_gl.psAlphaColorOnlyBlob = CompileShaderVariant("PSMainAlphaTestColorOnly", "ps_5_0");
	g_gl.psUntexturedColorOnlyBlob = CompileShaderVariant("PSMainUntexturedColorOnly", "ps_5_0");
	g_gl.postVsBlob = CompileShaderSourceVariant(kQD3D12PostHLSL, "VSMain", "vs_5_0");
	g_gl.postPsCopyBlob = CompileShaderSourceVariant(kQD3D12PostHLSL, "PSCopy", "ps_5_0");
	g_gl.postPsAddBlob = CompileShaderSourceVariant(kQD3D12PostHLSL, "PSAdd", "ps_5_0");
	g_gl.postPsDepthCopyBlob = CompileShaderSourceVariant(kQD3D12PostHLSL, "PSDepthCopy", "ps_5_0");
}

static const D3D12_INPUT_ELEMENT_DESC kGLVertexInputLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, (UINT)offsetof(GLVertex, px), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, (UINT)offsetof(GLVertex, nx), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, (UINT)offsetof(GLVertex, u0), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       0, (UINT)offsetof(GLVertex, u1), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)offsetof(GLVertex, r),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static const D3D12_INPUT_ELEMENT_DESC kGLVertexInputLayoutARB[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, (UINT)offsetof(GLVertex, px), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, (UINT)offsetof(GLVertex, nx), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, (UINT)offsetof(GLVertex, u0), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       0, (UINT)offsetof(GLVertex, u1), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)offsetof(GLVertex, r),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, (UINT)offsetof(GLVertex, tx), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, (UINT)offsetof(GLVertex, bx), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static UINT8 BuildColorWriteMask()
{
	UINT8 mask = 0;
	if (g_gl.colorMaskR) mask |= D3D12_COLOR_WRITE_ENABLE_RED;
	if (g_gl.colorMaskG) mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
	if (g_gl.colorMaskB) mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
	if (g_gl.colorMaskA) mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
	return mask;
}

static D3D12_CPU_DESCRIPTOR_HANDLE CurrentNormalRTV()
{
	QD3D12Window& w = *g_currentWindow;
	D3D12_CPU_DESCRIPTOR_HANDLE h = w.rtvHeap->GetCPUDescriptorHandleForHeapStart();
	h.ptr += SIZE_T(QD3D12_FrameCount + w.frameIndex) * SIZE_T(w.rtvStride);
	return h;
}

static D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildPSODesc(
	PipelineMode mode,
	ID3DBlob* vs,
	ID3DBlob* ps,
	const BatchKey& key,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
	bool nativeColorOnly)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
	d.pRootSignature = g_gl.rootSig.Get();
	d.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
	d.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

	d.InputLayout.pInputElementDescs = kGLVertexInputLayout;
	d.InputLayout.NumElements = _countof(kGLVertexInputLayout);

	d.PrimitiveTopologyType = topoType;

	d.NumRenderTargets = nativeColorOnly ? 1u : 4u;
	d.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	if (!nativeColorOnly)
	{
		d.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d.RTVFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d.RTVFormats[3] = QD3D12_VelocityFormat;
	}

	d.DSVFormat = QD3D12_DepthDsvFormat;
	d.SampleDesc.Count = 1;
	d.SampleMask = UINT_MAX;

	d.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	d.RasterizerState.DepthClipEnable = TRUE;

	d.BlendState.RenderTarget[0].RenderTargetWriteMask = key.colorWriteMask;
	if (!nativeColorOnly)
	{
		d.BlendState.RenderTarget[1].RenderTargetWriteMask = key.colorWriteMask ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;
		d.BlendState.RenderTarget[2].RenderTargetWriteMask = key.colorWriteMask ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;
		d.BlendState.RenderTarget[3].RenderTargetWriteMask = key.colorWriteMask ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;
	}

	d.BlendState.AlphaToCoverageEnable = FALSE;
	d.BlendState.IndependentBlendEnable = nativeColorOnly ? FALSE : TRUE;

	for (int i = 0; i < 4; ++i)
	{
		auto& rt = d.BlendState.RenderTarget[i];
		rt.BlendEnable = FALSE;
		rt.LogicOpEnable = FALSE;
		rt.SrcBlend = D3D12_BLEND_ONE;
		rt.DestBlend = D3D12_BLEND_ZERO;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rt.LogicOp = D3D12_LOGIC_OP_NOOP;
	}

	if (mode == PIPE_BLEND_TEX || mode == PIPE_BLEND_UNTEX)
	{
		auto& rt = d.BlendState.RenderTarget[0];
		rt.BlendEnable = TRUE;
		rt.SrcBlend = MapBlend(key.blendSrc);
		rt.DestBlend = MapBlend(key.blendDst);
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = MapBlendAlpha(key.blendSrc);
		rt.DestBlendAlpha = MapBlendAlpha(key.blendDst);
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		if (!nativeColorOnly)
		{
			// Transparent passes happen after glLightScene in the RTCW path, so they
			// must not scribble over the opaque G-buffer or motion vectors that the
			// raytracing and upscaling passes depend on.
			d.BlendState.RenderTarget[1].RenderTargetWriteMask = 0;
			d.BlendState.RenderTarget[2].RenderTargetWriteMask = 0;
			d.BlendState.RenderTarget[3].RenderTargetWriteMask = 0;
		}
	}

	ApplyRasterDepthStencilState(d, key);

	return d;
}

static void QD3D12_CreatePSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
	d.pRootSignature = g_gl.postRootSig.Get();
	d.VS = { g_gl.postVsBlob->GetBufferPointer(), g_gl.postVsBlob->GetBufferSize() };
	d.PS = { g_gl.postPsCopyBlob->GetBufferPointer(), g_gl.postPsCopyBlob->GetBufferSize() };
	d.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	d.SampleDesc.Count = 1;
	d.SampleMask = UINT_MAX;
	d.NumRenderTargets = 1;
	d.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	d.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	d.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	d.RasterizerState.DepthClipEnable = TRUE;
	d.BlendState.AlphaToCoverageEnable = FALSE;
	d.BlendState.IndependentBlendEnable = FALSE;
	d.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	d.BlendState.RenderTarget[0].BlendEnable = FALSE;
	d.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	d.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	d.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d.DepthStencilState.DepthEnable = FALSE;
	d.DepthStencilState.StencilEnable = FALSE;

	QD3D12_CHECK(g_gl.device->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&g_gl.postCopyPSO)));

	d.PS = { g_gl.postPsAddBlob->GetBufferPointer(), g_gl.postPsAddBlob->GetBufferSize() };
	d.BlendState.RenderTarget[0].BlendEnable = TRUE;
	d.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	d.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	d.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	QD3D12_CHECK(g_gl.device->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&g_gl.postAdditivePSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC depthDesc{};
	depthDesc.pRootSignature = g_gl.postRootSig.Get();
	depthDesc.VS = { g_gl.postVsBlob->GetBufferPointer(), g_gl.postVsBlob->GetBufferSize() };
	depthDesc.PS = { g_gl.postPsDepthCopyBlob->GetBufferPointer(), g_gl.postPsDepthCopyBlob->GetBufferSize() };
	depthDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleMask = UINT_MAX;
	depthDesc.NumRenderTargets = 0;
	depthDesc.DSVFormat = QD3D12_DepthDsvFormat;
	depthDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	depthDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	depthDesc.RasterizerState.DepthClipEnable = TRUE;
	depthDesc.BlendState.AlphaToCoverageEnable = FALSE;
	depthDesc.BlendState.IndependentBlendEnable = FALSE;
	depthDesc.DepthStencilState.DepthEnable = TRUE;
	depthDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depthDesc.DepthStencilState.StencilEnable = FALSE;
	QD3D12_CHECK(g_gl.device->CreateGraphicsPipelineState(&depthDesc, IID_PPV_ARGS(&g_gl.postDepthCopyPSO)));
}

static uint64_t MakePSOKey(
	const BatchKey& key,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
	bool nativeColorOnly)
{
	uint64_t h = 1469598103934665603ull;
	auto mix = [&](uint64_t v)
		{
			h ^= v;
			h *= 1099511628211ull;
		};

	mix(uint32_t(key.pipeline));
	mix(uint32_t(key.blendSrc));
	mix(uint32_t(key.blendDst));
	mix(key.depthTest ? 1ull : 0ull);
	mix(key.depthWrite ? 1ull : 0ull);
	mix(uint32_t(key.depthFunc));
	mix(uint32_t(topoType));
	mix(nativeColorOnly ? 1ull : 0ull);
	mix(uint32_t(key.colorWriteMask));
	mix(key.cullFaceEnabled ? 1ull : 0ull);
	mix(uint32_t(key.cullMode));
	mix(uint32_t(key.frontFace));
	mix(key.stencilTest ? 1ull : 0ull);
	mix(uint32_t(key.stencilReadMask));
	mix(uint32_t(key.stencilWriteMask));
	mix(uint32_t(key.stencilFrontFunc));
	mix(uint32_t(key.stencilFrontSFail));
	mix(uint32_t(key.stencilFrontDPFail));
	mix(uint32_t(key.stencilFrontDPPass));
	mix(uint32_t(key.stencilBackFunc));
	mix(uint32_t(key.stencilBackSFail));
	mix(uint32_t(key.stencilBackDPFail));
	mix(uint32_t(key.stencilBackDPPass));
	return h;
}

static std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>> g_psoCache;

static ID3D12PipelineState* QD3D12_GetPSO(
	const BatchKey& key,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
	bool nativeColorOnly)
{
	const uint64_t psoKey = MakePSOKey(key, topoType, nativeColorOnly);

	auto it = g_psoCache.find(psoKey);
	if (it != g_psoCache.end())
		return it->second.Get();

	ID3DBlob* psBlob = nullptr;
	switch (key.pipeline)
	{
	case PIPE_ALPHA_TEST_TEX:
		psBlob = nativeColorOnly ? g_gl.psAlphaColorOnlyBlob.Get() : g_gl.psAlphaBlob.Get();
		break;
	case PIPE_OPAQUE_UNTEX:
	case PIPE_BLEND_UNTEX:
		psBlob = nativeColorOnly ? g_gl.psUntexturedColorOnlyBlob.Get() : g_gl.psUntexturedBlob.Get();
		break;
	case PIPE_OPAQUE_TEX:
	case PIPE_BLEND_TEX:
	default:
		psBlob = nativeColorOnly ? g_gl.psMainColorOnlyBlob.Get() : g_gl.psMainBlob.Get();
		break;
	}

	auto desc = BuildPSODesc(
		key.pipeline,
		g_gl.vsMainBlob.Get(),
		psBlob,
		key,
		topoType,
		nativeColorOnly);

	ComPtr<ID3D12PipelineState> newPSO;
	QD3D12_CHECK(g_gl.device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newPSO)));

	ID3D12PipelineState* out = newPSO.Get();
	g_psoCache.emplace(psoKey, std::move(newPSO));
	return out;
}

static D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildARBPSODesc(
	ID3DBlob* vs,
	ID3DBlob* ps,
	const BatchKey& key,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
	bool nativeColorOnly)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
	d.pRootSignature = g_gl.rootSig.Get();
	d.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
	d.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

	d.InputLayout.pInputElementDescs = kGLVertexInputLayoutARB;
	d.InputLayout.NumElements = _countof(kGLVertexInputLayoutARB);
	d.PrimitiveTopologyType = topoType;

	d.NumRenderTargets = nativeColorOnly ? 1u : 4u;
	d.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	if (!nativeColorOnly)
	{
		d.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d.RTVFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d.RTVFormats[3] = QD3D12_VelocityFormat;
	}

	d.DSVFormat = QD3D12_DepthDsvFormat;
	d.SampleDesc.Count = 1;
	d.SampleMask = UINT_MAX;

	d.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	d.RasterizerState.DepthClipEnable = TRUE;

	d.BlendState.AlphaToCoverageEnable = FALSE;
	d.BlendState.IndependentBlendEnable = nativeColorOnly ? FALSE : TRUE;

	for (int i = 0; i < 4; ++i)
	{
		auto& rt = d.BlendState.RenderTarget[i];
		rt.BlendEnable = FALSE;
		rt.LogicOpEnable = FALSE;
		rt.SrcBlend = D3D12_BLEND_ONE;
		rt.DestBlend = D3D12_BLEND_ZERO;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rt.LogicOp = D3D12_LOGIC_OP_NOOP;
		rt.RenderTargetWriteMask = (i == 0) ? key.colorWriteMask : 0;
	}

	if (key.pipeline == PIPE_BLEND_TEX || key.pipeline == PIPE_BLEND_UNTEX)
	{
		auto& rt = d.BlendState.RenderTarget[0];
		rt.BlendEnable = TRUE;
		rt.SrcBlend = MapBlend(key.blendSrc);
		rt.DestBlend = MapBlend(key.blendDst);
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = MapBlendAlpha(key.blendSrc);
		rt.DestBlendAlpha = MapBlendAlpha(key.blendDst);
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	ApplyRasterDepthStencilState(d, key);

	return d;
}

static uint64_t MakeARBPSOKey(
	uint64_t programKey,
	const BatchKey& key,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
	bool nativeColorOnly)
{
	uint64_t h = programKey ? programKey : 1469598103934665603ull;
	auto mix = [&](uint64_t v)
		{
			h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
		};

	mix(uint32_t(key.pipeline));
	mix(uint32_t(key.blendSrc));
	mix(uint32_t(key.blendDst));
	mix(key.depthTest ? 1ull : 0ull);
	mix(key.depthWrite ? 1ull : 0ull);
	mix(uint32_t(key.depthFunc));
	mix(uint32_t(topoType));
	mix(nativeColorOnly ? 1ull : 0ull);
	mix(uint32_t(key.colorWriteMask));
	mix(key.cullFaceEnabled ? 1ull : 0ull);
	mix(uint32_t(key.cullMode));
	mix(uint32_t(key.frontFace));
	mix(key.stencilTest ? 1ull : 0ull);
	mix(uint32_t(key.stencilReadMask));
	mix(uint32_t(key.stencilWriteMask));
	mix(uint32_t(key.stencilFrontFunc));
	mix(uint32_t(key.stencilFrontSFail));
	mix(uint32_t(key.stencilFrontDPFail));
	mix(uint32_t(key.stencilFrontDPPass));
	mix(uint32_t(key.stencilBackFunc));
	mix(uint32_t(key.stencilBackSFail));
	mix(uint32_t(key.stencilBackDPFail));
	mix(uint32_t(key.stencilBackDPPass));
	return h;
}

static std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>> g_arbPsoCache;

static ID3D12PipelineState* QD3D12_GetARBPSO(
	const BatchKey& key,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
	bool nativeColorOnly)
{
	if (!key.arbVertexBlob || !key.arbFragmentBlob)
		return nullptr;

	const uint64_t programKey =
		(uint64_t(key.arbVertexProgram) << 48ull) ^
		(uint64_t(key.arbFragmentProgram) << 32ull) ^
		(uint64_t(key.arbVertexRevision) << 16ull) ^
		uint64_t(key.arbFragmentRevision);

	const uint64_t psoKey = MakeARBPSOKey(
		programKey,
		key,
		topoType,
		nativeColorOnly);

	auto it = g_arbPsoCache.find(psoKey);
	if (it != g_arbPsoCache.end())
		return it->second.Get();

	auto desc = BuildARBPSODesc(
		key.arbVertexBlob,
		key.arbFragmentBlob,
		key,
		topoType,
		nativeColorOnly);

	ComPtr<ID3D12PipelineState> newPSO;
	QD3D12_CHECK(g_gl.device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newPSO)));

	ID3D12PipelineState* out = newPSO.Get();
	g_arbPsoCache.emplace(psoKey, std::move(newPSO));
	return out;
}


static void QD3D12_CreateWhiteTexture()
{
	g_gl.whiteTexture.glId = 0;
	g_gl.whiteTexture.width = 1;
	g_gl.whiteTexture.height = 1;
	g_gl.whiteTexture.format = GL_RGBA;
	g_gl.whiteTexture.sysmem = { 255, 255, 255, 255 };
	g_gl.whiteTexture.srvIndex = 0;
	g_gl.whiteTexture.srvCpu = QD3D12_SrvCpu(0);
	g_gl.whiteTexture.srvGpu = QD3D12_SrvGpu(0);

	D3D12_RESOURCE_DESC rd{};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rd.Width = 1;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES hpDef{};
	hpDef.Type = D3D12_HEAP_TYPE_DEFAULT;
	QD3D12_CHECK(g_gl.device->CreateCommittedResource(&hpDef, D3D12_HEAP_FLAG_NONE, &rd,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&g_gl.whiteTexture.texture)));

	g_gl.whiteTexture.state = D3D12_RESOURCE_STATE_COPY_DEST;

	D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
	sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	sd.Texture2D.MipLevels = 1;
	g_gl.device->CreateShaderResourceView(g_gl.whiteTexture.texture.Get(), &sd, g_gl.whiteTexture.srvCpu);

	// Upload 1x1 white using a one-time command list.
	QD3D12_CHECK(g_currentWindow->frames[g_currentWindow->frameIndex].cmdAlloc->Reset());
	QD3D12_CHECK(g_gl.cmdList->Reset(g_currentWindow->frames[g_currentWindow->frameIndex].cmdAlloc.Get(), nullptr));

	const UINT64 uploadPitch = 256;
	const UINT64 uploadSize = uploadPitch;
	UploadAlloc alloc = QD3D12_AllocUpload((UINT)uploadSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
	memset(alloc.cpu, 0, uploadSize);
	((uint8_t*)alloc.cpu)[0] = 255;
	((uint8_t*)alloc.cpu)[1] = 255;
	((uint8_t*)alloc.cpu)[2] = 255;
	((uint8_t*)alloc.cpu)[3] = 255;

	D3D12_TEXTURE_COPY_LOCATION dst{};
	dst.pResource = g_gl.whiteTexture.texture.Get();
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src{};
	src.pResource = g_currentWindow->upload.resource[g_currentWindow->frameIndex].Get();
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint.Offset = alloc.offset;
	src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	src.PlacedFootprint.Footprint.Width = 1;
	src.PlacedFootprint.Footprint.Height = 1;
	src.PlacedFootprint.Footprint.Depth = 1;
	src.PlacedFootprint.Footprint.RowPitch = (UINT)uploadPitch;

	g_gl.cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

	D3D12_RESOURCE_BARRIER b{};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = g_gl.whiteTexture.texture.Get();
	b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	g_gl.cmdList->ResourceBarrier(1, &b);
	g_gl.whiteTexture.state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	QD3D12_CHECK(g_gl.cmdList->Close());
	ID3D12CommandList* lists[] = { g_gl.cmdList.Get() };
	g_gl.queue->ExecuteCommandLists(1, lists);
	QD3D12_WaitForGPU();

	g_gl.whiteTexture.gpuValid = true;
	QD3D12_ResetUploadRing();
}

static void QD3D12_UpdateViewportState()
{
	QD3D12Window& w = *g_currentWindow;
	const UINT activeWidth = QD3D12_ActiveRasterWidth(w);
	const UINT activeHeight = QD3D12_ActiveRasterHeight(w);

	const float scaleX = (w.width > 0) ? ((float)activeWidth / (float)w.width) : 1.0f;
	const float scaleY = (w.height > 0) ? ((float)activeHeight / (float)w.height) : 1.0f;

	const GLint rx = (GLint)std::lround((double)g_gl.viewportX * scaleX);
	const GLint ry = (GLint)std::lround((double)g_gl.viewportY * scaleY);
	const GLint rw = (GLint)std::lround((double)g_gl.viewportW * scaleX);
	const GLint rh = (GLint)std::lround((double)g_gl.viewportH * scaleY);

	const GLint rasterHeight = (GLint)activeHeight;
	w.viewport.TopLeftX = (float)rx;
	w.viewport.TopLeftY = (float)(rasterHeight - (ry + rh));
	w.viewport.Width = (float)rw;
	w.viewport.Height = (float)rh;
	w.viewport.MinDepth = (float)ClampValue<GLclampd>(g_gl.depthRangeNear, 0.0, 1.0);
	w.viewport.MaxDepth = (float)ClampValue<GLclampd>(g_gl.depthRangeFar, 0.0, 1.0);

	w.scissor.left = rx;
	w.scissor.top = rasterHeight - (ry + rh);
	w.scissor.right = rx + rw;
	w.scissor.bottom = rasterHeight - ry;
}

static void QD3D12_CreateSurfaceFrameResources(QD3D12Window& surf)
{
	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
	{
		surf.frames[i].fenceValue = 0;

		QD3D12_CHECK(
			g_gl.device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&surf.frames[i].cmdAlloc)));
	}
}

bool QD3D12_InitForQuakeWindow(struct QD3D12Window* window, HWND hwnd, int width, int height, bool fastPath)
{
	window->hwnd = hwnd;
	window->isPbuffer = false;
	if (!window->hdc && hwnd)
	{
		window->hdc = GetDC(hwnd);
		window->ownsHdc = (window->hdc != nullptr);
	}
	QD3D12_RegisterWindowDC(window);
	if (!g_qd3d12CurrentRC && window->hdc)
		g_qd3d12CurrentRC = QD3D12_CreateGLContextHandle(window->hdc, window);
	g_qd3d12CurrentDC = window->hdc;
	window->width = (UINT)width;
	window->height = (UINT)height;
	QD3D12_SelectRenderResolution(*window, window->width, window->height);

	g_gl.viewportW = width;
	g_gl.viewportH = height;

	g_currentWindow = window;

	if (!fastPath)
	{
		QD3D12_InitFrameVertexArena();
		QD3D12_CreateDevice();
		QD3D12_SelectRenderResolution(*window, window->width, window->height);
	}

	QD3D12_CreateSurfaceFrameResources(*window);
	QD3D12_CreateSwapChainForWindow(*window);

	// SRV heap must exist before any code that allocates SRV indices/handles.
	if (!fastPath)
	{
		QD3D12_CreateSrvHeap();
	}

	QD3D12_CreateRTVsForWindow(*window);
	QD3D12_CreateDSVForWindow(*window);

	if (!fastPath)
	{
		QD3D12_CreateCommandObjects();
	}

	QD3D12_CreateUploadRingForWindow(*window);

	if (!fastPath)
	{
		QD3D12_CompileShaders();
		QD3D12_CreateRootSignature();
		QD3D12_CreatePSOs();
		QD3D12_UpdateViewportState();
		QD3D12_CreateWhiteTexture();
		QD3D12_CreateOcclusionQueryObjects();
		QD3D12ARB_Init();
	}

	g_gl.motionHistoryReset = true;
	QD3D12_Log("QD3D12 initialized: output=%ux%u render=%ux%u", window->width, window->height, window->renderWidth, window->renderHeight);
	return true;
}

void QD3D12_ShutdownForQuake()
{
	if (g_gl.frameOpen || !g_gl.queuedBatches.empty())
		QD3D12_SubmitOpenFrameNoPresentAndWait();

#if defined(QD3D12_ENABLE_FFX)
	QD3D12_DestroyFfxUpscaleContext();
#endif

	QD3D12_ShutdownFrameVertexArena();
	QD3D12_WaitForGPU();

#if defined(QD3D12_ENABLE_STREAMLINE)
	if (g_qd3d12Sl.initialized)
	{
		slShutdown();
	}
#endif

	if (g_gl.fenceEvent)
		CloseHandle(g_gl.fenceEvent);

	QD3D12ARB_Shutdown();
	g_arbPsoCache.clear();

	g_gl = GLState{};
}

// ============================================================
// SECTION 8: frame control
// ============================================================

static D3D12_CPU_DESCRIPTOR_HANDLE CurrentRTV()
{
	QD3D12Window& w = *g_currentWindow;
	D3D12_CPU_DESCRIPTOR_HANDLE h = w.rtvHeap->GetCPUDescriptorHandleForHeapStart();
	h.ptr += SIZE_T(w.frameIndex) * SIZE_T(w.rtvStride);
	return h;
}

void QD3D12_BeginFrame()
{
	QD3D12Window& w = *g_currentWindow;

	if (g_gl.frameOpen)
	{
		if (g_gl.frameOwner == &w)
			return;

		QD3D12_SubmitOpenFrameNoPresentAndWait();
	}

	w.frameIndex = w.swapChain ? w.swapChain->GetCurrentBackBufferIndex() : 0;
	QD3D12_WaitForFrame(w.frameIndex);
	QD3D12_ResetUploadRing();

	g_gl.framePhase = QD3D12_FRAME_LOW_RES;
	g_gl.sceneResolvedThisFrame = false;
	g_gl.raytracedLightingReadyThisFrame = false;
	QD3D12_UpdateViewportState();

	FrameResources& fr = w.frames[w.frameIndex];
	QD3D12_CHECK(fr.cmdAlloc->Reset());
	QD3D12_CHECK(g_gl.cmdList->Reset(fr.cmdAlloc.Get(), nullptr));

	QD3D12_BindTargetsForCurrentPhase(w);

	const float normalClear[4] = { 0.0f, 0.0f, 1.0f, 0.5f };
	const float positionClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float velocityClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_gl.cmdList->ClearRenderTargetView(CurrentNormalRTV(), normalClear, 0, nullptr);
	g_gl.cmdList->ClearRenderTargetView(CurrentPositionRTV(), positionClear, 0, nullptr);
	g_gl.cmdList->ClearRenderTargetView(CurrentVelocityRTV(), velocityClear, 0, nullptr);

	g_gl.frameOpen = true;
	g_gl.frameOwner = &w;
}

void QD3D12_EndFrame()
{
	QD3D12Window& w = *g_currentWindow;

	if (g_gl.frameOpen && g_gl.frameOwner != &w)
	{
		QD3D12_SubmitOpenFrameNoPresentAndWait();
	}

	if (!g_gl.frameOpen)
	{
		if (g_gl.queuedBatches.empty())
			return;

		QD3D12_BeginFrame();
	}

	QD3D12_FlushQueuedBatches();

	if (!g_gl.sceneResolvedThisFrame)
	{
		QD3D12_TransitionResource(g_gl.cmdList.Get(), w.sceneColorBuffers[w.frameIndex].Get(), w.sceneColorState[w.frameIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		QD3D12_TransitionResource(g_gl.cmdList.Get(), w.normalBuffers[w.frameIndex].Get(), w.normalBufferState[w.frameIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		QD3D12_TransitionResource(g_gl.cmdList.Get(), w.positionBuffers[w.frameIndex].Get(), w.positionBufferState[w.frameIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		QD3D12_TransitionResource(g_gl.cmdList.Get(), w.velocityBuffers[w.frameIndex].Get(), w.velocityBufferState[w.frameIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		QD3D12_TransitionResource(g_gl.cmdList.Get(), w.depthBuffer.Get(), w.depthState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		QD3D12_TransitionResource(g_gl.cmdList.Get(), w.backBuffers[w.frameIndex].Get(), w.backBufferState[w.frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);

		QD3D12_RunUpscalerOrBlit(w);
	}

	QD3D12_TransitionResource(g_gl.cmdList.Get(), w.backBuffers[w.frameIndex].Get(), w.backBufferState[w.frameIndex], D3D12_RESOURCE_STATE_PRESENT);

	QD3D12_CHECK(g_gl.cmdList->Close());
	ID3D12CommandList* lists[] = { g_gl.cmdList.Get() };
	g_gl.queue->ExecuteCommandLists(1, lists);

	g_gl.frameOpen = false;
	g_gl.frameOwner = nullptr;
	if (!w.isPbuffer)
		QD3D12_CommitMotionHistoryForFrame();

	g_gl.framePhase = QD3D12_FRAME_LOW_RES;
	g_gl.sceneResolvedThisFrame = false;
	g_gl.raytracedLightingReadyThisFrame = false;
	QD3D12_UpdateViewportState();
}

void QD3D12_Present()
{
	QD3D12Window& w = *g_currentWindow;

	if (w.swapChain)
		QD3D12_CHECK(w.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));

	FrameResources& fr = w.frames[w.frameIndex];
	const UINT64 signalValue = g_gl.nextFenceValue++;
	QD3D12_CHECK(g_gl.queue->Signal(g_gl.fence.Get(), signalValue));
	fr.fenceValue = signalValue;
}


static void QD3D12_SubmitOpenFrameNoPresentAndWait()
{
	if (!g_gl.device || !g_gl.queue || !g_gl.cmdList)
		return;

	QD3D12Window* owner = g_gl.frameOwner ? g_gl.frameOwner : g_currentWindow;
	if (!owner)
		return;

	QD3D12Window* savedWindow = g_currentWindow;
	g_currentWindow = owner;

	if (g_gl.frameOpen || !g_gl.queuedBatches.empty())
	{
		QD3D12_EndFrame();

		FrameResources& fr = owner->frames[owner->frameIndex];
		const UINT64 signalValue = g_gl.nextFenceValue++;
		QD3D12_CHECK(g_gl.queue->Signal(g_gl.fence.Get(), signalValue));
		fr.fenceValue = signalValue;

		if (g_gl.fence->GetCompletedValue() < signalValue)
		{
			QD3D12_CHECK(g_gl.fence->SetEventOnCompletion(signalValue, g_gl.fenceEvent));
			WaitForSingleObject(g_gl.fenceEvent, INFINITE);
		}

		QD3D12_CollectRetiredResources();
		QD3D12_ResetFrameVertexArena();
	}

	g_currentWindow = savedWindow;
}

void QD3D12_SwapBuffers(HDC hdc) {
	if (g_gl.frameOpen && g_gl.frameOwner && g_gl.frameOwner != g_currentWindow)
		QD3D12_SubmitOpenFrameNoPresentAndWait();

	QD3D12_EndFrame();
	QD3D12_Present();
	QD3D12_CollectRetiredResources();
	QD3D12_ResetFrameVertexArena();
}

// ============================================================
// SECTION 9: texture upload/update
// ============================================================

static void EnsureTextureResource(TextureResource& tex)
{
	const DXGI_FORMAT dxgiFormat = tex.dxgiFormat;

	if (tex.width <= 0 || tex.height <= 0 || dxgiFormat == DXGI_FORMAT_UNKNOWN)
		return;

	auto EnsureSrvDescriptor = [&]()
		{
			if (!tex.texture)
				return;

			if (tex.srvIndex == UINT_MAX)
			{
				tex.srvIndex = g_gl.nextSrvIndex++;
				tex.srvCpu = QD3D12_SrvCpu(tex.srvIndex);
				tex.srvGpu = QD3D12_SrvGpu(tex.srvIndex);
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
			sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			sd.Format = dxgiFormat;
			sd.Shader4ComponentMapping = QD3D12_TextureShaderComponentMapping(tex);
			sd.Texture2D.MipLevels = 1;
			g_gl.device->CreateShaderResourceView(tex.texture.Get(), &sd, tex.srvCpu);
		};

	bool needsRecreate = false;

	if (!tex.texture)
	{
		needsRecreate = true;
	}
	else
	{
		D3D12_RESOURCE_DESC desc = tex.texture->GetDesc();
		if ((int)desc.Width != tex.width ||
			(int)desc.Height != tex.height ||
			desc.Format != dxgiFormat)
		{
			QD3D12_RetireResource(tex.texture);
			tex.gpuValid = false;
			tex.state = D3D12_RESOURCE_STATE_COPY_DEST;
			needsRecreate = true;
		}
	}

	if (!needsRecreate)
	{
		// Re-write the SRV descriptor so same-resource changes like RGB DXT1
		// alpha-forcing are reflected without having to recreate the texture.
		EnsureSrvDescriptor();
		return;
	}

	D3D12_RESOURCE_DESC rd{};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rd.Width = (UINT)tex.width;
	rd.Height = (UINT)tex.height;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.Format = dxgiFormat;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES hp{};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;

	QD3D12_CHECK(g_gl.device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&tex.texture)));

	tex.state = D3D12_RESOURCE_STATE_COPY_DEST;
	EnsureSrvDescriptor();
	tex.gpuValid = false;
}

static TextureResource* QD3D12_EnsureLightingTexture(int width, int height)
{
	const DXGI_FORMAT desiredFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	if (g_lightingTextureId == 0)
	{
		g_lightingTextureId = g_gl.nextTextureId++;

		TextureResource tex{};
		tex.glId = g_lightingTextureId;
		tex.width = (int)width;
		tex.height = (int)height;
		tex.format = GL_RGBA;
		tex.compressed = false;
		tex.compressedInternalFormat = 0;
		tex.compressedBlockBytes = 0;
		tex.compressedImageSize = 0;
		tex.forceOpaqueAlpha = false;
		tex.dxgiFormat = desiredFormat;
		tex.minFilter = GL_LINEAR;
		tex.magFilter = GL_LINEAR;
		tex.wrapS = GL_CLAMP;
		tex.wrapT = GL_CLAMP;

		tex.srvIndex = g_gl.nextSrvIndex++;
		tex.srvCpu = QD3D12_SrvCpu(tex.srvIndex);
		tex.srvGpu = QD3D12_SrvGpu(tex.srvIndex);

		auto it = g_gl.textures.emplace(g_lightingTextureId, std::move(tex)).first;
		g_lightingTexture = &it->second;
	}

	if (!g_lightingTexture)
	{
		auto it = g_gl.textures.find(g_lightingTextureId);
		if (it == g_gl.textures.end())
			return nullptr;
		g_lightingTexture = &it->second;
	}

	g_lightingTexture->dxgiFormat = desiredFormat;

	const bool needsCreate =
		!g_lightingTexture->texture ||
		g_lightingTexture->width != (int)width ||
		g_lightingTexture->height != (int)height ||
		g_lightingTexture->texture->GetDesc().Format != desiredFormat ||
		((g_lightingTexture->texture->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0);

	if (!needsCreate)
		return g_lightingTexture;

	if (g_lightingTexture->texture)
	{
		QD3D12_RetireResource(g_lightingTexture->texture);
	}

	g_lightingTexture->width = (int)width;
	g_lightingTexture->height = (int)height;
	g_lightingTexture->compressed = false;
	g_lightingTexture->compressedInternalFormat = 0;
	g_lightingTexture->compressedBlockBytes = 0;
	g_lightingTexture->compressedImageSize = 0;
	g_lightingTexture->forceOpaqueAlpha = false;
	g_lightingTexture->sysmem.clear();

	D3D12_RESOURCE_DESC rd{};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rd.Alignment = 0;
	rd.Width = width;
	rd.Height = height;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.Format = desiredFormat;
	rd.SampleDesc.Count = 1;
	rd.SampleDesc.Quality = 0;
	rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES hp{};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;

	QD3D12_CHECK(g_gl.device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&g_lightingTexture->texture)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Format = rd.Format;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Texture2D.MipLevels = 1;

	g_gl.device->CreateShaderResourceView(
		g_lightingTexture->texture.Get(),
		&srv,
		g_lightingTexture->srvCpu);

	g_lightingTexture->gpuValid = true;
	g_lightingTexture->state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	g_lightingTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	return g_lightingTexture;
}

static void ConvertToRGBA8(const TextureResource& tex, std::vector<uint8_t>& outRGBA)
{
	const int pixelCount = tex.width * tex.height;
	outRGBA.resize((size_t)pixelCount * 4);

	if (tex.sysmem.empty())
	{
		std::fill(outRGBA.begin(), outRGBA.end(), 255);
		return;
	}

	const uint8_t* src = tex.sysmem.data();
	uint8_t* dst = outRGBA.data();

	if (tex.format == GL_RGBA)
	{
		memcpy(dst, src, outRGBA.size());
		return;
	}

	if (tex.format == GL_RGB)
	{
#if defined(__SSSE3__) || (defined(_M_IX86_FP) || defined(_M_X64))
		// SSSE3 path: process 4 RGB pixels (12 bytes) -> 16 RGBA bytes at a time.
		// Output:
		// [r0 g0 b0 255 r1 g1 b1 255 r2 g2 b2 255 r3 g3 b3 255]
		//
		// We load 16 bytes even though we only logically consume 12. That means
		// the source buffer must have at least 4 readable bytes past the final
		// 12-byte chunk if you try to run this on the very last block. To keep it
		// safe, only use SIMD while at least 16 source bytes remain.
		//
		// So the SIMD loop runs while (i + 4) <= pixelCount AND enough source bytes
		// remain for a safe 16-byte load.
		const __m128i alphaMask = _mm_set1_epi32(0xFF000000);

		int i = 0;
		int srcByteOffset = 0;
		const int srcBytes = pixelCount * 3;

		// Need 16 readable bytes from src + srcByteOffset
		while (i + 4 <= pixelCount && srcByteOffset + 16 <= srcBytes)
		{
			__m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + srcByteOffset));

			// Pull RGB triples into 32-bit lanes:
			// lane0 = [r0 g0 b0 x]
			// lane1 = [r1 g1 b1 x]
			// lane2 = [r2 g2 b2 x]
			// lane3 = [r3 g3 b3 x]
			const __m128i shuffled = _mm_shuffle_epi8(
				in,
				_mm_setr_epi8(
					0, 1, 2, char(0x80),
					3, 4, 5, char(0x80),
					6, 7, 8, char(0x80),
					9, 10, 11, char(0x80)
				)
			);

			const __m128i out = _mm_or_si128(shuffled, alphaMask);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i * 4), out);

			i += 4;
			srcByteOffset += 12;
		}

		for (; i < pixelCount; ++i)
		{
			dst[i * 4 + 0] = src[i * 3 + 0];
			dst[i * 4 + 1] = src[i * 3 + 1];
			dst[i * 4 + 2] = src[i * 3 + 2];
			dst[i * 4 + 3] = 255;
		}
#else
		for (int i = 0; i < pixelCount; ++i)
		{
			dst[i * 4 + 0] = src[i * 3 + 0];
			dst[i * 4 + 1] = src[i * 3 + 1];
			dst[i * 4 + 2] = src[i * 3 + 2];
			dst[i * 4 + 3] = 255;
		}
#endif
		return;
	}

	if (tex.format == GL_ALPHA)
	{
#if defined(__AVX2__) || defined(_M_X64)
		int i = 0;

		// 32 pixels at a time
#if defined(__AVX2__)
		const __m256i white = _mm256_set1_epi32(0x00FFFFFF);

		for (; i + 32 <= pixelCount; i += 32)
		{
			__m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));

			__m256i lo16 = _mm256_unpacklo_epi8(_mm256_setzero_si256(), a);
			__m256i hi16 = _mm256_unpackhi_epi8(_mm256_setzero_si256(), a);

			__m256i p0 = _mm256_unpacklo_epi16(white, lo16);
			__m256i p1 = _mm256_unpackhi_epi16(white, lo16);
			__m256i p2 = _mm256_unpacklo_epi16(white, hi16);
			__m256i p3 = _mm256_unpackhi_epi16(white, hi16);

			_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (i + 0) * 4), p0);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (i + 8) * 4), p1);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (i + 16) * 4), p2);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (i + 24) * 4), p3);
		}
#endif

		for (; i < pixelCount; ++i)
		{
			dst[i * 4 + 0] = 255;
			dst[i * 4 + 1] = 255;
			dst[i * 4 + 2] = 255;
			dst[i * 4 + 3] = src[i];
		}
#else
		for (int i = 0; i < pixelCount; ++i)
		{
			dst[i * 4 + 0] = 255;
			dst[i * 4 + 1] = 255;
			dst[i * 4 + 2] = 255;
			dst[i * 4 + 3] = src[i];
		}
#endif
		return;
	}

	if (tex.format == GL_LUMINANCE || tex.format == GL_INTENSITY)
	{
		const bool intensity = (tex.format == GL_INTENSITY);

#if defined(__AVX2__) || defined(_M_X64)
		int i = 0;

#if defined(__AVX2__)
		for (; i + 32 <= pixelCount; i += 32)
		{
			__m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));

			__m256i lo16 = _mm256_unpacklo_epi8(v, v);
			__m256i hi16 = _mm256_unpackhi_epi8(v, v);

			__m256i p0 = _mm256_unpacklo_epi16(lo16, lo16);
			__m256i p1 = _mm256_unpackhi_epi16(lo16, lo16);
			__m256i p2 = _mm256_unpacklo_epi16(hi16, hi16);
			__m256i p3 = _mm256_unpackhi_epi16(hi16, hi16);

			if (!intensity)
			{
				// Force alpha to 255 for luminance
				const __m256i alphaMask = _mm256_set1_epi32(0xFF000000);
				const __m256i rgbMask = _mm256_set1_epi32(0x00FFFFFF);

				p0 = _mm256_or_si256(_mm256_and_si256(p0, rgbMask), alphaMask);
				p1 = _mm256_or_si256(_mm256_and_si256(p1, rgbMask), alphaMask);
				p2 = _mm256_or_si256(_mm256_and_si256(p2, rgbMask), alphaMask);
				p3 = _mm256_or_si256(_mm256_and_si256(p3, rgbMask), alphaMask);
			}

			_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (i + 0) * 4), p0);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (i + 8) * 4), p1);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (i + 16) * 4), p2);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (i + 24) * 4), p3);
		}
#endif

		for (; i < pixelCount; ++i)
		{
			const uint8_t v = src[i];
			dst[i * 4 + 0] = v;
			dst[i * 4 + 1] = v;
			dst[i * 4 + 2] = v;
			dst[i * 4 + 3] = intensity ? v : 255;
		}
#else
		for (int i = 0; i < pixelCount; ++i)
		{
			const uint8_t v = src[i];
			dst[i * 4 + 0] = v;
			dst[i * 4 + 1] = v;
			dst[i * 4 + 2] = v;
			dst[i * 4 + 3] = intensity ? v : 255;
		}
#endif
		return;
	}

	std::fill(outRGBA.begin(), outRGBA.end(), 255);
}
static void UploadTexture(TextureResource& tex)
{
	if (!tex.texture)
		return;

	if (tex.compressed)
	{
		const UINT blockBytes = tex.compressedBlockBytes ? tex.compressedBlockBytes : QD3D12_CompressedTextureBlockBytes(tex.compressedInternalFormat);
		if (blockBytes == 0 || tex.dxgiFormat == DXGI_FORMAT_UNKNOWN || tex.width <= 0 || tex.height <= 0)
		{
			g_gl.lastError = GL_INVALID_ENUM;
			return;
		}

		const UINT blocksWide = QD3D12_CompressedTextureBlocksWide(tex.width);
		const UINT blocksHigh = QD3D12_CompressedTextureBlocksHigh(tex.height);
		const UINT srcRowBytes = blocksWide * blockBytes;
		const UINT rowPitch = (srcRowBytes + 255u) & ~255u;
		const UINT uploadSize = rowPitch * blocksHigh;

		UploadAlloc alloc = QD3D12_AllocUpload(uploadSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
		uint8_t* dstBase = (uint8_t*)alloc.cpu;
		memset(dstBase, 0, uploadSize);

		if (!tex.sysmem.empty() && blockBytes != 0)
		{
			const uint8_t* srcBase = tex.sysmem.data();
			const size_t available = tex.sysmem.size();
			for (UINT row = 0; row < blocksHigh; ++row)
			{
				const size_t srcOff = (size_t)row * (size_t)srcRowBytes;
				if (srcOff >= available)
					break;

				const size_t copyBytes = std::min<size_t>((size_t)srcRowBytes, available - srcOff);
				memcpy(dstBase + (size_t)row * (size_t)rowPitch, srcBase + srcOff, copyBytes);
			}
		}

		if (tex.state != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			D3D12_RESOURCE_BARRIER toCopy{};
			toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toCopy.Transition.pResource = tex.texture.Get();
			toCopy.Transition.StateBefore = tex.state;
			toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			toCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			g_gl.cmdList->ResourceBarrier(1, &toCopy);
			tex.state = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		D3D12_TEXTURE_COPY_LOCATION srcLoc{};
		srcLoc.pResource = g_currentWindow->upload.resource[g_currentWindow->frameIndex].Get();
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLoc.PlacedFootprint.Offset = alloc.offset;
		srcLoc.PlacedFootprint.Footprint.Format = tex.dxgiFormat;
		srcLoc.PlacedFootprint.Footprint.Width = tex.width;
		srcLoc.PlacedFootprint.Footprint.Height = tex.height;
		srcLoc.PlacedFootprint.Footprint.Depth = 1;
		srcLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;

		D3D12_TEXTURE_COPY_LOCATION dstLoc{};
		dstLoc.pResource = tex.texture.Get();
		dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLoc.SubresourceIndex = 0;

		g_gl.cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

		D3D12_RESOURCE_BARRIER toSrv{};
		toSrv.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		toSrv.Transition.pResource = tex.texture.Get();
		toSrv.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		toSrv.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		toSrv.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		g_gl.cmdList->ResourceBarrier(1, &toSrv);

		tex.state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		tex.gpuValid = true;
		return;
	}

	const UINT srcRowBytes = (UINT)(tex.width * 4);
	const UINT rowPitch = (srcRowBytes + 255u) & ~255u;
	const UINT uploadSize = rowPitch * (UINT)tex.height;
	UploadAlloc alloc = QD3D12_AllocUpload(uploadSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

	uint8_t* dstBase = (uint8_t*)alloc.cpu;

	if (tex.sysmem.empty())
	{
		for (int y = 0; y < tex.height; ++y)
		{
			uint8_t* dst = dstBase + (size_t)y * rowPitch;
			memset(dst, 255, tex.width * 4);
		}
	}
	else if (tex.format == GL_RGBA)
	{
		const uint8_t* srcBase = tex.sysmem.data();
		for (int y = 0; y < tex.height; ++y)
		{
			memcpy(dstBase + (size_t)y * rowPitch, srcBase + (size_t)y * tex.width * 4, tex.width * 4);
		}
	}
	else if (tex.format == GL_RGB)
	{
		const uint8_t* srcBase = tex.sysmem.data();

		for (int y = 0; y < tex.height; ++y)
		{
			const uint8_t* src = srcBase + (size_t)y * tex.width * 3;
			uint8_t* dst = dstBase + (size_t)y * rowPitch;

#if defined(__SSSE3__) || defined(_M_X64)
			const int count = tex.width;
			int x = 0;
			int srcByteOffset = 0;
			const int srcBytes = count * 3;

			const __m128i alphaMask = _mm_set1_epi32(0xFF000000);

			while (x + 4 <= count && srcByteOffset + 16 <= srcBytes)
			{
				__m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + srcByteOffset));

				__m128i shuffled = _mm_shuffle_epi8(
					in,
					_mm_setr_epi8(
						0, 1, 2, char(0x80),
						3, 4, 5, char(0x80),
						6, 7, 8, char(0x80),
						9, 10, 11, char(0x80)
					)
				);

				__m128i out = _mm_or_si128(shuffled, alphaMask);
				_mm_storeu_si128(reinterpret_cast<__m128i*>(dst + x * 4), out);

				x += 4;
				srcByteOffset += 12;
			}

			for (; x < count; ++x)
			{
				dst[x * 4 + 0] = src[x * 3 + 0];
				dst[x * 4 + 1] = src[x * 3 + 1];
				dst[x * 4 + 2] = src[x * 3 + 2];
				dst[x * 4 + 3] = 255;
			}
#else
			for (int x = 0; x < tex.width; ++x)
			{
				dst[x * 4 + 0] = src[x * 3 + 0];
				dst[x * 4 + 1] = src[x * 3 + 1];
				dst[x * 4 + 2] = src[x * 3 + 2];
				dst[x * 4 + 3] = 255;
			}
#endif
		}
	}
	else if (tex.format == GL_ALPHA)
	{
		const uint8_t* srcBase = tex.sysmem.data();

		for (int y = 0; y < tex.height; ++y)
		{
			const uint8_t* src = srcBase + (size_t)y * tex.width;
			uint8_t* dst = dstBase + (size_t)y * rowPitch;

#if defined(__AVX2__)
			int x = 0;
			const __m256i white = _mm256_set1_epi32(0x00FFFFFF);

			for (; x + 32 <= tex.width; x += 32)
			{
				__m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + x));

				__m256i lo16 = _mm256_unpacklo_epi8(_mm256_setzero_si256(), a);
				__m256i hi16 = _mm256_unpackhi_epi8(_mm256_setzero_si256(), a);

				__m256i p0 = _mm256_unpacklo_epi16(white, lo16);
				__m256i p1 = _mm256_unpackhi_epi16(white, lo16);
				__m256i p2 = _mm256_unpacklo_epi16(white, hi16);
				__m256i p3 = _mm256_unpackhi_epi16(white, hi16);

				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (x + 0) * 4), p0);
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (x + 8) * 4), p1);
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (x + 16) * 4), p2);
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (x + 24) * 4), p3);
			}

			for (; x < tex.width; ++x)
			{
				dst[x * 4 + 0] = 255;
				dst[x * 4 + 1] = 255;
				dst[x * 4 + 2] = 255;
				dst[x * 4 + 3] = src[x];
			}
#else
			for (int x = 0; x < tex.width; ++x)
			{
				dst[x * 4 + 0] = 255;
				dst[x * 4 + 1] = 255;
				dst[x * 4 + 2] = 255;
				dst[x * 4 + 3] = src[x];
			}
#endif
		}
	}
	else if (tex.format == GL_LUMINANCE || tex.format == GL_INTENSITY)
	{
		const bool intensity = (tex.format == GL_INTENSITY);
		const uint8_t* srcBase = tex.sysmem.data();

		for (int y = 0; y < tex.height; ++y)
		{
			const uint8_t* src = srcBase + (size_t)y * tex.width;
			uint8_t* dst = dstBase + (size_t)y * rowPitch;

#if defined(__AVX2__)
			int x = 0;

			for (; x + 32 <= tex.width; x += 32)
			{
				__m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + x));

				__m256i lo16 = _mm256_unpacklo_epi8(v, v);
				__m256i hi16 = _mm256_unpackhi_epi8(v, v);

				__m256i p0 = _mm256_unpacklo_epi16(lo16, lo16);
				__m256i p1 = _mm256_unpackhi_epi16(lo16, lo16);
				__m256i p2 = _mm256_unpacklo_epi16(hi16, hi16);
				__m256i p3 = _mm256_unpackhi_epi16(hi16, hi16);

				if (!intensity)
				{
					const __m256i rgbMask = _mm256_set1_epi32(0x00FFFFFF);
					const __m256i alphaMask = _mm256_set1_epi32(0xFF000000);

					p0 = _mm256_or_si256(_mm256_and_si256(p0, rgbMask), alphaMask);
					p1 = _mm256_or_si256(_mm256_and_si256(p1, rgbMask), alphaMask);
					p2 = _mm256_or_si256(_mm256_and_si256(p2, rgbMask), alphaMask);
					p3 = _mm256_or_si256(_mm256_and_si256(p3, rgbMask), alphaMask);
				}

				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (x + 0) * 4), p0);
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (x + 8) * 4), p1);
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (x + 16) * 4), p2);
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + (x + 24) * 4), p3);
			}

			for (; x < tex.width; ++x)
			{
				uint8_t v = src[x];
				dst[x * 4 + 0] = v;
				dst[x * 4 + 1] = v;
				dst[x * 4 + 2] = v;
				dst[x * 4 + 3] = intensity ? v : 255;
			}
#else
			for (int x = 0; x < tex.width; ++x)
			{
				uint8_t v = src[x];
				dst[x * 4 + 0] = v;
				dst[x * 4 + 1] = v;
				dst[x * 4 + 2] = v;
				dst[x * 4 + 3] = intensity ? v : 255;
			}
#endif
		}
	}
	else
	{
		for (int y = 0; y < tex.height; ++y)
		{
			uint8_t* dst = dstBase + (size_t)y * rowPitch;
			memset(dst, 255, tex.width * 4);
		}
	}

	if (tex.state != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		D3D12_RESOURCE_BARRIER toCopy{};
		toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		toCopy.Transition.pResource = tex.texture.Get();
		toCopy.Transition.StateBefore = tex.state;
		toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		toCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		g_gl.cmdList->ResourceBarrier(1, &toCopy);

		tex.state = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	D3D12_TEXTURE_COPY_LOCATION srcLoc{};
	srcLoc.pResource = g_currentWindow->upload.resource[g_currentWindow->frameIndex].Get();
	srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLoc.PlacedFootprint.Offset = alloc.offset;
	srcLoc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srcLoc.PlacedFootprint.Footprint.Width = tex.width;
	srcLoc.PlacedFootprint.Footprint.Height = tex.height;
	srcLoc.PlacedFootprint.Footprint.Depth = 1;
	srcLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;

	D3D12_TEXTURE_COPY_LOCATION dstLoc{};
	dstLoc.pResource = tex.texture.Get();
	dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLoc.SubresourceIndex = 0;

	g_gl.cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

	D3D12_RESOURCE_BARRIER toSrv{};
	toSrv.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toSrv.Transition.pResource = tex.texture.Get();
	toSrv.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	toSrv.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	toSrv.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	g_gl.cmdList->ResourceBarrier(1, &toSrv);

	tex.state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	tex.gpuValid = true;
}



static GLenum QD3D12_NormalizeCopyTextureFormat(GLenum format)
{
	switch (format)
	{
	case 1:
		return GL_LUMINANCE;
	case 3:
		return GL_RGB;
	case 4:
		return GL_RGBA;
	case GL_ALPHA:
	case GL_LUMINANCE:
	case GL_INTENSITY:
	case GL_RGB:
	case GL_RGBA:
		return format;
	default:
		return GL_RGBA;
	}
}

static void QD3D12_PackRGBA8ToTextureFormat(const uint8_t* srcRGBA, int pixelCount, GLenum dstFormat, std::vector<uint8_t>& out)
{
	dstFormat = QD3D12_NormalizeCopyTextureFormat(dstFormat);

	switch (dstFormat)
	{
	case GL_RGB:
		out.resize((size_t)pixelCount * 3);
		for (int i = 0; i < pixelCount; ++i)
		{
			out[(size_t)i * 3 + 0] = srcRGBA[(size_t)i * 4 + 0];
			out[(size_t)i * 3 + 1] = srcRGBA[(size_t)i * 4 + 1];
			out[(size_t)i * 3 + 2] = srcRGBA[(size_t)i * 4 + 2];
		}
		break;

	case GL_ALPHA:
		out.resize((size_t)pixelCount);
		for (int i = 0; i < pixelCount; ++i)
		{
			out[(size_t)i] = srcRGBA[(size_t)i * 4 + 3];
		}
		break;

	case GL_LUMINANCE:
	case GL_INTENSITY:
		out.resize((size_t)pixelCount);
		for (int i = 0; i < pixelCount; ++i)
		{
			const uint8_t r = srcRGBA[(size_t)i * 4 + 0];
			const uint8_t g = srcRGBA[(size_t)i * 4 + 1];
			const uint8_t b = srcRGBA[(size_t)i * 4 + 2];
			out[(size_t)i] = (uint8_t)((77u * r + 150u * g + 29u * b + 128u) >> 8);
		}
		break;

	case GL_RGBA:
	default:
		out.assign(srcRGBA, srcRGBA + (size_t)pixelCount * 4);
		break;
	}
}

static bool QD3D12_ReadFramebufferRegionRGBA8(GLint x, GLint y, GLsizei width, GLsizei height, std::vector<uint8_t>& outRGBA)
{
	outRGBA.clear();

	if (!g_currentWindow || !g_gl.device || !g_gl.queue || !g_gl.cmdList)
		return false;

	if (width <= 0 || height <= 0)
		return false;

	QD3D12_EnsureFrameOpen();
	QD3D12_FlushQueuedBatches();

	QD3D12Window& w = *g_currentWindow;

	ID3D12Resource* srcResource = nullptr;
	D3D12_RESOURCE_STATES* trackedState = nullptr;
	UINT srcWidth = 0;
	UINT srcHeight = 0;

	const bool wantNativeBuffer =
		(g_gl.readBuffer == GL_FRONT) ||
		(g_gl.framePhase == QD3D12_FRAME_NATIVE_POST_UPSCALE) ||
		g_gl.sceneResolvedThisFrame;

	if (wantNativeBuffer)
	{
		srcResource = w.backBuffers[w.frameIndex].Get();
		trackedState = &w.backBufferState[w.frameIndex];
		srcWidth = w.width;
		srcHeight = w.height;
	}
	else
	{
		srcResource = w.sceneColorBuffers[w.frameIndex].Get();
		trackedState = &w.sceneColorState[w.frameIndex];
		srcWidth = w.renderWidth;
		srcHeight = w.renderHeight;
	}

	if (!srcResource || !trackedState || srcWidth == 0 || srcHeight == 0)
		return false;

	if (x < 0 || y < 0 || (UINT)(x + width) > srcWidth || (UINT)(y + height) > srcHeight)
		return false;

	const UINT srcLeft = (UINT)x;
	const UINT srcTop = srcHeight - (UINT)(y + height);
	const UINT copyWidth = (UINT)width;
	const UINT copyHeight = (UINT)height;
	const UINT rowPitch = (copyWidth * 4u + 255u) & ~255u;
	const UINT64 readbackBytes = (UINT64)rowPitch * (UINT64)copyHeight;

	ComPtr<ID3D12Resource> readback;
	D3D12_HEAP_PROPERTIES hp{};
	hp.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC rd{};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = readbackBytes;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	QD3D12_CHECK(g_gl.device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&readback)));

	const D3D12_RESOURCE_STATES originalState = *trackedState;
	QD3D12_TransitionResource(g_gl.cmdList.Get(), srcResource, *trackedState, D3D12_RESOURCE_STATE_COPY_SOURCE);

	D3D12_TEXTURE_COPY_LOCATION srcLoc{};
	srcLoc.pResource = srcResource;
	srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	srcLoc.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION dstLoc{};
	dstLoc.pResource = readback.Get();
	dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	dstLoc.PlacedFootprint.Offset = 0;
	dstLoc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dstLoc.PlacedFootprint.Footprint.Width = copyWidth;
	dstLoc.PlacedFootprint.Footprint.Height = copyHeight;
	dstLoc.PlacedFootprint.Footprint.Depth = 1;
	dstLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;

	D3D12_BOX srcBox{};
	srcBox.left = srcLeft;
	srcBox.top = srcTop;
	srcBox.front = 0;
	srcBox.right = srcLeft + copyWidth;
	srcBox.bottom = srcTop + copyHeight;
	srcBox.back = 1;

	g_gl.cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &srcBox);
	QD3D12_TransitionResource(g_gl.cmdList.Get(), srcResource, *trackedState, originalState);

	QD3D12_CHECK(g_gl.cmdList->Close());
	ID3D12CommandList* lists[] = { g_gl.cmdList.Get() };
	g_gl.queue->ExecuteCommandLists(1, lists);

	const UINT64 signalValue = g_gl.nextFenceValue++;
	QD3D12_CHECK(g_gl.queue->Signal(g_gl.fence.Get(), signalValue));
	if (g_gl.fence->GetCompletedValue() < signalValue)
	{
		QD3D12_CHECK(g_gl.fence->SetEventOnCompletion(signalValue, g_gl.fenceEvent));
		WaitForSingleObject(g_gl.fenceEvent, INFINITE);
	}

	outRGBA.resize((size_t)copyWidth * (size_t)copyHeight * 4);
	void* mapped = nullptr;
	QD3D12_CHECK(readback->Map(0, nullptr, &mapped));
	const uint8_t* srcBytes = (const uint8_t*)mapped;
	for (UINT row = 0; row < copyHeight; ++row)
	{
		memcpy(outRGBA.data() + (size_t)row * (size_t)copyWidth * 4,
			srcBytes + (size_t)row * rowPitch,
			(size_t)copyWidth * 4);
	}
	readback->Unmap(0, nullptr);

	FrameResources& fr = w.frames[w.frameIndex];
	QD3D12_CHECK(fr.cmdAlloc->Reset());
	QD3D12_CHECK(g_gl.cmdList->Reset(fr.cmdAlloc.Get(), nullptr));
	QD3D12_BindTargetsForCurrentPhase(w);

	return true;
}

static void QD3D12_CopyRGBARegionIntoTexture(TextureResource& tex, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, const uint8_t* rgbaBytes)
{
	if (width <= 0 || height <= 0 || !rgbaBytes)
		return;

	if (tex.compressed)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	tex.format = QD3D12_NormalizeCopyTextureFormat(tex.format);
	tex.compressed = false;
	tex.compressedInternalFormat = 0;
	tex.compressedBlockBytes = 0;
	tex.compressedImageSize = 0;
	tex.forceOpaqueAlpha = false;
	tex.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const int bpp = BytesPerPixel(tex.format, GL_UNSIGNED_BYTE);
	if (bpp <= 0 || tex.width <= 0 || tex.height <= 0)
		return;

	const size_t needed = (size_t)tex.width * (size_t)tex.height * (size_t)bpp;
	if (tex.sysmem.size() != needed)
		tex.sysmem.resize(needed, 0);

	std::vector<uint8_t> packed;
	QD3D12_PackRGBA8ToTextureFormat(rgbaBytes, width * height, tex.format, packed);

	for (int row = 0; row < height; ++row)
	{
		const size_t dstOff = ((size_t)(yoffset + row) * (size_t)tex.width + (size_t)xoffset) * (size_t)bpp;
		const size_t srcOff = (size_t)row * (size_t)width * (size_t)bpp;
		memcpy(tex.sysmem.data() + dstOff, packed.data() + srcOff, (size_t)width * (size_t)bpp);
	}

	tex.gpuValid = false;
}

static PipelineMode PickPipeline(bool useTex0, bool useTex1)
{
	const bool textured = useTex0 || useTex1;

	if (g_gl.blend)
		return textured ? PIPE_BLEND_TEX : PIPE_BLEND_UNTEX;

	if (g_gl.alphaTest)
		return textured ? PIPE_ALPHA_TEST_TEX : PIPE_OPAQUE_UNTEX;

	return textured ? PIPE_OPAQUE_TEX : PIPE_OPAQUE_UNTEX;
}

static void SetDynamicFixedFunctionState(ID3D12GraphicsCommandList* cl)
{
	cl->SetGraphicsRootSignature(g_gl.rootSig.Get());
	ID3D12DescriptorHeap* heaps[] = { g_gl.srvHeap.Get() };
	cl->SetDescriptorHeaps(1, heaps);
	cl->RSSetViewports(1, &g_currentWindow->viewport);
	cl->RSSetScissorRects(1, &g_currentWindow->scissor);
}

static Mat4 CurrentMVP()
{
	return Mat4::Multiply(g_gl.projStack.back(), g_gl.modelStack.back());
}

void glLoadModelMatrixf(const float* m16)
{
	Mat4 m = Mat4::Identity();

	if (m16)
	{
		memcpy(m.m, m16, sizeof(float) * 16);
	}

	memcpy(g_gl.modelMatrix.m, m.m, sizeof(g_gl.modelMatrix.m));
}

void APIENTRY glMultMatrixf(const GLfloat* m)
{
	if (!m)
		return;

	Mat4 rhs{};
	memcpy(rhs.m, m, sizeof(rhs.m));

	auto& top = QD3D12_CurrentMatrixStack().back();
	top = Mat4::Multiply(top, rhs);
}

static Mat4 CurrentModelMatrix()
{
	return g_gl.modelMatrix;
}

static inline void AppendVerticesFast(std::vector<GLVertex>& dst, const std::vector<GLVertex>& src)
{
	if (src.empty())
		return;

	const size_t oldSize = dst.size();
	const size_t addCount = src.size();
	dst.resize(oldSize + addCount);

	memcpy(dst.data() + oldSize, src.data(), addCount * sizeof(GLVertex));
}

static void FlushImmediate(GLenum mode, const GLVertex* src, size_t n)
{
	if (!src || n == 0)
		return;

	QD3D12_EnsureFrameOpen();

	const bool arbProgramsActive = QD3D12ARB_IsActive();
	const bool useTex0 = arbProgramsActive ? (g_gl.boundTexture[0] != 0) : g_gl.texture2D[0];
	const bool useTex1 = arbProgramsActive ? (g_gl.boundTexture[1] != 0) : g_gl.texture2D[1];

	TextureResource* boundTextures[QD3D12_MaxTextureUnits] = {};
	for (UINT unit = 0; unit < QD3D12_MaxTextureUnits; ++unit)
		boundTextures[unit] = &g_gl.whiteTexture;

	for (UINT unit = 0; unit < QD3D12_MaxTextureUnits; ++unit)
	{
		const bool wantsUnit = arbProgramsActive ? (g_gl.boundTexture[unit] != 0) : (g_gl.texture2D[unit] && unit < 2);
		if (!wantsUnit)
			continue;

		auto it = g_gl.textures.find(g_gl.boundTexture[unit]);
		if (it != g_gl.textures.end())
			boundTextures[unit] = &it->second;
	}

	for (UINT unit = 0; unit < QD3D12_MaxTextureUnits; ++unit)
	{
		TextureResource* tex = boundTextures[unit];
		if (tex && tex != &g_gl.whiteTexture && !tex->gpuValid)
		{
			EnsureTextureResource(*tex);
			UploadTexture(*tex);
		}
	}

	TextureResource* tex0 = boundTextures[0];
	TextureResource* tex1 = boundTextures[1];

	BatchKey key = BuildCurrentBatchKey(mode, tex0, tex1, boundTextures);
	const size_t markerCursor = g_gl.queryMarkers.size();

	QueuedBatch* batch = nullptr;

	if (!g_gl.queuedBatches.empty() &&
		BatchKeyEquals(g_gl.queuedBatches.back().key, key) &&
		g_gl.queuedBatches.back().markerEnd == markerCursor)
	{
		batch = &g_gl.queuedBatches.back();
	}
	else
	{
		QueuedBatch newBatch{};
		newBatch.key = key;
		newBatch.markerBegin = markerCursor;
		newBatch.markerEnd = markerCursor;
		newBatch.firstVertex = 0;
		newBatch.vertexCount = 0;
		g_gl.queuedBatches.push_back(newBatch);
		batch = &g_gl.queuedBatches.back();
	}

	auto AppendToBatch = [&](size_t outCount) -> GLVertex*
		{
			if (outCount == 0)
				return nullptr;

			size_t first = 0;
			GLVertex* dst = QD3D12_AllocFrameVertices(outCount, &first);
			if (!dst)
				return nullptr;

			if (batch->vertexCount == 0)
			{
				batch->firstVertex = first;
				batch->vertexCount = outCount;
			}
			else
			{
				// Since arena is linear, merged batches must remain contiguous.
				assert(batch->firstVertex + batch->vertexCount == first);
				batch->vertexCount += outCount;
			}

			return dst;
		};

	switch (mode)
	{
	case GL_TRIANGLES:
	case GL_POINTS:
	{
		GLVertex* out = AppendToBatch(n);
		if (!out)
			return;

		memcpy(out, src, n * sizeof(GLVertex));
		return;
	}

	case GL_LINES:
	{
		const size_t segCount = n >> 1;
		const size_t outCount = segCount * 2;

		GLVertex* out = AppendToBatch(outCount);
		if (!out)
			return;

		for (size_t i = 0, d = 0; i + 1 < n; i += 2, d += 2)
		{
			out[d + 0] = src[i + 0];
			out[d + 1] = src[i + 1];
		}
		return;
	}

	case GL_LINE_STRIP:
	{
		if (n < 2)
			return;

		const size_t outCount = (n - 1) * 2;

		GLVertex* out = AppendToBatch(outCount);
		if (!out)
			return;

		for (size_t i = 1, d = 0; i < n; ++i, d += 2)
		{
			out[d + 0] = src[i - 1];
			out[d + 1] = src[i];
		}
		return;
	}

	case GL_LINE_LOOP:
	{
		if (n < 2)
			return;

		const size_t outCount = n * 2;

		GLVertex* out = AppendToBatch(outCount);
		if (!out)
			return;

		size_t d = 0;
		for (size_t i = 1; i < n; ++i, d += 2)
		{
			out[d + 0] = src[i - 1];
			out[d + 1] = src[i];
		}

		out[d + 0] = src[n - 1];
		out[d + 1] = src[0];
		return;
	}

	case GL_TRIANGLE_STRIP:
	{
		if (n < 3)
			return;

		const size_t outCount = (n - 2) * 3;

		GLVertex* out = AppendToBatch(outCount);
		if (!out)
			return;

		size_t d = 0;
		for (size_t i = 2; i < n; ++i, d += 3)
		{
			if ((i & 1) == 0)
			{
				out[d + 0] = src[i - 2];
				out[d + 1] = src[i - 1];
				out[d + 2] = src[i];
			}
			else
			{
				out[d + 0] = src[i - 1];
				out[d + 1] = src[i - 2];
				out[d + 2] = src[i];
			}
		}
		return;
	}

	case GL_TRIANGLE_FAN:
	{
		if (n < 3)
			return;

		const size_t outCount = (n - 2) * 3;

		GLVertex* out = AppendToBatch(outCount);
		if (!out)
			return;

		const GLVertex v0 = src[0];
		size_t d = 0;

		for (size_t i = 2; i < n; ++i, d += 3)
		{
			out[d + 0] = v0;
			out[d + 1] = src[i - 1];
			out[d + 2] = src[i];
		}
		return;
	}

	case GL_QUADS:
	{
		const size_t quadCount = n >> 2;
		const size_t outCount = quadCount * 6;

		GLVertex* out = AppendToBatch(outCount);
		if (!out)
			return;

		size_t d = 0;
		for (size_t i = 0; i + 3 < n; i += 4, d += 6)
		{
			const GLVertex& v0 = src[i + 0];
			const GLVertex& v1 = src[i + 1];
			const GLVertex& v2 = src[i + 2];
			const GLVertex& v3 = src[i + 3];

			out[d + 0] = v0;
			out[d + 1] = v1;
			out[d + 2] = v2;
			out[d + 3] = v0;
			out[d + 4] = v2;
			out[d + 5] = v3;
		}
		return;
	}

	case GL_QUAD_STRIP:
	{
		if (n < 4)
			return;

		const size_t quadCount = (n - 2) >> 1;
		const size_t outCount = quadCount * 6;

		GLVertex* out = AppendToBatch(outCount);
		if (!out)
			return;

		size_t d = 0;
		for (size_t i = 0; i + 3 < n; i += 2, d += 6)
		{
			const GLVertex& v0 = src[i + 0];
			const GLVertex& v1 = src[i + 1];
			const GLVertex& v2 = src[i + 2];
			const GLVertex& v3 = src[i + 3];

			out[d + 0] = v0;
			out[d + 1] = v1;
			out[d + 2] = v2;
			out[d + 3] = v2;
			out[d + 4] = v1;
			out[d + 5] = v3;
		}
		return;
	}

	case GL_POLYGON:
	{
		// Keep this path separate for now.
		// Best next step is tessellating directly into the arena too.
		std::vector<GLVertex> tess;
		tess.reserve(n);

		std::vector<GLVertex> temp(src, src + n);
		TessellatePolygon(temp, tess);

		if (!tess.empty())
		{
			GLVertex* out = AppendToBatch(tess.size());
			if (!out)
				return;

			memcpy(out, tess.data(), tess.size() * sizeof(GLVertex));
		}
		return;
	}

	default:
		assert(!"Unknown FlushImmediate type!");
		return;
	}
}

static void QD3D12_EmitQueryMarkers(size_t beginIdx, size_t endIdx)
{
	for (size_t i = beginIdx; i < endIdx; ++i)
	{
		const QueryMarker& m = g_gl.queryMarkers[i];
		auto               it = g_gl.queries.find(m.id);
		if (it == g_gl.queries.end())
			continue;

		GLOcclusionQuery& q = it->second;
		if (q.heapIndex == UINT_MAX)
			continue;

		if (m.type == QueryMarker::Begin)
		{
			g_gl.cmdList->BeginQuery(g_gl.occlusionQueryHeap.Get(), D3D12_QUERY_TYPE_OCCLUSION, q.heapIndex);
		}
		else
		{
			g_gl.cmdList->EndQuery(g_gl.occlusionQueryHeap.Get(), D3D12_QUERY_TYPE_OCCLUSION, q.heapIndex);

			g_gl.cmdList->ResolveQueryData(
				g_gl.occlusionQueryHeap.Get(), D3D12_QUERY_TYPE_OCCLUSION, q.heapIndex, 1, g_gl.occlusionReadback.Get(), sizeof(UINT64) * q.heapIndex);

			q.submittedFence = QD3D12_CurrentSubmissionFenceValue();
		}
	}
}

static void QD3D12_FlushQueuedBatches()
{
	if (g_gl.queuedBatches.empty())
		return;

	SetDynamicFixedFunctionState(g_gl.cmdList.Get());
	QD3D12_BindTargetsForCurrentPhase(*g_currentWindow);

	const bool nativeColorOnly = (g_gl.framePhase == QD3D12_FRAME_NATIVE_POST_UPSCALE);

	ID3D12PipelineState* lastPSO = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE lastTex0{};
	D3D12_GPU_DESCRIPTOR_HANDLE lastTex1{};
	bool haveLastTex0 = false;
	bool haveLastTex1 = false;
	D3D12_PRIMITIVE_TOPOLOGY lastTopo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

	for (size_t i = 0; i < g_gl.queuedBatches.size(); ++i)
	{
		const QueuedBatch& batch = g_gl.queuedBatches[i];
		const GLVertex* verts = QD3D12_GetBatchVertices(batch);
		const size_t count = batch.vertexCount;

		if (count <= 0)
			continue;

		const UINT vbBytes = (UINT)(count * sizeof(GLVertex));
		const UINT cbBytes = (UINT)sizeof(DrawConstants);

		if (QD3D12_EnsureUploadSpaceForDraw(vbBytes, cbBytes))
		{
			// The command list was closed/executed/reset, so all cached command-list
			// state is invalid.
			SetDynamicFixedFunctionState(g_gl.cmdList.Get());
			QD3D12_BindTargetsForCurrentPhase(*g_currentWindow);

			lastPSO = nullptr;
			haveLastTex0 = false;
			haveLastTex1 = false;
			lastTopo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}

		QD3D12_EmitQueryMarkers(batch.markerBegin, batch.markerEnd);

		g_gl.cmdList->RSSetViewports(1, &batch.key.viewport);
		g_gl.cmdList->RSSetScissorRects(1, &batch.key.scissor);

		UploadAlloc vbAlloc = QD3D12_AllocUpload(vbBytes, 256);
		memcpy(vbAlloc.cpu, verts, vbBytes);

		UploadAlloc cbAlloc = QD3D12_AllocUpload(cbBytes, 256);
		DrawConstants* dc = reinterpret_cast<DrawConstants*>(cbAlloc.cpu);
		memset(dc, 0, sizeof(*dc));

		dc->mvp = batch.key.mvp;
		dc->prevMvp = batch.key.prevMvp;
		dc->geometryFlag = batch.key.geometryFlag;
		dc->roughness = batch.key.roughness;
		dc->materialType = batch.key.materialType;
		dc->alphaFunc = batch.key.alphaFunc;
		dc->modelMatrix = batch.key.modelMatrix;
		dc->alphaRef = batch.key.alphaRef;
		dc->useTex0 = batch.key.useTex0;
		dc->useTex1 = batch.key.useTex1;
		dc->tex1IsLightmap = batch.key.tex1IsLightmap;
		dc->texEnvMode0 = batch.key.texEnvMode0;
		dc->texEnvMode1 = batch.key.texEnvMode1;
		memcpy(dc->texComb0RGB, batch.key.texComb0RGB, sizeof(dc->texComb0RGB));
		memcpy(dc->texComb0Alpha, batch.key.texComb0Alpha, sizeof(dc->texComb0Alpha));
		memcpy(dc->texComb0Operand, batch.key.texComb0Operand, sizeof(dc->texComb0Operand));
		memcpy(dc->texEnvColor0, batch.key.texEnvColor0, sizeof(dc->texEnvColor0));
		memcpy(dc->texComb1RGB, batch.key.texComb1RGB, sizeof(dc->texComb1RGB));
		memcpy(dc->texComb1Alpha, batch.key.texComb1Alpha, sizeof(dc->texComb1Alpha));
		memcpy(dc->texComb1Operand, batch.key.texComb1Operand, sizeof(dc->texComb1Operand));
		memcpy(dc->texEnvColor1, batch.key.texEnvColor1, sizeof(dc->texEnvColor1));

		dc->fogEnabled = g_gl.fog ? 1.0f : 0.0f;

		switch (g_gl.fogMode)
		{
		case GL_LINEAR: dc->fogMode = 0.0f; break;
		case GL_EXP:    dc->fogMode = 1.0f; break;
		case GL_EXP2:   dc->fogMode = 2.0f; break;
		default:        dc->fogMode = 1.0f; break;
		}

		dc->fogDensity = g_gl.fogDensity;
		dc->fogStart = g_gl.fogStart;
		dc->fogEnd = g_gl.fogEnd;
		dc->fogColor[0] = g_gl.fogColor[0];
		dc->fogColor[1] = g_gl.fogColor[1];
		dc->fogColor[2] = g_gl.fogColor[2];
		dc->fogColor[3] = g_gl.fogColor[3];
		const float activeWidth = (float)QD3D12_ActiveRasterWidth(*g_currentWindow);
		const float activeHeight = (float)QD3D12_ActiveRasterHeight(*g_currentWindow);
		dc->renderSize[0] = activeWidth;
		dc->renderSize[1] = activeHeight;
		dc->invRenderSize[0] = activeWidth > 0.0f ? (1.0f / activeWidth) : 0.0f;
		dc->invRenderSize[1] = activeHeight > 0.0f ? (1.0f / activeHeight) : 0.0f;
		dc->jitterPixels[0] = g_gl.jitterX;
		dc->jitterPixels[1] = g_gl.jitterY;
		dc->prevJitterPixels[0] = g_gl.prevJitterX;
		dc->prevJitterPixels[1] = g_gl.prevJitterY;

		if (batch.key.useARBPrograms)
		{
			memcpy(dc->arbEnv, batch.key.arbConstants.env, sizeof(dc->arbEnv));
			memcpy(dc->arbLocalVP, batch.key.arbConstants.vertexLocal, sizeof(dc->arbLocalVP));
			memcpy(dc->arbLocalFP, batch.key.arbConstants.fragmentLocal, sizeof(dc->arbLocalFP));
		}

		D3D12_VERTEX_BUFFER_VIEW vbv{};
		vbv.BufferLocation = vbAlloc.gpu;
		vbv.SizeInBytes = (UINT)(count * sizeof(GLVertex));
		vbv.StrideInBytes = sizeof(GLVertex);

		ID3D12PipelineState* pso = nullptr;
		const D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType =
			GetTopologyTypeFromTopology(batch.key.topology);

		if (topoType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT) {
			dc->PointSize = g_gl.pointSize;
		}
		else {
			dc->PointSize = 1.0f;
		}

		if (batch.key.useARBPrograms)
		{
			pso = QD3D12_GetARBPSO(batch.key, topoType, nativeColorOnly);
			if (!pso)
				continue;
		}
		else
		{
			pso = QD3D12_GetPSO(batch.key, topoType, nativeColorOnly);
		}

		if (pso != lastPSO)
		{
			g_gl.cmdList->SetPipelineState(pso);
			lastPSO = pso;
		}

		if (batch.key.useARBPrograms)
		{
			for (UINT unit = 0; unit < QD3D12_MaxTextureUnits; ++unit)
			{
				UINT srvIndex = batch.key.textureSrvIndex[unit];
				if (srvIndex == UINT_MAX)
					srvIndex = g_gl.whiteTexture.srvIndex;

				g_gl.cmdList->SetGraphicsRootDescriptorTable(1 + unit, QD3D12_SrvGpu(srvIndex));
			}

			haveLastTex0 = false;
			haveLastTex1 = false;
		}
		else
		{
			D3D12_GPU_DESCRIPTOR_HANDLE tex0Gpu = QD3D12_SrvGpu(batch.key.tex0SrvIndex);
			D3D12_GPU_DESCRIPTOR_HANDLE tex1Gpu = QD3D12_SrvGpu(batch.key.tex1SrvIndex);

			if (!haveLastTex0 || tex0Gpu.ptr != lastTex0.ptr)
			{
				// Don't know if this should be here or not could burry bugs, but rather then crash set to white.
				if (batch.key.tex0SrvIndex == UINT_MAX)
				{
					tex0Gpu = QD3D12_SrvGpu(g_gl.whiteTexture.srvIndex);
				}
				g_gl.cmdList->SetGraphicsRootDescriptorTable(1, tex0Gpu);
				lastTex0 = tex0Gpu;
				haveLastTex0 = true;
			}

			if (!haveLastTex1 || tex1Gpu.ptr != lastTex1.ptr)
			{
				g_gl.cmdList->SetGraphicsRootDescriptorTable(2, tex1Gpu);
				lastTex1 = tex1Gpu;
				haveLastTex1 = true;
			}
		}

		g_gl.cmdList->SetGraphicsRootConstantBufferView(0, cbAlloc.gpu);
		g_gl.cmdList->OMSetStencilRef(batch.key.stencilRef);

		if (batch.key.topology != lastTopo)
		{
			g_gl.cmdList->IASetPrimitiveTopology(batch.key.topology);
			lastTopo = batch.key.topology;
		}

		g_gl.cmdList->IASetVertexBuffers(0, 1, &vbv);
		g_gl.cmdList->DrawInstanced((UINT)count, 1, 0, 0);
	}

	g_gl.queuedBatches.clear();
}

// ============================================================
// SECTION 11: GL exports
// ============================================================

const GLubyte* APIENTRY glGetString(GLenum name)
{
	switch (name)
	{
	case GL_VENDOR: return (const GLubyte*)vendor;
	case GL_RENDERER: return (const GLubyte*)renderer;
	case GL_VERSION: return (const GLubyte*)version;
	case GL_EXTENSIONS: return (const GLubyte*)extensions;
	case GL_PROGRAM_ERROR_STRING_ARB: return (const GLubyte*)QD3D12ARB_GetProgramErrorString();
	default: return (const GLubyte*)"";
	}
}

void APIENTRY glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
	g_gl.clearColor[0] = r;
	g_gl.clearColor[1] = g;
	g_gl.clearColor[2] = b;
	g_gl.clearColor[3] = a;
}

static D3D12_RECT QD3D12_GetActiveClearRect()
{
	const UINT activeWidth = QD3D12_ActiveRasterWidth(*g_currentWindow);
	const UINT activeHeight = QD3D12_ActiveRasterHeight(*g_currentWindow);

	if (g_gl.scissorTest)
	{
		const float scaleX = (g_currentWindow->width > 0) ? ((float)activeWidth / (float)g_currentWindow->width) : 1.0f;
		const float scaleY = (g_currentWindow->height > 0) ? ((float)activeHeight / (float)g_currentWindow->height) : 1.0f;

		const LONG leftGL = (LONG)std::lround((double)g_gl.scissorX * scaleX);
		const LONG rightGL = (LONG)std::lround((double)(g_gl.scissorX + g_gl.scissorW) * scaleX);
		const LONG topGL = (LONG)std::lround((double)(g_gl.scissorY + g_gl.scissorH) * scaleY);
		const LONG bottomGL = (LONG)std::lround((double)g_gl.scissorY * scaleY);

		D3D12_RECT r{};
		r.left = ClampValue<LONG>(leftGL, 0, (LONG)activeWidth);
		r.right = ClampValue<LONG>(rightGL, 0, (LONG)activeWidth);

		// OpenGL scissor is bottom-left origin, D3D12 is top-left origin.
		r.top = ClampValue<LONG>((LONG)activeHeight - topGL, 0, (LONG)activeHeight);
		r.bottom = ClampValue<LONG>((LONG)activeHeight - bottomGL, 0, (LONG)activeHeight);

		if (r.right < r.left)   std::swap(r.right, r.left);
		if (r.bottom < r.top)   std::swap(r.bottom, r.top);
		return r;
	}

	D3D12_RECT r{};
	r.left = 0;
	r.top = 0;
	r.right = (LONG)activeWidth;
	r.bottom = (LONG)activeHeight;
	return r;
}

static void QD3D12_EnsureFrameOpen()
{
	if (g_gl.frameOpen)
	{
		if (g_gl.frameOwner == g_currentWindow)
			return;

		QD3D12_SubmitOpenFrameNoPresentAndWait();
	}

	QD3D12_BeginFrame();
}

void APIENTRY glClear(GLbitfield mask)
{
	if (mask == 0)
		return;

	QD3D12_EnsureFrameOpen();
	QD3D12_FlushQueuedBatches();
	QD3D12_BindTargetsForCurrentPhase(*g_currentWindow);

	const D3D12_RECT clearRect = QD3D12_GetActiveClearRect();
	g_gl.cmdList->RSSetScissorRects(1, &clearRect);

	const bool nativePhase = (g_gl.framePhase == QD3D12_FRAME_NATIVE_POST_UPSCALE);

	if (mask & GL_COLOR_BUFFER_BIT)
	{
		const float cc[4] =
		{
			g_gl.clearColor[0],
			g_gl.clearColor[1],
			g_gl.clearColor[2],
			g_gl.clearColor[3]
		};

		if (nativePhase)
		{
			g_gl.cmdList->ClearRenderTargetView(CurrentBackBufferRTV(), cc, 1, &clearRect);
		}
		else
		{
			// Keep the auxiliary MRT clears fixed to their creation-time values so the
			// debug layer does not spam CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE.
			// Material metadata is written by draws, not by glClear.
			const float normalClear[4] = { 0.0f, 0.0f, 1.0f, 0.5f };
			const float positionClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			const float velocityClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			g_gl.cmdList->ClearRenderTargetView(CurrentRTV(), cc, 1, &clearRect);
			g_gl.cmdList->ClearRenderTargetView(CurrentNormalRTV(), normalClear, 1, &clearRect);
			g_gl.cmdList->ClearRenderTargetView(CurrentPositionRTV(), positionClear, 1, &clearRect);
			g_gl.cmdList->ClearRenderTargetView(CurrentVelocityRTV(), velocityClear, 1, &clearRect);
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE dsv = nativePhase ? CurrentNativeDepthDSV() : CurrentSceneDepthDSV();

	if (mask & GL_DEPTH_BUFFER_BIT)
	{
		g_gl.cmdList->ClearDepthStencilView(
			dsv,
			D3D12_CLEAR_FLAG_DEPTH,
			(FLOAT)ClampValue<GLclampd>(g_gl.clearDepthValue, 0.0, 1.0),
			0,
			1,
			&clearRect);
	}

	if (mask & GL_STENCIL_BUFFER_BIT)
	{
		g_gl.cmdList->ClearDepthStencilView(
			dsv,
			D3D12_CLEAR_FLAG_STENCIL,
			(FLOAT)ClampValue<GLclampd>(g_gl.clearDepthValue, 0.0, 1.0),
			(UINT8)(g_gl.clearStencilValue & 0xFF),
			1,
			&clearRect);
	}
}


void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	g_gl.viewportX = x;
	g_gl.viewportY = y;
	g_gl.viewportW = width;
	g_gl.viewportH = height;
	QD3D12_UpdateViewportState();
}

void APIENTRY glEnable(GLenum cap)
{
	switch (cap)
	{
	case GL_BLEND: g_gl.blend = true; break;
	case GL_ALPHA_TEST: g_gl.alphaTest = true; break;
	case GL_DEPTH_TEST: g_gl.depthTest = true; break;
	case GL_CULL_FACE: g_gl.cullFace = true; break;
	case GL_SCISSOR_TEST: g_gl.scissorTest = true; break;
	case GL_STENCIL_TEST: g_gl.stencilTest = true; break;
#ifdef GL_STENCIL_TEST_TWO_SIDE_EXT
	case GL_STENCIL_TEST_TWO_SIDE_EXT: g_gl.stencilTwoSide = true; g_gl.stencilTest = true; break;
#endif
#ifdef GL_DEPTH_BOUNDS_TEST_EXT
	case GL_DEPTH_BOUNDS_TEST_EXT: g_gl.depthBoundsTest = true; break;
#endif
	case GL_VERTEX_PROGRAM_ARB:
	case GL_FRAGMENT_PROGRAM_ARB:
		QD3D12ARB_SetEnabled(cap, true);
		break;
	case GL_TEXTURE_2D:
#ifdef GL_TEXTURE_RECTANGLE_ARB
	case GL_TEXTURE_RECTANGLE_ARB:
#endif
#ifdef GL_TEXTURE_CUBE_MAP_EXT
	case GL_TEXTURE_CUBE_MAP_EXT:
#endif
		g_gl.texture2D[g_gl.activeTextureUnit] = true;
		break;
	case GL_FOG: g_gl.fog = true; break;
	default: break;
	}
}

void APIENTRY glDisable(GLenum cap)
{
	switch (cap)
	{
	case GL_BLEND: g_gl.blend = false; break;
	case GL_ALPHA_TEST: g_gl.alphaTest = false; break;
	case GL_DEPTH_TEST: g_gl.depthTest = false; break;
	case GL_CULL_FACE: g_gl.cullFace = false; break;
	case GL_SCISSOR_TEST: g_gl.scissorTest = false; break;
	case GL_STENCIL_TEST: g_gl.stencilTest = false; break;
#ifdef GL_STENCIL_TEST_TWO_SIDE_EXT
	case GL_STENCIL_TEST_TWO_SIDE_EXT: g_gl.stencilTwoSide = false; break;
#endif
#ifdef GL_DEPTH_BOUNDS_TEST_EXT
	case GL_DEPTH_BOUNDS_TEST_EXT: g_gl.depthBoundsTest = false; break;
#endif
	case GL_VERTEX_PROGRAM_ARB:
	case GL_FRAGMENT_PROGRAM_ARB:
		QD3D12ARB_SetEnabled(cap, false);
		break;
	case GL_TEXTURE_2D:
#ifdef GL_TEXTURE_RECTANGLE_ARB
	case GL_TEXTURE_RECTANGLE_ARB:
#endif
#ifdef GL_TEXTURE_CUBE_MAP_EXT
	case GL_TEXTURE_CUBE_MAP_EXT:
#endif
		g_gl.texture2D[g_gl.activeTextureUnit] = false;
		break;
	case GL_FOG: g_gl.fog = false; break;
	default: break;
	}
}

void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	g_gl.blendSrc = sfactor;
	g_gl.blendDst = dfactor;
}

void APIENTRY glAlphaFunc(GLenum func, GLclampf ref)
{
	g_gl.alphaFunc = func;
	g_gl.alphaRef = ref;
}

void APIENTRY glDepthMask(GLboolean flag)
{
	g_gl.depthWrite = (flag != 0);
}

void APIENTRY glDepthRange(GLclampd zNear, GLclampd zFar)
{
	g_gl.depthRangeNear = ClampValue<GLclampd>(zNear, 0.0, 1.0);
	g_gl.depthRangeFar = ClampValue<GLclampd>(zFar, 0.0, 1.0);
	QD3D12_UpdateViewportState();
}

void APIENTRY glCullFace(GLenum mode)
{
	g_gl.cullMode = mode;
}

void APIENTRY glPolygonMode(GLenum, GLenum)
{
}

void APIENTRY glShadeModel(GLenum mode)
{
	g_gl.shadeModel = mode;
}

void APIENTRY glHint(GLenum target, GLenum mode)
{
	if (target == GL_FOG_HINT)
		g_gl.fogHint = mode;
}

void APIENTRY glFinish(void)
{
	if (!g_gl.device || !g_gl.queue || !g_gl.cmdList)
		return;

	if (!g_gl.frameOpen && g_gl.queuedBatches.empty())
	{
		QD3D12_WaitForGPU();
		return;
	}

	QD3D12_EnsureFrameOpen();
	QD3D12_FlushQueuedBatches();

	QD3D12_CHECK(g_gl.cmdList->Close());

	ID3D12CommandList* lists[] = { g_gl.cmdList.Get() };
	g_gl.queue->ExecuteCommandLists(1, lists);

	const UINT64 signalValue = g_gl.nextFenceValue++;
	QD3D12_CHECK(g_gl.queue->Signal(g_gl.fence.Get(), signalValue));

	if (g_gl.fence->GetCompletedValue() < signalValue)
	{
		QD3D12_CHECK(g_gl.fence->SetEventOnCompletion(signalValue, g_gl.fenceEvent));
		WaitForSingleObject(g_gl.fenceEvent, INFINITE);
	}

	FrameResources& fr = g_currentWindow->frames[g_currentWindow->frameIndex];
	fr.fenceValue = signalValue;

	QD3D12_CHECK(fr.cmdAlloc->Reset());
	QD3D12_CHECK(g_gl.cmdList->Reset(fr.cmdAlloc.Get(), nullptr));

	QD3D12_BindTargetsForCurrentPhase(*g_currentWindow);

	g_gl.frameOpen = true;
	g_gl.frameOwner = g_currentWindow;
}

void APIENTRY glMatrixMode(GLenum mode)
{
	g_gl.matrixMode = mode;
}

void APIENTRY glLoadIdentity(void)
{
	QD3D12_CurrentMatrixStack().back() = Mat4::Identity();
}

void APIENTRY glPushMatrix(void)
{
	auto& s = QD3D12_CurrentMatrixStack();
	s.push_back(s.back());
}

void APIENTRY glPopMatrix(void)
{
	auto& s = QD3D12_CurrentMatrixStack();
	if (s.size() > 1)
		s.pop_back();
}

void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	auto& t = QD3D12_CurrentMatrixStack().back();
	t = Mat4::Multiply(t, Mat4::Translation(x, y, z));
}

void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	auto& t = QD3D12_CurrentMatrixStack().back();
	t = Mat4::Multiply(t, Mat4::RotationAxisDeg(angle, x, y, z));
}

void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	auto& t = QD3D12_CurrentMatrixStack().back();
	t = Mat4::Multiply(t, Mat4::Scale(x, y, z));
}

void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	auto& t = QD3D12_CurrentMatrixStack().back();
	t = Mat4::Multiply(t, Mat4::Ortho(left, right, bottom, top, zNear, zFar));
}

void APIENTRY glBegin(GLenum mode)
{
	assert(!g_gl.inBeginEnd);
	g_gl.inBeginEnd = true;
	g_gl.currentPrim = mode;
	g_gl.immediateVerts.Clear();
}

void APIENTRY glEnd(void)
{
	assert(g_gl.inBeginEnd);
	g_gl.inBeginEnd = false;
	FlushImmediate(g_gl.currentPrim, g_gl.immediateVerts.Data(), g_gl.immediateVerts.Size());
}

void APIENTRY glVertex2f(GLfloat x, GLfloat y)
{
	glVertex3f(x, y, 0.0f);
}

void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	GLVertex& v = g_gl.immediateVerts.Push();

	v.px = x;
	v.py = y;
	v.pz = z;

	v.nx = 0.0f;
	v.ny = 0.0f;
	v.nz = 1.0f;
	v.tx = 1.0f;
	v.ty = 0.0f;
	v.tz = 0.0f;
	v.bx = 0.0f;
	v.by = 1.0f;
	v.bz = 0.0f;

	v.u0 = g_gl.curU[0];
	v.v0 = g_gl.curV[0];
	v.u1 = g_gl.curU[1];
	v.v1 = g_gl.curV[1];

	v.r = g_gl.curColor[0];
	v.g = g_gl.curColor[1];
	v.b = g_gl.curColor[2];
	v.a = g_gl.curColor[3];
}

void APIENTRY glVertex3fv(const GLfloat* v)
{
	glVertex3f(v[0], v[1], v[2]);
}

void APIENTRY glTexCoord2f(GLfloat s, GLfloat t)
{
	g_gl.curU[g_gl.activeTextureUnit] = s;
	g_gl.curV[g_gl.activeTextureUnit] = t;
}

void APIENTRY glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
	g_gl.curColor[0] = r;
	g_gl.curColor[1] = g;
	g_gl.curColor[2] = b;
	g_gl.curColor[3] = 1.0f;
}

void APIENTRY glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	g_gl.curColor[0] = r;
	g_gl.curColor[1] = g;
	g_gl.curColor[2] = b;
	g_gl.curColor[3] = a;
}

void APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
	for (GLsizei i = 0; i < n; ++i)
	{
		GLuint id = g_gl.nextTextureId++;
		TextureResource tex{};
		tex.glId = id;
		tex.srvIndex = UINT_MAX;
		tex.minFilter = g_gl.defaultMinFilter;
		tex.magFilter = g_gl.defaultMagFilter;
		tex.wrapS = g_gl.defaultWrapS;
		tex.wrapT = g_gl.defaultWrapT;

		g_gl.textures[id] = tex;
		textures[i] = id;
	}
}

void APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures)
{
	for (GLsizei i = 0; i < n; ++i)
	{
		const GLuint deadId = textures[i];

		for (UINT unit = 0; unit < QD3D12_MaxTextureUnits; ++unit)
		{
			if (g_gl.boundTexture[unit] == deadId)
				g_gl.boundTexture[unit] = 0;
		}

		auto it = g_gl.textures.find(deadId);
		if (it != g_gl.textures.end())
		{
			QD3D12_RetireResource(it->second.texture);
			it->second.gpuValid = false;
			g_gl.textures.erase(it);
		}
	}
}

#ifdef _DEBUG
#pragma optimize off
#endif
void APIENTRY glBindTexture(GLenum, GLuint texture)
{
	g_gl.boundTexture[g_gl.activeTextureUnit] = texture;

	if (texture == 0)
		return;

	auto& textures = g_gl.textures;
	auto it = textures.find(texture);
	if (it == textures.end())
	{
		TextureResource tex{};
		tex.glId = texture;
		tex.srvIndex = UINT_MAX;
		tex.minFilter = g_gl.defaultMinFilter;
		tex.magFilter = g_gl.defaultMagFilter;
		tex.wrapS = g_gl.defaultWrapS;
		tex.wrapT = g_gl.defaultWrapT;
		textures.emplace(texture, tex);
	}
}
#ifdef _DEBUG
#pragma optimize on
#endif

void APIENTRY glLoadMatrixf(const GLfloat* m)
{
	if (!m)
		return;

	auto& top = QD3D12_CurrentMatrixStack().back();
	memcpy(top.m, m, sizeof(top.m));
}

void APIENTRY glGetIntegerv(GLenum pname, GLint* params)
{
	if (!params)
		return;

	switch (pname)
	{
	case GL_MAX_TEXTURES_SGIS:
	case GL_MAX_ACTIVE_TEXTURES_ARB:
	case GL_MAX_TEXTURE_IMAGE_UNITS_ARB:
	case GL_MAX_TEXTURE_COORDS_ARB:
		*params = (GLint)QD3D12_MaxTextureUnits;
		break;

	case GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
	case GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
	case GL_MAX_PROGRAM_PARAMETERS_ARB:
	case GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
		*params = (GLint)QD3D12_ARB_MAX_PROGRAM_PARAMETERS;
		break;

	case GL_MAX_VERTEX_ATTRIBS_ARB:
	case GL_MAX_PROGRAM_ATTRIBS_ARB:
	case GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
		*params = (GLint)QD3D12_ARB_MAX_VERTEX_ATTRIBS;
		break;

	case GL_MAX_PROGRAM_INSTRUCTIONS_ARB:
	case GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
		*params = 1024;
		break;

	case GL_MAX_PROGRAM_TEMPORARIES_ARB:
	case GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
		*params = 64;
		break;

	case GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
	case GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
		*params = 1;
		break;

	case GL_MAX_PROGRAM_MATRICES_ARB:
		*params = 8;
		break;

	case GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB:
		*params = 4;
		break;

	case GL_VERTEX_PROGRAM_BINDING_ARB:
		*params = (GLint)QD3D12ARB_GetBoundVertexProgram();
		break;

	case GL_FRAGMENT_PROGRAM_BINDING_ARB:
		*params = (GLint)QD3D12ARB_GetBoundFragmentProgram();
		break;

	case GL_PROGRAM_ERROR_POSITION_ARB:
		*params = QD3D12ARB_GetProgramErrorPosition();
		break;

	case GL_VIEWPORT:
		params[0] = g_gl.viewportX;
		params[1] = g_gl.viewportY;
		params[2] = (GLint)g_gl.viewportW;
		params[3] = (GLint)g_gl.viewportH;
		break;

	case GL_DRAW_BUFFER:
		*params = (GLint)g_gl.drawBuffer;
		break;

	case GL_READ_BUFFER:
		*params = (GLint)g_gl.readBuffer;
		break;

	case GL_FOG_MODE:
		*params = (GLint)g_gl.fogMode;
		break;
	case GL_FOG_HINT:
		*params = (GLint)g_gl.fogHint;
		break;

	case GL_SELECTED_TEXTURE_SGIS:
		*params = (GLint)(GL_TEXTURE0_SGIS + g_gl.activeTextureUnit);
		break;

	case GL_ACTIVE_TEXTURE_ARB:
		*params = (GLint)(GL_TEXTURE0_ARB + g_gl.activeTextureUnit);
		break;

	case GL_CLIENT_ACTIVE_TEXTURE_ARB:
		*params = (GLint)(GL_TEXTURE0_ARB + g_gl.clientActiveTextureUnit);
		break;

#ifdef GL_ACTIVE_STENCIL_FACE_EXT
	case GL_ACTIVE_STENCIL_FACE_EXT:
		*params = (GLint)g_gl.activeStencilFace;
		break;
#endif

	case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
		*params = 4;
		break;

	case GL_COMPRESSED_TEXTURE_FORMATS_ARB:
		params[0] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		params[1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		params[2] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		params[3] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;

	case GL_MAX_TEXTURE_SIZE:
		*params = 4096;
		break;

	case GL_CURRENT_COLOR:
		params[0] = (GLint)g_gl.curColor[0];
		params[1] = (GLint)g_gl.curColor[1];
		params[2] = (GLint)g_gl.curColor[2];
		params[3] = (GLint)g_gl.curColor[3];
		break;

	default:
		*params = 0;
		break;
	}
}

void APIENTRY glGetFloatv(GLenum pname, GLfloat* params)
{
	if (!params)
		return;

	switch (pname)
	{
	case GL_FOG_DENSITY:
		params[0] = g_gl.fogDensity;
		break;
	case GL_FOG_START:
		params[0] = g_gl.fogStart;
		break;
	case GL_FOG_END:
		params[0] = g_gl.fogEnd;
		break;
	case GL_FOG_COLOR:
		params[0] = g_gl.fogColor[0];
		params[1] = g_gl.fogColor[1];
		params[2] = g_gl.fogColor[2];
		params[3] = g_gl.fogColor[3];
		break;
#ifdef GL_DEPTH_BOUNDS_EXT
	case GL_DEPTH_BOUNDS_EXT:
		params[0] = (GLfloat)g_gl.depthBoundsMin;
		params[1] = (GLfloat)g_gl.depthBoundsMax;
		break;
#endif
	case GL_MODELVIEW_MATRIX:
		memcpy(params, g_gl.modelStack.back().m, sizeof(GLfloat) * 16);
		break;
	case GL_PROJECTION_MATRIX:
		memcpy(params, g_gl.projStack.back().m, sizeof(GLfloat) * 16);
		break;
	case GL_CURRENT_COLOR:
		params[0] = g_gl.curColor[0];
		params[1] = g_gl.curColor[1];
		params[2] = g_gl.curColor[2];
		params[3] = g_gl.curColor[3];
		break;
	default:
		memset(params, 0, sizeof(GLfloat) * 16);
		break;
	}
}

void APIENTRY glGetDoublev(GLenum pname, GLdouble* params)
{
	if (!params)
		return;

	switch (pname)
	{
	case GL_MODELVIEW_MATRIX:
		for (int i = 0; i < 16; ++i)
			params[i] = (GLdouble)g_gl.modelStack.back().m[i];
		break;

	case GL_PROJECTION_MATRIX:
		for (int i = 0; i < 16; ++i)
			params[i] = (GLdouble)g_gl.projStack.back().m[i];
		break;

	case GL_CURRENT_COLOR:
		params[0] = (GLdouble)g_gl.curColor[0];
		params[1] = (GLdouble)g_gl.curColor[1];
		params[2] = (GLdouble)g_gl.curColor[2];
		params[3] = (GLdouble)g_gl.curColor[3];
		break;

	default:
		for (int i = 0; i < 16; ++i)
			params[i] = 0.0;
		break;
	}
}

void APIENTRY glFrustum(GLdouble left, GLdouble right,
	GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	auto Quantize = [](float v, float steps) -> float
		{
			return floorf(v * steps + 0.5f) / steps;
		};

	const float l = (float)left;
	const float r = (float)right;
	const float b = (float)bottom;
	const float t = (float)top;
	const float n = (float)zNear;
	const float fz = (float)zFar;

	const float invW = 1.0f / (r - l);
	const float invH = 1.0f / (t - b);
	const float invD = 1.0f / (fz - n);

	float xScale = (2.0f * n) * invW;
	float yScale = (2.0f * n) * invH;
	float xCenter = (r + l) * invW;
	float yCenter = (t + b) * invH;
	float zScale = -(fz + n) * invD;
	float zTrans = -(2.0f * fz * n) * invD;

	//
	// Intentionally degrade precision a bit to feel less "perfect OpenGL".
	// Tune these to taste.
	//
	// Lower numbers = chunkier / more old-school distortion feel.
	//
	const float scaleQuant = 256.0f;
	const float centerQuant = 128.0f;
	const float depthQuant = 1024.0f;

	xScale = Quantize(xScale, scaleQuant);
	yScale = Quantize(yScale, scaleQuant);
	xCenter = Quantize(xCenter, centerQuant);
	yCenter = Quantize(yCenter, centerQuant);

	// Optional: make depth a little less precise / less "modern perfect".
	zScale = Quantize(zScale, depthQuant);
	zTrans = Quantize(zTrans, depthQuant);

	Mat4 proj{};
	proj.m[0] = xScale;
	proj.m[5] = yScale;
	proj.m[8] = xCenter;
	proj.m[9] = yCenter;
	proj.m[10] = zScale;
	proj.m[11] = -1.0f;
	proj.m[14] = zTrans;

	auto& topMat = QD3D12_CurrentMatrixStack().back();
	topMat = Mat4::Multiply(topMat, proj);
}

void APIENTRY glDepthFunc(GLenum func)
{
	g_gl.depthFunc = func;
}

void APIENTRY glColor4fv(const GLfloat* v)
{
	if (!v)
		return;

	g_gl.curColor[0] = v[0];
	g_gl.curColor[1] = v[1];
	g_gl.curColor[2] = v[2];
	g_gl.curColor[3] = v[3];
}

void APIENTRY glTexParameterf(GLenum, GLenum pname, GLfloat param)
{
	GLenum value = (GLenum)param;
	GLuint bound = g_gl.boundTexture[g_gl.activeTextureUnit];

	if (bound == 0 || g_gl.textures.empty())
	{
		switch (pname)
		{
		case GL_TEXTURE_MIN_FILTER: g_gl.defaultMinFilter = value; break;
		case GL_TEXTURE_MAG_FILTER: g_gl.defaultMagFilter = value; break;
		case GL_TEXTURE_WRAP_S:     g_gl.defaultWrapS = value; break;
		case GL_TEXTURE_WRAP_T:     g_gl.defaultWrapT = value; break;
		default: break;
		}
		return;
	}

	auto it = g_gl.textures.find(bound);
	if (it == g_gl.textures.end())
		return;

	TextureResource& tex = it->second;
	switch (pname)
	{
	case GL_TEXTURE_MIN_FILTER: tex.minFilter = value; break;
	case GL_TEXTURE_MAG_FILTER: tex.magFilter = value; break;
	case GL_TEXTURE_WRAP_S:     tex.wrapS = value; break;
	case GL_TEXTURE_WRAP_T:     tex.wrapT = value; break;
	default: break;
	}
}

void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	glTexParameterf(target, pname, (GLfloat)param);
}

void APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
	if (!params)
		return;

	glTexParameteri(target, pname, params[0]);
}

static bool QD3D12_IsValidTexEnvMode(GLenum mode)
{
	switch (mode)
	{
	case GL_MODULATE:
	case GL_REPLACE:
	case GL_BLEND:
#ifdef GL_ADD
	case GL_ADD:
#endif
#ifdef GL_COMBINE_ARB
	case GL_COMBINE_ARB:
#endif
		return true;
	default:
		return false;
	}
}

static bool QD3D12_IsValidTexCombineMode(GLenum mode)
{
	switch (mode)
	{
	case GL_REPLACE:
	case GL_MODULATE:
#ifdef GL_ADD
	case GL_ADD:
#endif
#ifdef GL_ADD_SIGNED_ARB
	case GL_ADD_SIGNED_ARB:
#endif
#ifdef GL_INTERPOLATE_ARB
	case GL_INTERPOLATE_ARB:
#endif
		return true;
	default:
		return false;
	}
}

static bool QD3D12_IsValidTexCombineSource(GLenum source)
{
	switch (source)
	{
	case GL_TEXTURE:
#ifdef GL_PRIMARY_COLOR_ARB
	case GL_PRIMARY_COLOR_ARB:
#endif
#ifdef GL_PREVIOUS_ARB
	case GL_PREVIOUS_ARB:
#endif
#ifdef GL_CONSTANT_ARB
	case GL_CONSTANT_ARB:
#endif
		return true;
	default:
		return false;
	}
}

static bool QD3D12_IsValidTexCombineOperand(GLenum operand)
{
	switch (operand)
	{
	case GL_SRC_COLOR:
	case GL_ONE_MINUS_SRC_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
		return true;
	default:
		return false;
	}
}

void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	if (target != GL_TEXTURE_ENV)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	const UINT unit = ClampValue<UINT>(g_gl.activeTextureUnit, 0, QD3D12_MaxTextureUnits - 1);

	switch (pname)
	{
	case GL_TEXTURE_ENV_MODE:
	{
		GLenum mode = (GLenum)param;
		if (!QD3D12_IsValidTexEnvMode(mode))
			mode = GL_MODULATE;
		g_gl.texEnvMode[unit] = mode;
		break;
	}

#ifdef GL_COMBINE_RGB_ARB
	case GL_COMBINE_RGB_ARB:
	{
		GLenum mode = (GLenum)param;
		if (!QD3D12_IsValidTexCombineMode(mode))
			mode = GL_MODULATE;
		g_gl.texCombineRGB[unit] = mode;
		break;
	}
#endif

#ifdef GL_COMBINE_ALPHA_ARB
	case GL_COMBINE_ALPHA_ARB:
	{
		GLenum mode = (GLenum)param;
		if (!QD3D12_IsValidTexCombineMode(mode))
			mode = GL_MODULATE;
		g_gl.texCombineAlpha[unit] = mode;
		break;
	}
#endif

#ifdef GL_SOURCE0_RGB_ARB
	case GL_SOURCE0_RGB_ARB:
	{
		GLenum source = (GLenum)param;
		if (!QD3D12_IsValidTexCombineSource(source))
			source = GL_TEXTURE;
		g_gl.texSource0RGB[unit] = source;
		break;
	}
#endif
#ifdef GL_SOURCE1_RGB_ARB
	case GL_SOURCE1_RGB_ARB:
	{
		GLenum source = (GLenum)param;
		if (!QD3D12_IsValidTexCombineSource(source))
			source = GL_PREVIOUS_ARB;
		g_gl.texSource1RGB[unit] = source;
		break;
	}
#endif
#ifdef GL_SOURCE0_ALPHA_ARB
	case GL_SOURCE0_ALPHA_ARB:
	{
		GLenum source = (GLenum)param;
		if (!QD3D12_IsValidTexCombineSource(source))
			source = GL_TEXTURE;
		g_gl.texSource0Alpha[unit] = source;
		break;
	}
#endif
#ifdef GL_SOURCE1_ALPHA_ARB
	case GL_SOURCE1_ALPHA_ARB:
	{
		GLenum source = (GLenum)param;
		if (!QD3D12_IsValidTexCombineSource(source))
			source = GL_PREVIOUS_ARB;
		g_gl.texSource1Alpha[unit] = source;
		break;
	}
#endif

#ifdef GL_OPERAND0_RGB_ARB
	case GL_OPERAND0_RGB_ARB:
	{
		GLenum operand = (GLenum)param;
		if (!QD3D12_IsValidTexCombineOperand(operand))
			operand = GL_SRC_COLOR;
		g_gl.texOperand0RGB[unit] = operand;
		break;
	}
#endif
#ifdef GL_OPERAND1_RGB_ARB
	case GL_OPERAND1_RGB_ARB:
	{
		GLenum operand = (GLenum)param;
		if (!QD3D12_IsValidTexCombineOperand(operand))
			operand = GL_SRC_COLOR;
		g_gl.texOperand1RGB[unit] = operand;
		break;
	}
#endif
#ifdef GL_OPERAND0_ALPHA_ARB
	case GL_OPERAND0_ALPHA_ARB:
	{
		GLenum operand = (GLenum)param;
		if (!QD3D12_IsValidTexCombineOperand(operand))
			operand = GL_SRC_ALPHA;
		g_gl.texOperand0Alpha[unit] = operand;
		break;
	}
#endif
#ifdef GL_OPERAND1_ALPHA_ARB
	case GL_OPERAND1_ALPHA_ARB:
	{
		GLenum operand = (GLenum)param;
		if (!QD3D12_IsValidTexCombineOperand(operand))
			operand = GL_SRC_ALPHA;
		g_gl.texOperand1Alpha[unit] = operand;
		break;
	}
#endif

#ifdef GL_RGB_SCALE_ARB
	case GL_RGB_SCALE_ARB:
		g_gl.texRGBScale[unit] = ClampValue<GLfloat>(param, 1.0f, 4.0f);
		break;
#endif

#ifdef GL_ALPHA_SCALE
	case GL_ALPHA_SCALE:
		g_gl.texAlphaScale[unit] = ClampValue<GLfloat>(param, 1.0f, 4.0f);
		break;
#endif

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		break;
	}
}

void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	glTexEnvf(target, pname, (GLfloat)param);
}

void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat* params)
{
	if (!params)
		return;

	if (target != GL_TEXTURE_ENV)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	const UINT unit = ClampValue<UINT>(g_gl.activeTextureUnit, 0, QD3D12_MaxTextureUnits - 1);
	if (pname == GL_TEXTURE_ENV_COLOR)
	{
		g_gl.texEnvColor[unit][0] = params[0];
		g_gl.texEnvColor[unit][1] = params[1];
		g_gl.texEnvColor[unit][2] = params[2];
		g_gl.texEnvColor[unit][3] = params[3];
		return;
	}

	glTexEnvf(target, pname, params[0]);
}

void APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint* params)
{
	if (!params)
		return;

	if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_COLOR)
	{
		GLfloat c[4] = { (GLfloat)params[0], (GLfloat)params[1], (GLfloat)params[2], (GLfloat)params[3] };
		glTexEnvfv(target, pname, c);
		return;
	}

	glTexEnvi(target, pname, params[0]);
}

void APIENTRY glTexImage2D(GLenum, GLint level, GLint internalFormat,
	GLsizei width, GLsizei height, GLint, GLenum format, GLenum type, const GLvoid* pixels)
{
	auto it = g_gl.textures.find(g_gl.boundTexture[g_gl.activeTextureUnit]);
	if (it == g_gl.textures.end())
		return;

	if (level > 0)
		return;

	TextureResource& tex = it->second;

	tex.width = width;
	tex.height = height;
	tex.format = (format != 0) ? format : (GLenum)internalFormat;
	tex.compressed = false;
	tex.compressedInternalFormat = 0;
	tex.compressedBlockBytes = 0;
	tex.compressedImageSize = 0;
	tex.forceOpaqueAlpha = false;
	tex.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	const int bpp = BytesPerPixel(tex.format, type);
	tex.sysmem.resize((size_t)width * (size_t)height * (size_t)bpp);
	if (pixels)
		memcpy(tex.sysmem.data(), pixels, tex.sysmem.size());
	else
		memset(tex.sysmem.data(), 0, tex.sysmem.size());

	EnsureTextureResource(tex);
	tex.gpuValid = false;
}

void APIENTRY glCompressedTexImage2DARB(GLenum target, GLint level, GLenum internalFormat,
	GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data)
{
	target = QD3D12_MapCompatTextureTarget(target);
	if (target != GL_TEXTURE_2D)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (level < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	auto it = g_gl.textures.find(g_gl.boundTexture[g_gl.activeTextureUnit]);
	if (it == g_gl.textures.end())
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	if (level > 0)
		return;

	if (width <= 0 || height <= 0 || border != 0 || imageSize < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	if (!QD3D12_IsCompressedTextureFormat(internalFormat))
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	const UINT expectedSize = QD3D12_CompressedTextureImageSize(width, height, internalFormat);
	if ((UINT)imageSize < expectedSize)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	TextureResource& tex = it->second;
	tex.width = width;
	tex.height = height;
	tex.format = internalFormat;
	tex.compressed = true;
	tex.compressedInternalFormat = internalFormat;
	tex.compressedBlockBytes = QD3D12_CompressedTextureBlockBytes(internalFormat);
	tex.compressedImageSize = expectedSize;
	tex.forceOpaqueAlpha = (internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
	tex.dxgiFormat = QD3D12_MapCompressedTextureFormat(internalFormat);

	tex.sysmem.resize(expectedSize);
	if (data && expectedSize > 0)
		memcpy(tex.sysmem.data(), data, expectedSize);
	else if (expectedSize > 0)
		memset(tex.sysmem.data(), 0, expectedSize);

	EnsureTextureResource(tex);
	tex.gpuValid = false;
}

void APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalFormat,
	GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data)
{
	glCompressedTexImage2DARB(target, level, internalFormat, width, height, border, imageSize, data);
}

void APIENTRY glCompressedTexImage2DEXT(GLenum target, GLint level, GLenum internalFormat,
	GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data)
{
	glCompressedTexImage2DARB(target, level, internalFormat, width, height, border, imageSize, data);
}

void APIENTRY glCompressedTexSubImage2DARB(GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	GLenum format, GLsizei imageSize, const GLvoid* data)
{
	target = QD3D12_MapCompatTextureTarget(target);
	if (target != GL_TEXTURE_2D)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (level < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	auto it = g_gl.textures.find(g_gl.boundTexture[g_gl.activeTextureUnit]);
	if (it == g_gl.textures.end())
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	if (level > 0)
		return;

	if (xoffset < 0 || yoffset < 0 || width <= 0 || height <= 0 || imageSize < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	TextureResource& tex = it->second;
	if (!tex.compressed || tex.width <= 0 || tex.height <= 0)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	if (format != tex.compressedInternalFormat || !QD3D12_IsCompressedTextureFormat(format))
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (xoffset + width > tex.width || yoffset + height > tex.height)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	const bool touchesRightEdge = (xoffset + width) == tex.width;
	const bool touchesBottomEdge = (yoffset + height) == tex.height;
	if ((xoffset & 3) != 0 || (yoffset & 3) != 0 ||
		(!touchesRightEdge && (width & 3) != 0) ||
		(!touchesBottomEdge && (height & 3) != 0))
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	const UINT blockBytes = QD3D12_CompressedTextureBlockBytes(format);
	const UINT fullBlocksWide = QD3D12_CompressedTextureBlocksWide(tex.width);
	const UINT subBlocksWide = QD3D12_CompressedTextureBlocksWide(width);
	const UINT subBlocksHigh = QD3D12_CompressedTextureBlocksHigh(height);
	const size_t srcRowBytes = (size_t)subBlocksWide * (size_t)blockBytes;
	const size_t expectedSize = srcRowBytes * (size_t)subBlocksHigh;
	if ((size_t)imageSize < expectedSize)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	const size_t fullSize = (size_t)QD3D12_CompressedTextureImageSize(tex.width, tex.height, tex.compressedInternalFormat);
	if (tex.sysmem.size() != fullSize)
		tex.sysmem.resize(fullSize, 0);

	const uint8_t* src = static_cast<const uint8_t*>(data);
	if (!src && expectedSize > 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	const UINT dstBlockX = (UINT)xoffset / 4u;
	const UINT dstBlockY = (UINT)yoffset / 4u;
	for (UINT row = 0; row < subBlocksHigh; ++row)
	{
		const size_t dstOff = ((size_t)(dstBlockY + row) * (size_t)fullBlocksWide + (size_t)dstBlockX) * (size_t)blockBytes;
		const size_t srcOff = (size_t)row * srcRowBytes;
		memcpy(tex.sysmem.data() + dstOff, src + srcOff, srcRowBytes);
	}

	tex.compressedImageSize = (UINT)fullSize;
	tex.gpuValid = false;
}

void APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	GLenum format, GLsizei imageSize, const GLvoid* data)
{
	glCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void APIENTRY glCompressedTexSubImage2DEXT(GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	GLenum format, GLsizei imageSize, const GLvoid* data)
{
	glCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void APIENTRY glGetCompressedTexImageARB(GLenum target, GLint level, GLvoid* img)
{
	target = QD3D12_MapCompatTextureTarget(target);
	if (target != GL_TEXTURE_2D)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (level > 0 || !img)
		return;

	auto it = g_gl.textures.find(g_gl.boundTexture[g_gl.activeTextureUnit]);
	if (it == g_gl.textures.end() || !it->second.compressed)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	TextureResource& tex = it->second;
	if (!tex.sysmem.empty())
		memcpy(img, tex.sysmem.data(), tex.sysmem.size());
}


void APIENTRY glTexSubImage2D(GLenum, GLint level, GLint xoffset, GLint yoffset,
	GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
	auto it = g_gl.textures.find(g_gl.boundTexture[g_gl.activeTextureUnit]);
	if (it == g_gl.textures.end() || !pixels)
		return;

	if (level > 0)
		return;

	TextureResource& tex = it->second;
	if (tex.width <= 0 || tex.height <= 0)
		return;

	if (tex.compressed)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	const int bpp = BytesPerPixel(format, type);
	if (tex.sysmem.empty())
		tex.sysmem.resize((size_t)tex.width * (size_t)tex.height * (size_t)bpp);

	const uint8_t* src = (const uint8_t*)pixels;
	for (int row = 0; row < height; ++row)
	{
		size_t dstOff = ((size_t)(yoffset + row) * (size_t)tex.width + (size_t)xoffset) * (size_t)bpp;
		size_t srcOff = (size_t)row * (size_t)width * (size_t)bpp;
		memcpy(tex.sysmem.data() + dstOff, src + srcOff, (size_t)width * (size_t)bpp);
	}

	tex.gpuValid = false;
}


void APIENTRY glCopyTexImage2D(GLenum, GLint, GLenum internalFormat,
	GLint x, GLint y, GLsizei width, GLsizei height, GLint)
{
	auto it = g_gl.textures.find(g_gl.boundTexture[g_gl.activeTextureUnit]);
	if (it == g_gl.textures.end() || width <= 0 || height <= 0)
		return;

	std::vector<uint8_t> rgba;
	if (!QD3D12_ReadFramebufferRegionRGBA8(x, y, width, height, rgba) || rgba.empty())
		return;

	TextureResource& tex = it->second;
	tex.width = width;
	tex.height = height;
	tex.format = QD3D12_NormalizeCopyTextureFormat(internalFormat);
	tex.compressed = false;
	tex.compressedInternalFormat = 0;
	tex.compressedBlockBytes = 0;
	tex.compressedImageSize = 0;
	tex.forceOpaqueAlpha = false;
	tex.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	QD3D12_PackRGBA8ToTextureFormat(rgba.data(), width * height, tex.format, tex.sysmem);

	EnsureTextureResource(tex);
	tex.gpuValid = false;
}

void APIENTRY glCopyTexSubImage2D(GLenum, GLint,
	GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	auto it = g_gl.textures.find(g_gl.boundTexture[g_gl.activeTextureUnit]);
	if (it == g_gl.textures.end() || width <= 0 || height <= 0)
		return;

	TextureResource& tex = it->second;
	if (tex.width <= 0 || tex.height <= 0)
		return;

	if (tex.compressed)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	if (xoffset < 0 || yoffset < 0 || xoffset + width > tex.width || yoffset + height > tex.height)
		return;

	std::vector<uint8_t> rgba;
	if (!QD3D12_ReadFramebufferRegionRGBA8(x, y, width, height, rgba) || rgba.empty())
		return;

	QD3D12_CopyRGBARegionIntoTexture(tex, xoffset, yoffset, width, height, rgba.data());
}

void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data)
{
	if (!data)
		return;

	const size_t dstBytes = (size_t)width * (size_t)height * (size_t)BytesPerPixel(format, type);
	if (dstBytes == 0)
		return;

	if (type != GL_UNSIGNED_BYTE)
	{
		memset(data, 0, dstBytes);
		return;
	}

	std::vector<uint8_t> rgba;
	if (!QD3D12_ReadFramebufferRegionRGBA8(x, y, width, height, rgba) || rgba.empty())
	{
		memset(data, 0, dstBytes);
		return;
	}

	std::vector<uint8_t> packed;
	QD3D12_PackRGBA8ToTextureFormat(rgba.data(), width * height, format, packed);
	if (packed.size() < dstBytes)
	{
		memset(data, 0, dstBytes);
		memcpy(data, packed.data(), packed.size());
		return;
	}

	memcpy(data, packed.data(), dstBytes);
}

void APIENTRY glDrawBuffer(GLenum mode)
{
	g_gl.drawBuffer = mode;
}

void APIENTRY glReadBuffer(GLenum mode)
{
	g_gl.readBuffer = mode;
}

void APIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params)
{
	if (!params)
		return;

	target = QD3D12_MapCompatTextureTarget(target);
	if (target != GL_TEXTURE_2D)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (level != 0)
	{
		*params = 0;
		return;
	}

	auto it = g_gl.textures.find(g_gl.boundTexture[g_gl.activeTextureUnit]);
	if (it == g_gl.textures.end())
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	const TextureResource& tex = it->second;
	switch (pname)
	{
	case GL_TEXTURE_WIDTH:
		*params = tex.width;
		break;
	case GL_TEXTURE_HEIGHT:
		*params = tex.height;
		break;
	case GL_TEXTURE_INTERNAL_FORMAT:
		*params = (GLint)tex.format;
		break;
	case GL_TEXTURE_COMPRESSED_ARB:
		*params = tex.compressed ? GL_TRUE : GL_FALSE;
		break;
	case GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB:
		*params = tex.compressed ? (GLint)tex.compressedImageSize : 0;
		break;
	default:
		*params = 0;
		break;
	}
}

void APIENTRY glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params)
{
	if (!params)
		return;

	GLint value = 0;
	glGetTexLevelParameteriv(target, level, pname, &value);
	*params = (GLfloat)value;
}

// ============================================================
// SECTION 12: optional convenience for Quake code
// ============================================================

void QD3D12_DrawArrays(GLenum mode, const GLVertex* verts, size_t count)
{
	FlushImmediate(mode, verts, count);
}

void QD3D12_ReleaseWindowSizeResources(QD3D12Window& w)
{
	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
	{
		w.sceneColorBuffers[i].Reset();
		w.backBuffers[i].Reset();
		w.normalBuffers[i].Reset();
		w.positionBuffers[i].Reset();
		w.velocityBuffers[i].Reset();

		w.sceneColorState[i] = D3D12_RESOURCE_STATE_COMMON;
		w.backBufferState[i] = D3D12_RESOURCE_STATE_COMMON;
		w.normalBufferState[i] = D3D12_RESOURCE_STATE_COMMON;
		w.positionBufferState[i] = D3D12_RESOURCE_STATE_COMMON;
		w.velocityBufferState[i] = D3D12_RESOURCE_STATE_COMMON;
	}

	w.depthBuffer.Reset();
	w.depthState = D3D12_RESOURCE_STATE_COMMON;
	w.nativeDepthBuffer.Reset();
	w.nativeDepthState = D3D12_RESOURCE_STATE_COMMON;

	w.rtvHeap.Reset();
	w.dsvHeap.Reset();
}
void QD3D12_Resize()
{
	if (!g_currentWindow)
		return;

	RECT rc{};
	if (!GetClientRect(g_currentWindow->hwnd, &rc))
		return;

	UINT width = (UINT)(rc.right - rc.left);
	UINT height = (UINT)(rc.bottom - rc.top);

	if (width == 0 || height == 0)
		return;

	if (g_currentWindow->width == width && g_currentWindow->height == height)
		return;

	if (g_gl.frameOpen || !g_gl.queuedBatches.empty())
		QD3D12_SubmitOpenFrameNoPresentAndWait();

	QD3D12_WaitForGPU();

	g_gl.queuedBatches.clear();

#if defined(QD3D12_ENABLE_FFX)
	QD3D12_DestroyFfxUpscaleContext();
#endif

	QD3D12_ReleaseWindowSizeResources(*g_currentWindow);

	QD3D12_CHECK(g_currentWindow->swapChain->ResizeBuffers(
		QD3D12_FrameCount,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
	));

	g_currentWindow->width = width;
	g_currentWindow->height = height;
	QD3D12_SelectRenderResolution(*g_currentWindow, width, height);
	g_currentWindow->frameIndex = g_currentWindow->swapChain->GetCurrentBackBufferIndex();

	g_gl.viewportW = (GLsizei)width;
	g_gl.viewportH = (GLsizei)height;
	g_gl.motionHistoryReset = true;

	QD3D12_CreateRTVsForWindow(*g_currentWindow);
	QD3D12_CreateDSVForWindow(*g_currentWindow);
	g_psoCache.clear();
	g_arbPsoCache.clear();
	g_gl.framePhase = QD3D12_FRAME_LOW_RES;
	g_gl.sceneResolvedThisFrame = false;
	g_gl.raytracedLightingReadyThisFrame = false;
	QD3D12_UpdateViewportState();
}

static void QD3D12_ReconfigureCurrentWindowForUpscalerChange()
{
	if (!g_currentWindow || !g_currentWindow->swapChain || !g_gl.device)
		return;

	const UINT width = g_currentWindow->width;
	const UINT height = g_currentWindow->height;

	if (width == 0 || height == 0)
		return;

	if (g_gl.frameOpen || !g_gl.queuedBatches.empty())
		QD3D12_SubmitOpenFrameNoPresentAndWait();

	QD3D12_WaitForGPU();

	g_gl.queuedBatches.clear();

#if defined(QD3D12_ENABLE_FFX)
	QD3D12_DestroyFfxUpscaleContext();
#endif

	QD3D12_ReleaseWindowSizeResources(*g_currentWindow);

	QD3D12_CHECK(g_currentWindow->swapChain->ResizeBuffers(
		QD3D12_FrameCount,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
	));

	QD3D12_SelectRenderResolution(*g_currentWindow, width, height);
	g_currentWindow->frameIndex = g_currentWindow->swapChain->GetCurrentBackBufferIndex();

	g_gl.viewportW = (GLsizei)width;
	g_gl.viewportH = (GLsizei)height;
	g_gl.motionHistoryReset = true;

	QD3D12_CreateRTVsForWindow(*g_currentWindow);
	QD3D12_CreateDSVForWindow(*g_currentWindow);
	g_psoCache.clear();
	g_arbPsoCache.clear();
	g_gl.framePhase = QD3D12_FRAME_LOW_RES;
	g_gl.sceneResolvedThisFrame = false;
	g_gl.raytracedLightingReadyThisFrame = false;
	QD3D12_UpdateViewportState();
}

GLenum APIENTRY glGetError(void) {
	GLenum e = g_gl.lastError;
	if (e != GL_NO_ERROR)
	{
		g_gl.lastError = GL_NO_ERROR;
		return e;
	}

	return QD3D12ARB_ConsumeError();
}

void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
	g_gl.scissorX = x;
	g_gl.scissorY = y;
	g_gl.scissorW = width;
	g_gl.scissorH = height;
}

void APIENTRY glClearDepth(GLclampd depth) {
	g_gl.clearDepthValue = depth;
}

void APIENTRY glClipPlane(GLenum plane, const GLdouble* equation) {
	if (plane == GL_CLIP_PLANE0 && equation) {
		memcpy(g_gl.clipPlane0, equation, sizeof(g_gl.clipPlane0));
	}
}

void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units) {
	g_gl.polygonOffsetFactor = factor;
	g_gl.polygonOffsetUnits = units;
}

void APIENTRY glTexCoord2fv(const GLfloat* v) {
	if (!v) return;
	glTexCoord2f(v[0], v[1]);
}

void APIENTRY glColor4ubv(const GLubyte* v) {
	if (!v) return;
	g_gl.curColor[0] = v[0] / 255.0f;
	g_gl.curColor[1] = v[1] / 255.0f;
	g_gl.curColor[2] = v[2] / 255.0f;
	g_gl.curColor[3] = v[3] / 255.0f;
}

void APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
	if (!params) return;
	glTexParameterf(target, pname, params[0]);
}

static bool QD3D12_IsValidStencilFace(GLenum face)
{
	return face == GL_FRONT || face == GL_BACK || face == GL_FRONT_AND_BACK;
}

static void QD3D12_SetStencilMaskForFace(GLenum face, GLuint mask)
{
	if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
		g_gl.stencilFrontMask = mask;
	if (face == GL_BACK || face == GL_FRONT_AND_BACK)
		g_gl.stencilBackMask = mask;
}

static void QD3D12_SetStencilFuncForFace(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
	{
		g_gl.stencilFrontFunc = func;
		g_gl.stencilFrontRef = ref;
		g_gl.stencilFrontFuncMask = mask;
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK)
	{
		g_gl.stencilBackFunc = func;
		g_gl.stencilBackRef = ref;
		g_gl.stencilBackFuncMask = mask;
	}
}

static void QD3D12_SetStencilOpForFace(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
	{
		g_gl.stencilFrontSFail = sfail;
		g_gl.stencilFrontDPFail = dpfail;
		g_gl.stencilFrontDPPass = dppass;
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK)
	{
		g_gl.stencilBackSFail = sfail;
		g_gl.stencilBackDPFail = dpfail;
		g_gl.stencilBackDPPass = dppass;
	}
}

void APIENTRY glStencilMask(GLuint mask)
{
	g_gl.stencilMask = mask;
	QD3D12_SetStencilMaskForFace(GL_FRONT_AND_BACK, mask);
}

void APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask)
{
	if (!QD3D12_IsValidStencilFace(face))
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}
	QD3D12_SetStencilMaskForFace(face, mask);
}

void APIENTRY glClearStencil(GLint s)
{
	g_gl.clearStencilValue = s;
}

void APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	g_gl.stencilFunc = func;
	g_gl.stencilRef = ref;
	g_gl.stencilFuncMask = mask;
	QD3D12_SetStencilFuncForFace(GL_FRONT_AND_BACK, func, ref, mask);
}

void APIENTRY glStencilFuncSeparateATI(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask)
{
	QD3D12_SetStencilFuncForFace(GL_FRONT, frontfunc, ref, mask);
	QD3D12_SetStencilFuncForFace(GL_BACK, backfunc, ref, mask);
}

void APIENTRY glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)
{
	g_gl.stencilSFail = sfail;
	g_gl.stencilDPFail = dpfail;
	g_gl.stencilDPPass = dppass;

	const GLenum face = g_gl.stencilTwoSide ? g_gl.activeStencilFace : GL_FRONT_AND_BACK;
	QD3D12_SetStencilOpForFace(face, sfail, dpfail, dppass);
}

void APIENTRY glStencilOpSeparateATI(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	if (!QD3D12_IsValidStencilFace(face))
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}
	QD3D12_SetStencilOpForFace(face, sfail, dpfail, dppass);
}

void APIENTRY glActiveStencilFaceEXT(GLenum face)
{
	if (face != GL_FRONT && face != GL_BACK)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}
	g_gl.activeStencilFace = face;
}

void APIENTRY glDepthBoundsEXT(GLclampd zmin, GLclampd zmax)
{
	g_gl.depthBoundsMin = ClampValue<GLclampd>(zmin, 0.0, 1.0);
	g_gl.depthBoundsMax = ClampValue<GLclampd>(zmax, 0.0, 1.0);
	if (g_gl.depthBoundsMax < g_gl.depthBoundsMin)
		std::swap(g_gl.depthBoundsMin, g_gl.depthBoundsMax);
}

void APIENTRY glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
	g_gl.colorMaskR = r;
	g_gl.colorMaskG = g;
	g_gl.colorMaskB = b;
	g_gl.colorMaskA = a;
}

void APIENTRY glClientActiveTextureARB(GLenum texture) {
	if (texture >= GL_TEXTURE0_ARB)
		g_gl.clientActiveTextureUnit = ClampValue<GLuint>((GLuint)(texture - GL_TEXTURE0_ARB), 0, QD3D12_MaxTextureUnits - 1);
	else
		g_gl.clientActiveTextureUnit = 0;
}

void APIENTRY glLockArraysEXT(GLint first, GLsizei count) {
	(void)first;
	(void)count;
}

void APIENTRY glUnlockArraysEXT(void) {
}

void APIENTRY glNormalPointer(GLenum type, GLsizei stride, const void* pointer)
{
	g_gl.normalArray.size = 3;
	g_gl.normalArray.type = type;
	g_gl.normalArray.stride = stride;
	g_gl.normalArray.ptr = QD3D12_ResolveArrayPointer(pointer);
}

void APIENTRY glEnableClientState(GLenum array)
{
	switch (array)
	{
	case GL_VERTEX_ARRAY:
		g_gl.vertexArray.enabled = true;
		break;
	case GL_NORMAL_ARRAY:
		g_gl.normalArray.enabled = true;
		break;
	case GL_COLOR_ARRAY:
		g_gl.colorArray.enabled = true;
		break;
	case GL_TEXTURE_COORD_ARRAY:
		g_gl.texCoordArray[g_gl.clientActiveTextureUnit].enabled = true;
		break;
	default:
		break;
	}
}

void APIENTRY glDisableClientState(GLenum array)
{
	switch (array)
	{
	case GL_VERTEX_ARRAY:
		g_gl.vertexArray.enabled = false;
		break;
	case GL_NORMAL_ARRAY:
		g_gl.normalArray.enabled = false;
		break;
	case GL_COLOR_ARRAY:
		g_gl.colorArray.enabled = false;
		break;
	case GL_TEXTURE_COORD_ARRAY:
		g_gl.texCoordArray[g_gl.clientActiveTextureUnit].enabled = false;
		break;
	default:
		break;
	}
}

void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr)
{
	g_gl.vertexArray.size = size;
	g_gl.vertexArray.type = type;
	g_gl.vertexArray.stride = stride;
	g_gl.vertexArray.ptr = QD3D12_ResolveArrayPointer(ptr);
}

void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr)
{
	g_gl.colorArray.size = size;
	g_gl.colorArray.type = type;
	g_gl.colorArray.stride = stride;
	g_gl.colorArray.ptr = QD3D12_ResolveArrayPointer(ptr);
}

void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr)
{
	auto& tc = g_gl.texCoordArray[g_gl.clientActiveTextureUnit];
	tc.size = size;
	tc.type = type;
	tc.stride = stride;
	tc.ptr = QD3D12_ResolveArrayPointer(ptr);
}

void APIENTRY glArrayElement(GLint i)
{
	GLVertex& v = g_gl.immediateVerts.Push();
	QD3D12_FetchArrayVertex(i, v);
}

void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	if (count <= 0)
		return;

	const void* resolvedIndices = QD3D12_ResolveElementPointer(indices, type, count);
	if (!resolvedIndices)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	g_gl.immediateVerts.Clear();

	switch (type)
	{
	case GL_UNSIGNED_INT:
	{
		const GLuint* idx = static_cast<const GLuint*>(resolvedIndices);
		for (GLsizei i = 0; i < count; ++i)
		{
			GLVertex& v = g_gl.immediateVerts.Push();
			QD3D12_FetchArrayVertex(static_cast<GLint>(idx[i]), v);
		}
		break;
	}

	case GL_UNSIGNED_SHORT:
	{
		const GLushort* idx = static_cast<const GLushort*>(resolvedIndices);
		for (GLsizei i = 0; i < count; ++i)
		{
			GLVertex& v = g_gl.immediateVerts.Push();
			QD3D12_FetchArrayVertex(static_cast<GLint>(idx[i]), v);
		}
		break;
	}

	case GL_UNSIGNED_BYTE:
	{
		const GLubyte* idx = static_cast<const GLubyte*>(resolvedIndices);
		for (GLsizei i = 0; i < count; ++i)
		{
			GLVertex& v = g_gl.immediateVerts.Push();
			QD3D12_FetchArrayVertex(static_cast<GLint>(idx[i]), v);
		}
		break;
	}

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (!g_gl.colorArray.enabled)
	{
		for (int i = 0; i < g_gl.immediateVerts.Size(); i++)
		{
			g_gl.immediateVerts.Data()[i].r = g_gl.curColor[0];
			g_gl.immediateVerts.Data()[i].g = g_gl.curColor[1];
			g_gl.immediateVerts.Data()[i].b = g_gl.curColor[2];
			g_gl.immediateVerts.Data()[i].a = g_gl.curColor[3];
		}
	}
	

	FlushImmediate(mode, g_gl.immediateVerts.Data(), g_gl.immediateVerts.Size());
}



static inline int QD3D12_CompatAttribTexUnit(GLuint index)
{
	switch (index)
	{
	case 8:  return 0; // Doom 3 base texture coordinate
	default: return -1;
	}
}

static inline bool QD3D12_CompatAttribIsNormal(GLuint index)
{
	return (index == 2 || index == 9);
}

static inline bool QD3D12_CompatAttribIsTangent(GLuint index)
{
	return index == 10;
}

static inline bool QD3D12_CompatAttribIsBitangent(GLuint index)
{
	return index == 11;
}

void APIENTRY glVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)
{
	(void)normalized;

	if (index == 0)
	{
		glVertexPointer(size, type, stride, pointer);
		return;
	}

	if (index == 3)
	{
		glColorPointer(size, type, stride, pointer);
		return;
	}

	if (QD3D12_CompatAttribIsNormal(index))
	{
		glNormalPointer(type, stride, pointer);
		return;
	}

	if (QD3D12_CompatAttribIsTangent(index))
	{
		g_gl.tangentArray.size = size;
		g_gl.tangentArray.type = type;
		g_gl.tangentArray.stride = stride;
		g_gl.tangentArray.ptr = QD3D12_ResolveArrayPointer(pointer);
		return;
	}

	if (QD3D12_CompatAttribIsBitangent(index))
	{
		g_gl.bitangentArray.size = size;
		g_gl.bitangentArray.type = type;
		g_gl.bitangentArray.stride = stride;
		g_gl.bitangentArray.ptr = QD3D12_ResolveArrayPointer(pointer);
		return;
	}

	const int texUnit = QD3D12_CompatAttribTexUnit(index);
	if (texUnit >= 0 && texUnit < (int)QD3D12_MaxTextureUnits)
	{
		const GLuint oldUnit = g_gl.clientActiveTextureUnit;
		glClientActiveTextureARB(GL_TEXTURE0_ARB + (GLenum)texUnit);
		glTexCoordPointer(size, type, stride, pointer);
		glClientActiveTextureARB(GL_TEXTURE0_ARB + oldUnit);
		return;
	}

	// Best-effort compatibility only: unsupported generic attributes are intentionally ignored.
}

void APIENTRY glEnableVertexAttribArrayARB(GLuint index)
{
	if (index == 0)
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		return;
	}

	if (index == 3)
	{
		glEnableClientState(GL_COLOR_ARRAY);
		return;
	}

	if (QD3D12_CompatAttribIsNormal(index))
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		return;
	}

	if (QD3D12_CompatAttribIsTangent(index))
	{
		g_gl.tangentArray.enabled = true;
		return;
	}

	if (QD3D12_CompatAttribIsBitangent(index))
	{
		g_gl.bitangentArray.enabled = true;
		return;
	}

	const int texUnit = QD3D12_CompatAttribTexUnit(index);
	if (texUnit >= 0 && texUnit < (int)QD3D12_MaxTextureUnits)
	{
		const GLuint oldUnit = g_gl.clientActiveTextureUnit;
		glClientActiveTextureARB(GL_TEXTURE0_ARB + (GLenum)texUnit);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTextureARB(GL_TEXTURE0_ARB + oldUnit);
		return;
	}
}

void APIENTRY glDisableVertexAttribArrayARB(GLuint index)
{
	if (index == 0)
	{
		glDisableClientState(GL_VERTEX_ARRAY);
		return;
	}

	if (index == 3)
	{
		glDisableClientState(GL_COLOR_ARRAY);
		return;
	}

	if (QD3D12_CompatAttribIsNormal(index))
	{
		glDisableClientState(GL_NORMAL_ARRAY);
		return;
	}

	if (QD3D12_CompatAttribIsTangent(index))
	{
		g_gl.tangentArray.enabled = false;
		return;
	}

	if (QD3D12_CompatAttribIsBitangent(index))
	{
		g_gl.bitangentArray.enabled = false;
		return;
	}

	const int texUnit = QD3D12_CompatAttribTexUnit(index);
	if (texUnit >= 0 && texUnit < (int)QD3D12_MaxTextureUnits)
	{
		const GLuint oldUnit = g_gl.clientActiveTextureUnit;
		glClientActiveTextureARB(GL_TEXTURE0_ARB + (GLenum)texUnit);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTextureARB(GL_TEXTURE0_ARB + oldUnit);
		return;
	}
}


static inline GLenum QD3D12_MapCompatBufferTarget(GLenum target)
{
	switch (target)
	{
	case GL_ARRAY_BUFFER_ARB:         return GL_ARRAY_BUFFER;
	case GL_ELEMENT_ARRAY_BUFFER_ARB: return GL_ELEMENT_ARRAY_BUFFER;
	default:                          return target;
	}
}

static inline GLenum QD3D12_MapCompatTextureTarget(GLenum target)
{
	switch (target)
	{
	case GL_TEXTURE_3D:
	case GL_TEXTURE_CUBE_MAP_EXT:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT:
		return GL_TEXTURE_2D;
	default:
		return target;
	}
}


// ============================================================
// Minimal WGL pbuffer compatibility
// Supports the rvTexRenderTarget flow:
// choose pixel format -> create pbuffer/DC/context -> make current -> render -> bind tex image.
// ============================================================

#ifndef WGL_ARB_pbuffer
DECLARE_HANDLE(HPBUFFERARB);
#endif

#ifndef WGL_DRAW_TO_PBUFFER_ARB
#define WGL_DRAW_TO_PBUFFER_ARB 0x202D
#endif
#ifndef WGL_PBUFFER_WIDTH_ARB
#define WGL_PBUFFER_WIDTH_ARB 0x2034
#endif
#ifndef WGL_PBUFFER_HEIGHT_ARB
#define WGL_PBUFFER_HEIGHT_ARB 0x2035
#endif
#ifndef WGL_PBUFFER_LOST_ARB
#define WGL_PBUFFER_LOST_ARB 0x2036
#endif
#ifndef WGL_BIND_TO_TEXTURE_RGB_ARB
#define WGL_BIND_TO_TEXTURE_RGB_ARB 0x2070
#endif
#ifndef WGL_BIND_TO_TEXTURE_RGBA_ARB
#define WGL_BIND_TO_TEXTURE_RGBA_ARB 0x2071
#endif
#ifndef WGL_TEXTURE_FORMAT_ARB
#define WGL_TEXTURE_FORMAT_ARB 0x2072
#endif
#ifndef WGL_TEXTURE_TARGET_ARB
#define WGL_TEXTURE_TARGET_ARB 0x2073
#endif
#ifndef WGL_MIPMAP_TEXTURE_ARB
#define WGL_MIPMAP_TEXTURE_ARB 0x2074
#endif
#ifndef WGL_TEXTURE_RGB_ARB
#define WGL_TEXTURE_RGB_ARB 0x2075
#endif
#ifndef WGL_TEXTURE_RGBA_ARB
#define WGL_TEXTURE_RGBA_ARB 0x2076
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_ARB
#define WGL_TEXTURE_CUBE_MAP_ARB 0x2078
#endif
#ifndef WGL_TEXTURE_2D_ARB
#define WGL_TEXTURE_2D_ARB 0x207A
#endif
#ifndef WGL_MIPMAP_LEVEL_ARB
#define WGL_MIPMAP_LEVEL_ARB 0x207B
#endif
#ifndef WGL_CUBE_MAP_FACE_ARB
#define WGL_CUBE_MAP_FACE_ARB 0x207C
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x207D
#endif
#ifndef WGL_FRONT_LEFT_ARB
#define WGL_FRONT_LEFT_ARB 0x2083
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV 0x20A0
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV 0x20A1
#endif
#ifndef WGL_TEXTURE_RECTANGLE_NV
#define WGL_TEXTURE_RECTANGLE_NV 0x20A2
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV 0x20B1
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV 0x20B2
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV 0x20B3
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV 0x20B4
#endif
#ifndef WGL_FLOAT_COMPONENTS_NV
#define WGL_FLOAT_COMPONENTS_NV 0x20B0
#endif

struct QD3D12GLContext
{
	HDC dc = nullptr;
	QD3D12Window* window = nullptr;
};

struct QD3D12Pbuffer
{
	QD3D12Window window;
	HDC dc = nullptr;
	int width = 0;
	int height = 0;
	int textureTarget = WGL_TEXTURE_2D_ARB;
	int textureFormat = WGL_TEXTURE_RGBA_ARB;
	int cubeFace = WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
	GLuint boundTexture = 0;
};

static std::unordered_map<HDC, QD3D12Window*> g_qd3d12DcToWindow;
static std::unordered_map<HGLRC, QD3D12GLContext*> g_qd3d12Contexts;
static std::unordered_map<HPBUFFERARB, QD3D12Pbuffer*> g_qd3d12Pbuffers;

static void QD3D12_RegisterWindowDC(QD3D12Window* w)
{
	if (w && w->hdc)
		g_qd3d12DcToWindow[w->hdc] = w;
}

static void QD3D12_UnregisterWindowDC(HDC dc)
{
	if (dc)
		g_qd3d12DcToWindow.erase(dc);
}

static QD3D12Window* QD3D12_FindWindowForDC(HDC dc)
{
	if (!dc)
		return nullptr;

	auto it = g_qd3d12DcToWindow.find(dc);
	if (it != g_qd3d12DcToWindow.end())
		return it->second;

	for (auto& kv : g_windows)
	{
		if (kv.second.hdc == dc)
		{
			g_qd3d12DcToWindow[dc] = &kv.second;
			return &kv.second;
		}
	}

	return nullptr;
}

static HGLRC QD3D12_CreateGLContextHandle(HDC dc, QD3D12Window* window)
{
	QD3D12GLContext* ctx = new QD3D12GLContext();
	ctx->dc = dc;
	ctx->window = window ? window : QD3D12_FindWindowForDC(dc);

	HGLRC handle = reinterpret_cast<HGLRC>(ctx);
	g_qd3d12Contexts[handle] = ctx;
	return handle;
}

static QD3D12GLContext* QD3D12_GetGLContext(HGLRC rc)
{
	auto it = g_qd3d12Contexts.find(rc);
	if (it == g_qd3d12Contexts.end())
		return nullptr;
	return it->second;
}

static QD3D12Pbuffer* QD3D12_GetPbuffer(HPBUFFERARB handle)
{
	auto it = g_qd3d12Pbuffers.find(handle);
	if (it == g_qd3d12Pbuffers.end())
		return nullptr;
	return it->second;
}

static int QD3D12_FindAttribInt(const int* attribs, int attrib, int defaultValue)
{
	if (!attribs)
		return defaultValue;

	for (const int* p = attribs; p[0] != 0; p += 2)
	{
		if (p[0] == attrib)
			return p[1];
	}

	return defaultValue;
}

static bool QD3D12_InitPbufferWindow(QD3D12Pbuffer& pb, int width, int height)
{
	if (!g_gl.device || !g_gl.srvHeap || !g_gl.cmdList || !g_gl.fence)
		return false;

	QD3D12Window& w = pb.window;
	w = QD3D12Window{};
	w.isPbuffer = true;
	w.hwnd = nullptr;
	w.hdc = pb.dc;
	w.ownsHdc = false;
	w.width = (UINT)max(1, width);
	w.height = (UINT)max(1, height);
	w.renderWidth = w.width;
	w.renderHeight = w.height;
	w.frameIndex = 0;

	QD3D12_CreateSurfaceFrameResources(w);
	QD3D12_CreateRTVsForWindow(w);
	QD3D12_CreateDSVForWindow(w);
	QD3D12_CreateUploadRingForWindow(w);

	w.viewport.TopLeftX = 0.0f;
	w.viewport.TopLeftY = 0.0f;
	w.viewport.Width = (float)w.width;
	w.viewport.Height = (float)w.height;
	w.viewport.MinDepth = 0.0f;
	w.viewport.MaxDepth = 1.0f;
	w.scissor.left = 0;
	w.scissor.top = 0;
	w.scissor.right = (LONG)w.width;
	w.scissor.bottom = (LONG)w.height;

	QD3D12_RegisterWindowDC(&w);
	return true;
}

static void QD3D12_DestroyPbufferWindow(QD3D12Pbuffer& pb)
{
	QD3D12Window& w = pb.window;

	if ((g_gl.frameOpen && g_gl.frameOwner == &w) || g_currentWindow == &w)
		QD3D12_SubmitOpenFrameNoPresentAndWait();

	if (g_currentWindow == &w)
		g_currentWindow = nullptr;

	QD3D12_WaitForGPU();
	QD3D12_UnregisterWindowDC(w.hdc);
	QD3D12_DestroyUploadRingForWindow(w);
	QD3D12_ReleaseWindowSizeResources(w);
	for (UINT i = 0; i < QD3D12_FrameCount; ++i)
		w.frames[i].cmdAlloc.Reset();
}

static bool QD3D12_CopyPbufferToBoundTexture(QD3D12Pbuffer& pb)
{
	GLuint textureName = g_gl.boundTexture[g_gl.activeTextureUnit];
	if (textureName == 0)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return false;
	}

	auto it = g_gl.textures.find(textureName);
	if (it == g_gl.textures.end())
	{
		TextureResource tex{};
		tex.glId = textureName;
		tex.srvIndex = UINT_MAX;
		tex.minFilter = g_gl.defaultMinFilter;
		tex.magFilter = g_gl.defaultMagFilter;
		tex.wrapS = g_gl.defaultWrapS;
		tex.wrapT = g_gl.defaultWrapT;
		it = g_gl.textures.emplace(textureName, std::move(tex)).first;
	}

	TextureResource& tex = it->second;
	tex.width = pb.width;
	tex.height = pb.height;
	tex.format = GL_RGBA;
	tex.compressed = false;
	tex.compressedInternalFormat = 0;
	tex.compressedBlockBytes = 0;
	tex.compressedImageSize = 0;
	tex.forceOpaqueAlpha = false;
	tex.dxgiFormat = QD3D12_SceneColorFormat;
	tex.sysmem.clear();

	QD3D12Window* savedWindow = g_currentWindow;
	HDC savedDC = g_qd3d12CurrentDC;
	HGLRC savedRC = g_qd3d12CurrentRC;

	g_currentWindow = &pb.window;
	g_qd3d12CurrentDC = pb.dc;

	if (g_gl.frameOpen || !g_gl.queuedBatches.empty())
		QD3D12_SubmitOpenFrameNoPresentAndWait();

	QD3D12Window& w = pb.window;
	w.frameIndex = 0;
	QD3D12_WaitForFrame(w.frameIndex);
	QD3D12_CHECK(w.frames[w.frameIndex].cmdAlloc->Reset());
	QD3D12_CHECK(g_gl.cmdList->Reset(w.frames[w.frameIndex].cmdAlloc.Get(), nullptr));

	EnsureTextureResource(tex);
	if (!tex.texture)
	{
		g_currentWindow = savedWindow;
		g_qd3d12CurrentDC = savedDC;
		g_qd3d12CurrentRC = savedRC;
		return false;
	}

	ID3D12Resource* src = w.backBuffers[w.frameIndex].Get();
	ID3D12Resource* dst = tex.texture.Get();
	if (!src || !dst)
	{
		g_currentWindow = savedWindow;
		g_qd3d12CurrentDC = savedDC;
		g_qd3d12CurrentRC = savedRC;
		return false;
	}

	QD3D12_TransitionResource(g_gl.cmdList.Get(), src, w.backBufferState[w.frameIndex], D3D12_RESOURCE_STATE_COPY_SOURCE);
	QD3D12_TransitionResource(g_gl.cmdList.Get(), dst, tex.state, D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_TEXTURE_COPY_LOCATION srcLoc{};
	srcLoc.pResource = src;
	srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	srcLoc.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION dstLoc{};
	dstLoc.pResource = dst;
	dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLoc.SubresourceIndex = 0;

	g_gl.cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

	QD3D12_TransitionResource(g_gl.cmdList.Get(), dst, tex.state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	QD3D12_TransitionResource(g_gl.cmdList.Get(), src, w.backBufferState[w.frameIndex], D3D12_RESOURCE_STATE_PRESENT);

	QD3D12_CHECK(g_gl.cmdList->Close());
	ID3D12CommandList* lists[] = { g_gl.cmdList.Get() };
	g_gl.queue->ExecuteCommandLists(1, lists);

	const UINT64 signalValue = g_gl.nextFenceValue++;
	QD3D12_CHECK(g_gl.queue->Signal(g_gl.fence.Get(), signalValue));
	w.frames[w.frameIndex].fenceValue = signalValue;

	if (g_gl.fence->GetCompletedValue() < signalValue)
	{
		QD3D12_CHECK(g_gl.fence->SetEventOnCompletion(signalValue, g_gl.fenceEvent));
		WaitForSingleObject(g_gl.fenceEvent, INFINITE);
	}

	tex.gpuValid = true;
	pb.boundTexture = textureName;

	g_currentWindow = savedWindow;
	g_qd3d12CurrentDC = savedDC;
	g_qd3d12CurrentRC = savedRC;
	return true;
}

//HDC WINAPI wglGetCurrentDC(void)
//{
//	return g_qd3d12CurrentDC;
//}

BOOL WINAPI wglDeleteContext(HGLRC hglrc)
{
	if (!hglrc)
		return TRUE;

	if (g_qd3d12CurrentRC == hglrc)
		g_qd3d12CurrentRC = nullptr;

	auto it = g_qd3d12Contexts.find(hglrc);
	if (it != g_qd3d12Contexts.end())
	{
		delete it->second;
		g_qd3d12Contexts.erase(it);
	}

	return TRUE;
}

BOOL WINAPI wglShareLists(HGLRC, HGLRC)
{
	// The shim keeps GL objects in global CPU/D3D12 state, so contexts already share resources.
	return TRUE;
}

const char* WINAPI wglGetExtensionsStringARB(HDC)
{
	return "WGL_ARB_pixel_format WGL_ARB_pbuffer WGL_ARB_render_texture WGL_NV_render_texture_rectangle WGL_NV_float_buffer";
}

const char* WINAPI wglGetExtensionsStringEXT(void)
{
	return wglGetExtensionsStringARB(g_qd3d12CurrentDC);
}

BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
	if (!hdc && !hglrc)
	{
		if (g_gl.frameOpen || !g_gl.queuedBatches.empty())
			QD3D12_SubmitOpenFrameNoPresentAndWait();
		g_qd3d12CurrentDC = nullptr;
		g_qd3d12CurrentRC = nullptr;
		g_currentWindow = nullptr;
		return TRUE;
	}

	QD3D12Window* window = nullptr;
	if (QD3D12GLContext* ctx = QD3D12_GetGLContext(hglrc))
		window = ctx->window;

	if (!window)
		window = QD3D12_FindWindowForDC(hdc);

	if (!window)
		return FALSE;

	g_qd3d12CurrentDC = hdc;
	g_qd3d12CurrentRC = hglrc;
	QD3D12_SetCurrentWindow(window);
	return TRUE;
}

BOOL WINAPI wglChoosePixelFormatARB(HDC, const int*, const FLOAT*, UINT nMaxFormats, int* piFormats, UINT* nNumFormats)
{
	if (nNumFormats)
		*nNumFormats = (nMaxFormats > 0 && piFormats) ? 1u : 0u;

	if (piFormats && nMaxFormats > 0)
		piFormats[0] = 1;

	return TRUE;
}

BOOL WINAPI wglGetPixelFormatAttribivARB(HDC, int, int, UINT nAttributes, const int* piAttributes, int* piValues)
{
	if (!piAttributes || !piValues)
		return FALSE;

	for (UINT i = 0; i < nAttributes; ++i)
	{
		switch (piAttributes[i])
		{
		case WGL_DRAW_TO_PBUFFER_ARB:
		case WGL_BIND_TO_TEXTURE_RGB_ARB:
		case WGL_BIND_TO_TEXTURE_RGBA_ARB:
		case WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV:
		case WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV:
		case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV:
		case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV:
		case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV:
		case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV:
		case WGL_FLOAT_COMPONENTS_NV:
			piValues[i] = 1;
			break;
		default:
			piValues[i] = 0;
			break;
		}
	}

	return TRUE;
}

HPBUFFERARB WINAPI wglCreatePbufferARB(HDC, int, int iWidth, int iHeight, const int* piAttribList)
{
	if (iWidth <= 0 || iHeight <= 0)
		return nullptr;

	QD3D12Pbuffer* pb = new QD3D12Pbuffer();
	pb->width = iWidth;
	pb->height = iHeight;
	pb->textureTarget = QD3D12_FindAttribInt(piAttribList, WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB);
	pb->textureFormat = QD3D12_FindAttribInt(piAttribList, WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB);
	pb->cubeFace = WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
	pb->dc = reinterpret_cast<HDC>(pb);

	if (!QD3D12_InitPbufferWindow(*pb, iWidth, iHeight))
	{
		delete pb;
		return nullptr;
	}

	HPBUFFERARB handle = reinterpret_cast<HPBUFFERARB>(pb);
	g_qd3d12Pbuffers[handle] = pb;
	return handle;
}

HDC WINAPI wglGetPbufferDCARB(HPBUFFERARB hPbuffer)
{
	QD3D12Pbuffer* pb = QD3D12_GetPbuffer(hPbuffer);
	return pb ? pb->dc : nullptr;
}

int WINAPI wglReleasePbufferDCARB(HPBUFFERARB hPbuffer, HDC hdc)
{
	QD3D12Pbuffer* pb = QD3D12_GetPbuffer(hPbuffer);
	return (pb && pb->dc == hdc) ? 1 : 0;
}

BOOL WINAPI wglDestroyPbufferARB(HPBUFFERARB hPbuffer)
{
	QD3D12Pbuffer* pb = QD3D12_GetPbuffer(hPbuffer);
	if (!pb)
		return FALSE;

	for (auto it = g_qd3d12Contexts.begin(); it != g_qd3d12Contexts.end(); )
	{
		if (it->second && it->second->window == &pb->window)
		{
			if (g_qd3d12CurrentRC == it->first)
				g_qd3d12CurrentRC = nullptr;
			delete it->second;
			it = g_qd3d12Contexts.erase(it);
		}
		else
		{
			++it;
		}
	}

	QD3D12_DestroyPbufferWindow(*pb);
	g_qd3d12Pbuffers.erase(hPbuffer);
	delete pb;
	return TRUE;
}

BOOL WINAPI wglQueryPbufferARB(HPBUFFERARB hPbuffer, int iAttribute, int* piValue)
{
	if (!piValue)
		return FALSE;

	QD3D12Pbuffer* pb = QD3D12_GetPbuffer(hPbuffer);
	if (!pb)
		return FALSE;

	switch (iAttribute)
	{
	case WGL_PBUFFER_WIDTH_ARB:
		*piValue = pb->width;
		return TRUE;
	case WGL_PBUFFER_HEIGHT_ARB:
		*piValue = pb->height;
		return TRUE;
	case WGL_PBUFFER_LOST_ARB:
		*piValue = 0;
		return TRUE;
	case WGL_TEXTURE_TARGET_ARB:
		*piValue = pb->textureTarget;
		return TRUE;
	case WGL_TEXTURE_FORMAT_ARB:
		*piValue = pb->textureFormat;
		return TRUE;
	case WGL_CUBE_MAP_FACE_ARB:
		*piValue = pb->cubeFace;
		return TRUE;
	default:
		*piValue = 0;
		return TRUE;
	}
}

BOOL WINAPI wglSetPbufferAttribARB(HPBUFFERARB hPbuffer, const int* piAttribList)
{
	QD3D12Pbuffer* pb = QD3D12_GetPbuffer(hPbuffer);
	if (!pb)
		return FALSE;

	if (!piAttribList)
		return TRUE;

	for (const int* p = piAttribList; p[0] != 0; p += 2)
	{
		if (p[0] == WGL_CUBE_MAP_FACE_ARB)
			pb->cubeFace = p[1];
		else if (p[0] == WGL_MIPMAP_LEVEL_ARB)
			; // ignored; this shim only backs level 0
	}

	return TRUE;
}

BOOL WINAPI wglBindTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
	if (iBuffer != WGL_FRONT_LEFT_ARB)
		return FALSE;

	QD3D12Pbuffer* pb = QD3D12_GetPbuffer(hPbuffer);
	if (!pb)
		return FALSE;

	return QD3D12_CopyPbufferToBoundTexture(*pb) ? TRUE : FALSE;
}

BOOL WINAPI wglReleaseTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
	if (iBuffer != WGL_FRONT_LEFT_ARB)
		return FALSE;

	QD3D12Pbuffer* pb = QD3D12_GetPbuffer(hPbuffer);
	if (!pb)
		return FALSE;

	pb->boundTexture = 0;
	return TRUE;
}

BOOL WINAPI wglSwapBuffers(HDC hdc)
{
	QD3D12_SwapBuffers(hdc);
	return TRUE;
}

void APIENTRY glFlush(void)
{
	glFinish();
}

void APIENTRY glFrontFace(GLenum mode)
{
	if (mode != GL_CW && mode != GL_CCW)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}
	g_gl.frontFace = mode;
}

void APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat* params)
{
	(void)face;
	(void)pname;
	(void)params;
}

void APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	(void)face;
	(void)pname;
	(void)param;
}

void APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat* params)
{
	(void)light;
	(void)pname;
	(void)params;
}

void APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param)
{
	(void)light;
	(void)pname;
	(void)param;
}

void APIENTRY glActiveTexture(GLenum texture)
{
	glActiveTextureARB(texture);
}

void APIENTRY glClientActiveTexture(GLenum texture)
{
	glClientActiveTextureARB(texture);
}

void APIENTRY glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t)
{
	glMultiTexCoord2fARB(texture, s, t);
}

void APIENTRY glGenBuffersARB(GLsizei n, GLuint* buffers)
{
	glGenBuffers(n, buffers);
}

void APIENTRY glDeleteBuffersARB(GLsizei n, const GLuint* buffers)
{
	glDeleteBuffers(n, buffers);
}

void APIENTRY glBindBufferARB(GLenum target, GLuint buffer)
{
	glBindBuffer(QD3D12_MapCompatBufferTarget(target), buffer);
}

void APIENTRY glBufferDataARB(GLenum target, GLsizeiptrARB size, const void* data, GLenum usage)
{
	glBufferData(QD3D12_MapCompatBufferTarget(target), (GLsizeiptr)size, data, usage);
}

void APIENTRY glBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void* data)
{
	glBufferSubData(QD3D12_MapCompatBufferTarget(target), (GLintptr)offset, (GLsizeiptr)size, data);
}

void* APIENTRY glMapBufferARB(GLenum target, GLenum access)
{
	target = QD3D12_MapCompatBufferTarget(target);

	GLuint bound = 0;
	switch (target)
	{
	case GL_ARRAY_BUFFER:
		bound = g_gl.boundArrayBuffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		bound = g_gl.boundElementArrayBuffer;
		break;
	default:
		g_gl.lastError = GL_INVALID_ENUM;
		return nullptr;
	}

	GLBufferObject* bo = QD3D12_GetBuffer(bound);
	if (!bo)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return nullptr;
	}

	GLbitfield flags = 0;
	switch (access)
	{
	case GL_READ_ONLY:
		flags = GL_MAP_READ_BIT;
		break;
	case GL_WRITE_ONLY:
		flags = GL_MAP_WRITE_BIT;
		break;
	case GL_READ_WRITE:
	default:
		flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		break;
	}

	return glMapBufferRange(target, 0, (GLsizeiptr)bo->data.size(), flags);
}

GLboolean APIENTRY glUnmapBufferARB(GLenum target)
{
	return glUnmapBuffer(QD3D12_MapCompatBufferTarget(target));
}

void APIENTRY glTexImage3D(GLenum target, GLint level, GLint internalFormat,
	GLsizei width, GLsizei height, GLsizei depth, GLint border,
	GLenum format, GLenum type, const GLvoid* pixels)
{
	if (depth <= 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	glTexImage2D(QD3D12_MapCompatTextureTarget(target), level, internalFormat,
		width, height, border, format, type, pixels);
}

void APIENTRY glTexSubImage3D(GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLint zoffset,
	GLsizei width, GLsizei height, GLsizei depth,
	GLenum format, GLenum type, const GLvoid* pixels)
{
	(void)zoffset;

	if (depth <= 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	glTexSubImage2D(QD3D12_MapCompatTextureTarget(target), level,
		xoffset, yoffset, width, height, format, type, pixels);
}

void APIENTRY glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width,
	GLenum format, GLenum type, const GLvoid* data)
{
	(void)target;
	(void)internalformat;
	(void)width;
	(void)format;
	(void)type;
	(void)data;
}

PROC WINAPI qd3d12_wglGetProcAddress(LPCSTR name) {
	if (!name) return nullptr;

	struct ProcMap { const char* name; PROC proc; };
	static const ProcMap table[] = {
		{ "glActiveTextureARB",         (PROC)glActiveTextureARB },
		{ "glClientActiveTextureARB",   (PROC)glClientActiveTextureARB },
		{ "glMultiTexCoord2fARB",       (PROC)glMultiTexCoord2fARB },
		{ "glSelectTextureSGIS",        (PROC)glSelectTextureSGIS },
		{ "glMTexCoord2fSGIS",          (PROC)glMTexCoord2fSGIS },
		{ "glLockArraysEXT",            (PROC)glLockArraysEXT },
		{ "glUnlockArraysEXT",          (PROC)glUnlockArraysEXT },
		{ "glVertexAttribPointerARB",   (PROC)glVertexAttribPointerARB },
		{ "glEnableVertexAttribArrayARB", (PROC)glEnableVertexAttribArrayARB },
		{ "glDisableVertexAttribArrayARB", (PROC)glDisableVertexAttribArrayARB },
		{ "glGenProgramsARB",           (PROC)glGenProgramsARB },
		{ "glDeleteProgramsARB",        (PROC)glDeleteProgramsARB },
		{ "glBindProgramARB",           (PROC)glBindProgramARB },
		{ "glProgramStringARB",         (PROC)glProgramStringARB },
		{ "glProgramEnvParameter4fARB", (PROC)glProgramEnvParameter4fARB },
		{ "glProgramEnvParameter4fvARB", (PROC)glProgramEnvParameter4fvARB },
		{ "glProgramLocalParameter4fARB", (PROC)glProgramLocalParameter4fARB },
		{ "glProgramLocalParameter4fvARB", (PROC)glProgramLocalParameter4fvARB },
		{ "glGetProgramEnvParameterfvARB", (PROC)glGetProgramEnvParameterfvARB },
		{ "glGetProgramLocalParameterfvARB", (PROC)glGetProgramLocalParameterfvARB },
		{ "glGetProgramivARB",          (PROC)glGetProgramivARB },
		{ "glGetProgramStringARB",      (PROC)glGetProgramStringARB },
		{ "glIsProgramARB",             (PROC)glIsProgramARB },
		{ "glDepthBoundsEXT",           (PROC)glDepthBoundsEXT },
		{ "glActiveStencilFaceEXT",     (PROC)glActiveStencilFaceEXT },
		{ "glStencilOpSeparateATI",     (PROC)glStencilOpSeparateATI },
		{ "glStencilFuncSeparateATI",   (PROC)glStencilFuncSeparateATI },
		{ "glStencilMaskSeparate",      (PROC)glStencilMaskSeparate },
		{ "wglSwapIntervalEXT",         (PROC)qd3d12_wglSwapIntervalEXT },
		{ "wglGetDeviceGammaRamp3DFX",  (PROC)qd3d12_wglGetDeviceGammaRamp3DFX },
		{ "wglSetDeviceGammaRamp3DFX",  (PROC)qd3d12_wglSetDeviceGammaRamp3DFX },
		{ "glBindTextureEXT",           (PROC)glBindTextureEXT },
		{ "glCompressedTexImage2DARB", (PROC)glCompressedTexImage2DARB },
		{ "glCompressedTexImage2DEXT", (PROC)glCompressedTexImage2DEXT },
		{ "glCompressedTexImage2D",    (PROC)glCompressedTexImage2D },
		{ "glCompressedTexSubImage2DARB", (PROC)glCompressedTexSubImage2DARB },
		{ "glGetCompressedTexImageARB",   (PROC)glGetCompressedTexImageARB },
		{ "glCompressedTexSubImage2DEXT", (PROC)glCompressedTexSubImage2DEXT },
		{ "glCompressedTexSubImage2D",    (PROC)glCompressedTexSubImage2D },
		{ "glGetTexLevelParameteriv", (PROC)glGetTexLevelParameteriv },
		{ "glGetTexLevelParameterfv", (PROC)glGetTexLevelParameterfv },
		{ "wglGetExtensionsStringARB",  (PROC)wglGetExtensionsStringARB },
		{ "wglGetExtensionsStringEXT",  (PROC)wglGetExtensionsStringEXT },
		{ "wglChoosePixelFormatARB",    (PROC)wglChoosePixelFormatARB },
		{ "wglGetPixelFormatAttribivARB", (PROC)wglGetPixelFormatAttribivARB },
		{ "wglCreatePbufferARB",        (PROC)wglCreatePbufferARB },
		{ "wglGetPbufferDCARB",         (PROC)wglGetPbufferDCARB },
		{ "wglReleasePbufferDCARB",     (PROC)wglReleasePbufferDCARB },
		{ "wglDestroyPbufferARB",       (PROC)wglDestroyPbufferARB },
		{ "wglQueryPbufferARB",         (PROC)wglQueryPbufferARB },
		{ "wglSetPbufferAttribARB",     (PROC)wglSetPbufferAttribARB },
		{ "wglBindTexImageARB",         (PROC)wglBindTexImageARB },
		{ "wglReleaseTexImageARB",      (PROC)wglReleaseTexImageARB },
	};

	for (size_t i = 0; i < _countof(table); ++i) {
		if (!strcmp(name, table[i].name))
			return table[i].proc;
	}
	return nullptr;
}

void APIENTRY glFogf(GLenum pname, GLfloat param)
{
	switch (pname)
	{
	case GL_FOG_MODE:
		g_gl.fogMode = (GLenum)param;
		break;
	case GL_FOG_DENSITY:
		g_gl.fogDensity = param;
		break;
	case GL_FOG_START:
		g_gl.fogStart = param;
		break;
	case GL_FOG_END:
		g_gl.fogEnd = param;
		break;
	default:
		g_gl.lastError = GL_INVALID_ENUM;
		break;
	}
}

void APIENTRY glFogi(GLenum pname, GLint param)
{
	glFogf(pname, (GLfloat)param);
}

void APIENTRY glFogfv(GLenum pname, const GLfloat* params)
{
	if (!params)
		return;

	switch (pname)
	{
	case GL_FOG_MODE:
		g_gl.fogMode = (GLenum)params[0];
		break;
	case GL_FOG_DENSITY:
		g_gl.fogDensity = params[0];
		break;
	case GL_FOG_START:
		g_gl.fogStart = params[0];
		break;
	case GL_FOG_END:
		g_gl.fogEnd = params[0];
		break;
	case GL_FOG_COLOR:
		g_gl.fogColor[0] = params[0];
		g_gl.fogColor[1] = params[1];
		g_gl.fogColor[2] = params[2];
		g_gl.fogColor[3] = params[3];
		break;
	default:
		g_gl.lastError = GL_INVALID_ENUM;
		break;
	}
}

void APIENTRY glFogiv(GLenum pname, const GLint* params)
{
	if (!params)
		return;

	switch (pname)
	{
	case GL_FOG_MODE:
		g_gl.fogMode = (GLenum)params[0];
		break;
	case GL_FOG_DENSITY:
		g_gl.fogDensity = (GLfloat)params[0];
		break;
	case GL_FOG_START:
		g_gl.fogStart = (GLfloat)params[0];
		break;
	case GL_FOG_END:
		g_gl.fogEnd = (GLfloat)params[0];
		break;
	case GL_FOG_COLOR:
		g_gl.fogColor[0] = (GLfloat)params[0];
		g_gl.fogColor[1] = (GLfloat)params[1];
		g_gl.fogColor[2] = (GLfloat)params[2];
		g_gl.fogColor[3] = (GLfloat)params[3];
		break;
	default:
		g_gl.lastError = GL_INVALID_ENUM;
		break;
	}
}

ID3D12Device* QD3D12_GetDevice(void)
{
	return g_gl.device.Get();
}

ID3D12CommandQueue* QD3D12_GetQueue(void)
{
	return g_gl.queue.Get();
}

ID3D12GraphicsCommandList* QD3D12_GetCommandList(void)
{
	return g_gl.cmdList.Get();
}

ID3D12CommandAllocator* QD3D12_GetFrameCommandAllocator(void)
{
	return g_currentWindow->frames[g_currentWindow->frameIndex].cmdAlloc.Get();
}

UINT QD3D12_GetFrameIndex(void)
{
	return g_currentWindow->frameIndex;
}

void QD3D12_WaitForGPU_External(void)
{
	if (g_gl.frameOpen || !g_gl.queuedBatches.empty())
		QD3D12_SubmitOpenFrameNoPresentAndWait();

	QD3D12_WaitForGPU();
}

ID3D12Resource* glRaytracingGetTopLevelAS(void);

static void QD3D12_TransitionResource(
	ID3D12GraphicsCommandList* cl,
	ID3D12Resource* res,
	D3D12_RESOURCE_STATES& trackedState,
	D3D12_RESOURCE_STATES newState)
{
	if (!res || trackedState == newState)
		return;

	D3D12_RESOURCE_BARRIER b{};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = res;
	b.Transition.StateBefore = trackedState;
	b.Transition.StateAfter = newState;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cl->ResourceBarrier(1, &b);

	trackedState = newState;
}

static void QD3D12_BindLowResSceneTargets(QD3D12Window& w)
{
	ID3D12GraphicsCommandList* cl = g_gl.cmdList.Get();
	if (!cl)
		return;

	QD3D12_TransitionResource(cl, w.sceneColorBuffers[w.frameIndex].Get(), w.sceneColorState[w.frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
	QD3D12_TransitionResource(cl, w.normalBuffers[w.frameIndex].Get(), w.normalBufferState[w.frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
	QD3D12_TransitionResource(cl, w.positionBuffers[w.frameIndex].Get(), w.positionBufferState[w.frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
	QD3D12_TransitionResource(cl, w.velocityBuffers[w.frameIndex].Get(), w.velocityBufferState[w.frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
	QD3D12_TransitionResource(cl, w.depthBuffer.Get(), w.depthState, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[4] =
	{
		CurrentRTV(),
		CurrentNormalRTV(),
		CurrentPositionRTV(),
		CurrentVelocityRTV()
	};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = CurrentSceneDepthDSV();
	cl->OMSetRenderTargets(4, rtvs, FALSE, &dsv);
	cl->RSSetViewports(1, &w.viewport);
	cl->RSSetScissorRects(1, &w.scissor);
}

static void QD3D12_CopySceneDepthToNative(QD3D12Window& w)
{
	ID3D12GraphicsCommandList* cl = g_gl.cmdList.Get();
	if (!cl || !w.nativeDepthBuffer)
		return;

	QD3D12_TransitionResource(cl, w.depthBuffer.Get(), w.depthState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	QD3D12_TransitionResource(cl, w.nativeDepthBuffer.Get(), w.nativeDepthState, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	D3D12_CPU_DESCRIPTOR_HANDLE dsv = CurrentNativeDepthDSV();
	D3D12_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (float)w.width;
	viewport.Height = (float)w.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor{};
	scissor.left = 0;
	scissor.top = 0;
	scissor.right = (LONG)w.width;
	scissor.bottom = (LONG)w.height;

	cl->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	if (!g_gl.postDepthCopyPSO)
		return;

	ID3D12DescriptorHeap* heaps[] = { g_gl.srvHeap.Get() };
	cl->SetDescriptorHeaps(_countof(heaps), heaps);
	cl->SetGraphicsRootSignature(g_gl.postRootSig.Get());
	cl->SetPipelineState(g_gl.postDepthCopyPSO.Get());
	cl->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
	cl->RSSetViewports(1, &viewport);
	cl->RSSetScissorRects(1, &scissor);
	cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cl->SetGraphicsRootDescriptorTable(0, w.depthSrvGpu);
	cl->DrawInstanced(3, 1, 0, 0);
}

static void QD3D12_BindNativePostUpscaleTargets(QD3D12Window& w)
{
	ID3D12GraphicsCommandList* cl = g_gl.cmdList.Get();
	if (!cl)
		return;

	QD3D12_TransitionResource(cl, w.backBuffers[w.frameIndex].Get(), w.backBufferState[w.frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = CurrentBackBufferRTV();
	if (w.nativeDepthBuffer)
	{
		QD3D12_TransitionResource(cl, w.nativeDepthBuffer.Get(), w.nativeDepthState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		D3D12_CPU_DESCRIPTOR_HANDLE dsv = CurrentNativeDepthDSV();
		cl->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
	}
	else
	{
		cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	}

	cl->RSSetViewports(1, &w.viewport);
	cl->RSSetScissorRects(1, &w.scissor);
}

static void QD3D12_BindTargetsForCurrentPhase(QD3D12Window& w)
{
	if (g_gl.framePhase == QD3D12_FRAME_NATIVE_POST_UPSCALE)
		QD3D12_BindNativePostUpscaleTargets(w);
	else
		QD3D12_BindLowResSceneTargets(w);
}

static void QD3D12_ResolveSceneToOutputAndEnterNativePhase(QD3D12Window& w)
{
	if (g_gl.sceneResolvedThisFrame)
		return;

	ID3D12GraphicsCommandList* cl = g_gl.cmdList.Get();
	if (!cl)
		return;

	const bool useLightingUpscaleInput = QD3D12_UseLightingTextureAsUpscaleInput(w);

	QD3D12_TransitionResource(cl, w.sceneColorBuffers[w.frameIndex].Get(), w.sceneColorState[w.frameIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	QD3D12_TransitionResource(cl, w.velocityBuffers[w.frameIndex].Get(), w.velocityBufferState[w.frameIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	QD3D12_TransitionResource(cl, w.depthBuffer.Get(), w.depthState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	if (useLightingUpscaleInput)
	{
		QD3D12_TransitionResource(cl, w.normalBuffers[w.frameIndex].Get(), w.normalBufferState[w.frameIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		if (g_lightingTexture && g_lightingTexture->texture)
		{
			QD3D12_TransitionResource(cl, g_lightingTexture->texture.Get(), g_lightingTextureState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}

	QD3D12_TransitionResource(cl, w.backBuffers[w.frameIndex].Get(), w.backBufferState[w.frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);

	QD3D12_RunUpscalerOrBlit(w);
	QD3D12_CopySceneDepthToNative(w);

	g_gl.sceneResolvedThisFrame = true;
	g_gl.framePhase = QD3D12_FRAME_NATIVE_POST_UPSCALE;
	QD3D12_UpdateViewportState();
	QD3D12_BindTargetsForCurrentPhase(w);
}

void glLightScene(void)
{
	const int width = (int)g_currentWindow->renderWidth;
	const int height = (int)g_currentWindow->renderHeight;

	if (width <= 0 || height <= 0)
		return;

	if (!g_gl.device || !g_gl.cmdList)
		return;

	QD3D12_EnsureFrameOpen();

	TextureResource* lightingTex = QD3D12_EnsureLightingTexture(width, height);
	if (!lightingTex || !lightingTex->texture)
		return;

	glRaytracingBuildScene();

	QD3D12_FlushQueuedBatches();

	ID3D12GraphicsCommandList* cl = g_gl.cmdList.Get();
	ID3D12Resource* sceneColor = g_currentWindow->sceneColorBuffers[g_currentWindow->frameIndex].Get();
	ID3D12Resource* sceneNormal = g_currentWindow->normalBuffers[g_currentWindow->frameIndex].Get();
	ID3D12Resource* scenePosition = g_currentWindow->positionBuffers[g_currentWindow->frameIndex].Get();
	ID3D12Resource* sceneDepth = g_currentWindow->depthBuffer.Get();
	ID3D12Resource* tlas = glRaytracingGetTopLevelAS();

	if (!sceneColor || !sceneNormal || !scenePosition || !sceneDepth || !tlas)
		return;

	QD3D12_TransitionResource(
		cl,
		sceneColor,
		g_currentWindow->sceneColorState[g_currentWindow->frameIndex],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	QD3D12_TransitionResource(
		cl,
		sceneNormal,
		g_currentWindow->normalBufferState[g_currentWindow->frameIndex],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	QD3D12_TransitionResource(
		cl,
		scenePosition,
		g_currentWindow->positionBufferState[g_currentWindow->frameIndex],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	QD3D12_TransitionResource(
		cl,
		sceneDepth,
		g_currentWindow->depthState,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// The external DXR module records and executes its own command list. Because
	// of that, the scene input transitions must be submitted on the main command
	// list before we call into glRaytracingLightingExecute.
	if (g_lightingTextureState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	{
		QD3D12_TransitionResource(
			cl,
			lightingTex->texture.Get(),
			g_lightingTextureState,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	QD3D12_ExecuteMainCommandListAndWait(*g_currentWindow);
	cl = g_gl.cmdList.Get();

	glRaytracingLightingPassDesc_t pass = {};
	pass.albedoTexture = sceneColor;
	pass.albedoFormat = QD3D12_SceneColorFormat;

	pass.normalTexture = sceneNormal;
	pass.normalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	pass.positionTexture = scenePosition;
	pass.positionFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	pass.depthTexture = sceneDepth;
	pass.depthFormat = QD3D12_DepthSrvFormat;

	pass.outputTexture = lightingTex->texture.Get();
	pass.outputFormat = lightingTex->dxgiFormat;

	pass.topLevelAS = tlas;
	pass.width = (uint32_t)width;
	pass.height = (uint32_t)height;

	if (!glRaytracingLightingExecute(&pass))
	{
		QD3D12_TransitionResource(
			cl,
			sceneColor,
			g_currentWindow->sceneColorState[g_currentWindow->frameIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		QD3D12_TransitionResource(
			cl,
			sceneNormal,
			g_currentWindow->normalBufferState[g_currentWindow->frameIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		QD3D12_TransitionResource(
			cl,
			scenePosition,
			g_currentWindow->positionBufferState[g_currentWindow->frameIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		QD3D12_TransitionResource(
			cl,
			sceneDepth,
			g_currentWindow->depthState,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		return;
	}

	g_lightingTextureState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	QD3D12_RunRayAIDenoiseIfEnabled(cl, *g_currentWindow, lightingTex->texture.Get());

	QD3D12_TransitionResource(
		cl,
		lightingTex->texture.Get(),
		g_lightingTextureState,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	g_gl.raytracedLightingReadyThisFrame = true;

	if (!QD3D12_UseLightingTextureAsUpscaleInput(*g_currentWindow))
	{
		QD3D12_TransitionResource(
			cl,
			sceneColor,
			g_currentWindow->sceneColorState[g_currentWindow->frameIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		D3D12_VIEWPORT viewport{};
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = (float)g_currentWindow->renderWidth;
		viewport.Height = (float)g_currentWindow->renderHeight;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissor{};
		scissor.left = 0;
		scissor.top = 0;
		scissor.right = (LONG)g_currentWindow->renderWidth;
		scissor.bottom = (LONG)g_currentWindow->renderHeight;

		// Non-RR path: replace the low-resolution scene color with the lit result,
		// then upscale that scene color into the backbuffer.
		QD3D12_PostFullscreenPass(
			cl,
			g_gl.postCopyPSO.Get(),
			g_lightingTexture->srvGpu,
			CurrentRTV(),
			viewport,
			scissor);
	}
	else
	{
		// DLSS Ray Reconstruction consumes the noisy ray-traced lighting texture
		// directly as the scaling input, so keep sceneColor intact as the albedo
		// guide instead of overwriting it here.
	}

	QD3D12_ResolveSceneToOutputAndEnterNativePhase(*g_currentWindow);
}

ID3D12Resource* QD3D12_GetCurrentBackBuffer()
{
	if (!g_currentWindow->swapChain)
		return nullptr;

	const UINT index = g_currentWindow->frameIndex;
	if (index >= QD3D12_FrameCount)
		return nullptr;

	return g_currentWindow->backBuffers[index].Get();
}

void APIENTRY glGeometryFlagf(GLfloat flag)
{
	g_gl.currentGeometryFlag = flag;
}

void APIENTRY glMotionObjectIdui(GLuint objectId)
{
	g_gl.currentMotionObjectId = objectId;
}

void APIENTRY glSurfaceRoughnessf(GLfloat roughness)
{
	g_gl.currentSurfaceRoughness = ClampValue<float>(roughness, 0.0f, 1.0f);
}

void APIENTRY glMaterialTypef(GLfloat materialType)
{
	g_gl.currentMaterialType = materialType;
}

void QD3D12_SetUpscalerBackend(int backend)
{
	switch (backend)
	{
	case QD3D12_UPSCALER_DLSS:
		g_gl.upscalerBackend = QD3D12_UPSCALER_DLSS;
		break;
	case QD3D12_UPSCALER_FSR:
		g_gl.upscalerBackend = QD3D12_UPSCALER_FSR;
		break;
	default:
		g_gl.upscalerBackend = QD3D12_UPSCALER_NONE;
		break;
	}

	g_gl.motionHistoryReset = true;
	QD3D12_ReconfigureCurrentWindowForUpscalerChange();
}

void QD3D12_SetUpscalerQuality(int quality)
{
	switch (quality)
	{
	case QD3D12_QUALITY_NATIVE:
	case QD3D12_QUALITY_QUALITY:
	case QD3D12_QUALITY_BALANCED:
	case QD3D12_QUALITY_PERFORMANCE:
	case QD3D12_QUALITY_ULTRA_PERFORMANCE:
		g_gl.upscalerQuality = (QD3D12UpscalerQuality)quality;
		break;
	default:
		g_gl.upscalerQuality = QD3D12_QUALITY_QUALITY;
		break;
	}

	g_gl.motionHistoryReset = true;
	QD3D12_ReconfigureCurrentWindowForUpscalerChange();
}

void QD3D12_SetUpscalerSharpness(float sharpness)
{
	g_gl.upscalerSharpness = ClampValue<float>(sharpness, 0.0f, 1.0f);
}

void QD3D12_EnableRayAIDenoise(int enabled)
{
	g_gl.enableRayAIDenoise = enabled ? true : false;
}

void QD3D12_EnableDLSSRayReconstruction(int enabled)
{
	const bool newValue = enabled ? true : false;
	if (g_gl.enableDLSSRayReconstruction == newValue)
		return;

	g_gl.enableDLSSRayReconstruction = newValue;
	g_gl.enableRayAIDenoise = false;
	g_gl.motionHistoryReset = true;
	QD3D12_ReconfigureCurrentWindowForUpscalerChange();
}

void QD3D12_EnableFSRRayRegeneration(int enabled)
{
	g_gl.enableFSRRayRegeneration = enabled ? true : false;
}

void QD3D12_ResetTemporalHistory(void)
{
	g_gl.motionHistoryReset = true;
	g_gl.prevObjectMVPs.clear();
	g_gl.currObjectMVPs.clear();
	g_gl.prevJitterX = 0.0f;
	g_gl.prevJitterY = 0.0f;
}

void QD3D12_SetProjectionJitterPixels(float jitterX, float jitterY)
{
	g_gl.jitterX = jitterX;
	g_gl.jitterY = jitterY;
}

void QD3D12_SetCameraInfo(
	const float* viewToClip,
	const float* clipToView,
	const float* clipToPrevClip,
	const float* prevClipToClip,
	const float* cameraPos,
	const float* cameraRight,
	const float* cameraUp,
	const float* cameraForward,
	float nearPlane,
	float farPlane,
	float verticalFovRadians,
	float aspectRatio)
{
	if (viewToClip)
		memcpy(g_gl.cameraState.viewToClip.m, viewToClip, sizeof(float) * 16);

	if (clipToView)
		memcpy(g_gl.cameraState.clipToView.m, clipToView, sizeof(float) * 16);

	if (clipToPrevClip)
		memcpy(g_gl.cameraState.clipToPrevClip.m, clipToPrevClip, sizeof(float) * 16);

	if (prevClipToClip)
		memcpy(g_gl.cameraState.prevClipToClip.m, prevClipToClip, sizeof(float) * 16);

	if (cameraPos)
	{
		g_gl.cameraState.cameraPos[0] = cameraPos[0];
		g_gl.cameraState.cameraPos[1] = cameraPos[1];
		g_gl.cameraState.cameraPos[2] = cameraPos[2];
	}

	if (cameraRight)
	{
		g_gl.cameraState.cameraRight[0] = cameraRight[0];
		g_gl.cameraState.cameraRight[1] = cameraRight[1];
		g_gl.cameraState.cameraRight[2] = cameraRight[2];
	}

	if (cameraUp)
	{
		g_gl.cameraState.cameraUp[0] = cameraUp[0];
		g_gl.cameraState.cameraUp[1] = cameraUp[1];
		g_gl.cameraState.cameraUp[2] = cameraUp[2];
	}

	if (cameraForward)
	{
		g_gl.cameraState.cameraForward[0] = cameraForward[0];
		g_gl.cameraState.cameraForward[1] = cameraForward[1];
		g_gl.cameraState.cameraForward[2] = cameraForward[2];
	}

	g_gl.cameraState.nearPlane = nearPlane;
	g_gl.cameraState.farPlane = farPlane;
	g_gl.cameraState.verticalFovRadians = verticalFovRadians;
	g_gl.cameraState.aspectRatio = aspectRatio;
	g_gl.cameraState.valid = true;
}

void APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{
	if (n < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	if (!buffers)
		return;

	for (GLsizei i = 0; i < n; ++i)
	{
		GLuint         id = g_gl.nextBufferId++;
		GLBufferObject bo{};
		bo.id = id;
		g_gl.buffers.emplace(id, std::move(bo));
		buffers[i] = id;
	}
}

void APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	if (n < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	if (!buffers)
		return;

	for (GLsizei i = 0; i < n; ++i)
	{
		const GLuint id = buffers[i];
		if (id == 0)
			continue;

		if (g_gl.boundArrayBuffer == id)
			g_gl.boundArrayBuffer = 0;

		if (g_gl.boundElementArrayBuffer == id)
			g_gl.boundElementArrayBuffer = 0;

		g_gl.buffers.erase(id);
	}
}

GLboolean APIENTRY glIsBuffer(GLuint buffer) { return g_gl.buffers.find(buffer) != g_gl.buffers.end() ? GL_TRUE : GL_FALSE; }

void APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
	switch (target)
	{
	case GL_ARRAY_BUFFER:
		g_gl.boundArrayBuffer = buffer;
		break;

	case GL_ELEMENT_ARRAY_BUFFER:
		g_gl.boundElementArrayBuffer = buffer;
		break;

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (buffer == 0)
		return;

	auto it = g_gl.buffers.find(buffer);
	if (it == g_gl.buffers.end())
	{
		GLBufferObject bo{};
		bo.id = buffer;
		bo.target = target;
		g_gl.buffers.emplace(buffer, std::move(bo));
	}
	else
	{
		it->second.target = target;
	}
}

void APIENTRY glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags)
{
	GLuint bound = 0;

	switch (target)
	{
	case GL_ARRAY_BUFFER:
		bound = g_gl.boundArrayBuffer;
		break;

	case GL_ELEMENT_ARRAY_BUFFER:
		bound = g_gl.boundElementArrayBuffer;
		break;

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (size < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	if (bound == 0)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	GLBufferObject* bo = QD3D12_GetBuffer(bound);
	if (!bo)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	bo->target = target;
	bo->storageFlags = flags;
	bo->data.resize((size_t)size);

	if (size > 0)
	{
		if (data)
			memcpy(bo->data.data(), data, (size_t)size);
		else
			memset(bo->data.data(), 0, (size_t)size);
	}
}

void APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
{
	(void)usage;
	glBufferStorage(target, size, data, 0);
}

void APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
{
	GLuint bound = 0;

	switch (target)
	{
	case GL_ARRAY_BUFFER:
		bound = g_gl.boundArrayBuffer;
		break;

	case GL_ELEMENT_ARRAY_BUFFER:
		bound = g_gl.boundElementArrayBuffer;
		break;

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (offset < 0 || size < 0 || !data)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	GLBufferObject* bo = QD3D12_GetBuffer(bound);
	if (!bo)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	if ((size_t)offset > bo->data.size() || (size_t)size > (bo->data.size() - (size_t)offset))
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	memcpy(bo->data.data() + (size_t)offset, data, (size_t)size);
}

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	if (count <= 0)
		return;

	if (first < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	g_gl.immediateVerts.Clear();

	for (GLsizei i = 0; i < count; ++i)
	{
		GLVertex& v = g_gl.immediateVerts.Push();
		QD3D12_FetchArrayVertex(first + i, v);
	}

	FlushImmediate(mode, g_gl.immediateVerts.Data(), g_gl.immediateVerts.Size());
}

void* APIENTRY glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
	GLuint bound = 0;

	switch (target)
	{
	case GL_ARRAY_BUFFER:
		bound = g_gl.boundArrayBuffer;
		break;

	case GL_ELEMENT_ARRAY_BUFFER:
		bound = g_gl.boundElementArrayBuffer;
		break;

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		return nullptr;
	}

	if (offset < 0 || length < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return nullptr;
	}

	GLBufferObject* bo = QD3D12_GetBuffer(bound);
	if (!bo)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return nullptr;
	}

	if (bo->mapped)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return nullptr;
	}

	if ((size_t)offset > bo->data.size() || (size_t)length > (bo->data.size() - (size_t)offset))
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return nullptr;
	}

	// optional light validation
	if ((access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) == 0)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return nullptr;
	}

	// emulate invalidation/orphan-ish behavior on CPU backing store
#ifdef GL_MAP_INVALIDATE_RANGE_BIT
	if ((access & GL_MAP_INVALIDATE_RANGE_BIT) && length > 0)
	{
		memset(bo->data.data() + offset, 0, (size_t)length);
	}
#endif

#ifdef GL_MAP_INVALIDATE_BUFFER_BIT
	if ((access & GL_MAP_INVALIDATE_BUFFER_BIT) && !bo->data.empty())
	{
		memset(bo->data.data(), 0, bo->data.size());
	}
#endif

	bo->mapped = true;
	bo->mappedOffset = offset;
	bo->mappedLength = length;
	bo->mappedAccess = access;

	return bo->data.data() + offset;
}

GLboolean APIENTRY glUnmapBuffer(GLenum target)
{
	GLuint bound = 0;

	switch (target)
	{
	case GL_ARRAY_BUFFER:
		bound = g_gl.boundArrayBuffer;
		break;

	case GL_ELEMENT_ARRAY_BUFFER:
		bound = g_gl.boundElementArrayBuffer;
		break;

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		return GL_FALSE;
	}

	GLBufferObject* bo = QD3D12_GetBuffer(bound);
	if (!bo)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return GL_FALSE;
	}

	if (!bo->mapped)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return GL_FALSE;
	}

	bo->mapped = false;
	bo->mappedOffset = 0;
	bo->mappedLength = 0;
	bo->mappedAccess = 0;

	return GL_TRUE;
}

void APIENTRY glGenQueries(GLsizei n, GLuint* ids)
{
	if (n < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	if (!ids)
		return;

	for (GLsizei i = 0; i < n; ++i)
	{
		GLuint           id = g_gl.nextQueryId++;
		GLOcclusionQuery q{};
		q.id = id;
		q.heapIndex = id % QD3D12_MaxQueries;  // simple scheme; good enough if IDs stay bounded
		g_gl.queries.emplace(id, q);
		ids[i] = id;
	}
}

void APIENTRY glDeleteQueries(GLsizei n, const GLuint* ids)
{
	if (n < 0)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	if (!ids)
		return;

	for (GLsizei i = 0; i < n; ++i)
	{
		GLuint id = ids[i];
		if (id == 0)
			continue;

		if (g_gl.currentQuery == id)
		{
			g_gl.lastError = GL_INVALID_OPERATION;
			return;
		}

		g_gl.queries.erase(id);
	}
}

GLboolean APIENTRY glIsQuery(GLuint id) { return g_gl.queries.find(id) != g_gl.queries.end() ? GL_TRUE : GL_FALSE; }

void APIENTRY glBeginQuery(GLenum target, GLuint id)
{
	if (target != GL_SAMPLES_PASSED)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	auto it = g_gl.queries.find(id);
	if (it == g_gl.queries.end() || id == 0)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	if (g_gl.currentQuery != 0)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	GLOcclusionQuery& q = it->second;
	q.active = true;
	q.pending = false;
	q.resultReady = false;
	q.result = 0;

	g_gl.currentQuery = id;

	QueryMarker m{};
	m.type = QueryMarker::Begin;
	m.id = id;
	g_gl.queryMarkers.push_back(m);

	if (!g_gl.queuedBatches.empty())
		g_gl.queuedBatches.back().markerEnd = g_gl.queryMarkers.size();
}

void APIENTRY glEndQuery(GLenum target)
{
	if (target != GL_SAMPLES_PASSED)
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	if (g_gl.currentQuery == 0)
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	auto it = g_gl.queries.find(g_gl.currentQuery);
	if (it == g_gl.queries.end())
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		g_gl.currentQuery = 0;
		return;
	}

	it->second.active = false;
	it->second.pending = true;

	QueryMarker m{};
	m.type = QueryMarker::End;
	m.id = g_gl.currentQuery;
	g_gl.queryMarkers.push_back(m);

	g_gl.currentQuery = 0;

	if (!g_gl.queuedBatches.empty())
		g_gl.queuedBatches.back().markerEnd = g_gl.queryMarkers.size();
}

static void QD3D12_UpdateQueryResult(GLOcclusionQuery& q)
{
	if (!q.pending || q.resultReady)
		return;

	if (q.submittedFence == 0)
		return;

	if (g_gl.fence->GetCompletedValue() < q.submittedFence)
		return;

	q.result = g_gl.occlusionReadbackCpu[q.heapIndex];
	q.resultReady = true;
	q.pending = false;
}

void APIENTRY glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)
{
	if (!params)
		return;

	auto it = g_gl.queries.find(id);
	if (it == g_gl.queries.end())
	{
		g_gl.lastError = GL_INVALID_OPERATION;
		return;
	}

	GLOcclusionQuery& q = it->second;
	QD3D12_UpdateQueryResult(q);

	switch (pname)
	{
	case GL_QUERY_RESULT_AVAILABLE:
		*params = q.resultReady ? GL_TRUE : GL_FALSE;
		break;

	case GL_QUERY_RESULT:
		if (!q.resultReady)
		{
			QD3D12_WaitForGPU();
			QD3D12_UpdateQueryResult(q);
		}
		*params = (GLuint)q.result;
		break;

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		break;
	}
}

void APIENTRY glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params)
{
	if (!params)
		return;

	GLuint u = 0;
	glGetQueryObjectuiv(id, pname, &u);
	*params = (GLint)u;
}

void APIENTRY glPointSize(GLfloat size)
{
	if (size <= 0.0f)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	g_gl.pointSize = size;
}

void APIENTRY glPointParameterfEXT(GLenum pname, GLfloat param)
{
	switch (pname)
	{
	case GL_POINT_SIZE_MIN_EXT:
		if (param < 0.0f)
		{
			g_gl.lastError = GL_INVALID_VALUE;
			return;
		}
		g_gl.pointSizeMin = param;
		break;

	case GL_POINT_SIZE_MAX_EXT:
		if (param < 0.0f)
		{
			g_gl.lastError = GL_INVALID_VALUE;
			return;
		}
		g_gl.pointSizeMax = param;
		break;

	case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
		if (param < 0.0f)
		{
			g_gl.lastError = GL_INVALID_VALUE;
			return;
		}
		g_gl.pointFadeThresholdSize = param;
		break;

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		break;
	}
}

void APIENTRY glPointParameterfvEXT(GLenum pname, const GLfloat* params)
{
	if (!params)
	{
		g_gl.lastError = GL_INVALID_VALUE;
		return;
	}

	switch (pname)
	{
	case GL_POINT_SIZE_MIN_EXT:
		if (params[0] < 0.0f)
		{
			g_gl.lastError = GL_INVALID_VALUE;
			return;
		}
		g_gl.pointSizeMin = params[0];
		break;

	case GL_POINT_SIZE_MAX_EXT:
		if (params[0] < 0.0f)
		{
			g_gl.lastError = GL_INVALID_VALUE;
			return;
		}
		g_gl.pointSizeMax = params[0];
		break;

	case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
		if (params[0] < 0.0f)
		{
			g_gl.lastError = GL_INVALID_VALUE;
			return;
		}
		g_gl.pointFadeThresholdSize = params[0];
		break;

	case GL_DISTANCE_ATTENUATION_EXT:
		g_gl.pointDistanceAttenuation[0] = params[0];
		g_gl.pointDistanceAttenuation[1] = params[1];
		g_gl.pointDistanceAttenuation[2] = params[2];
		break;

	default:
		g_gl.lastError = GL_INVALID_ENUM;
		break;
	}
}

void APIENTRY glColor3fv(const GLfloat* v)
{
	if (!v)
		return;

	glColor3f(v[0], v[1], v[2]);
}

static GLuint g_qd3d12ListBaseCompat = 0;
static GLfloat g_qd3d12CompatRasterPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static GLboolean g_qd3d12CompatRasterPosValid = GL_TRUE;

void APIENTRY glRasterPos3fv(const GLfloat* v)
{
	if (!v)
		return;

	glRasterPos3f(v[0], v[1], v[2]);
}

void APIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid* lists)
{
	if (n <= 0 || !lists)
		return;

	switch (type)
	{
	case GL_UNSIGNED_BYTE:
	{
		const GLubyte* p = static_cast<const GLubyte*>(lists);
		for (GLsizei i = 0; i < n; ++i)
			glCallList(g_qd3d12ListBaseCompat + (GLuint)p[i]);
		break;
	}
	case GL_BYTE:
	{
		const GLbyte* p = static_cast<const GLbyte*>(lists);
		for (GLsizei i = 0; i < n; ++i)
			glCallList(g_qd3d12ListBaseCompat + (GLuint)(unsigned char)p[i]);
		break;
	}
	case GL_UNSIGNED_SHORT:
	{
		const GLushort* p = static_cast<const GLushort*>(lists);
		for (GLsizei i = 0; i < n; ++i)
			glCallList(g_qd3d12ListBaseCompat + (GLuint)p[i]);
		break;
	}
	case GL_SHORT:
	{
		const GLshort* p = static_cast<const GLshort*>(lists);
		for (GLsizei i = 0; i < n; ++i)
			glCallList(g_qd3d12ListBaseCompat + (GLuint)(unsigned short)p[i]);
		break;
	}
	case GL_UNSIGNED_INT:
	{
		const GLuint* p = static_cast<const GLuint*>(lists);
		for (GLsizei i = 0; i < n; ++i)
			glCallList(g_qd3d12ListBaseCompat + p[i]);
		break;
	}
	case GL_INT:
	{
		const GLint* p = static_cast<const GLint*>(lists);
		for (GLsizei i = 0; i < n; ++i)
			glCallList(g_qd3d12ListBaseCompat + (GLuint)p[i]);
		break;
	}
	default:
		g_gl.lastError = GL_INVALID_ENUM;
		break;
	}
}

// opengl.cpp

void APIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
	glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void APIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	glRotatef((GLfloat)angle, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void APIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
	(void)coord;
	(void)pname;
	(void)param;
}

void APIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param)
{
	glTexGenf(coord, pname, (GLfloat)param);
}

void APIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat* params)
{
	if (!params)
		return;

	glTexGenf(coord, pname, params[0]);
}

void APIENTRY glTexGeniv(GLenum coord, GLenum pname, const GLint* params)
{
	if (!params)
		return;

	glTexGeni(coord, pname, params[0]);
}

void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();
}

void APIENTRY glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
	g_qd3d12CompatRasterPos[0] = x;
	g_qd3d12CompatRasterPos[1] = y;
	g_qd3d12CompatRasterPos[2] = z;
	g_qd3d12CompatRasterPos[3] = 1.0f;
	g_qd3d12CompatRasterPosValid = GL_TRUE;
}

// -----------------------------------------------------------------------------
// Minimal legacy-GL compatibility state
// -----------------------------------------------------------------------------

struct QD3D12AttribState
{
	GLfloat currentColor[4];
	GLfloat currentNormal[3];
	GLfloat rasterPos[4];
	GLboolean rasterPosValid;

	GLfloat lineWidth;
	GLint lineStippleFactor;
	GLushort lineStipplePattern;
	GLboolean lightingEnabled;
	GLboolean lineStippleEnabled;

	GLfloat lightModelAmbient[4];
	GLuint listBase;
};

static QD3D12AttribState g_glState =
{
	{1,1,1,1},
	{0,0,1},
	{0,0,0,1},
	GL_TRUE,
	1.0f,
	1,
	0xFFFF,
	GL_FALSE,
	GL_FALSE,
	{0.2f, 0.2f, 0.2f, 1.0f},
	0
};

static std::vector<QD3D12AttribState> g_attribStack;
static GLfloat g_qd3d12PixelZoomX = 1.0f;
static GLfloat g_qd3d12PixelZoomY = 1.0f;

static bool QD3D12_ConvertPixelsToRGBA8(GLsizei width, GLsizei height, GLenum format, GLenum type,
	const GLvoid* pixels, std::vector<uint8_t>& outRGBA)
{
	if (width <= 0 || height <= 0 || !pixels)
		return false;

	if (type != GL_UNSIGNED_BYTE)
		return false;

	const uint8_t* src = static_cast<const uint8_t*>(pixels);
	outRGBA.resize((size_t)width * (size_t)height * 4);

	for (GLsizei i = 0; i < width * height; ++i)
	{
		uint8_t r = 0, g = 0, b = 0, a = 255;
		switch (format)
		{
		case GL_RGBA:
			b = src[i * 4 + 2];
			g = src[i * 4 + 1];
			r = src[i * 4 + 0];
			a = src[i * 4 + 3];
			break;

		case GL_BGRA_EXT:
			r = src[i * 4 + 2];
			g = src[i * 4 + 1];
			b = src[i * 4 + 0];
			a = src[i * 4 + 3];
			break;

		case GL_RGB:
			r = src[i * 3 + 0];
			g = src[i * 3 + 1];
			b = src[i * 3 + 2];
			break;

		case GL_BGR_EXT:
			r = src[i * 3 + 2];
			g = src[i * 3 + 1];
			b = src[i * 3 + 0];
			break;

		case GL_LUMINANCE:
			r = g = b = src[i];
			break;

		case GL_ALPHA:
			a = src[i];
			r = g = b = 255;
			break;

		case GL_LUMINANCE_ALPHA:
			r = g = b = src[i * 2 + 0];
			a = src[i * 2 + 1];
			break;

		default:
			return false;
		}

		outRGBA[(size_t)i * 4 + 0] = r;
		outRGBA[(size_t)i * 4 + 1] = g;
		outRGBA[(size_t)i * 4 + 2] = b;
		outRGBA[(size_t)i * 4 + 3] = a;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Raster position
// -----------------------------------------------------------------------------

void APIENTRY glRasterPos2f(GLfloat x, GLfloat y)
{
	g_qd3d12CompatRasterPos[0] = x;
	g_qd3d12CompatRasterPos[1] = y;
	g_qd3d12CompatRasterPos[2] = 0.0f;
	g_qd3d12CompatRasterPos[3] = 1.0f;
	g_qd3d12CompatRasterPosValid = GL_TRUE;
}

void APIENTRY glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
	g_qd3d12PixelZoomX = xfactor;
	g_qd3d12PixelZoomY = yfactor;
}

void APIENTRY glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
	if (!g_qd3d12CompatRasterPosValid || width <= 0 || height <= 0 || !pixels)
		return;

	std::vector<uint8_t> rgba;
	if (!QD3D12_ConvertPixelsToRGBA8(width, height, format, type, pixels, rgba))
	{
		g_gl.lastError = GL_INVALID_ENUM;
		return;
	}

	const GLuint unit = g_gl.activeTextureUnit;
	const GLuint oldBound = g_gl.boundTexture[unit];
	const bool oldTexture2D = g_gl.texture2D[unit];
	const float oldColor[4] = {
		g_gl.curColor[0], g_gl.curColor[1], g_gl.curColor[2], g_gl.curColor[3]
	};

	GLuint tempTexture = 0;
	glGenTextures(1, &tempTexture);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tempTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

	const GLfloat x1 = g_qd3d12CompatRasterPos[0];
	const GLfloat y1 = g_qd3d12CompatRasterPos[1];
	const GLfloat x2 = x1 + (GLfloat)width * g_qd3d12PixelZoomX;
	const GLfloat y2 = y1 + (GLfloat)height * g_qd3d12PixelZoomY;

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(x1, y1);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(x2, y1);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(x2, y2);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(x1, y2);
	glEnd();

	glDeleteTextures(1, &tempTexture);

	if (oldTexture2D)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, oldBound);
	glColor4f(oldColor[0], oldColor[1], oldColor[2], oldColor[3]);
}

// -----------------------------------------------------------------------------
// Current color / normal
// -----------------------------------------------------------------------------

void APIENTRY glNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
	g_glState.currentNormal[0] = x;
	g_glState.currentNormal[1] = y;
	g_glState.currentNormal[2] = z;
}

void APIENTRY glNormal3fv(const GLfloat* v)
{
	if (!v) return;
	glNormal3f(v[0], v[1], v[2]);
}

// -----------------------------------------------------------------------------
// Attribute stack
// -----------------------------------------------------------------------------

void APIENTRY glPushAttrib(GLbitfield mask)
{
	(void)mask;
	g_attribStack.push_back(g_glState);
}

void APIENTRY glPopAttrib(void)
{
	if (g_attribStack.empty())
		return;

	g_glState = g_attribStack.back();
	g_attribStack.pop_back();
}

// -----------------------------------------------------------------------------
// Misc legacy state
// -----------------------------------------------------------------------------

GLboolean APIENTRY glIsEnabled(GLenum cap)
{
	switch (cap)
	{
	case GL_VERTEX_PROGRAM_ARB:
		return QD3D12ARB_IsVertexEnabled() ? GL_TRUE : GL_FALSE;

	case GL_FRAGMENT_PROGRAM_ARB:
		return QD3D12ARB_IsFragmentEnabled() ? GL_TRUE : GL_FALSE;

	case GL_BLEND: return g_gl.blend ? GL_TRUE : GL_FALSE;
	case GL_ALPHA_TEST: return g_gl.alphaTest ? GL_TRUE : GL_FALSE;
	case GL_DEPTH_TEST: return g_gl.depthTest ? GL_TRUE : GL_FALSE;
	case GL_CULL_FACE: return g_gl.cullFace ? GL_TRUE : GL_FALSE;
	case GL_SCISSOR_TEST: return g_gl.scissorTest ? GL_TRUE : GL_FALSE;
	case GL_STENCIL_TEST: return g_gl.stencilTest ? GL_TRUE : GL_FALSE;
	case GL_FOG: return g_gl.fog ? GL_TRUE : GL_FALSE;
#ifdef GL_DEPTH_BOUNDS_TEST_EXT
	case GL_DEPTH_BOUNDS_TEST_EXT: return g_gl.depthBoundsTest ? GL_TRUE : GL_FALSE;
#endif
#ifdef GL_STENCIL_TEST_TWO_SIDE_EXT
	case GL_STENCIL_TEST_TWO_SIDE_EXT: return g_gl.stencilTwoSide ? GL_TRUE : GL_FALSE;
#endif
	case GL_TEXTURE_2D: return g_gl.texture2D[g_gl.activeTextureUnit] ? GL_TRUE : GL_FALSE;

	case GL_LIGHTING:
		return g_glState.lightingEnabled;

	case GL_LINE_STIPPLE:
		return g_glState.lineStippleEnabled;

	default:
		return GL_FALSE;
	}
}

void APIENTRY glLineWidth(GLfloat width)
{
	g_glState.lineWidth = (width > 0.0f) ? width : 1.0f;
}

void APIENTRY glLineStipple(GLint factor, GLushort pattern)
{
	g_glState.lineStippleFactor = (factor > 0) ? factor : 1;
	g_glState.lineStipplePattern = pattern;
}

void APIENTRY glLightModelfv(GLenum pname, const GLfloat* params)
{
	if (!params)
		return;

	if (pname == GL_LIGHT_MODEL_AMBIENT)
	{
		g_glState.lightModelAmbient[0] = params[0];
		g_glState.lightModelAmbient[1] = params[1];
		g_glState.lightModelAmbient[2] = params[2];
		g_glState.lightModelAmbient[3] = params[3];
	}
}

void APIENTRY glPolygonStipple(const GLubyte* mask)
{
	(void)mask;
	// Stub: keep for compatibility. Real implementation only matters
	// if your renderer actually emulates polygon stipple.
}

// -----------------------------------------------------------------------------
// Display list stubs
// -----------------------------------------------------------------------------

static GLuint g_nextListId = 1;
static GLuint g_currentList = 0;
static GLenum g_currentListMode = 0;

GLuint APIENTRY glGenLists(GLsizei range)
{
	if (range <= 0)
		return 0;

	GLuint first = g_nextListId;
	g_nextListId += (GLuint)range;
	return first;
}

void APIENTRY glNewList(GLuint list, GLenum mode)
{
	g_currentList = list;
	g_currentListMode = mode;
}

void APIENTRY glEndList(void)
{
	g_currentList = 0;
	g_currentListMode = 0;
}

void APIENTRY glCallList(GLuint list)
{
	(void)list;
	// Stub. Needed for link/compile.
	// Replace with recorded command playback if you add true list support.
}

void APIENTRY glDeleteLists(GLuint list, GLsizei range)
{
	(void)list;
	(void)range;
}

void APIENTRY glListBase(GLuint base)
{
	g_glState.listBase = base;
	g_qd3d12ListBaseCompat = base;
}


void APIENTRY glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	(void)type;
	// Stub. Often only needed for old UI or editor paths.
}

static void ExpandImmediateSlow(GLenum mode, ImmediateVertexBuffer& src, std::vector<glRaytracingVertex_t>& out)
{
	const size_t n = src.count;

	switch (mode)
	{
	case GL_TRIANGLES:
	{
		out.resize(g_gl.immediateVerts.count);
		for (int i = 0; i < g_gl.immediateVerts.count; i++)
		{
			out[i].xyz[0] = g_gl.immediateVerts.Data()[i].px;
			out[i].xyz[1] = g_gl.immediateVerts.Data()[i].py;
			out[i].xyz[2] = g_gl.immediateVerts.Data()[i].pz;

			out[i].normal[0] = g_gl.immediateVerts.Data()[i].nx;
			out[i].normal[1] = g_gl.immediateVerts.Data()[i].ny;
			out[i].normal[2] = g_gl.immediateVerts.Data()[i].nz;

			out[i].st[0] = g_gl.immediateVerts.Data()[i].u0;
			out[i].st[1] = g_gl.immediateVerts.Data()[i].v0;
		}
		return;
	}

	case GL_TRIANGLE_STRIP:
	{
		if (n < 3)
		{
			out.clear();
			return;
		}

		const size_t triCount = n - 2;
		out.resize(triCount * 3);

		size_t d = 0;
		for (size_t i = 2; i < n; ++i, d += 3)
		{
			if ((i & 1) == 0)
			{
				out[d + 0] = src.Data()[i - 2];
				out[d + 1] = src.Data()[i - 1];
				out[d + 2] = src.Data()[i];
			}
			else
			{
				out[d + 0] = src.Data()[i - 1];
				out[d + 1] = src.Data()[i - 2];
				out[d + 2] = src.Data()[i];
			}
		}
		return;
	}

	case GL_TRIANGLE_FAN:
	{
		if (n < 3)
		{
			out.clear();
			return;
		}

		const size_t triCount = n - 2;
		out.resize(triCount * 3);

		const GLVertex v0 = src.Data()[0];
		size_t d = 0;
		for (size_t i = 2; i < n; ++i, d += 3)
		{
			out[d + 0] = v0;
			out[d + 1] = src.Data()[i - 1];
			out[d + 2] = src.Data()[i];
		}
		return;
	}

	case GL_QUADS:
	{
		const size_t quadCount = n >> 2;
		out.resize(quadCount * 6);

		size_t d = 0;
		for (size_t i = 0; i + 3 < n; i += 4, d += 6)
		{
			const GLVertex& v0 = src.Data()[i + 0];
			const GLVertex& v1 = src.Data()[i + 1];
			const GLVertex& v2 = src.Data()[i + 2];
			const GLVertex& v3 = src.Data()[i + 3];

			out[d + 0] = v0;
			out[d + 1] = v1;
			out[d + 2] = v2;
			out[d + 3] = v0;
			out[d + 4] = v2;
			out[d + 5] = v3;
		}
		return;
	}

	case GL_QUAD_STRIP:
	{
		if (n < 4)
		{
			out.clear();
			return;
		}

		const size_t quadCount = (n - 2) >> 1;
		out.resize(quadCount * 6);

		size_t d = 0;
		for (size_t i = 0; i + 3 < n; i += 2, d += 6)
		{
			const GLVertex& v0 = src.Data()[i + 0];
			const GLVertex& v1 = src.Data()[i + 1];
			const GLVertex& v2 = src.Data()[i + 2];
			const GLVertex& v3 = src.Data()[i + 3];

			out[d + 0] = v0;
			out[d + 1] = v1;
			out[d + 2] = v2;
			out[d + 3] = v2;
			out[d + 4] = v1;
			out[d + 5] = v3;
		}
		return;
	}

	//	case GL_POLYGON:
	//	{
	//		// This is still going to be the expensive path.
	//		TessellatePolygon(src, out);
	//		return;
	//	}

	default:
	{
		assert(!"Unknown ExpandImmediate type!");
		out.clear();
		return;
	}
	}
}

void glUpdateBottomAccelStructure(bool opaque, uint32_t& meshHandle)
{
	std::vector<glRaytracingVertex_t> drawVertexes;
	ExpandImmediateSlow(g_gl.currentPrim, g_gl.immediateVerts, drawVertexes);

	std::vector<unsigned int> drawIndexes(drawVertexes.size());
	for (int i = 0; i < drawVertexes.size(); i++)
	{
		drawIndexes[i] = i;
	}
	glRaytracingMeshDesc_t meshDesc;
	memset(&meshDesc, 0, sizeof(meshDesc));
	meshDesc.vertices = &drawVertexes[0];
	meshDesc.vertexCount = (uint32_t)drawVertexes.size();
	meshDesc.indices = &drawIndexes[0];
	meshDesc.indexCount = (uint32_t)drawIndexes.size();
	meshDesc.allowUpdate = 1;
	meshDesc.opaque = opaque;

	if (meshHandle)
		glRaytracingUpdateMesh(meshHandle, &meshDesc);
	else
		meshHandle = glRaytracingCreateMesh(&meshDesc);
}

void glUpdateTopLevelAceelStructure(uint32_t mesh, float* transform, uint32_t& topLevelHandle) {
	glRaytracingInstanceDesc_t instDesc = {};
	instDesc.meshHandle = mesh;
	instDesc.instanceID = 0;
	instDesc.mask = 0xFF;

	if (transform == NULL)
	{
		instDesc.transform[0] = 1.0f; instDesc.transform[1] = 0.0f; instDesc.transform[2] = 0.0f; instDesc.transform[3] = 0.0f;
		instDesc.transform[4] = 0.0f; instDesc.transform[5] = 1.0f; instDesc.transform[6] = 0.0f; instDesc.transform[7] = 0.0f;
		instDesc.transform[8] = 0.0f; instDesc.transform[9] = 0.0f; instDesc.transform[10] = 1.0f; instDesc.transform[11] = 0.0f;
	}
	else
	{
		memcpy(instDesc.transform, transform, sizeof(float) * 12);
	}

	if (topLevelHandle == 0)
	{
		topLevelHandle = glRaytracingCreateInstance(&instDesc); 
	}
	else {
		glRaytracingUpdateInstance(topLevelHandle, &instDesc);
	}

}