#include "opengl.h"

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <wrl/client.h>

#include <stdint.h>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

// ============================================================
// Logging / checks
// ============================================================

static void glRaytracingLog(const char* fmt, ...)
{
	char buffer[4096];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	OutputDebugStringA(buffer);
	OutputDebugStringA("\n");
}

static void glRaytracingFatal(const char* fmt, ...)
{
	char buffer[4096];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	OutputDebugStringA(buffer);
	OutputDebugStringA("\n");
	MessageBoxA(nullptr, buffer, "glRaytracing Fatal", MB_OK | MB_ICONERROR);
	DebugBreak();
}

#define GLR_CHECK(x) \
    do { HRESULT _hr = (x); if (FAILED(_hr)) { glRaytracingFatal("HRESULT 0x%08X failed at %s:%d", (unsigned)_hr, __FILE__, __LINE__); return 0; } } while (0)

#define GLR_CHECKV(x) \
    do { HRESULT _hr = (x); if (FAILED(_hr)) { glRaytracingFatal("HRESULT 0x%08X failed at %s:%d", (unsigned)_hr, __FILE__, __LINE__); return; } } while (0)

// ============================================================
// Helpers
// ============================================================

static UINT64 glRaytracingAlignUp(UINT64 v, UINT64 a)
{
	return (v + (a - 1)) & ~(a - 1);
}

template<typename T>
static T glRaytracingClamp(T v, T lo, T hi)
{
	return (v < lo) ? lo : ((v > hi) ? hi : v);
}

static DXGI_FORMAT glRaytracingGetSrvFormatForDepth(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_D32_FLOAT:         return DXGI_FORMAT_R32_FLOAT;
	case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case DXGI_FORMAT_D16_UNORM:         return DXGI_FORMAT_R16_UNORM;
	default:                            return fmt;
	}
}

struct glRaytracingBuffer_t
{
	ComPtr<ID3D12Resource> resource;
	UINT64 size;
	D3D12_GPU_VIRTUAL_ADDRESS gpuVA;

	glRaytracingBuffer_t()
	{
		size = 0;
		gpuVA = 0;
	}
};

static glRaytracingBuffer_t glRaytracingCreateBuffer(
	ID3D12Device* device,
	UINT64 size,
	D3D12_HEAP_TYPE heapType,
	D3D12_RESOURCE_STATES initialState,
	D3D12_RESOURCE_FLAGS flags)
{
	glRaytracingBuffer_t out;

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = heapType;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = size;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.Format = DXGI_FORMAT_UNKNOWN;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rd.Flags = flags;

	HRESULT hr = device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		initialState,
		nullptr,
		IID_PPV_ARGS(&out.resource));

	if (FAILED(hr))
	{
		glRaytracingFatal("CreateCommittedResource failed 0x%08X", (unsigned)hr);
		return out;
	}

	out.size = size;
	out.gpuVA = out.resource->GetGPUVirtualAddress();
	return out;
}

static void glRaytracingMapCopy(ID3D12Resource* res, const void* src, size_t bytes)
{
	void* dst = nullptr;
	HRESULT hr = res->Map(0, nullptr, &dst);
	if (FAILED(hr))
	{
		glRaytracingFatal("Map failed 0x%08X", (unsigned)hr);
		return;
	}

	memcpy(dst, src, bytes);
	res->Unmap(0, nullptr);
}

static void glRaytracingTransition(
	ID3D12GraphicsCommandList* cmd,
	ID3D12Resource* res,
	D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after)
{
	if (!res || before == after)
		return;

	D3D12_RESOURCE_BARRIER b = {};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = res;
	b.Transition.StateBefore = before;
	b.Transition.StateAfter = after;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmd->ResourceBarrier(1, &b);
}

static D3D12_CPU_DESCRIPTOR_HANDLE glRaytracingOffsetCpu(D3D12_CPU_DESCRIPTOR_HANDLE h, UINT stride, UINT idx)
{
	h.ptr += UINT64(stride) * UINT64(idx);
	return h;
}

static D3D12_GPU_DESCRIPTOR_HANDLE glRaytracingOffsetGpu(D3D12_GPU_DESCRIPTOR_HANDLE h, UINT stride, UINT idx)
{
	h.ptr += UINT64(stride) * UINT64(idx);
	return h;
}

// ============================================================
// Shared command context
// ============================================================

struct glRaytracingCmdContext_t
{
	ComPtr<ID3D12Device5> device;
	ComPtr<ID3D12CommandQueue> queue;

	ComPtr<ID3D12CommandAllocator> cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList4> cmdList;
	UINT64 cmdLastFenceValue;

	ComPtr<ID3D12CommandAllocator> blasCmdAlloc;
	ComPtr<ID3D12GraphicsCommandList4> blasCmdList;
	UINT64 blasLastFenceValue;

	ComPtr<ID3D12CommandAllocator> tlasCmdAlloc;
	ComPtr<ID3D12GraphicsCommandList4> tlasCmdList;
	UINT64 tlasLastFenceValue;

	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	UINT64 nextFenceValue;
	bool initialized;

	glRaytracingCmdContext_t()
	{
		cmdLastFenceValue = 0;
		blasLastFenceValue = 0;
		tlasLastFenceValue = 0;
		fenceEvent = nullptr;
		nextFenceValue = 0;
		initialized = false;
	}
};

static glRaytracingCmdContext_t g_glRaytracingCmd;

static void glRaytracingWaitFenceValue(UINT64 value)
{
	if (!value || !g_glRaytracingCmd.fence)
		return;

	if (g_glRaytracingCmd.fence->GetCompletedValue() >= value)
		return;

	g_glRaytracingCmd.fence->SetEventOnCompletion(value, g_glRaytracingCmd.fenceEvent);
	WaitForSingleObject(g_glRaytracingCmd.fenceEvent, INFINITE);
}

static UINT64 glRaytracingSignalQueue(void)
{
	if (!g_glRaytracingCmd.queue || !g_glRaytracingCmd.fence)
		return 0;

	const UINT64 value = ++g_glRaytracingCmd.nextFenceValue;
	g_glRaytracingCmd.queue->Signal(g_glRaytracingCmd.fence.Get(), value);
	return value;
}

static void glRaytracingWaitIdle(void)
{
	const UINT64 value = glRaytracingSignalQueue();
	glRaytracingWaitFenceValue(value);
}

static int glRaytracingInitCmdContext(void)
{
	if (g_glRaytracingCmd.initialized)
		return 1;

	ID3D12Device* baseDevice = QD3D12_GetDevice();
	ID3D12CommandQueue* baseQueue = QD3D12_GetQueue();

	if (!baseDevice || !baseQueue)
	{
		glRaytracingFatal("glRaytracingInitCmdContext: missing device or queue");
		return 0;
	}

	GLR_CHECK(baseDevice->QueryInterface(IID_PPV_ARGS(&g_glRaytracingCmd.device)));
	g_glRaytracingCmd.queue = baseQueue;

	GLR_CHECK(g_glRaytracingCmd.device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&g_glRaytracingCmd.cmdAlloc)));

	GLR_CHECK(g_glRaytracingCmd.device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_glRaytracingCmd.cmdAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(&g_glRaytracingCmd.cmdList)));

	GLR_CHECK(g_glRaytracingCmd.cmdList->Close());

	GLR_CHECK(g_glRaytracingCmd.device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&g_glRaytracingCmd.blasCmdAlloc)));

	GLR_CHECK(g_glRaytracingCmd.device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_glRaytracingCmd.blasCmdAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(&g_glRaytracingCmd.blasCmdList)));

	GLR_CHECK(g_glRaytracingCmd.blasCmdList->Close());

	GLR_CHECK(g_glRaytracingCmd.device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&g_glRaytracingCmd.tlasCmdAlloc)));

	GLR_CHECK(g_glRaytracingCmd.device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_glRaytracingCmd.tlasCmdAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(&g_glRaytracingCmd.tlasCmdList)));

	GLR_CHECK(g_glRaytracingCmd.tlasCmdList->Close());

	GLR_CHECK(g_glRaytracingCmd.device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&g_glRaytracingCmd.fence)));

	g_glRaytracingCmd.fenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
	if (!g_glRaytracingCmd.fenceEvent)
	{
		glRaytracingFatal("CreateEventA failed");
		return 0;
	}

	g_glRaytracingCmd.initialized = true;
	return 1;
}

static void glRaytracingShutdownCmdContext(void)
{
	if (!g_glRaytracingCmd.initialized)
		return;

	glRaytracingWaitIdle();

	if (g_glRaytracingCmd.fenceEvent)
	{
		CloseHandle(g_glRaytracingCmd.fenceEvent);
		g_glRaytracingCmd.fenceEvent = nullptr;
	}

	g_glRaytracingCmd = glRaytracingCmdContext_t();
}

static int glRaytracingBeginCmd(void)
{
	glRaytracingWaitFenceValue(g_glRaytracingCmd.cmdLastFenceValue);

	GLR_CHECK(g_glRaytracingCmd.cmdAlloc->Reset());
	GLR_CHECK(g_glRaytracingCmd.cmdList->Reset(g_glRaytracingCmd.cmdAlloc.Get(), nullptr));
	return 1;
}

static int glRaytracingEndCmd(void)
{
	GLR_CHECK(g_glRaytracingCmd.cmdList->Close());

	ID3D12CommandList* lists[] = { g_glRaytracingCmd.cmdList.Get() };
	g_glRaytracingCmd.queue->ExecuteCommandLists(1, lists);

	g_glRaytracingCmd.cmdLastFenceValue = glRaytracingSignalQueue();
	glRaytracingWaitFenceValue(g_glRaytracingCmd.cmdLastFenceValue);
	return 1;
}

static int glRaytracingBeginBlasCmd(void)
{
	glRaytracingWaitFenceValue(g_glRaytracingCmd.blasLastFenceValue);

	GLR_CHECK(g_glRaytracingCmd.blasCmdAlloc->Reset());
	GLR_CHECK(g_glRaytracingCmd.blasCmdList->Reset(g_glRaytracingCmd.blasCmdAlloc.Get(), nullptr));
	return 1;
}

static UINT64 glRaytracingEndBlasCmd(void)
{
	GLR_CHECK(g_glRaytracingCmd.blasCmdList->Close());

	ID3D12CommandList* lists[] = { g_glRaytracingCmd.blasCmdList.Get() };
	g_glRaytracingCmd.queue->ExecuteCommandLists(1, lists);

	g_glRaytracingCmd.blasLastFenceValue = glRaytracingSignalQueue();
	return g_glRaytracingCmd.blasLastFenceValue;
}

static int glRaytracingBeginTlasCmd(void)
{
	glRaytracingWaitFenceValue(g_glRaytracingCmd.tlasLastFenceValue);

	GLR_CHECK(g_glRaytracingCmd.tlasCmdAlloc->Reset());
	GLR_CHECK(g_glRaytracingCmd.tlasCmdList->Reset(g_glRaytracingCmd.tlasCmdAlloc.Get(), nullptr));
	return 1;
}

static UINT64 glRaytracingEndTlasCmd(void)
{
	GLR_CHECK(g_glRaytracingCmd.tlasCmdList->Close());

	ID3D12CommandList* lists[] = { g_glRaytracingCmd.tlasCmdList.Get() };
	g_glRaytracingCmd.queue->ExecuteCommandLists(1, lists);

	g_glRaytracingCmd.tlasLastFenceValue = glRaytracingSignalQueue();
	return g_glRaytracingCmd.tlasLastFenceValue;
}

// ============================================================
// Scene builder state
// ============================================================

struct glRaytracingMeshRecord_t
{
	uint32_t handle;
	int      alive;

	glRaytracingMeshDesc_t descCpu;

	std::vector<glRaytracingVertex_t> verticesCpu;
	std::vector<uint32_t>             indicesCpu;

	glRaytracingBuffer_t vertexBuffer;
	glRaytracingBuffer_t indexBuffer;

	glRaytracingBuffer_t blasScratch;
	glRaytracingBuffer_t blasResult[2];

	UINT64 blasScratchSize;
	UINT64 blasResultSize;

	int blasBuilt;
	int dirty;
	int currentBlasIndex;

