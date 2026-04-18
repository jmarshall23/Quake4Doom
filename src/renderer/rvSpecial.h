#pragma once

class idMaterial;
class idImage;
class idPBufferImage;
class rvNewShaderStage;
class viewDef_s;
class viewEntity_s;
class drawSurf_s;
class idRenderSystemLocal;

struct idVec3;
struct idPlane;
struct drawSurfsCommand_t;

class rvBlurTexture {
public:
	rvBlurTexture();

	bool CreateBuffer(const char* name, idPBufferImage*& image);
	void Render(const drawSurfsCommand_t* drawSurfs);
	void Display(const viewEntity_s* viewEnts, bool prePass);

public:
	idPBufferImage* mDepthImage;
	idPBufferImage* mBlurImage[1];
	const idMaterial* mDepthMaterial;
	float* regs;
	float				shaderParms[8];
};

class rvAL {
public:
	rvAL();

	bool CreateBuffer(const char* name, idPBufferImage*& image);
	void Render(const drawSurfsCommand_t* drawSurfs);
	void Display(const viewEntity_s* viewEnts, bool prePass);
	void DrawLight(idVec3& origin, float size, idVec3& color);

public:
	static const int MAX_LIGHTS = 100;

	idPBufferImage* mDepthImage;
	idPBufferImage* mBlurImage[1];
	const idMaterial* mDepthMaterial;
	float* regs;
	float				shaderParms[8];

	idVec3				lOrigin[MAX_LIGHTS];
	idVec3				lColor[MAX_LIGHTS];
	float				lSize[MAX_LIGHTS];

	float				offset;
	int					count;

	rvNewShaderStage* mainStage;
	int					mLightLocParm;
	int					mLightColorParm;
	int					mLightSizeParm;
	int					mLightMinDistanceParm;
};

// globals
extern rvBlurTexture* DepthTexture;
extern rvAL* ptr;
extern const idMaterial* ALMaterial;
extern int				numDrawn;

// renderer support
void RB_RestoreDrawingView();
void R_AddSpecialEffects(viewDef_s* parms);
void RB_T_FillDepthTexture(const drawSurf_s* surf);
void RB_T_FillDepthTextureAL(const drawSurf_s* surf);
void RB_SetGL2D2();
void R_ShutdownSpecialEffects();
void RB_DrawDepthTexture(const drawSurfsCommand_t* data);
void RB_DisplaySpecialEffects(const viewEntity_s* viewEnts, bool prePass);