#include "opengl.h"

// Your D3D12 wrapper exports
extern bool QD3D12_InitForQuakeWindow(struct QD3D12Window* window, HWND hwnd, int width, int height, bool fastPath);
extern void QD3D12_ShutdownForQuake(void);
extern void QD3D12_BeginFrame();
void APIENTRY glSelectTextureSGIS(GLenum texture);
void APIENTRY glMTexCoord2fSGIS(GLenum texture, GLfloat s, GLfloat t);
void APIENTRY glActiveTextureARB(GLenum texture);
void APIENTRY glMultiTexCoord2fARB(GLenum texture, GLfloat s, GLfloat t);
void QD3D12_Resize();

void QD3D12_Present();
void QD3D12_EndFrame();
void QD3D12_CollectRetiredResources();

// Existing GL wrapper function from your compatibility layer
void APIENTRY glBindTexture(unsigned int target, unsigned int texture);

void QD3D12_SetCurrentWindow(struct QD3D12Window* window);

struct QD3D12FakeContext
{
    HDC   dc;
    HWND  hwnd;
    bool  initialized;
    struct QD3D12Window* window;
};

static QD3D12FakeContext* g_currentContext = nullptr;
static HDC                g_currentDC = nullptr;

struct QD3D12Window* AllocD3D12Window();
void FreeD3D12Window(struct QD3D12Window* wnd);

static void QD3D12_GetClientSize(HWND hwnd, int& w, int& h)
{
    RECT rc = {};
    GetClientRect(hwnd, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;

    if (w <= 0) w = 640;
    if (h <= 0) h = 480;
}

QD3D12_HGLRC WINAPI qd3d12_wglCreateContext(HDC hdc)
{
    if (!hdc)
        return nullptr;

    HWND hwnd = WindowFromDC(hdc);
    if (!hwnd)
        return nullptr;

    QD3D12FakeContext* ctx = new QD3D12FakeContext();
    ctx->dc = hdc;
    ctx->hwnd = hwnd;
    ctx->initialized = false;
    ctx->window = AllocD3D12Window();
    return (QD3D12_HGLRC)ctx;
}

BOOL WINAPI qd3d12_wglMakeCurrent(HDC hdc, QD3D12_HGLRC hglrc)
{
    static bool basicInitDone = false;

    if (!hdc || !hglrc)
    {
        g_currentDC = nullptr;
        g_currentContext = nullptr;
        QD3D12_SetCurrentWindow(NULL);
        return TRUE;
    }

    QD3D12FakeContext* ctx = (QD3D12FakeContext*)hglrc;
    ctx->dc = hdc;

    
    int w = 640, h = 480;
    QD3D12_GetClientSize(ctx->hwnd, w, h);

    if (!ctx->initialized)
    {
		if (!QD3D12_InitForQuakeWindow(ctx->window, ctx->hwnd, w, h, basicInitDone))
			return FALSE;
    }
    

	if (!basicInitDone)
	{
        if(!glRaytracingInit())
            return FALSE;

        if (!glRaytracingLightingInit())
            return FALSE;

        basicInitDone = true;
       
        QD3D12_BeginFrame();
    }

	if (ctx->initialized) {
//		glFinish();
		QD3D12_EndFrame();
	}

    g_currentDC = hdc;
    g_currentContext = ctx;
    
    QD3D12_SetCurrentWindow(ctx->window);
    QD3D12_Resize();

	if (ctx->initialized) {
		QD3D12_BeginFrame();
		QD3D12_CollectRetiredResources();
	}

	ctx->initialized = true;
    return TRUE;
}

HDC WINAPI qd3d12_wglGetCurrentDC(void)
{
    return g_currentDC;
}

QD3D12_HGLRC WINAPI qd3d12_wglGetCurrentContext(void)
{
    return (QD3D12_HGLRC)g_currentContext;
}

BOOL WINAPI qd3d12_wglDeleteContext(QD3D12_HGLRC hglrc)
{
    if (!hglrc)
        return FALSE;

    QD3D12FakeContext* ctx = (QD3D12FakeContext*)hglrc;

    if (ctx == g_currentContext)
    {
        if (ctx->initialized)
            QD3D12_ShutdownForQuake();

        g_currentContext = nullptr;
        g_currentDC = nullptr;
    }

    FreeD3D12Window(ctx->window);

    delete ctx;
    return TRUE;
}

void APIENTRY glBindTextureEXT(unsigned int target, unsigned int texture)
{
    glBindTexture(target, texture);
}

int WINAPI qd3d12_wglDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd) {
    return DescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
}

BOOL WINAPI qd3d12_wglSetPixelFormat(HDC hdc, int format, const PIXELFORMATDESCRIPTOR* ppfd) {
    return 0;
}

BOOL WINAPI qd3d12_wglSwapIntervalEXT(int interval) {
    (void)interval;
    return TRUE;
}

BOOL WINAPI qd3d12_wglGetDeviceGammaRamp3DFX(HDC hdc, LPVOID ramp) {
    return FALSE;
}

BOOL WINAPI qd3d12_wglSetDeviceGammaRamp3DFX(HDC hdc, LPVOID ramp) {
    return FALSE;
}

__declspec(dllexport) BOOL  WINAPI wglUseFontBitmapsA(HDC, DWORD, DWORD, DWORD) { return FALSE; }
 __declspec(dllexport) BOOL  WINAPI wglUseFontBitmapsW(HDC, DWORD, DWORD, DWORD) { return FALSE; }