	glRaytracingMeshRecord_t()
	{
		handle = 0;
		alive = 0;
		memset(&descCpu, 0, sizeof(descCpu));
		blasScratchSize = 0;
		blasResultSize = 0;
		blasBuilt = 0;
		dirty = 0;
		currentBlasIndex = 0;
	}
};

struct glRaytracingInstanceRecord_t
{
	uint32_t handle;
	int      alive;
	glRaytracingInstanceDesc_t descCpu;
	int dirty;
	int cachedActive;
	D3D12_GPU_VIRTUAL_ADDRESS cachedBlasGpuVA;
	D3D12_RAYTRACING_INSTANCE_DESC cachedDescCpu;

	glRaytracingInstanceRecord_t()
	{
		handle = 0;
		alive = 0;
		memset(&descCpu, 0, sizeof(descCpu));
		dirty = 0;
		cachedActive = 0;
		cachedBlasGpuVA = 0;
		memset(&cachedDescCpu, 0, sizeof(cachedDescCpu));
	}
};

struct glRaytracingSceneUploadBuffer_t
{
	glRaytracingBuffer_t buffer;
	UINT64 capacityBytes;
	D3D12_RAYTRACING_INSTANCE_DESC* mapped;

	glRaytracingSceneUploadBuffer_t()
	{
		capacityBytes = 0;
		mapped = nullptr;
	}
};

struct glRaytracingSceneState_t
{
	std::vector<glRaytracingMeshRecord_t> meshes;
	std::vector<glRaytracingInstanceRecord_t> instances;
	std::vector<int> activeInstanceIndices;
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> cpuInstanceDescs;

	std::vector<int> meshHandleToIndex;
	std::vector<int> instanceHandleToIndex;

	uint32_t nextMeshHandle;
	uint32_t nextInstanceHandle;

	glRaytracingSceneUploadBuffer_t instanceDescUpload[2];
	glRaytracingBuffer_t tlasScratch;
	glRaytracingBuffer_t tlasResult[2];

	UINT64 tlasScratchSize;
	UINT64 tlasResultSize;

	int initialized;

	UINT activeInstanceCount;
	UINT builtInstanceCount;

	int tlasBuilt;
	int tlasNeedsRebuild;
	int tlasNeedsUpdate;
	int currentTLASIndex;

	glRaytracingSceneState_t()
	{
		nextMeshHandle = 1;
		nextInstanceHandle = 1;
		tlasScratchSize = 0;
		tlasResultSize = 0;
		initialized = 0;

		activeInstanceCount = 0;
		builtInstanceCount = 0;
		tlasBuilt = 0;
		tlasNeedsRebuild = 1;
		tlasNeedsUpdate = 1;
		currentTLASIndex = 0;
	}
};

static glRaytracingSceneState_t g_glRaytracingScene;

void glRaytracingClear(void);

static void glRaytracingEnsureMeshHandleTable(uint32_t handle)
{
	if (handle >= g_glRaytracingScene.meshHandleToIndex.size())
		g_glRaytracingScene.meshHandleToIndex.resize((size_t)handle + 1, -1);
}

static void glRaytracingEnsureInstanceHandleTable(uint32_t handle)
{
	if (handle >= g_glRaytracingScene.instanceHandleToIndex.size())
		g_glRaytracingScene.instanceHandleToIndex.resize((size_t)handle + 1, -1);
}

static glRaytracingBuffer_t* glRaytracingGetMeshCurrentBLAS(glRaytracingMeshRecord_t* mesh)
{
	if (!mesh)
		return nullptr;
	return &mesh->blasResult[mesh->currentBlasIndex & 1];
}

static const glRaytracingBuffer_t* glRaytracingGetMeshCurrentBLASConst(const glRaytracingMeshRecord_t* mesh)
{
	if (!mesh)
		return nullptr;
	return &mesh->blasResult[mesh->currentBlasIndex & 1];
}

static int glRaytracingGetInactiveTLASIndex(void)
{
	return g_glRaytracingScene.currentTLASIndex ^ 1;
}

static glRaytracingSceneUploadBuffer_t* glRaytracingGetBuildInstanceUpload(void)
{
	return &g_glRaytracingScene.instanceDescUpload[glRaytracingGetInactiveTLASIndex()];
}

static glRaytracingBuffer_t* glRaytracingGetCurrentTLASBuffer(void)
{
	return &g_glRaytracingScene.tlasResult[g_glRaytracingScene.currentTLASIndex & 1];
}

static const glRaytracingBuffer_t* glRaytracingGetCurrentTLASBufferConst(void)
{
	return &g_glRaytracingScene.tlasResult[g_glRaytracingScene.currentTLASIndex & 1];
}

static glRaytracingBuffer_t* glRaytracingGetBuildTLASBuffer(void)
{
	return &g_glRaytracingScene.tlasResult[glRaytracingGetInactiveTLASIndex()];
}


