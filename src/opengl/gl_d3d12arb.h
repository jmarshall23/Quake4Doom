#pragma once

static const UINT QD3D12_ARB_MAX_PROGRAM_PARAMETERS = 128;
static const UINT QD3D12_ARB_MAX_VERTEX_ATTRIBS = 16;
static const UINT QD3D12_ARB_MAX_TEXTURE_UNITS = 8;

struct QD3D12ARBDrawConstantArrays
{
	float env[QD3D12_ARB_MAX_PROGRAM_PARAMETERS][4];
	float vertexLocal[QD3D12_ARB_MAX_PROGRAM_PARAMETERS][4];
	float fragmentLocal[QD3D12_ARB_MAX_PROGRAM_PARAMETERS][4];
};


void        QD3D12ARB_Init(void);
void        QD3D12ARB_Shutdown(void);
void        QD3D12ARB_DeviceLost(void);

void        QD3D12ARB_SetEnabled(GLenum cap, bool enabled);
bool        QD3D12ARB_IsVertexEnabled(void);
bool        QD3D12ARB_IsFragmentEnabled(void);
bool        QD3D12ARB_IsActive(void);

GLuint      QD3D12ARB_GetBoundVertexProgram(void);
GLuint      QD3D12ARB_GetBoundFragmentProgram(void);
uint32_t    QD3D12ARB_GetBoundVertexRevision(void);
uint32_t    QD3D12ARB_GetBoundFragmentRevision(void);

ID3DBlob* QD3D12ARB_GetVertexShaderBlob(void);
ID3DBlob* QD3D12ARB_GetFragmentShaderBlob(void);
void        QD3D12ARB_FillDrawConstantArrays(QD3D12ARBDrawConstantArrays* outArrays);

GLenum      QD3D12ARB_ConsumeError(void);
GLint       QD3D12ARB_GetProgramErrorPosition(void);
const char* QD3D12ARB_GetProgramErrorString(void);