static int glRaytracingEnsureTLASBuffers(
	const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* inputs)
{
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild = {};
	g_glRaytracingCmd.device->GetRaytracingAccelerationStructurePrebuildInfo(inputs, &prebuild);

	if (prebuild.ResultDataMaxSizeInBytes == 0)
	{
		glRaytracingFatal("TLAS prebuild size is zero");
		return 0;
	}

	const UINT64 requiredScratch = glRaytracingAlignUp(
		prebuild.ScratchDataSizeInBytes,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	const UINT64 requiredResult = glRaytracingAlignUp(
		prebuild.ResultDataMaxSizeInBytes,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	if (!g_glRaytracingScene.tlasScratch.resource ||
		g_glRaytracingScene.tlasScratchSize < requiredScratch)
	{
		g_glRaytracingScene.tlasScratch.resource.Reset();
		g_glRaytracingScene.tlasScratch = glRaytracingCreateBuffer(
			g_glRaytracingCmd.device.Get(),
			requiredScratch,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		if (!g_glRaytracingScene.tlasScratch.resource)
			return 0;

		g_glRaytracingScene.tlasScratchSize = requiredScratch;
	}

	for (int i = 0; i < 2; ++i)
	{
		if (!g_glRaytracingScene.tlasResult[i].resource ||
			g_glRaytracingScene.tlasResultSize < requiredResult)
		{
			g_glRaytracingScene.tlasResult[i].resource.Reset();
			g_glRaytracingScene.tlasResult[i] = glRaytracingCreateBuffer(
				g_glRaytracingCmd.device.Get(),
				requiredResult,
				D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			if (!g_glRaytracingScene.tlasResult[i].resource)
				return 0;
		}
	}

	g_glRaytracingScene.tlasResultSize = requiredResult;
	return 1;
}

static glRaytracingMeshRecord_t* glRaytracingFindMesh(uint32_t handle)
{
	if (handle == 0 || handle >= g_glRaytracingScene.meshHandleToIndex.size())
		return nullptr;

	const int index = g_glRaytracingScene.meshHandleToIndex[handle];
	if (index < 0 || (size_t)index >= g_glRaytracingScene.meshes.size())
		return nullptr;

	glRaytracingMeshRecord_t& mesh = g_glRaytracingScene.meshes[(size_t)index];
	if (!mesh.alive || mesh.handle != handle)
		return nullptr;

	return &mesh;
}

static const glRaytracingMeshRecord_t* glRaytracingFindMeshConst(uint32_t handle)
{
	if (handle == 0 || handle >= g_glRaytracingScene.meshHandleToIndex.size())
		return nullptr;

	const int index = g_glRaytracingScene.meshHandleToIndex[handle];
	if (index < 0 || (size_t)index >= g_glRaytracingScene.meshes.size())
		return nullptr;

	const glRaytracingMeshRecord_t& mesh = g_glRaytracingScene.meshes[(size_t)index];
	if (!mesh.alive || mesh.handle != handle)
		return nullptr;

	return &mesh;
}

static glRaytracingInstanceRecord_t* glRaytracingFindInstance(uint32_t handle)
{
	if (handle == 0 || handle >= g_glRaytracingScene.instanceHandleToIndex.size())
		return nullptr;

	const int index = g_glRaytracingScene.instanceHandleToIndex[handle];
	if (index < 0 || (size_t)index >= g_glRaytracingScene.instances.size())
		return nullptr;

	glRaytracingInstanceRecord_t& inst = g_glRaytracingScene.instances[(size_t)index];
	if (!inst.alive || inst.handle != handle)
		return nullptr;

	return &inst;
}

static int glRaytracingEnsureMeshScratch(glRaytracingMeshRecord_t* mesh, UINT64 requiredScratch)
{
	if (!mesh)
		return 0;

	requiredScratch = glRaytracingAlignUp(requiredScratch, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	if (!mesh->blasScratch.resource || mesh->blasScratchSize < requiredScratch)
	{
		mesh->blasScratch.resource.Reset();
		mesh->blasScratch = glRaytracingCreateBuffer(
			g_glRaytracingCmd.device.Get(),
			requiredScratch,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		if (!mesh->blasScratch.resource)
			return 0;

		mesh->blasScratchSize = requiredScratch;
	}

	return 1;
}

static int glRaytracingEnsureMeshResultBuffers(glRaytracingMeshRecord_t* mesh, UINT64 requiredResult)
{
	if (!mesh)
		return 0;

	requiredResult = glRaytracingAlignUp(requiredResult, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	const int resultCount = mesh->descCpu.allowUpdate ? 2 : 1;
	for (int i = 0; i < resultCount; ++i)
	{
		if (!mesh->blasResult[i].resource || mesh->blasResultSize < requiredResult)
		{
			mesh->blasResult[i].resource.Reset();
			mesh->blasResult[i] = glRaytracingCreateBuffer(
				g_glRaytracingCmd.device.Get(),
				requiredResult,
				D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			if (!mesh->blasResult[i].resource)
				return 0;
		}
	}

	if (!mesh->descCpu.allowUpdate)
		mesh->blasResult[1].resource.Reset();

	mesh->blasResultSize = requiredResult;
	return 1;
}

static inline void glRaytracingBuildInstanceDesc(
	D3D12_RAYTRACING_INSTANCE_DESC* outDesc,
	const glRaytracingInstanceRecord_t& inst,
	D3D12_GPU_VIRTUAL_ADDRESS blasGpuVA)
{
	memcpy(outDesc->Transform, inst.descCpu.transform, sizeof(float) * 12);
	outDesc->InstanceID = inst.descCpu.instanceID;
	outDesc->InstanceMask = (UINT8)(inst.descCpu.mask ? inst.descCpu.mask : 0xFF);
	outDesc->InstanceContributionToHitGroupIndex = 0;
	outDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	outDesc->AccelerationStructure = blasGpuVA;
}

static void glRaytracingInvalidateInstanceCache(glRaytracingInstanceRecord_t* inst)
{
	if (!inst)
		return;

	inst->cachedActive = 0;
	inst->cachedBlasGpuVA = 0;
	memset(&inst->cachedDescCpu, 0, sizeof(inst->cachedDescCpu));
}

static int glRaytracingResolveInstanceDesc(
	glRaytracingInstanceRecord_t* inst,
	D3D12_RAYTRACING_INSTANCE_DESC* outDesc,
	D3D12_GPU_VIRTUAL_ADDRESS* outBlasGpuVA)
{
	if (!inst || !inst->alive)
		return 0;

	const glRaytracingMeshRecord_t* mesh = glRaytracingFindMeshConst(inst->descCpu.meshHandle);
	if (!mesh || !mesh->blasBuilt)
		return 0;

	const glRaytracingBuffer_t* blas = glRaytracingGetMeshCurrentBLASConst(mesh);
	if (!blas || !blas->resource || blas->gpuVA == 0)
		return 0;

	if (outDesc)
		glRaytracingBuildInstanceDesc(outDesc, *inst, blas->gpuVA);
	if (outBlasGpuVA)
		*outBlasGpuVA = blas->gpuVA;
	return 1;
}

static int glRaytracingRebuildActiveInstanceCache(void)
{
	g_glRaytracingScene.activeInstanceIndices.clear();
	g_glRaytracingScene.cpuInstanceDescs.clear();
	g_glRaytracingScene.activeInstanceIndices.reserve(g_glRaytracingScene.instances.size());
	g_glRaytracingScene.cpuInstanceDescs.reserve(g_glRaytracingScene.instances.size());

	for (size_t i = 0; i < g_glRaytracingScene.instances.size(); ++i)
	{
		glRaytracingInstanceRecord_t& inst = g_glRaytracingScene.instances[i];
		glRaytracingInvalidateInstanceCache(&inst);

		if (!inst.alive)
			continue;

		D3D12_RAYTRACING_INSTANCE_DESC desc = {};
		D3D12_GPU_VIRTUAL_ADDRESS blasGpuVA = 0;
		if (!glRaytracingResolveInstanceDesc(&inst, &desc, &blasGpuVA))
			continue;

		inst.cachedActive = 1;
		inst.cachedBlasGpuVA = blasGpuVA;
		inst.cachedDescCpu = desc;
		inst.dirty = 0;

		g_glRaytracingScene.activeInstanceIndices.push_back((int)i);
		g_glRaytracingScene.cpuInstanceDescs.push_back(desc);
	}

	g_glRaytracingScene.activeInstanceCount = (UINT)g_glRaytracingScene.cpuInstanceDescs.size();
	return 1;
}

static int glRaytracingRefreshDirtyInstanceCache(void)
{
	for (size_t listIndex = 0; listIndex < g_glRaytracingScene.activeInstanceIndices.size(); ++listIndex)
	{
		const int instIndex = g_glRaytracingScene.activeInstanceIndices[listIndex];
		if (instIndex < 0 || (size_t)instIndex >= g_glRaytracingScene.instances.size())
			return 0;

		glRaytracingInstanceRecord_t& inst = g_glRaytracingScene.instances[(size_t)instIndex];
		if (!inst.alive)
			return 0;

		D3D12_RAYTRACING_INSTANCE_DESC desc = {};
		D3D12_GPU_VIRTUAL_ADDRESS blasGpuVA = 0;
		if (!glRaytracingResolveInstanceDesc(&inst, &desc, &blasGpuVA))
			return 0;

		if (inst.dirty || !inst.cachedActive || inst.cachedBlasGpuVA != blasGpuVA)
		{
			inst.cachedActive = 1;
			inst.cachedBlasGpuVA = blasGpuVA;
			inst.cachedDescCpu = desc;
			g_glRaytracingScene.cpuInstanceDescs[listIndex] = desc;
		}

		inst.dirty = 0;
	}

	g_glRaytracingScene.activeInstanceCount = (UINT)g_glRaytracingScene.cpuInstanceDescs.size();
	return 1;
}

static int glRaytracingEnsureSceneUploadBuffer(UINT64 requiredBytes);
static int glRaytracingUploadCachedInstanceDescs(void)
{
	const UINT activeCount = (UINT)g_glRaytracingScene.cpuInstanceDescs.size();
	const UINT64 instBytes = glRaytracingAlignUp(
		(UINT64)activeCount * (UINT64)sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

	if (!glRaytracingEnsureSceneUploadBuffer(instBytes))
		return 0;

	glRaytracingSceneUploadBuffer_t* upload = glRaytracingGetBuildInstanceUpload();
	if (!upload->mapped)
		return 0;

	if (activeCount > 0)
		memcpy(upload->mapped, g_glRaytracingScene.cpuInstanceDescs.data(), (size_t)activeCount * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

	return 1;
}


static int glRaytracingUploadMeshBuffers(glRaytracingMeshRecord_t* mesh);

static int glRaytracingBuildDirtyMeshesInternal(void)
{
	std::vector<glRaytracingMeshRecord_t*> dirtyMeshes;
	dirtyMeshes.reserve(g_glRaytracingScene.meshes.size());

	for (size_t i = 0; i < g_glRaytracingScene.meshes.size(); ++i)
	{
		glRaytracingMeshRecord_t& mesh = g_glRaytracingScene.meshes[i];
		if (!mesh.alive)
			continue;

		if (!mesh.blasBuilt || mesh.dirty)
			dirtyMeshes.push_back(&mesh);
	}

	if (dirtyMeshes.empty())
		return 1;

	struct glRaytracingMeshBuildInfo_t
	{
		glRaytracingMeshRecord_t* mesh;
		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc;
		ID3D12Resource* barrierResource;
		int newBlasIndex;
	};

	std::vector<glRaytracingMeshBuildInfo_t> builds;
	builds.resize(dirtyMeshes.size());

	for (size_t i = 0; i < dirtyMeshes.size(); ++i)
	{
		glRaytracingMeshRecord_t* mesh = dirtyMeshes[i];
		if (!mesh->vertexBuffer.resource || !mesh->indexBuffer.resource)
		{
			if (!glRaytracingUploadMeshBuffers(mesh))
				return 0;
		}

		glRaytracingMeshBuildInfo_t& info = builds[i];
		memset(&info, 0, sizeof(info));
		info.mesh = mesh;

		info.geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		info.geomDesc.Flags = mesh->descCpu.opaque
			? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE
			: D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		info.geomDesc.Triangles.Transform3x4 = 0;
		info.geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		info.geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		info.geomDesc.Triangles.IndexCount = (UINT)mesh->indicesCpu.size();
		info.geomDesc.Triangles.VertexCount = (UINT)mesh->verticesCpu.size();
		info.geomDesc.Triangles.IndexBuffer = mesh->indexBuffer.gpuVA;
		info.geomDesc.Triangles.VertexBuffer.StartAddress = mesh->vertexBuffer.gpuVA;
		info.geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(glRaytracingVertex_t);

		info.inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		info.inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		info.inputs.NumDescs = 1;
		info.inputs.pGeometryDescs = &info.geomDesc;
		info.inputs.Flags = mesh->descCpu.allowUpdate
			? (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
				D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
			: D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		const bool canUpdateInPlace = (mesh->blasBuilt != 0) && (mesh->descCpu.allowUpdate != 0);
		if (canUpdateInPlace)
			info.inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild = {};
		g_glRaytracingCmd.device->GetRaytracingAccelerationStructurePrebuildInfo(&info.inputs, &prebuild);
		if (prebuild.ResultDataMaxSizeInBytes == 0)
		{
			glRaytracingFatal("BLAS prebuild size is zero");
			return 0;
		}

		if (!glRaytracingEnsureMeshScratch(mesh, prebuild.ScratchDataSizeInBytes))
			return 0;
		if (!glRaytracingEnsureMeshResultBuffers(mesh, prebuild.ResultDataMaxSizeInBytes))
			return 0;

		const int oldIndex = mesh->currentBlasIndex & 1;
		info.newBlasIndex = (mesh->descCpu.allowUpdate && mesh->blasBuilt) ? (oldIndex ^ 1) : oldIndex;

		info.buildDesc.Inputs = info.inputs;
		info.buildDesc.ScratchAccelerationStructureData = mesh->blasScratch.gpuVA;
		info.buildDesc.DestAccelerationStructureData = mesh->blasResult[info.newBlasIndex].gpuVA;
		info.buildDesc.SourceAccelerationStructureData = 0;

		if (canUpdateInPlace)
			info.buildDesc.SourceAccelerationStructureData = mesh->blasResult[oldIndex].gpuVA;

		info.barrierResource = mesh->blasResult[info.newBlasIndex].resource.Get();
	}

	if (!glRaytracingBeginBlasCmd())
		return 0;

	for (size_t i = 0; i < builds.size(); ++i)
	{
		g_glRaytracingCmd.blasCmdList->BuildRaytracingAccelerationStructure(&builds[i].buildDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER uav = {};
		uav.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uav.UAV.pResource = builds[i].barrierResource;
		g_glRaytracingCmd.blasCmdList->ResourceBarrier(1, &uav);
	}

	const UINT64 blasFenceValue = glRaytracingEndBlasCmd();
	if (!blasFenceValue)
		return 0;

	glRaytracingWaitFenceValue(blasFenceValue);

	for (size_t i = 0; i < builds.size(); ++i)
	{
		glRaytracingMeshRecord_t* mesh = builds[i].mesh;
		mesh->currentBlasIndex = builds[i].newBlasIndex;
		mesh->blasBuilt = 1;
		mesh->dirty = 0;
	}

	g_glRaytracingScene.tlasNeedsRebuild = 1;
	g_glRaytracingScene.tlasNeedsUpdate = 0;
	return 1;
}

static int glRaytracingUploadMeshBuffers(glRaytracingMeshRecord_t* mesh)
{
	if (!mesh)
		return 0;

	if (mesh->verticesCpu.empty() || mesh->indicesCpu.empty())
		return 0;

	const UINT64 vbBytes = UINT64(mesh->verticesCpu.size()) * sizeof(glRaytracingVertex_t);
	const UINT64 ibBytes = UINT64(mesh->indicesCpu.size()) * sizeof(uint32_t);

	mesh->vertexBuffer = glRaytracingCreateBuffer(
		g_glRaytracingCmd.device.Get(),
		vbBytes,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_FLAG_NONE);

	mesh->indexBuffer = glRaytracingCreateBuffer(
		g_glRaytracingCmd.device.Get(),
		ibBytes,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_FLAG_NONE);

	if (!mesh->vertexBuffer.resource || !mesh->indexBuffer.resource)
		return 0;

	glRaytracingMapCopy(mesh->vertexBuffer.resource.Get(), mesh->verticesCpu.data(), (size_t)vbBytes);
	glRaytracingMapCopy(mesh->indexBuffer.resource.Get(), mesh->indicesCpu.data(), (size_t)ibBytes);

	return 1;
}

static int glRaytracingBuildMeshInternal(glRaytracingMeshRecord_t* mesh)
{
	if (!mesh)
		return 0;

	const int oldDirty = mesh->dirty;
	mesh->dirty = 1;
	const int ok = glRaytracingBuildDirtyMeshesInternal();
	if (!ok)
		mesh->dirty = oldDirty;
	return ok;
}

static int glRaytracingEnsureSceneUploadBuffer(UINT64 requiredBytes)
{
	glRaytracingSceneUploadBuffer_t* upload = glRaytracingGetBuildInstanceUpload();

	if (requiredBytes == 0)
		requiredBytes = D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT;

	requiredBytes = glRaytracingAlignUp(
		requiredBytes,
		D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

	if (upload->buffer.resource &&
		upload->capacityBytes >= requiredBytes &&
		upload->mapped)
	{
		return 1;
	}

	if (upload->buffer.resource && upload->mapped)
		upload->buffer.resource->Unmap(0, nullptr);

	upload->mapped = nullptr;
	upload->buffer.resource.Reset();
	upload->capacityBytes = 0;

	upload->buffer = glRaytracingCreateBuffer(
		g_glRaytracingCmd.device.Get(),
		requiredBytes,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_FLAG_NONE);

	if (!upload->buffer.resource)
		return 0;

	void* mapped = nullptr;
	D3D12_RANGE readRange = {};
	if (FAILED(upload->buffer.resource->Map(0, &readRange, &mapped)) || !mapped)
	{
		upload->buffer.resource.Reset();
		return 0;
	}

	upload->mapped = (D3D12_RAYTRACING_INSTANCE_DESC*)mapped;
	upload->capacityBytes = requiredBytes;
	return 1;
}

struct glRaytracingResolvedInstance_t
{
	const glRaytracingInstanceRecord_t* inst;
	D3D12_GPU_VIRTUAL_ADDRESS blasGpuVA;
};

static int glRaytracingBuildSceneInternal(void)
{
	UINT aliveCount = 0;
	int anyDirty = 0;
	int needsRebuild = g_glRaytracingScene.tlasNeedsRebuild;
	int needsUpdate = g_glRaytracingScene.tlasNeedsUpdate;

	for (size_t i = 0; i < g_glRaytracingScene.instances.size(); ++i)
	{
		const glRaytracingInstanceRecord_t& inst = g_glRaytracingScene.instances[i];
		if (!inst.alive)
			continue;

		++aliveCount;
		if (inst.dirty)
			anyDirty = 1;
	}

	if (aliveCount == 0)
	{
		g_glRaytracingScene.activeInstanceIndices.clear();
		g_glRaytracingScene.cpuInstanceDescs.clear();
		g_glRaytracingScene.activeInstanceCount = 0;
		g_glRaytracingScene.builtInstanceCount = 0;
		g_glRaytracingScene.tlasBuilt = 0;
		g_glRaytracingScene.tlasNeedsRebuild = 0;
		g_glRaytracingScene.tlasNeedsUpdate = 0;
		return 1;
	}

	if (!g_glRaytracingScene.tlasBuilt)
		needsRebuild = 1;

	if ((UINT)g_glRaytracingScene.activeInstanceIndices.size() != g_glRaytracingScene.builtInstanceCount)
		needsRebuild = 1;

	if (needsRebuild)
	{
		if (!glRaytracingRebuildActiveInstanceCache())
			return 0;
	}
	else
	{
		if (!needsUpdate && !anyDirty)
		{
			g_glRaytracingScene.activeInstanceCount = (UINT)g_glRaytracingScene.cpuInstanceDescs.size();
			return 1;
		}

		if (!glRaytracingRefreshDirtyInstanceCache())
		{
			g_glRaytracingScene.tlasNeedsRebuild = 1;
			if (!glRaytracingRebuildActiveInstanceCache())
				return 0;
			needsRebuild = 1;
		}
	}

	const UINT activeCount = (UINT)g_glRaytracingScene.cpuInstanceDescs.size();
	if (activeCount == 0)
	{
		g_glRaytracingScene.activeInstanceCount = 0;
		g_glRaytracingScene.builtInstanceCount = 0;
		g_glRaytracingScene.tlasBuilt = 0;
		g_glRaytracingScene.tlasNeedsRebuild = 0;
		g_glRaytracingScene.tlasNeedsUpdate = 0;
		return 1;
	}

	if (!g_glRaytracingScene.tlasBuilt || activeCount != g_glRaytracingScene.builtInstanceCount)
		needsRebuild = 1;

	if (!glRaytracingUploadCachedInstanceDescs())
		return 0;

	glRaytracingSceneUploadBuffer_t* upload = glRaytracingGetBuildInstanceUpload();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = activeCount;
	inputs.InstanceDescs = upload->buffer.gpuVA;
	inputs.Flags =
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	if (!glRaytracingEnsureTLASBuffers(&inputs))
		return 0;

	glRaytracingBuffer_t* dstTLAS = glRaytracingGetBuildTLASBuffer();
	const glRaytracingBuffer_t* srcTLAS = glRaytracingGetCurrentTLASBufferConst();

	glRaytracingWaitFenceValue(g_glRaytracingCmd.blasLastFenceValue);

	if (!glRaytracingBeginTlasCmd())
		return 0;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = g_glRaytracingScene.tlasScratch.gpuVA;
	buildDesc.DestAccelerationStructureData = dstTLAS->gpuVA;
	buildDesc.SourceAccelerationStructureData = 0;

	if (!needsRebuild && g_glRaytracingScene.tlasBuilt)
	{
		buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		buildDesc.SourceAccelerationStructureData = srcTLAS->gpuVA;
	}

	g_glRaytracingCmd.tlasCmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uav = {};
	uav.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uav.UAV.pResource = dstTLAS->resource.Get();
	g_glRaytracingCmd.tlasCmdList->ResourceBarrier(1, &uav);

	const UINT64 tlasFenceValue = glRaytracingEndTlasCmd();
	if (!tlasFenceValue)
		return 0;

	glRaytracingWaitFenceValue(tlasFenceValue);

	g_glRaytracingScene.currentTLASIndex = glRaytracingGetInactiveTLASIndex();
	g_glRaytracingScene.activeInstanceCount = activeCount;
	g_glRaytracingScene.builtInstanceCount = activeCount;
	g_glRaytracingScene.tlasBuilt = 1;
	g_glRaytracingScene.tlasNeedsRebuild = 0;
	g_glRaytracingScene.tlasNeedsUpdate = 0;

	for (size_t i = 0; i < g_glRaytracingScene.instances.size(); ++i)
	{
		if (g_glRaytracingScene.instances[i].alive)
			g_glRaytracingScene.instances[i].dirty = 0;
	}

	return 1;
}

// ============================================================
// Scene public API
// ============================================================

int glRaytracingInit(void)
{
	if (g_glRaytracingScene.initialized)
		return 1;

	if (!glRaytracingInitCmdContext())
		return 0;

	g_glRaytracingScene.initialized = 1;
	glRaytracingLog("glRaytracingInit ok");
	return 1;
}

void glRaytracingShutdown(void)
{
	if (!g_glRaytracingScene.initialized)
		return;

	glRaytracingClear();
	g_glRaytracingScene = glRaytracingSceneState_t();
	glRaytracingShutdownCmdContext();
}

void glRaytracingClear(void)
{
	g_glRaytracingScene.meshes.clear();
	g_glRaytracingScene.instances.clear();
	g_glRaytracingScene.activeInstanceIndices.clear();
	g_glRaytracingScene.cpuInstanceDescs.clear();
	g_glRaytracingScene.meshHandleToIndex.clear();
	g_glRaytracingScene.instanceHandleToIndex.clear();

	for (int i = 0; i < 2; ++i)
	{
		if (g_glRaytracingScene.instanceDescUpload[i].buffer.resource && g_glRaytracingScene.instanceDescUpload[i].mapped)
			g_glRaytracingScene.instanceDescUpload[i].buffer.resource->Unmap(0, nullptr);

		g_glRaytracingScene.instanceDescUpload[i] = glRaytracingSceneUploadBuffer_t();
		g_glRaytracingScene.tlasResult[i].resource.Reset();
	}

	g_glRaytracingScene.tlasScratch.resource.Reset();
	g_glRaytracingScene.tlasScratchSize = 0;
	g_glRaytracingScene.tlasResultSize = 0;
	g_glRaytracingScene.activeInstanceCount = 0;
	g_glRaytracingScene.builtInstanceCount = 0;
	g_glRaytracingScene.tlasBuilt = 0;
	g_glRaytracingScene.tlasNeedsRebuild = 1;
	g_glRaytracingScene.tlasNeedsUpdate = 1;
	g_glRaytracingScene.currentTLASIndex = 0;
}

glRaytracingMeshHandle_t glRaytracingCreateMesh(const glRaytracingMeshDesc_t* desc)
{
	if (!g_glRaytracingScene.initialized || !desc)
		return 0;

	if (!desc->vertices || !desc->indices || desc->vertexCount == 0 || desc->indexCount == 0)
		return 0;

	glRaytracingMeshRecord_t mesh;
	mesh.handle = g_glRaytracingScene.nextMeshHandle++;
	mesh.alive = 1;
	mesh.descCpu = *desc;
	mesh.verticesCpu.assign(desc->vertices, desc->vertices + desc->vertexCount);
	mesh.indicesCpu.assign(desc->indices, desc->indices + desc->indexCount);
	mesh.descCpu.vertices = nullptr;
	mesh.descCpu.indices = nullptr;
	mesh.dirty = 1;

	g_glRaytracingScene.meshes.push_back(mesh);
	const size_t newIndex = g_glRaytracingScene.meshes.size() - 1;
	glRaytracingEnsureMeshHandleTable(mesh.handle);
	g_glRaytracingScene.meshHandleToIndex[mesh.handle] = (int)newIndex;

	return mesh.handle;
}

int glRaytracingUpdateMesh(glRaytracingMeshHandle_t meshHandle, const glRaytracingMeshDesc_t* desc)
{
	if (!g_glRaytracingScene.initialized || !desc)
		return 0;

	glRaytracingMeshRecord_t* mesh = glRaytracingFindMesh(meshHandle);
	if (!mesh)
		return 0;

	if (!desc->vertices || !desc->indices || desc->vertexCount == 0 || desc->indexCount == 0)
		return 0;

	mesh->descCpu = *desc;
	mesh->verticesCpu.assign(desc->vertices, desc->vertices + desc->vertexCount);
	mesh->indicesCpu.assign(desc->indices, desc->indices + desc->indexCount);
	mesh->descCpu.vertices = nullptr;
	mesh->descCpu.indices = nullptr;

	mesh->vertexBuffer.resource.Reset();
	mesh->indexBuffer.resource.Reset();
	mesh->blasScratch.resource.Reset();
	mesh->blasResult[0].resource.Reset();
	mesh->blasResult[1].resource.Reset();
	mesh->blasBuilt = 0;
	mesh->dirty = 1;
	mesh->currentBlasIndex = 0;

	for (size_t i = 0; i < g_glRaytracingScene.instances.size(); ++i)
	{
		if (g_glRaytracingScene.instances[i].alive &&
			g_glRaytracingScene.instances[i].descCpu.meshHandle == meshHandle)
		{
			glRaytracingInvalidateInstanceCache(&g_glRaytracingScene.instances[i]);
			g_glRaytracingScene.instances[i].dirty = 1;
		}
	}
	g_glRaytracingScene.tlasNeedsRebuild = 1;
	g_glRaytracingScene.tlasNeedsUpdate = 0;

	return 1;
}

void glRaytracingDeleteMesh(glRaytracingMeshHandle_t meshHandle)
{
	glRaytracingMeshRecord_t* mesh = glRaytracingFindMesh(meshHandle);
	if (!mesh)
		return;

	for (size_t i = 0; i < g_glRaytracingScene.instances.size(); ++i)
	{
		if (g_glRaytracingScene.instances[i].alive &&
			g_glRaytracingScene.instances[i].descCpu.meshHandle == meshHandle)
		{
			glRaytracingInvalidateInstanceCache(&g_glRaytracingScene.instances[i]);
			g_glRaytracingScene.instances[i].alive = 0;
			if (g_glRaytracingScene.instances[i].handle < g_glRaytracingScene.instanceHandleToIndex.size())
				g_glRaytracingScene.instanceHandleToIndex[g_glRaytracingScene.instances[i].handle] = -1;
		}
	}

	mesh->alive = 0;

	if (meshHandle < g_glRaytracingScene.meshHandleToIndex.size())
		g_glRaytracingScene.meshHandleToIndex[meshHandle] = -1;

	g_glRaytracingScene.tlasNeedsRebuild = 1;
}

glRaytracingInstanceHandle_t glRaytracingCreateInstance(const glRaytracingInstanceDesc_t* desc)
{
	if (!g_glRaytracingScene.initialized || !desc)
		return 0;

	if (!glRaytracingFindMeshConst(desc->meshHandle))
		return 0;

	glRaytracingInstanceRecord_t inst;
	inst.handle = g_glRaytracingScene.nextInstanceHandle++;
	inst.alive = 1;
	inst.descCpu = *desc;
	inst.dirty = 1;

	g_glRaytracingScene.instances.push_back(inst);
	const size_t newIndex = g_glRaytracingScene.instances.size() - 1;
	glRaytracingEnsureInstanceHandleTable(inst.handle);
	g_glRaytracingScene.instanceHandleToIndex[inst.handle] = (int)newIndex;
	g_glRaytracingScene.tlasNeedsRebuild = 1;
	return inst.handle;
}

int glRaytracingUpdateInstance(glRaytracingInstanceHandle_t instanceHandle, const glRaytracingInstanceDesc_t* desc)
{
	if (!g_glRaytracingScene.initialized || !desc)
		return 0;

	if (!glRaytracingFindMeshConst(desc->meshHandle))
		return 0;

	glRaytracingInstanceRecord_t* inst = glRaytracingFindInstance(instanceHandle);
	if (!inst)
		return 0;

	const uint32_t oldMeshHandle = inst->descCpu.meshHandle;

	inst->descCpu = *desc;
	inst->dirty = 1;

	if (oldMeshHandle != desc->meshHandle)
	{
		glRaytracingInvalidateInstanceCache(inst);
		g_glRaytracingScene.tlasNeedsRebuild = 1;
	}
	else
	{
		g_glRaytracingScene.tlasNeedsUpdate = 1;
	}

	return 1;
}

void glRaytracingDeleteInstance(glRaytracingInstanceHandle_t instanceHandle)
{
	glRaytracingInstanceRecord_t* inst = glRaytracingFindInstance(instanceHandle);
	if (!inst)
		return;

	glRaytracingInvalidateInstanceCache(inst);
	inst->alive = 0;
	if (instanceHandle < g_glRaytracingScene.instanceHandleToIndex.size())
		g_glRaytracingScene.instanceHandleToIndex[instanceHandle] = -1;
	g_glRaytracingScene.tlasNeedsRebuild = 1;
	g_glRaytracingScene.tlasNeedsUpdate = 0;
}

int glRaytracingBuildMesh(glRaytracingMeshHandle_t meshHandle)
{
	if (!g_glRaytracingScene.initialized)
		return 0;

	glRaytracingMeshRecord_t* mesh = glRaytracingFindMesh(meshHandle);
	if (!mesh)
		return 0;

	return glRaytracingBuildMeshInternal(mesh);
}

int glRaytracingBuildAllMeshes(void)
{
	if (!g_glRaytracingScene.initialized)
		return 0;

	return glRaytracingBuildDirtyMeshesInternal();
}

int glRaytracingBuildScene(void)
{
	if (!g_glRaytracingScene.initialized)
		return 0;

	if (!glRaytracingBuildAllMeshes())
		return 0;

	return glRaytracingBuildSceneInternal();
}

ID3D12Resource* glRaytracingGetTopLevelAS(void)
{
	return glRaytracingGetCurrentTLASBuffer()->resource.Get();
}

uint32_t glRaytracingGetMeshCount(void)
{
	uint32_t count = 0;
	for (size_t i = 0; i < g_glRaytracingScene.meshes.size(); ++i)
	{
		if (g_glRaytracingScene.meshes[i].alive)
			++count;
	}
	return count;
}

uint32_t glRaytracingGetInstanceCount(void)
{
	uint32_t count = 0;
	for (size_t i = 0; i < g_glRaytracingScene.instances.size(); ++i)
	{
		if (g_glRaytracingScene.instances[i].alive)
			++count;
	}
	return count;
}

// ============================================================
// Lighting state
// ============================================================

struct glRaytracingLightingConstants_t
{
	float invViewProj[16];
	float invViewMatrix[16];
	float cameraPos[4];
	float ambientColor[4];
	float screenSize[4];
	float normalReconstructZ;
	uint32_t lightCount;
	uint32_t enableSpecular;
	uint32_t enableHalfLambert;
	float shadowBias;
};

struct glRaytracingLightingState_t
{
	std::vector<glRaytracingLight_t> cpuLights;
	glRaytracingLightingConstants_t constants;

	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	UINT descriptorStride;

	glRaytracingBuffer_t constantBuffer;
	glRaytracingBuffer_t lightBuffer;

	ComPtr<ID3D12RootSignature> globalRootSig;
	ComPtr<ID3D12RootSignature> localRootSig;

	ComPtr<ID3D12StateObject> rtStateObject;
	ComPtr<ID3D12StateObjectProperties> rtStateProps;

	glRaytracingBuffer_t raygenTable;
	glRaytracingBuffer_t missTable;
	glRaytracingBuffer_t hitTable;

	bool initialized;

	glRaytracingLightingState_t()
	{
		memset(&constants, 0, sizeof(constants));
		descriptorStride = 0;
		initialized = false;
	}
};

static glRaytracingLightingState_t g_glRaytracingLighting;
static const char* g_glRaytracingLightingHlsl = R"(
struct Light
{
    float3 position;
    float  radius;

    float3 color;
    float  intensity;

    float3 normal;
    uint   type;

    float3 axisU;
    float  halfWidth;

    float3 axisV;
    float  halfHeight;

    uint   samples;
    uint   twoSided;
    float  persistant;
    float  pad1;
};

struct ShadowPayload
{
    uint hit;
};

cbuffer LightingCB : register(b0)
{
    float4x4 gInvViewProj;
    float4x4 gInvViewMatrix;
    float4   gCameraPos;
    float4   gAmbientColor;
    float4   gScreenSize;
    float    gNormalReconstructZ;
    uint     gLightCount;
    uint     gEnableSpecular;
    uint     gEnableHalfLambert;
    float    gShadowBias;
};

StructuredBuffer<Light> gLights : register(t0);
Texture2D<float4>       gAlbedoTex   : register(t1);
Texture2D<float>        gDepthTex    : register(t2);
Texture2D<float4>       gNormalTex   : register(t3);
Texture2D<float4>       gPositionTex : register(t4);
RaytracingAccelerationStructure gSceneBVH : register(t5);
RWTexture2D<float4>     gOutputTex   : register(u0);

static const uint GL_RAYTRACING_LIGHT_TYPE_POINT = 0;
static const uint GL_RAYTRACING_LIGHT_TYPE_RECT  = 1;

static const uint GEOMETRY_FLAG_SKELETAL = 1;
static const uint GEOMETRY_FLAG_UNLIT    = 2;

float3 LoadScenePosition(uint2 pixel)
{
    float4 p = gPositionTex.Load(int3(pixel, 0));
    return p.xyz;
}

float4 LoadSceneNormal(uint2 pixel)
{
    float4 nSample = gNormalTex.Load(int3(pixel, 0));
    return nSample;
}

[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    payload.hit = 0;
}

[shader("closesthit")]
void ShadowClosestHit(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.hit = 1;
}

float TraceShadow(float3 origin, float3 dir, float maxT)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.001;
    ray.TMax = maxT;

    ShadowPayload payload;
    payload.hit = 0;

    TraceRay(
        gSceneBVH,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
        0xFF,
        0,
        1,
        0,
        ray,
        payload);

    return (payload.hit != 0) ? 0.0 : 1.0;
}

float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float2 Hammersley2D(uint i, uint N, float rand)
{
    float e1 = frac((float)i / (float)N + rand);

    uint bits = i;
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555u) << 1) | ((bits & 0xAAAAAAAAu) >> 1);
    bits = ((bits & 0x33333333u) << 2) | ((bits & 0xCCCCCCCCu) >> 2);
    bits = ((bits & 0x0F0F0F0Fu) << 4) | ((bits & 0xF0F0F0F0u) >> 4);
    bits = ((bits & 0x00FF00FFu) << 8) | ((bits & 0xFF00FF00u) >> 8);

    float e2 = (float)bits * 2.3283064365386963e-10;
    return float2(e1, e2);
}

float2 ConcentricSampleDisk(float2 u)
{
    float2 uOffset = 2.0 * u - 1.0;

    if (uOffset.x == 0.0 && uOffset.y == 0.0)
        return float2(0.0, 0.0);

    float r, theta;
    if (abs(uOffset.x) > abs(uOffset.y))
    {
        r = uOffset.x;
        theta = (3.14159265 / 4.0) * (uOffset.y / uOffset.x);
    }
    else
    {
        r = uOffset.y;
        theta = (3.14159265 / 2.0) - (3.14159265 / 4.0) * (uOffset.x / uOffset.y);
    }

    return r * float2(cos(theta), sin(theta));
}

void BuildOrthonormalBasis(float3 n, out float3 t, out float3 b)
{
    float3 up = (abs(n.z) < 0.999) ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

float3 CosineSampleHemisphere(float2 u)
{
    float2 d = ConcentricSampleDisk(u);
    float z = sqrt(saturate(1.0 - dot(d, d)));
    return float3(d.x, d.y, z);
}

float TraceSoftShadow(float3 worldPos, float3 N, Light Lgt, float3 toLight, float dist)
{
    const uint SHADOW_SAMPLES = 12;

    float3 L = toLight / max(dist, 1e-6);

    float3 tangent, bitangent;
    BuildOrthonormalBasis(L, tangent, bitangent);

    float areaRadius = max(Lgt.radius * 0.03, 0.12);

    float shadowAccum = 0.0;
    float rand = Hash12(worldPos.xy + float2(worldPos.z, dist));

    [unroll]
    for (uint s = 0; s < SHADOW_SAMPLES; ++s)
    {
        float2 xi = Hammersley2D(s, SHADOW_SAMPLES, rand);
        float2 d  = ConcentricSampleDisk(xi) * areaRadius;

        float3 sampleLightPos = Lgt.position + tangent * d.x + bitangent * d.y;
        float3 sampleVec      = sampleLightPos - worldPos;
        float  sampleDist     = length(sampleVec);

        if (sampleDist <= 1e-4)
        {
            shadowAccum += 1.0;
            continue;
        }

        float3 sampleDir = sampleVec / sampleDist;

        float NdotLRaw   = saturate(dot(N, sampleDir));
        float normalBias = lerp(gShadowBias * 3.0, gShadowBias * 0.75, NdotLRaw);

        float3 shadowOrigin = worldPos + N * normalBias + sampleDir * (gShadowBias * 0.5);
        float  shadowTMax   = max(sampleDist - gShadowBias * 0.5, 0.001);

        shadowAccum += TraceShadow(shadowOrigin, sampleDir, shadowTMax);
    }

    return shadowAccum / (float)SHADOW_SAMPLES;
}

float RectLightShadow(float3 worldPos, float3 N, Light Lgt, uint2 pixel)
{
    uint sampleCount = max(Lgt.samples, 1u);
    sampleCount = min(sampleCount, 16u);

    float visibility = 0.0;

    float rand = Hash12((float2)pixel + worldPos.xy + float2(worldPos.z, dot(N.xy, N.xy)));

    float NoL_center = saturate(dot(N, normalize(Lgt.position - worldPos)));
    float normalBias = lerp(gShadowBias * 4.0, gShadowBias * 0.75, NoL_center);
    float3 baseOrigin = worldPos + N * normalBias;

    [loop]
    for (uint s = 0; s < sampleCount; ++s)
    {
        float2 xi = Hammersley2D(s, sampleCount, rand);
        float2 uv = xi * 2.0 - 1.0;

        float3 sampleLightPos =
            Lgt.position +
            Lgt.axisU * (uv.x * Lgt.halfWidth) +
            Lgt.axisV * (uv.y * Lgt.halfHeight);

        float3 toLight = sampleLightPos - baseOrigin;
        float distToLight = length(toLight);

        if (distToLight <= 1e-4)
        {
            visibility += 1.0;
            continue;
        }

        float3 L = toLight / distToLight;

        float NdotL = dot(N, L);
        if (NdotL <= 0.0)
        {
            continue;
        }

        float emitTerm = (Lgt.twoSided != 0)
            ? abs(dot(Lgt.normal, -L))
            : dot(Lgt.normal, -L);

        if (emitTerm <= 0.0)
        {
            continue;
        }

        float3 shadowOrigin = baseOrigin + L * (gShadowBias * 0.5);
        float shadowTMax = max(distToLight - gShadowBias, 0.001);

        visibility += TraceShadow(shadowOrigin, L, shadowTMax);
    }

    return visibility / (float)sampleCount;
}

float ComputeAmbientOcclusion(float3 worldPos, float3 N, uint2 pixel)
{
    const uint AO_SAMPLES = 24;
    const float AO_RADIUS = 32.0;

    float3 tangent, bitangent;
    BuildOrthonormalBasis(N, tangent, bitangent);

    float rand = Hash12((float2)pixel + worldPos.xy + worldPos.zz);

    float visibility = 0.0;

    [unroll]
    for (uint i = 0; i < AO_SAMPLES; ++i)
    {
        float2 xi = Hammersley2D(i, AO_SAMPLES, rand);
        float3 h  = CosineSampleHemisphere(xi);

        float3 aoDir =
            tangent   * h.x +
            bitangent * h.y +
            N         * h.z;

        aoDir = normalize(aoDir);

        float3 aoOrigin = worldPos + N * (gShadowBias * 0.15);

        visibility += TraceShadow(aoOrigin, aoDir, AO_RADIUS);
    }

    visibility /= (float)AO_SAMPLES;
    visibility = saturate(pow(visibility, 1.5));

    return visibility;
}


float ComputeSkyVisibility(float3 worldPos, float3 N, uint2 pixel)
{
    const uint SKY_SAMPLES = 8;
    const float SKY_TMAX   = 1000000.0;

    float3 tangent, bitangent;
    BuildOrthonormalBasis(N, tangent, bitangent);

    float rand = Hash12((float2)pixel * 1.37 + worldPos.xy + float2(worldPos.z, dot(N.xy, N.xy)));

    float vis = 0.0;

    [unroll]
    for (uint i = 0; i < SKY_SAMPLES; ++i)
    {
        float2 xi = Hammersley2D(i, SKY_SAMPLES, rand);
        float3 h  = CosineSampleHemisphere(xi);

        float3 skyDir =
            tangent   * h.x +
            bitangent * h.y +
            N         * h.z;

        skyDir = normalize(skyDir);

        if (skyDir.z <= 0.05)
            continue;

        float3 skyOrigin = worldPos + N * (gShadowBias * 2.0) + skyDir * (gShadowBias * 2.0);
        vis += TraceShadow(skyOrigin, skyDir, SKY_TMAX);
    }

    vis /= (float)SKY_SAMPLES;
    return saturate(vis);
}

float ComputeCavity(uint2 pixel, float3 worldPos, float3 N)
{
    static const int2 taps[12] =
    {
        int2(-2,  0), int2( 2,  0),
        int2( 0, -2), int2( 0,  2),
        int2(-2, -2), int2( 2, -2),
        int2(-2,  2), int2( 2,  2),
        int2(-4,  0), int2( 4,  0),
        int2( 0, -4), int2( 0,  4)
    };

    float accum = 0.0;
    float weightSum = 0.0;

    [unroll]
    for (int i = 0; i < 12; ++i)
    {
        int2 sp = int2(pixel) + taps[i];

        if (sp.x < 0 || sp.y < 0 || sp.x >= (int)gScreenSize.x || sp.y >= (int)gScreenSize.y)
            continue;

        float3 samplePos = gPositionTex.Load(int3(sp, 0)).xyz;
        float3 sampleN   = normalize(gNormalTex.Load(int3(sp, 0)).xyz);

        float3 d = samplePos - worldPos;
        float distSq = dot(d, d);

        if (distSq > (24.0 * 24.0))
            continue;

        float nd = dot(N, sampleN);
        if (nd < 0.65)
            continue;

        float curvature = 1.0 - saturate(nd);
        float w = 1.0 / (1.0 + distSq * 0.02);

        accum += curvature * w;
        weightSum += w;
    }

    float cavity = (weightSum > 0.0) ? (accum / weightSum) : 0.0;
    cavity = saturate(cavity * 2.0);

    return 1.0 - cavity * 0.18;
}

float3 ComputeSpecular(float3 N, float3 V, float3 L, float3 lightColor, float lightIntensity, float atten, float shadow, float3 baseAlbedo)
{
    if (gEnableSpecular == 0)
        return 0.0;

    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    if (NdotL <= 0.0 || NdotV <= 0.0)
        return 0.0;

    float shininess = 48.0;
    float specPow = pow(NdotH, shininess);

    float3 dielectricF0 = float3(0.04, 0.04, 0.04);
    float luminance = dot(baseAlbedo, float3(0.299, 0.587, 0.114));
    float metalHint = saturate((max(max(baseAlbedo.r, baseAlbedo.g), baseAlbedo.b) - 0.75) * 1.5);
    float3 F0 = lerp(dielectricF0, baseAlbedo, metalHint);

    float3 fresnel = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    float energyNorm = (shininess + 8.0) * 0.125;
    float3 spec = fresnel * specPow * energyNorm;

    return lightColor * (lightIntensity * atten * shadow * NdotL) * spec;
}
)"
R"(
[shader("raygeneration")]
void RayGen()
{
    uint2 pixel = DispatchRaysIndex().xy;

    if (pixel.x >= (uint)gScreenSize.x || pixel.y >= (uint)gScreenSize.y)
        return;

    float4 albedoSample = gAlbedoTex.Load(int3(pixel, 0));
    float depthSample   = gDepthTex.Load(int3(pixel, 0));

    if (depthSample <= 0.0 || depthSample >= 1.0)
    {
        gOutputTex[pixel] = albedoSample;
        return;
    }

    float3 baseAlbedo    = albedoSample.rgb;
    float3 worldPos      = LoadScenePosition(pixel);
    float4 normalSample  = LoadSceneNormal(pixel);
    float3 N             = normalize(normalSample.xyz);
    float3 V             = normalize(gCameraPos.xyz - worldPos);

    float geoFlag = normalSample.w;

    float cavity = ComputeCavity(pixel, worldPos, N);
    float microShadow = lerp(0.75, 1.0, cavity);
    float3 albedo = baseAlbedo * cavity;
    albedo *= microShadow;

    float aoRay      = ComputeAmbientOcclusion(worldPos, N, pixel);
	float ao         = aoRay;
    float skyVis  = ComputeSkyVisibility(worldPos, N, pixel);

    float upness = saturate(N.z * 0.5 + 0.5);

    float3 skyColor =
        float3(0.5, 0.5, 0.5) * (0.35 + 0.65 * upness);

    float skyStrength = 2.0;

    float3 lightingAccum = 0.2;
    float3 specularAccum = 0.0;

    lightingAccum += skyColor * (skyStrength * skyVis);

    if (geoFlag == GEOMETRY_FLAG_SKELETAL)
    {
        lightingAccum += 0.2;
    }

    [loop]
    for (uint i = 0; i < gLightCount; ++i)
    {
        Light Lgt = gLights[i];

        if (Lgt.type == GL_RAYTRACING_LIGHT_TYPE_POINT)
        {
            float3 toLight = Lgt.position - worldPos;
            float  distSq  = dot(toLight, toLight);
            float  dist    = sqrt(max(distSq, 1e-6));
            float3 L       = toLight / dist;

            float radius = max(Lgt.radius, 1e-4);

            float atten = saturate((radius - dist) / radius);
            atten = atten * atten;

            float wrap = 0.35;
            float NdotLWrap = saturate((dot(N, L) + wrap) / (1.0 + wrap));

            float shadow = 1.0;
            if (NdotLWrap > 0.0001 && atten > 0.0 && dist > 0.01)
            {
                shadow = TraceSoftShadow(worldPos, N, Lgt, toLight, dist);
            }

            float3 diffuse = Lgt.color * (Lgt.intensity * atten * NdotLWrap * shadow);
            lightingAccum += diffuse;

            specularAccum += ComputeSpecular(N, V, L, Lgt.color, Lgt.intensity, atten, shadow, baseAlbedo);
        }
        else if (Lgt.type == GL_RAYTRACING_LIGHT_TYPE_RECT)
        {
            float3 toCenter = Lgt.position - worldPos;
            float  centerDistSq = dot(toCenter, toCenter);
            float  centerDist = sqrt(max(centerDistSq, 1e-6));
            float3 centerDir = toCenter / centerDist;

            float attenRadius = max(Lgt.radius, 1e-4);
            float atten = saturate((attenRadius - centerDist) / attenRadius);
            atten = atten * atten * atten * atten;

            float shadow = 1.0;
            if (atten > 0.0 && centerDist > 0.01)
            {
                shadow = RectLightShadow(worldPos, N, Lgt, pixel);
            }

            uint sampleCount = max(Lgt.samples, 1u);
            sampleCount = min(sampleCount, 16u);

            float3 rectDiffuseAccum = 0.0;
            float3 rectSpecAccum = 0.0;
            float rand = Hash12((float2)pixel * 0.73 + worldPos.xy + float2(worldPos.z, centerDist));

            [loop]
            for (uint s = 0; s < sampleCount; ++s)
            {
                float2 xi = Hammersley2D(s, sampleCount, rand);
                float2 uv = xi * 2.0 - 1.0;

                float3 sampleLightPos =
                    Lgt.position +
                    Lgt.axisU * (uv.x * Lgt.halfWidth) +
                    Lgt.axisV * (uv.y * Lgt.halfHeight);

                float3 sampleVec = sampleLightPos - worldPos;
                float  sampleDistSq = dot(sampleVec, sampleVec);
                float  sampleDist = sqrt(max(sampleDistSq, 1e-6));
                float3 L = sampleVec / sampleDist;

                float NdotL = saturate(dot(N, L));

                float faceTerm = (Lgt.twoSided != 0)
                    ? abs(dot(-L, Lgt.normal))
                    : saturate(dot(-L, Lgt.normal));

                float sampleWeight = Lgt.intensity * NdotL * faceTerm;

                rectDiffuseAccum += Lgt.color * sampleWeight;

                rectSpecAccum += ComputeSpecular(
                    N,
                    V,
                    L,
                    Lgt.color,
                    Lgt.intensity * faceTerm,
                    1.0,
                    1.0,
                    baseAlbedo);
            }

            rectDiffuseAccum /= (float)sampleCount;
            rectSpecAccum    /= (float)sampleCount;

            lightingAccum += clamp(rectDiffuseAccum * atten * shadow, 0.0, 4.0);
            specularAccum += rectSpecAccum * atten * shadow;
        }
    }

    lightingAccum *= ao;
    specularAccum *= ao;

    if (geoFlag == GEOMETRY_FLAG_SKELETAL)
    {
        lightingAccum *= 1.2;
        specularAccum *= 1.15;
    }

    if (geoFlag == GEOMETRY_FLAG_UNLIT)
    {
        gOutputTex[pixel] = float4(baseAlbedo, albedoSample.a);
    }
    else
    {
        float3 finalColor = albedo * lightingAccum + specularAccum;
        gOutputTex[pixel] = float4(finalColor, albedoSample.a);
    }
}
)";

static ComPtr<IDxcBlob> glRaytracingLightingCompileLibrary(const char* src)
{
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;
	ComPtr<IDxcIncludeHandler> includeHandler;

	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
	if (FAILED(hr))
	{
		glRaytracingFatal("DxcCreateInstance utils failed 0x%08X", (unsigned)hr);
		return nullptr;
	}

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	if (FAILED(hr))
	{
		glRaytracingFatal("DxcCreateInstance compiler failed 0x%08X", (unsigned)hr);
		return nullptr;
	}

	hr = utils->CreateDefaultIncludeHandler(&includeHandler);
	if (FAILED(hr))
	{
		glRaytracingFatal("CreateDefaultIncludeHandler failed 0x%08X", (unsigned)hr);
		return nullptr;
	}

	DxcBuffer source = {};
	source.Ptr = src;
	source.Size = strlen(src);
	source.Encoding = DXC_CP_UTF8;

	const wchar_t* args[] =
	{
		L"-T", L"lib_6_3",
		L"-Zi",
		L"-Qembed_debug",
		L"-O3",
		L"-all_resources_bound"
	};

	ComPtr<IDxcResult> result;
	hr = compiler->Compile(&source, args, _countof(args), includeHandler.Get(), IID_PPV_ARGS(&result));
	if (FAILED(hr))
	{
		glRaytracingFatal("DXC compile failed 0x%08X", (unsigned)hr);
		return nullptr;
	}

	ComPtr<IDxcBlobUtf8> errors;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
	if (errors && errors->GetStringLength() > 0)
	{
		OutputDebugStringA(errors->GetStringPointer());
		OutputDebugStringA("\n");
	}

	HRESULT status = S_OK;
	result->GetStatus(&status);
	if (FAILED(status))
	{
		glRaytracingFatal("DXIL compile status failed 0x%08X", (unsigned)status);
		return nullptr;
	}

	ComPtr<IDxcBlob> dxil;
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxil), nullptr);
	return dxil;
}

static int glRaytracingLightingCreateDescriptorHeap(void)
{
	D3D12_DESCRIPTOR_HEAP_DESC hd = {};
	hd.NumDescriptors = 7;
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	GLR_CHECK(g_glRaytracingCmd.device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&g_glRaytracingLighting.descriptorHeap)));
	g_glRaytracingLighting.descriptorStride =
		g_glRaytracingCmd.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return 1;
}

static int glRaytracingLightingCreateRootSignatures(void)
{
	{
		D3D12_DESCRIPTOR_RANGE ranges[2] = {};

		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 6;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = 0;

		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[1].NumDescriptors = 1;
		ranges[1].BaseShaderRegister = 0;
		ranges[1].RegisterSpace = 0;
		ranges[1].OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER params[3] = {};

		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[0].DescriptorTable.NumDescriptorRanges = 1;
		params[0].DescriptorTable.pDescriptorRanges = &ranges[0];
		params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[1].DescriptorTable.NumDescriptorRanges = 1;
		params[1].DescriptorTable.pDescriptorRanges = &ranges[1];
		params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[2].Descriptor.ShaderRegister = 0;
		params[2].Descriptor.RegisterSpace = 0;
		params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rsd = {};
		rsd.NumParameters = _countof(params);
		rsd.pParameters = params;
		rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		ComPtr<ID3DBlob> sig;
		ComPtr<ID3DBlob> err;
		GLR_CHECK(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err));
		GLR_CHECK(g_glRaytracingCmd.device->CreateRootSignature(
			0, sig->GetBufferPointer(), sig->GetBufferSize(),
			IID_PPV_ARGS(&g_glRaytracingLighting.globalRootSig)));
	}

	{
		D3D12_ROOT_SIGNATURE_DESC rsd = {};
		rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		ComPtr<ID3DBlob> sig;
		ComPtr<ID3DBlob> err;
		GLR_CHECK(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err));
		GLR_CHECK(g_glRaytracingCmd.device->CreateRootSignature(
			0, sig->GetBufferPointer(), sig->GetBufferSize(),
			IID_PPV_ARGS(&g_glRaytracingLighting.localRootSig)));
	}

	return 1;
}

static int glRaytracingLightingCreateBuffers(void)
{
	g_glRaytracingLighting.constantBuffer = glRaytracingCreateBuffer(
		g_glRaytracingCmd.device.Get(),
		glRaytracingAlignUp(sizeof(glRaytracingLightingConstants_t), 256),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_FLAG_NONE);

	g_glRaytracingLighting.lightBuffer = glRaytracingCreateBuffer(
		g_glRaytracingCmd.device.Get(),
		sizeof(glRaytracingLight_t) * GL_RAYTRACING_MAX_LIGHTS,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_FLAG_NONE);

	return g_glRaytracingLighting.constantBuffer.resource && g_glRaytracingLighting.lightBuffer.resource;
}

static void glRaytracingLightingUpdateConstants(void)
{
	glRaytracingMapCopy(
		g_glRaytracingLighting.constantBuffer.resource.Get(),
		&g_glRaytracingLighting.constants,
		sizeof(g_glRaytracingLighting.constants));
}

static void glRaytracingLightingUpdateLights(void)
{
	if (g_glRaytracingLighting.cpuLights.empty())
		return;

	glRaytracingMapCopy(
		g_glRaytracingLighting.lightBuffer.resource.Get(),
		g_glRaytracingLighting.cpuLights.data(),
		g_glRaytracingLighting.cpuLights.size() * sizeof(glRaytracingLight_t));
}

static void glRaytracingLightingCreatePersistentLightSRV(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
	srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Format = DXGI_FORMAT_UNKNOWN;
	srv.Buffer.FirstElement = 0;
	srv.Buffer.NumElements = GL_RAYTRACING_MAX_LIGHTS;
	srv.Buffer.StructureByteStride = sizeof(glRaytracingLight_t);
	srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE base = g_glRaytracingLighting.descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	g_glRaytracingCmd.device->CreateShaderResourceView(
		g_glRaytracingLighting.lightBuffer.resource.Get(),
		&srv,
		glRaytracingOffsetCpu(base, g_glRaytracingLighting.descriptorStride, 0));
}

static int glRaytracingLightingCreateStateObject(void)
{
	ComPtr<IDxcBlob> dxil = glRaytracingLightingCompileLibrary(g_glRaytracingLightingHlsl);
	if (!dxil)
		return 0;

	D3D12_EXPORT_DESC exports[3] = {};
	exports[0].Name = L"RayGen";
	exports[1].Name = L"ShadowMiss";
	exports[2].Name = L"ShadowClosestHit";

	D3D12_DXIL_LIBRARY_DESC libDesc = {};
	D3D12_SHADER_BYTECODE libBytecode = {};
	libBytecode.pShaderBytecode = dxil->GetBufferPointer();
	libBytecode.BytecodeLength = dxil->GetBufferSize();
	libDesc.DXILLibrary = libBytecode;
	libDesc.NumExports = _countof(exports);
	libDesc.pExports = exports;

	D3D12_HIT_GROUP_DESC hitGroup = {};
	hitGroup.HitGroupExport = L"ShadowHitGroup";
	hitGroup.ClosestHitShaderImport = L"ShadowClosestHit";
	hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxPayloadSizeInBytes = sizeof(uint32_t);
	shaderConfig.MaxAttributeSizeInBytes = 8;

	D3D12_GLOBAL_ROOT_SIGNATURE globalRS = {};
	globalRS.pGlobalRootSignature = g_glRaytracingLighting.globalRootSig.Get();

	D3D12_LOCAL_ROOT_SIGNATURE localRS = {};
	localRS.pLocalRootSignature = g_glRaytracingLighting.localRootSig.Get();

	D3D12_STATE_SUBOBJECT subobjects[8] = {};
	UINT sub = 0;

	subobjects[sub].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[sub].pDesc = &libDesc;
	++sub;

	subobjects[sub].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobjects[sub].pDesc = &hitGroup;
	++sub;

	subobjects[sub].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subobjects[sub].pDesc = &shaderConfig;
	++sub;

	subobjects[sub].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobjects[sub].pDesc = &globalRS;
	++sub;

	subobjects[sub].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[sub].pDesc = &localRS;
	++sub;

	LPCWSTR localExports[] = { L"RayGen", L"ShadowMiss", L"ShadowHitGroup" };
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assoc = {};
	assoc.pSubobjectToAssociate = &subobjects[4];
	assoc.NumExports = _countof(localExports);
	assoc.pExports = localExports;

	subobjects[sub].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[sub].pDesc = &assoc;
	++sub;

	D3D12_RAYTRACING_PIPELINE_CONFIG pipeConfig = {};
	pipeConfig.MaxTraceRecursionDepth = 1;

	subobjects[sub].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobjects[sub].pDesc = &pipeConfig;
	++sub;

	D3D12_STATE_OBJECT_DESC soDesc = {};
	soDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	soDesc.NumSubobjects = sub;
	soDesc.pSubobjects = subobjects;

	GLR_CHECK(g_glRaytracingCmd.device->CreateStateObject(&soDesc, IID_PPV_ARGS(&g_glRaytracingLighting.rtStateObject)));
	GLR_CHECK(g_glRaytracingLighting.rtStateObject.As(&g_glRaytracingLighting.rtStateProps));
	return 1;
}

static int glRaytracingLightingCreateShaderTables(void)
{
	void* raygenId = g_glRaytracingLighting.rtStateProps->GetShaderIdentifier(L"RayGen");
	void* missId = g_glRaytracingLighting.rtStateProps->GetShaderIdentifier(L"ShadowMiss");
	void* hitId = g_glRaytracingLighting.rtStateProps->GetShaderIdentifier(L"ShadowHitGroup");

	if (!raygenId || !missId || !hitId)
	{
		glRaytracingFatal("Failed to fetch shader identifiers");
		return 0;
	}

	const UINT shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	const UINT recordSize = (UINT)glRaytracingAlignUp(shaderIdSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	g_glRaytracingLighting.raygenTable = glRaytracingCreateBuffer(
		g_glRaytracingCmd.device.Get(),
		recordSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_FLAG_NONE);

	g_glRaytracingLighting.missTable = glRaytracingCreateBuffer(
		g_glRaytracingCmd.device.Get(),
		recordSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_FLAG_NONE);

	g_glRaytracingLighting.hitTable = glRaytracingCreateBuffer(
		g_glRaytracingCmd.device.Get(),
		recordSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_FLAG_NONE);

	if (!g_glRaytracingLighting.raygenTable.resource ||
		!g_glRaytracingLighting.missTable.resource ||
		!g_glRaytracingLighting.hitTable.resource)
	{
		return 0;
	}

	uint8_t temp[256] = {};

	memset(temp, 0, sizeof(temp));
	memcpy(temp, raygenId, shaderIdSize);
	glRaytracingMapCopy(g_glRaytracingLighting.raygenTable.resource.Get(), temp, recordSize);

	memset(temp, 0, sizeof(temp));
	memcpy(temp, missId, shaderIdSize);
	glRaytracingMapCopy(g_glRaytracingLighting.missTable.resource.Get(), temp, recordSize);

	memset(temp, 0, sizeof(temp));
	memcpy(temp, hitId, shaderIdSize);
	glRaytracingMapCopy(g_glRaytracingLighting.hitTable.resource.Get(), temp, recordSize);

	return 1;
}

static void glRaytracingLightingCreatePerPassDescriptors(const glRaytracingLightingPassDesc_t* pass)
{
	D3D12_CPU_DESCRIPTOR_HANDLE base = g_glRaytracingLighting.descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC albedoSrv = {};
	albedoSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	albedoSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	albedoSrv.Format = pass->albedoFormat;
	albedoSrv.Texture2D.MipLevels = 1;
	g_glRaytracingCmd.device->CreateShaderResourceView(
		pass->albedoTexture,
		&albedoSrv,
		glRaytracingOffsetCpu(base, g_glRaytracingLighting.descriptorStride, 1));

	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrv = {};
	depthSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthSrv.Format = glRaytracingGetSrvFormatForDepth(pass->depthFormat);
	depthSrv.Texture2D.MipLevels = 1;
	g_glRaytracingCmd.device->CreateShaderResourceView(
		pass->depthTexture,
		&depthSrv,
		glRaytracingOffsetCpu(base, g_glRaytracingLighting.descriptorStride, 2));

	D3D12_SHADER_RESOURCE_VIEW_DESC normalSrv = {};
	normalSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	normalSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	normalSrv.Format = pass->normalFormat;
	normalSrv.Texture2D.MipLevels = 1;
	g_glRaytracingCmd.device->CreateShaderResourceView(
		pass->normalTexture,
		&normalSrv,
		glRaytracingOffsetCpu(base, g_glRaytracingLighting.descriptorStride, 3));

	D3D12_SHADER_RESOURCE_VIEW_DESC positionSrv = {};
	positionSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	positionSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	positionSrv.Format = pass->positionFormat;
	positionSrv.Texture2D.MipLevels = 1;
	g_glRaytracingCmd.device->CreateShaderResourceView(
		pass->positionTexture,
		&positionSrv,
		glRaytracingOffsetCpu(base, g_glRaytracingLighting.descriptorStride, 4));

	D3D12_SHADER_RESOURCE_VIEW_DESC tlasSrv = {};
	tlasSrv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	tlasSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	tlasSrv.RaytracingAccelerationStructure.Location = pass->topLevelAS->GetGPUVirtualAddress();
	g_glRaytracingCmd.device->CreateShaderResourceView(
		nullptr,
		&tlasSrv,
		glRaytracingOffsetCpu(base, g_glRaytracingLighting.descriptorStride, 5));

	D3D12_UNORDERED_ACCESS_VIEW_DESC outputUav = {};
	outputUav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	outputUav.Format = pass->outputFormat;
	g_glRaytracingCmd.device->CreateUnorderedAccessView(
		pass->outputTexture,
		nullptr,
		&outputUav,
		glRaytracingOffsetCpu(base, g_glRaytracingLighting.descriptorStride, 6));
}

// ============================================================
// Lighting public API
// ============================================================

bool glRaytracingLightingInit(void)
{
	if (g_glRaytracingLighting.initialized)
		return true;

	if (!glRaytracingInitCmdContext())
		return false;

	if (!glRaytracingLightingCreateDescriptorHeap())
		return false;

	if (!glRaytracingLightingCreateRootSignatures())
		return false;

	if (!glRaytracingLightingCreateBuffers())
		return false;

	glRaytracingLightingCreatePersistentLightSRV();

	if (!glRaytracingLightingCreateStateObject())
		return false;

	if (!glRaytracingLightingCreateShaderTables())
		return false;

	memset(&g_glRaytracingLighting.constants, 0, sizeof(g_glRaytracingLighting.constants));
	g_glRaytracingLighting.constants.ambientColor[0] = 0.08f;
	g_glRaytracingLighting.constants.ambientColor[1] = 0.08f;
	g_glRaytracingLighting.constants.ambientColor[2] = 0.09f;
	g_glRaytracingLighting.constants.ambientColor[3] = 1.0f;
	g_glRaytracingLighting.constants.enableSpecular = 1;
	g_glRaytracingLighting.constants.enableHalfLambert = 1;
	g_glRaytracingLighting.constants.normalReconstructZ = 1.0f;
	g_glRaytracingLighting.constants.shadowBias = 1.5f;

	glRaytracingLightingUpdateConstants();

	g_glRaytracingLighting.initialized = true;
	glRaytracingLog("glRaytracingLightingInit ok");
	return true;
}

void glRaytracingLightingShutdown(void)
{
	if (!g_glRaytracingLighting.initialized)
		return;

	g_glRaytracingLighting = glRaytracingLightingState_t();
}

bool glRaytracingLightingIsInitialized(void)
{
	return g_glRaytracingLighting.initialized;
}

void glRaytracingLightingClearLights(bool clearPersistant)
{
	if (clearPersistant)
	{
		g_glRaytracingLighting.cpuLights.clear();
	}
	else
	{
		size_t writeIndex = 0;

		for (size_t i = 0; i < g_glRaytracingLighting.cpuLights.size(); ++i)
		{
			if (g_glRaytracingLighting.cpuLights[i].persistant)
			{
				if (writeIndex != i)
				{
					g_glRaytracingLighting.cpuLights[writeIndex] = g_glRaytracingLighting.cpuLights[i];
				}
				++writeIndex;
			}
		}

		g_glRaytracingLighting.cpuLights.resize(writeIndex);
	}

	g_glRaytracingLighting.constants.lightCount =
		(uint32_t)g_glRaytracingLighting.cpuLights.size();

	glRaytracingLightingUpdateConstants();
}

bool glRaytracingLightingAddLight(const glRaytracingLight_t* light)
{
	if (!g_glRaytracingLighting.initialized || !light)
		return false;

	if (g_glRaytracingLighting.cpuLights.size() >= GL_RAYTRACING_MAX_LIGHTS)
		return false;

	g_glRaytracingLighting.cpuLights.push_back(*light);
	g_glRaytracingLighting.constants.lightCount = (uint32_t)g_glRaytracingLighting.cpuLights.size();

	glRaytracingLightingUpdateLights();
	glRaytracingLightingUpdateConstants();
	return true;
}

void glRaytracingLightingSetAmbient(float r, float g, float b, float intensity)
{
	g_glRaytracingLighting.constants.ambientColor[0] = r;
	g_glRaytracingLighting.constants.ambientColor[1] = g;
	g_glRaytracingLighting.constants.ambientColor[2] = b;
	g_glRaytracingLighting.constants.ambientColor[3] = intensity;
	glRaytracingLightingUpdateConstants();
}

void glRaytracingLightingSetCameraPosition(float x, float y, float z)
{
	g_glRaytracingLighting.constants.cameraPos[0] = x;
	g_glRaytracingLighting.constants.cameraPos[1] = y;
	g_glRaytracingLighting.constants.cameraPos[2] = z;
	g_glRaytracingLighting.constants.cameraPos[3] = 1.0f;
	glRaytracingLightingUpdateConstants();
}

void glRaytracingLightingSetInvViewProjMatrix(const float* m16)
{
	if (!m16)
		return;

	memcpy(g_glRaytracingLighting.constants.invViewProj, m16, sizeof(float) * 16);
	glRaytracingLightingUpdateConstants();
}

void glRaytracingLightingSetInvViewMatrix(const float* m16)
{
	if (!m16)
		return;

	memcpy(g_glRaytracingLighting.constants.invViewMatrix, m16, sizeof(float) * 16);
	glRaytracingLightingUpdateConstants();
}

void glRaytracingLightingSetNormalReconstructSign(float signValue)
{
	g_glRaytracingLighting.constants.normalReconstructZ = signValue;
	glRaytracingLightingUpdateConstants();
}

void glRaytracingLightingEnableSpecular(int enable)
{
	g_glRaytracingLighting.constants.enableSpecular = enable ? 1u : 0u;
	glRaytracingLightingUpdateConstants();
}

void glRaytracingLightingEnableHalfLambert(int enable)
{
	g_glRaytracingLighting.constants.enableHalfLambert = enable ? 1u : 0u;
	glRaytracingLightingUpdateConstants();
}

void glRaytracingLightingSetShadowBias(float bias)
{
	g_glRaytracingLighting.constants.shadowBias = bias;
	glRaytracingLightingUpdateConstants();
}

bool glRaytracingLightingExecute(const glRaytracingLightingPassDesc_t* pass)
{
	if (!g_glRaytracingLighting.initialized || !pass)
		return false;

	if (!pass->albedoTexture ||
		!pass->depthTexture ||
		!pass->normalTexture ||
		!pass->positionTexture ||
		!pass->outputTexture ||
		!pass->topLevelAS)
	{
		return false;
	}

	if (pass->width == 0 || pass->height == 0)
		return false;

	g_glRaytracingLighting.constants.screenSize[0] = (float)pass->width;
	g_glRaytracingLighting.constants.screenSize[1] = (float)pass->height;
	g_glRaytracingLighting.constants.screenSize[2] = 1.0f / (float)pass->width;
	g_glRaytracingLighting.constants.screenSize[3] = 1.0f / (float)pass->height;
	g_glRaytracingLighting.constants.lightCount =
		(uint32_t)glRaytracingClamp<size_t>(g_glRaytracingLighting.cpuLights.size(), 0, GL_RAYTRACING_MAX_LIGHTS);

	glRaytracingLightingUpdateLights();
	glRaytracingLightingUpdateConstants();
	glRaytracingLightingCreatePerPassDescriptors(pass);

	if (!glRaytracingBeginCmd())
		return false;

	glRaytracingTransition(
		g_glRaytracingCmd.cmdList.Get(),
		pass->outputTexture,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	ID3D12DescriptorHeap* heaps[] = { g_glRaytracingLighting.descriptorHeap.Get() };
	g_glRaytracingCmd.cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	g_glRaytracingCmd.cmdList->SetComputeRootSignature(g_glRaytracingLighting.globalRootSig.Get());

	D3D12_GPU_DESCRIPTOR_HANDLE gpuBase = g_glRaytracingLighting.descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	g_glRaytracingCmd.cmdList->SetComputeRootDescriptorTable(
		0,
		glRaytracingOffsetGpu(gpuBase, g_glRaytracingLighting.descriptorStride, 0));
	g_glRaytracingCmd.cmdList->SetComputeRootDescriptorTable(
		1,
		glRaytracingOffsetGpu(gpuBase, g_glRaytracingLighting.descriptorStride, 6));
	g_glRaytracingCmd.cmdList->SetComputeRootConstantBufferView(
		2,
		g_glRaytracingLighting.constantBuffer.gpuVA);

	g_glRaytracingCmd.cmdList->SetPipelineState1(g_glRaytracingLighting.rtStateObject.Get());

	const UINT shaderRecordSize =
		(UINT)glRaytracingAlignUp(
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
			D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	D3D12_DISPATCH_RAYS_DESC rays = {};
	rays.RayGenerationShaderRecord.StartAddress = g_glRaytracingLighting.raygenTable.gpuVA;
	rays.RayGenerationShaderRecord.SizeInBytes = shaderRecordSize;

	rays.MissShaderTable.StartAddress = g_glRaytracingLighting.missTable.gpuVA;
	rays.MissShaderTable.SizeInBytes = shaderRecordSize;
	rays.MissShaderTable.StrideInBytes = shaderRecordSize;

	rays.HitGroupTable.StartAddress = g_glRaytracingLighting.hitTable.gpuVA;
	rays.HitGroupTable.SizeInBytes = shaderRecordSize;
	rays.HitGroupTable.StrideInBytes = shaderRecordSize;

	rays.Width = pass->width;
	rays.Height = pass->height;
	rays.Depth = 1;

	g_glRaytracingCmd.cmdList->DispatchRays(&rays);

	D3D12_RESOURCE_BARRIER uav = {};
	uav.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uav.UAV.pResource = pass->outputTexture;
	g_glRaytracingCmd.cmdList->ResourceBarrier(1, &uav);

	ID3D12Resource* backBuffer = QD3D12_GetCurrentBackBuffer();
	if (backBuffer)
	{
		glRaytracingTransition(
			g_glRaytracingCmd.cmdList.Get(),
			pass->outputTexture,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE);

		glRaytracingTransition(
			g_glRaytracingCmd.cmdList.Get(),
			backBuffer,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_COPY_DEST);

		g_glRaytracingCmd.cmdList->CopyResource(backBuffer, pass->outputTexture);

		glRaytracingTransition(
			g_glRaytracingCmd.cmdList.Get(),
			backBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PRESENT);

		glRaytracingTransition(
			g_glRaytracingCmd.cmdList.Get(),
			pass->outputTexture,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		glRaytracingTransition(
			g_glRaytracingCmd.cmdList.Get(),
			pass->outputTexture,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	if (!glRaytracingEndCmd())
		return false;

	return true;
}

glRaytracingLight_t glRaytracingLightingMakePointLight(
	float px, float py, float pz,
	float radius,
	float r, float g, float b,
	float intensity)
{
	glRaytracingLight_t l = {};
	l.position.x = px;
	l.position.y = py;
	l.position.z = pz;
	l.radius = radius;

	l.color.x = r;
	l.color.y = g;
	l.color.z = b;
	l.intensity = intensity;

	l.normal.x = 0.0f;
	l.normal.y = 0.0f;
	l.normal.z = 1.0f;
	l.type = GL_RAYTRACING_LIGHT_TYPE_POINT;

	l.axisU.x = 1.0f;
	l.axisU.y = 0.0f;
	l.axisU.z = 0.0f;
	l.halfWidth = 0.0f;

	l.axisV.x = 0.0f;
	l.axisV.y = 1.0f;
	l.axisV.z = 0.0f;
	l.halfHeight = 0.0f;

	l.samples = 1;
	l.twoSided = 0;
	l.persistant = 0.0f;
	l.pad1 = 0.0f;
	return l;
}

glRaytracingLight_t glRaytracingLightingMakeRectLight(
	float px, float py, float pz,
	float nx, float ny, float nz,
	float ux, float uy, float uz,
	float vx, float vy, float vz,
	float halfWidth, float halfHeight,
	float r, float g, float b,
	float intensity,
	uint32_t samples,
	uint32_t twoSided)
{
	glRaytracingLight_t l = {};

	glRaytracingNormalize3(nx, ny, nz);
	glRaytracingNormalize3(ux, uy, uz);
	glRaytracingNormalize3(vx, vy, vz);

	if ((nx == 0.0f && ny == 0.0f && nz == 0.0f) &&
		!((ux == 0.0f && uy == 0.0f && uz == 0.0f) ||
			(vx == 0.0f && vy == 0.0f && vz == 0.0f)))
	{
		glRaytracingCross3(ux, uy, uz, vx, vy, vz, nx, ny, nz);
		glRaytracingNormalize3(nx, ny, nz);
	}

	l.position.x = px;
	l.position.y = py;
	l.position.z = pz;

	// Reuse radius as influence/falloff range for the rect light.
	l.radius = (halfWidth > halfHeight ? halfWidth : halfHeight) * 6.0f;

	l.color.x = r;
	l.color.y = g;
	l.color.z = b;
	l.intensity = intensity;

	l.normal.x = nx;
	l.normal.y = ny;
	l.normal.z = nz;
	l.type = GL_RAYTRACING_LIGHT_TYPE_RECT;

	l.axisU.x = ux;
	l.axisU.y = uy;
	l.axisU.z = uz;
	l.halfWidth = halfWidth;

	l.axisV.x = vx;
	l.axisV.y = vy;
	l.axisV.z = vz;
	l.halfHeight = halfHeight;

	l.samples = samples ? samples : 4u;
	l.twoSided = twoSided ? 1u : 0u;
	l.persistant = 0.0f;
	l.pad1 = 0.0f;

	return l;
}

uint32_t glRaytracingLightingGetLightCount(void)
{
	return (uint32_t)g_glRaytracingLighting.cpuLights.size();
}

