/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"

#include "cg_explicit.h"

CGcontext cg_context;

#define Q4_ARB2_ENABLE_RVMESH _MD5R_SUPPORT

const rvVertexFormat supportedMD5RVertexFormats[3];
const rvVertexFormat supportedMD5RDepthVertexFormats[3];
const rvVertexFormat supportedMD5RShadowVolVertexFormats[3];


// Matches the Quake 4 renderer cvar name.  Default is off so a stock Doom 3 tree
// keeps its original interaction shader unless glprogs/SimpleInteraction.vfp exists
// and the user explicitly enables it.
idCVar r_useSimpleInteraction(
	"r_useSimpleInteraction",
	"0",
	CVAR_RENDERER | CVAR_BOOL,
	"use the Quake 4 ARB2 SimpleInteraction.vfp program when it is available"
);

idCVar r_q4SpecularColorX2(
	"r_q4SpecularColorX2",
	"1",
	CVAR_RENDERER | CVAR_BOOL,
	"Quake 4 ARB2 behavior: upload specular interaction color multiplied by two"
);

idCVar r_q4InteractionPolygonOffset(
	"r_q4InteractionPolygonOffset",
	"1",
	CVAR_RENDERER | CVAR_BOOL,
	"Quake 4 ARB2 behavior: honor material polygon offset around interaction draws"
);

idCVar r_q4AutoSimpleLighting(
	"r_q4AutoSimpleLighting",
	"1",
	CVAR_RENDERER | CVAR_BOOL,
	"prefer r_useSimpleInteraction on renderer strings Quake 4 treated as simple-lighting cards"
);

// OpenGL program object identifiers.  Doom 3 dynamically assigns PROG_USER + progIndex
// for material programs, so keep the Quake 4 additions far outside that range.
enum {
	Q4_VPROG_SIMPLE_INTERACTION = PROG_USER + 512,
	Q4_FPROG_SIMPLE_INTERACTION,

	Q4_VPROG_MD5R_INTERACTION_4,
	Q4_VPROG_MD5R_INTERACTION_1,
	Q4_VPROG_MD5R_INTERACTION,

	Q4_VPROG_MD5R_SIMPLE_4,
	Q4_VPROG_MD5R_SIMPLE_1,
	Q4_VPROG_MD5R_SIMPLE,

	Q4_VPROG_MD5R_STD_TEX_4,
	Q4_VPROG_MD5R_STD_TEX_1,
	Q4_VPROG_MD5R_STD_TEX,

	Q4_VPROG_MD5R_SKYBOX_4,
	Q4_VPROG_MD5R_SKYBOX_1,
	Q4_VPROG_MD5R_SKYBOX,

	Q4_VPROG_MD5R_ENV_NORMAL_4,
	Q4_VPROG_MD5R_ENV_NORMAL_1,
	Q4_VPROG_MD5R_ENV_NORMAL,

	Q4_VPROG_MD5R_ENV_REFLECT_4,
	Q4_VPROG_MD5R_ENV_REFLECT_1,
	Q4_VPROG_MD5R_ENV_REFLECT,

	Q4_VPROG_MD5R_ENV_BUMP_4,
	Q4_VPROG_MD5R_ENV_BUMP_1,
	Q4_VPROG_MD5R_ENV_BUMP,

	Q4_VPROG_MD5R_SHADOW_4,
	Q4_VPROG_MD5R_SHADOW_1,
	Q4_VPROG_MD5R_SHADOW,

	Q4_VPROG_MD5R_BASIC_FOG_4,
	Q4_VPROG_MD5R_BASIC_FOG_1,
	Q4_VPROG_MD5R_BASIC_FOG
};

static bool q4SimpleInteractionVPAvailable = false;
static bool q4SimpleInteractionFPAvailable = false;

static const int Q4_PP_MVP_X = 0x4B;	// 75
static const int Q4_PP_MVP_Y = 0x4C;
static const int Q4_PP_MVP_Z = 0x4D;
static const int Q4_PP_MVP_W = 0x4E;
static const int Q4_PP_LOCAL_LIGHT = 0x4F;
static const int Q4_PP_LOCAL_VIEW_ORIGIN = 0x50;
static const int Q4_PP_PROJECTION_X = 0x51;
static const int Q4_PP_PROJECTION_Y = 0x52;
static const int Q4_PP_PROJECTION_Z = 0x53;
static const int Q4_PP_PROJECTION_W = 0x54;
static const int Q4_PP_TEXTURE_MATRIX_S = 0x55;
static const int Q4_PP_TEXTURE_MATRIX_T = 0x56;
static const int Q4_PP_MODEL_MATRIX_X = 0x57;
static const int Q4_PP_MODEL_MATRIX_Y = 0x58;
static const int Q4_PP_MODEL_MATRIX_Z = 0x59;
static const int Q4_PP_STAGE_COLOR_MODULATE = 0x5B;
static const int Q4_PP_STAGE_COLOR_ADD = 0x5C;
static const int Q4_MD5R_INTERACTION_PARAM_BASE = 0x4B;	// Quake 4 offsets regular interaction params by this for rvMesh

static const float q4TexIdentityS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
static const float q4TexIdentityT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };

static bool RB_Q4_ContainsNoCase(const char* text, const char* needle) {
	if (!text || !needle || !needle[0]) {
		return false;
	}

	for (const char* p = text; *p; ++p) {
		const char* a = p;
		const char* b = needle;

		while (*a && *b) {
			char ca = *a;
			char cb = *b;

			if (ca >= 'A' && ca <= 'Z') {
				ca = ca - 'A' + 'a';
			}
			if (cb >= 'A' && cb <= 'Z') {
				cb = cb - 'A' + 'a';
			}
			if (ca != cb) {
				break;
			}
			++a;
			++b;
		}

		if (!*b) {
			return true;
		}
	}

	return false;
}

static bool RB_Q4_HasFileExtension(const char* name, const char* extension) {
	if (!name || !extension) {
		return false;
	}

	const int nameLen = strlen(name);
	const int extLen = strlen(extension);
	if (extLen <= 0 || nameLen < extLen) {
		return false;
	}

	return idStr::Icmp(name + nameLen - extLen, extension) == 0;
}

static bool RB_Q4_RendererPrefersSimpleLighting(void) {
	const char* renderer = glConfig.renderer_string;
	if (!renderer) {
		return false;
	}

	// Quake 4 marked Radeon 9600/9700-class renderers as preferring the
	// simpler ARB2 lighting path.  Keep this as an opt-in hint for Doom 3.
	if (RB_Q4_ContainsNoCase(renderer, "RADEON") &&
		(RB_Q4_ContainsNoCase(renderer, "9700") || RB_Q4_ContainsNoCase(renderer, "9600"))) {
		return true;
	}

	return false;
}

static bool RB_Q4_SimpleInteractionAvailable(void) {
	return q4SimpleInteractionVPAvailable && q4SimpleInteractionFPAvailable;
}

static void RB_Q4_MarkProgramLoaded(GLuint ident, bool loaded) {
	if (ident == Q4_VPROG_SIMPLE_INTERACTION) {
		q4SimpleInteractionVPAvailable = loaded;
	}
	else if (ident == Q4_FPROG_SIMPLE_INTERACTION) {
		q4SimpleInteractionFPAvailable = loaded;
	}
}

static void RB_Q4_MaybeInteractionPolygonOffset(const drawInteraction_t* din, bool enable) {
	if (!r_q4InteractionPolygonOffset.GetBool() || !din || !din->surf || !din->surf->material) {
		return;
	}

	if (!din->surf->material->TestMaterialFlag(MF_POLYGONOFFSET)) {
		return;
	}

	if (enable) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat());
	}
	else {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

static void RB_Q4_LoadMatrixRowIntoVPParam(const float m[16], int row, int vpParam) {
	float parm[4];

	parm[0] = m[row + 0];
	parm[1] = m[row + 4];
	parm[2] = m[row + 8];
	parm[3] = m[row + 12];

	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParam, parm);
}

void RB_ARB2_LoadMVPMatrixIntoVPParams(const drawSurf_t* surf) {
	if (!surf || !surf->space || !backEnd.viewDef) {
		return;
	}

	float mvp[16];
	myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mvp);

	RB_Q4_LoadMatrixRowIntoVPParam(mvp, 0, Q4_PP_MVP_X);
	RB_Q4_LoadMatrixRowIntoVPParam(mvp, 1, Q4_PP_MVP_Y);
	RB_Q4_LoadMatrixRowIntoVPParam(mvp, 2, Q4_PP_MVP_Z);
	RB_Q4_LoadMatrixRowIntoVPParam(mvp, 3, Q4_PP_MVP_W);
}

void RB_ARB2_LoadProjectionMatrixIntoVPParams(void) {
	if (!backEnd.viewDef) {
		return;
	}

	RB_Q4_LoadMatrixRowIntoVPParam(backEnd.viewDef->projectionMatrix, 0, Q4_PP_PROJECTION_X);
	RB_Q4_LoadMatrixRowIntoVPParam(backEnd.viewDef->projectionMatrix, 1, Q4_PP_PROJECTION_Y);
	RB_Q4_LoadMatrixRowIntoVPParam(backEnd.viewDef->projectionMatrix, 2, Q4_PP_PROJECTION_Z);
	RB_Q4_LoadMatrixRowIntoVPParam(backEnd.viewDef->projectionMatrix, 3, Q4_PP_PROJECTION_W);
}

void RB_ARB2_LoadShaderTextureMatrixIntoVPParams(const float* shaderRegisters, const textureStage_t* texture) {
	if (!shaderRegisters || !texture) {
		return;
	}

	float matrix[16];
	RB_GetShaderTextureMatrix(shaderRegisters, texture, matrix);

	RB_Q4_LoadMatrixRowIntoVPParam(matrix, 0, Q4_PP_TEXTURE_MATRIX_S);
	RB_Q4_LoadMatrixRowIntoVPParam(matrix, 1, Q4_PP_TEXTURE_MATRIX_T);
}

void RB_ARB2_LoadModelMatrixIntoVPParams(const drawSurf_t* surf) {
	if (!surf || !surf->space) {
		return;
	}

	RB_Q4_LoadMatrixRowIntoVPParam(surf->space->modelMatrix, 0, Q4_PP_MODEL_MATRIX_X);
	RB_Q4_LoadMatrixRowIntoVPParam(surf->space->modelMatrix, 1, Q4_PP_MODEL_MATRIX_Y);
	RB_Q4_LoadMatrixRowIntoVPParam(surf->space->modelMatrix, 2, Q4_PP_MODEL_MATRIX_Z);
}

void RB_ARB2_LoadModelViewMatrixIntoVPParams(const drawSurf_t* surf) {
	if (!surf || !surf->space) {
		return;
	}

	RB_Q4_LoadMatrixRowIntoVPParam(surf->space->modelViewMatrix, 0, Q4_PP_MODEL_MATRIX_X);
	RB_Q4_LoadMatrixRowIntoVPParam(surf->space->modelViewMatrix, 1, Q4_PP_MODEL_MATRIX_Y);
	RB_Q4_LoadMatrixRowIntoVPParam(surf->space->modelViewMatrix, 2, Q4_PP_MODEL_MATRIX_Z);
}

void RB_ARB2_LoadLocalViewOriginIntoVPParams(const drawSurf_t* surf) {
	if (!surf || !surf->space || !backEnd.viewDef) {
		return;
	}

	idVec3 localPos;
	float parm[4];

	R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localPos);

	parm[0] = localPos[0];
	parm[1] = localPos[1];
	parm[2] = localPos[2];
	parm[3] = 1.0f;

	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_LOCAL_VIEW_ORIGIN, parm);
}


static void cg_error_callback(void) {
	CGerror i = cgGetError();
	common->Printf("Cg error (%d): %s\n", i, cgGetErrorString(i));
}


#if defined( Q4_ARB2_ENABLE_RVMESH )
static void RB_ARB2_MD5R_SetupInteractionVertexProgram(const drawSurf_t* surf, bool useSimpleInteraction);
void RB_ARB2_MD5R_DrawDepthElements(const drawSurf_t* surf);
void RB_ARB2_MD5R_DrawShadowElements(const drawSurf_t* surf, int numIndices);
void RB_ARB2_PrepareStageTexturing(const shaderStage_t* pStage, const drawSurf_t* surf, bool fillingDepth);
void RB_ARB2_DisableStageTexturing(const shaderStage_t* pStage, const drawSurf_t* surf);
#endif

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
====================
GL_SelectTextureNoClient
====================
*/
static void GL_SelectTextureNoClient(int unit) {
	backEnd.glState.currenttmu = unit;
	glActiveTextureARB(GL_TEXTURE0_ARB + unit);
	RB_LogComment("glActiveTextureARB( %i )\n", unit);
}

/*
==================
RB_ARB2_DrawInteraction
==================
*/
void	RB_ARB2_DrawInteraction(const drawInteraction_t* din) {
	int vpParamOffset = 0;

#if defined( Q4_ARB2_ENABLE_RVMESH )
	// Quake 4 uses a second constant-register bank for Raven rvMesh / MD5R surfaces.
	if (din && din->surf && din->surf->geo && din->surf->geo->primBatchMesh) {
		RB_ARB2_LoadMVPMatrixIntoVPParams(din->surf);
		vpParamOffset = Q4_MD5R_INTERACTION_PARAM_BASE;
	}
#endif

	// load all the vertex program parameters
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_LIGHT_ORIGIN, din->localLightOrigin.ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_VIEW_ORIGIN, din->localViewOrigin.ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_LIGHT_PROJECT_S, din->lightProjection[0].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_LIGHT_PROJECT_T, din->lightProjection[1].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_LIGHT_PROJECT_Q, din->lightProjection[2].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_LIGHT_FALLOFF_S, din->lightProjection[3].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_BUMP_MATRIX_S, din->bumpMatrix[0].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_BUMP_MATRIX_T, din->bumpMatrix[1].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_DIFFUSE_MATRIX_S, din->diffuseMatrix[0].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_DIFFUSE_MATRIX_T, din->diffuseMatrix[1].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_SPECULAR_MATRIX_S, din->specularMatrix[0].ToFloatPtr());
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_SPECULAR_MATRIX_T, din->specularMatrix[1].ToFloatPtr());

	// testing fragment based normal mapping
	if (r_testARBProgram.GetBool()) {
		glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, din->localLightOrigin.ToFloatPtr());
		glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 3, din->localViewOrigin.ToFloatPtr());
	}

	static const float zero[4] = { 0, 0, 0, 0 };
	static const float one[4] = { 1, 1, 1, 1 };
	static const float negOne[4] = { -1, -1, -1, -1 };

	switch (din->vertexColor) {
	case SVC_IGNORE:
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_COLOR_MODULATE, zero);
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_COLOR_ADD, one);
		break;
	case SVC_MODULATE:
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_COLOR_MODULATE, one);
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_COLOR_ADD, zero);
		break;
	case SVC_INVERSE_MODULATE:
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_COLOR_MODULATE, negOne);
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, vpParamOffset + PP_COLOR_ADD, one);
		break;
	}

	// set the constant colors
	glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, din->diffuseColor.ToFloatPtr());

	if (r_q4SpecularColorX2.GetBool()) {
		float specularColorX2[4];
		const float* src = din->specularColor.ToFloatPtr();

		specularColorX2[0] = src[0] + src[0];
		specularColorX2[1] = src[1] + src[1];
		specularColorX2[2] = src[2] + src[2];
		specularColorX2[3] = src[3] + src[3];

		glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, specularColorX2);
	}
	else {
		glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, din->specularColor.ToFloatPtr());
	}

	// set the textures

	// texture 1 will be the per-surface bump map
	GL_SelectTextureNoClient(1);
	din->bumpImage->Bind();

	// texture 2 will be the light falloff texture
	GL_SelectTextureNoClient(2);
	din->lightFalloffImage->Bind();

	// texture 3 will be the light projection texture
	GL_SelectTextureNoClient(3);
	din->lightImage->Bind();

	// texture 4 is the per-surface diffuse map
	GL_SelectTextureNoClient(4);
	din->diffuseImage->Bind();

	// texture 5 is the per-surface specular map
	GL_SelectTextureNoClient(5);
	din->specularImage->Bind();

	// draw it
	RB_Q4_MaybeInteractionPolygonOffset(din, true);
	RB_DrawElementsWithCounters(din->surf->geo);
	RB_Q4_MaybeInteractionPolygonOffset(din, false);
}


/*
=============
RB_ARB2_CreateDrawInteractions

=============
*/
void RB_ARB2_CreateDrawInteractions(const drawSurf_t* surf) {
	if (!surf) {
		return;
	}

	// perform setup here that will be constant for all interactions
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc);

	const bool useSimpleInteraction = (!r_testARBProgram.GetBool() &&
		r_useSimpleInteraction.GetBool() && RB_Q4_SimpleInteractionAvailable());

	// bind the vertex program
	if (r_testARBProgram.GetBool()) {
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VPROG_TEST);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST);
	}
	else if (useSimpleInteraction) {
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, Q4_VPROG_SIMPLE_INTERACTION);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, Q4_FPROG_SIMPLE_INTERACTION);
	}
	else {
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION);
	}

	glEnable(GL_VERTEX_PROGRAM_ARB);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);

	// enable the vertex arrays
	glEnableVertexAttribArrayARB(8);
	glEnableVertexAttribArrayARB(9);
	glEnableVertexAttribArrayARB(10);
	glEnableVertexAttribArrayARB(11);
	glEnableClientState(GL_COLOR_ARRAY);

	// texture 0 is the normalization cube map for the vector towards the light
	GL_SelectTextureNoClient(0);
	if (backEnd.vLight->lightShader->IsAmbientLight()) {
		globalImages->ambientNormalMap->Bind();
	}
	else {
		globalImages->normalCubeMapImage->Bind();
	}

	// texture 6 is the specular lookup table for the original Doom 3 interaction
	// shader.  The Quake 4 SimpleInteraction path does not require it.
	if (!useSimpleInteraction) {
		GL_SelectTextureNoClient(6);
		if (r_testARBProgram.GetBool()) {
			globalImages->specular2DTableImage->Bind();	// variable specularity in alpha channel
		}
		else {
			globalImages->specularTableImage->Bind();
		}
	}

	for (; surf; surf = surf->nextOnLight) {
#if defined( Q4_ARB2_ENABLE_RVMESH )
		// Raven's Quake 4 renderer routes rvMesh/MD5R surfaces through vertex
		// buffers and MD5R-specific ARB vertex programs.  This branch is only
		// valid after Doom 3's srfTriangles_t has been extended with primBatchMesh
		// and skinToModelTransforms, and after rvMesh/rvVertexFormat support has
		// been added to the renderer.
		if (surf->geo && surf->geo->primBatchMesh) {
			RB_ARB2_MD5R_SetupInteractionVertexProgram(surf, useSimpleInteraction);
			RB_CreateSingleDrawInteractions(surf, RB_ARB2_DrawInteraction);
			continue;
		}
#endif

		// perform setup here that will not change over multiple interaction passes

		// set the vertex pointers
		idDrawVert* ac = (idDrawVert*)vertexCache.Position(surf->geo->ambientCache);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(idDrawVert), ac->color);
		glVertexAttribPointerARB(11, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
		glVertexAttribPointerARB(10, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());
		glVertexAttribPointerARB(9, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
		glVertexAttribPointerARB(8, 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());
		glVertexPointer(3, GL_FLOAT, sizeof(idDrawVert), ac->xyz.ToFloatPtr());

		// this may cause RB_ARB2_DrawInteraction to be executed multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions(surf, RB_ARB2_DrawInteraction);
	}

	glDisableVertexAttribArrayARB(8);
	glDisableVertexAttribArrayARB(9);
	glDisableVertexAttribArrayARB(10);
	glDisableVertexAttribArrayARB(11);
	glDisableClientState(GL_COLOR_ARRAY);

	// disable features
	GL_SelectTextureNoClient(6);
	globalImages->BindNull();

	GL_SelectTextureNoClient(5);
	globalImages->BindNull();

	GL_SelectTextureNoClient(4);
	globalImages->BindNull();

	GL_SelectTextureNoClient(3);
	globalImages->BindNull();

	GL_SelectTextureNoClient(2);
	globalImages->BindNull();

	GL_SelectTextureNoClient(1);
	globalImages->BindNull();

	backEnd.glState.currenttmu = -1;
	GL_SelectTexture(0);

	glDisable(GL_VERTEX_PROGRAM_ARB);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

/*
==================
RB_ARB2_DrawInteractions
==================
*/
void RB_ARB2_DrawInteractions(void) {
	viewLight_t* vLight;
	const idMaterial* lightShader;

	GL_SelectTexture(0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	//
	// for each light, perform adding and shadowing
	//
	for (vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next) {
		backEnd.vLight = vLight;

		// do fogging later
		if (vLight->lightShader->IsFogLight()) {
			continue;
		}
		if (vLight->lightShader->IsBlendLight()) {
			continue;
		}

		if (!vLight->localInteractions && !vLight->globalInteractions
			&& !vLight->translucentInteractions) {
			continue;
		}

		lightShader = vLight->lightShader;

		// clear the stencil buffer if needed
		if (vLight->globalShadows || vLight->localShadows) {
			backEnd.currentScissor = vLight->scissorRect;
			if (r_useScissor.GetBool()) {
				glScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
			}
			glClear(GL_STENCIL_BUFFER_BIT);
		}
		else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			glStencilFunc(GL_ALWAYS, 128, 255);
		}

		if (r_useShadowVertexProgram.GetBool()) {
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW);
			RB_StencilShadowPass(vLight->globalShadows);
			RB_ARB2_CreateDrawInteractions(vLight->localInteractions);
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW);
			RB_StencilShadowPass(vLight->localShadows);
			RB_ARB2_CreateDrawInteractions(vLight->globalInteractions);
			glDisable(GL_VERTEX_PROGRAM_ARB);	// if there weren't any globalInteractions, it would have stayed on
		}
		else {
			RB_StencilShadowPass(vLight->globalShadows);
			RB_ARB2_CreateDrawInteractions(vLight->localInteractions);
			RB_StencilShadowPass(vLight->localShadows);
			RB_ARB2_CreateDrawInteractions(vLight->globalInteractions);
		}

		// translucent surfaces never get stencil shadowed
		if (r_skipTranslucent.GetBool()) {
			continue;
		}

		glStencilFunc(GL_ALWAYS, 128, 255);

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_ARB2_CreateDrawInteractions(vLight->translucentInteractions);

		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	glStencilFunc(GL_ALWAYS, 128, 255);

	GL_SelectTexture(0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}


#if defined( Q4_ARB2_ENABLE_RVMESH )

/*
=========================================================================================

QUAKE 4 MD5R / rvMesh ARB2 HOOKS

Stock Doom 3 GPL does not ship Raven's rvMesh batching path.  Enable this block only
after adding the Raven-side mesh structures and the matching glprogs to the renderer.

Expected external pieces:
	- srfTriangles_t::primBatchMesh
	- srfTriangles_t::skinToModelTransforms
	- rvMesh and rvVertexFormat
	- supportedMD5RVertexFormats / supportedMD5RDepthVertexFormats /
	  supportedMD5RShadowVolVertexFormats arrays

=========================================================================================
*/

static const GLuint q4Md5rInteractionPrograms[3] = {
	Q4_VPROG_MD5R_INTERACTION_4,
	Q4_VPROG_MD5R_INTERACTION_1,
	Q4_VPROG_MD5R_INTERACTION
};

static const GLuint q4Md5rSimplePrograms[3] = {
	Q4_VPROG_MD5R_SIMPLE_4,
	Q4_VPROG_MD5R_SIMPLE_1,
	Q4_VPROG_MD5R_SIMPLE
};

static const GLuint q4Md5rStdTexPrograms[3] = {
	Q4_VPROG_MD5R_STD_TEX_4,
	Q4_VPROG_MD5R_STD_TEX_1,
	Q4_VPROG_MD5R_STD_TEX
};

static const GLuint q4Md5rSkyBoxPrograms[3] = {
	Q4_VPROG_MD5R_SKYBOX_4,
	Q4_VPROG_MD5R_SKYBOX_1,
	Q4_VPROG_MD5R_SKYBOX
};

static const GLuint q4Md5rEnvNormalPrograms[3] = {
	Q4_VPROG_MD5R_ENV_NORMAL_4,
	Q4_VPROG_MD5R_ENV_NORMAL_1,
	Q4_VPROG_MD5R_ENV_NORMAL
};

static const GLuint q4Md5rEnvReflectPrograms[3] = {
	Q4_VPROG_MD5R_ENV_REFLECT_4,
	Q4_VPROG_MD5R_ENV_REFLECT_1,
	Q4_VPROG_MD5R_ENV_REFLECT
};

static const GLuint q4Md5rEnvBumpPrograms[3] = {
	Q4_VPROG_MD5R_ENV_BUMP_4,
	Q4_VPROG_MD5R_ENV_BUMP_1,
	Q4_VPROG_MD5R_ENV_BUMP
};

static const GLuint q4Md5rShadowPrograms[3] = {
	Q4_VPROG_MD5R_SHADOW_4,
	Q4_VPROG_MD5R_SHADOW_1,
	Q4_VPROG_MD5R_SHADOW
};

static int RB_Q4_FindMD5RFormatIndex(const rvVertexFormat* format, const rvVertexFormat* supported, int count) {
	if (!format || !supported) {
		return -1;
	}

	for (int i = 0; i < count; ++i) {
		if (format->HasSameComponents(supported[i])) {
			return i;
		}
	}

	return -1;
}

static void RB_ARB2_MD5R_SetupInteractionVertexProgram(const drawSurf_t* surf, bool useSimpleInteraction) {
	if (!surf || !surf->geo || !surf->geo->primBatchMesh) {
		return;
	}

	rvMesh* mesh = surf->geo->primBatchMesh;
	const rvVertexFormat* format = &mesh->GetDrawVertexBufferFormat()->m_format;
	const int formatIndex = RB_Q4_FindMD5RFormatIndex(format, supportedMD5RVertexFormats, 3);

	if (formatIndex < 0) {
		return;
	}

	glBindProgramARB(GL_VERTEX_PROGRAM_ARB,
		useSimpleInteraction ? q4Md5rSimplePrograms[formatIndex] : q4Md5rInteractionPrograms[formatIndex]);

	mesh->SetupForDrawRender(&supportedMD5RVertexFormats[formatIndex]);
}

void RB_ARB2_MD5R_DrawDepthElements(const drawSurf_t* surf) {
	if (!surf || !surf->geo || !surf->geo->primBatchMesh) {
		return;
	}

	rvMesh* mesh = surf->geo->primBatchMesh;
	const rvVertexFormat* format = &mesh->GetDrawVertexBufferFormat()->m_format;
	const int formatIndex = RB_Q4_FindMD5RFormatIndex(format, supportedMD5RDepthVertexFormats, 3);

	if (formatIndex < 0) {
		return;
	}

	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, q4Md5rSimplePrograms[formatIndex]);
	glEnable(GL_VERTEX_PROGRAM_ARB);

	RB_ARB2_LoadMVPMatrixIntoVPParams(surf);

	mesh->SetupForDrawRender(&supportedMD5RDepthVertexFormats[formatIndex]);
	glDisableVertexAttribArrayARB(4);
	mesh->Draw(surf->geo->skinToModelTransforms, &supportedMD5RDepthVertexFormats[formatIndex]);

	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
	glDisable(GL_VERTEX_PROGRAM_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
}

void RB_ARB2_MD5R_DrawShadowElements(const drawSurf_t* surf, int numIndices) {
	if (!surf || !surf->geo || !surf->geo->primBatchMesh) {
		return;
	}

	rvMesh* mesh = surf->geo->primBatchMesh;
	const rvVertexFormat* format = &mesh->GetShadowVolVertexBufferFormat()->m_format;
	const int formatIndex = RB_Q4_FindMD5RFormatIndex(format, supportedMD5RShadowVolVertexFormats, 3);

	if (formatIndex < 0) {
		return;
	}

	idVec3 localLight3;
	idVec4 localLight;
	R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, localLight3);
	localLight[0] = localLight3[0];
	localLight[1] = localLight3[1];
	localLight[2] = localLight3[2];
	localLight[3] = 0.0f;

	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, q4Md5rShadowPrograms[formatIndex]);
	glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_LOCAL_LIGHT, localLight.ToFloatPtr());

	RB_ARB2_LoadMVPMatrixIntoVPParams(surf);

	mesh->SetupForShadowVolRender(&supportedMD5RShadowVolVertexFormats[formatIndex]);
	mesh->DrawShadowVolume(
		surf->geo->skinToModelTransforms,
		surf->geo->indexes,
		numIndices == surf->geo->numIndexes,
		&supportedMD5RShadowVolVertexFormats[formatIndex]);

	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
}

void RB_ARB2_PrepareStageTexturing(const shaderStage_t* pStage, const drawSurf_t* surf, bool fillingDepth) {
	if (!pStage || !surf || !surf->geo || !surf->geo->primBatchMesh) {
		return;
	}

	rvMesh* mesh = surf->geo->primBatchMesh;
	const rvVertexFormat* format = &mesh->GetDrawVertexBufferFormat()->m_format;
	const int formatIndex = RB_Q4_FindMD5RFormatIndex(format, supportedMD5RVertexFormats, 3);

	if (formatIndex < 0) {
		return;
	}

	if (pStage->privatePolygonOffset != 0.0f) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset);
	}

	if (!mesh->m_drawSetUp) {
		mesh->SetupForDrawRender(&supportedMD5RVertexFormats[formatIndex]);
	}

	switch (pStage->texture.texgen) {
	case TG_DIFFUSE_CUBE:
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, q4Md5rEnvNormalPrograms[formatIndex]);
		glEnable(GL_VERTEX_PROGRAM_ARB);
		break;

	case TG_REFLECT_CUBE: {
		const shaderStage_t* bumpStage = surf->material->GetBumpStage();
		if (bumpStage) {
			GL_SelectTexture(1);
			bumpStage->texture.image->Bind();
			GL_SelectTexture(0);
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FPROG_BUMPY_ENVIRONMENT);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, q4Md5rEnvBumpPrograms[formatIndex]);
			glEnable(GL_VERTEX_PROGRAM_ARB);
			RB_ARB2_LoadModelMatrixIntoVPParams(surf);
		}
		else {
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, q4Md5rEnvReflectPrograms[formatIndex]);
			glEnable(GL_VERTEX_PROGRAM_ARB);
		}
		RB_ARB2_LoadLocalViewOriginIntoVPParams(surf);
		break;
	}

	case TG_SKYBOX_CUBE:
	case TG_WOBBLESKY_CUBE:
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, q4Md5rSkyBoxPrograms[formatIndex]);
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_LOCAL_VIEW_ORIGIN, (const float*)surf->texGenTransformAndViewOrg + 12);
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_PROJECTION_X, surf->texGenTransformAndViewOrg);
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_PROJECTION_Y, (const float*)surf->texGenTransformAndViewOrg + 4);
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_PROJECTION_Z, (const float*)surf->texGenTransformAndViewOrg + 8);
		break;

	case TG_SCREEN:
		return;

	default:
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, q4Md5rStdTexPrograms[formatIndex]);
		glEnable(GL_VERTEX_PROGRAM_ARB);
		break;
	}

	float color[4];

	if (fillingDepth) {
		color[0] = color[1] = color[2] = color[3] = 0.0f;
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_STAGE_COLOR_MODULATE, color);
		color[3] = surf->shaderRegisters[pStage->color.registers[3]];
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_STAGE_COLOR_ADD, color);
	}
	else if (pStage->vertexColor) {
		color[0] = color[1] = color[2] = color[3] = 1.0f;
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_STAGE_COLOR_MODULATE, color);
		color[0] = color[1] = color[2] = color[3] = 0.0f;
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_STAGE_COLOR_ADD, color);
	}
	else {
		color[0] = color[1] = color[2] = color[3] = 0.0f;
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_STAGE_COLOR_MODULATE, color);

		color[0] = surf->shaderRegisters[pStage->color.registers[0]];
		color[1] = surf->shaderRegisters[pStage->color.registers[1]];
		color[2] = surf->shaderRegisters[pStage->color.registers[2]];
		color[3] = surf->shaderRegisters[pStage->color.registers[3]];
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_STAGE_COLOR_ADD, color);
	}

	RB_ARB2_LoadMVPMatrixIntoVPParams(surf);

	if (pStage->texture.hasMatrix) {
		RB_ARB2_LoadShaderTextureMatrixIntoVPParams(surf->shaderRegisters, &pStage->texture);
	}
	else {
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_TEXTURE_MATRIX_S, q4TexIdentityS);
		glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, Q4_PP_TEXTURE_MATRIX_T, q4TexIdentityT);
	}
}

void RB_ARB2_DisableStageTexturing(const shaderStage_t* pStage, const drawSurf_t* surf) {
	if (pStage && pStage->privatePolygonOffset != 0.0f &&
		(!surf || !surf->material || !surf->material->TestMaterialFlag(MF_POLYGONOFFSET))) {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	glDisable(GL_VERTEX_PROGRAM_ARB);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

	if (pStage && surf && pStage->texture.texgen == TG_REFLECT_CUBE) {
		if (surf->material && surf->material->GetBumpStage()) {
			GL_SelectTexture(1);
			globalImages->BindNull();
			GL_SelectTexture(0);
		}
	}
}

#endif // Q4_ARB2_ENABLE_RVMESH


//===================================================================================


typedef struct {
	GLenum			target;
	GLuint			ident;
	char			name[64];
} progDef_t;

static	const int	MAX_GLPROGS = 200;

// a single file can have both a vertex program and a fragment program
static progDef_t	progs[MAX_GLPROGS] = {
	{ GL_VERTEX_PROGRAM_ARB, VPROG_TEST, "test.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST, "test.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION, "interaction.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION, "interaction.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_AMBIENT, "ambientLight.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_AMBIENT, "ambientLight.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_SIMPLE_INTERACTION, "SimpleInteraction.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, Q4_FPROG_SIMPLE_INTERACTION, "SimpleInteraction.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW, "shadow.vp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_R200_INTERACTION, "R200_interaction.vp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_NV20_BUMP_AND_LIGHT, "nv20_bumpAndLight.vp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_NV20_DIFFUSE_COLOR, "nv20_diffuseColor.vp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_NV20_SPECULAR_COLOR, "nv20_specularColor.vp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_NV20_DIFFUSE_AND_SPECULAR_COLOR, "nv20_diffuseAndSpecularColor.vp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_ENVIRONMENT, "environment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_ENVIRONMENT, "environment.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_GLASSWARP, "arbVP_glasswarp.txt" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_GLASSWARP, "arbFP_glasswarp.txt" },

#if defined( Q4_ARB2_ENABLE_RVMESH )
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_INTERACTION_4, "md5rInteraction4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_INTERACTION_1, "md5rInteraction1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_INTERACTION, "md5rInteraction.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SIMPLE_4, "md5rSimple4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SIMPLE_1, "md5rSimple1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SIMPLE, "md5rSimple.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_STD_TEX_4, "md5rStdTex4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_STD_TEX_1, "md5rStdTex1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_STD_TEX, "md5rStdTex.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SKYBOX_4, "md5rSkyBox4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SKYBOX_1, "md5rSkyBox1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SKYBOX, "md5rSkyBox.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_NORMAL_4, "md5rEnvNormal4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_NORMAL_1, "md5rEnvNormal1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_NORMAL, "md5rEnvNormal.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_REFLECT_4, "md5rEnvReflect4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_REFLECT_1, "md5rEnvReflect1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_REFLECT, "md5rEnvReflect.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_BUMP_4, "md5rEnvBump4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_BUMP_1, "md5rEnvBump1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_ENV_BUMP, "md5rEnvBump.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SHADOW_4, "md5rShadow4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SHADOW_1, "md5rShadow1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_SHADOW, "md5rShadow.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_BASIC_FOG_4, "md5rBasicFog4.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_BASIC_FOG_1, "md5rBasicFog1.vp" },
	{ GL_VERTEX_PROGRAM_ARB, Q4_VPROG_MD5R_BASIC_FOG, "md5rBasicFog.vp" },
#endif

	// additional programs can be dynamically specified in materials
};

/*
=================
R_LoadARBProgram
=================
*/
void R_LoadARBProgram(int progIndex) {
	int		ofs;
	int		err;
	char* fileBuffer;
	char* buffer;
	char* start, * end;
	const char* expecting = NULL;

	idStr programName = progs[progIndex].name;

	// Quake 4 accepted Cg-style logical names and resolved them to the
	// target-specific ARB program text file before loading.
	if (RB_Q4_HasFileExtension(programName.c_str(), ".cg")) {
		programName.StripFileExtension();

		if (progs[progIndex].target == GL_VERTEX_PROGRAM_ARB) {
			programName += ".vp";
		}
		else if (progs[progIndex].target == GL_FRAGMENT_PROGRAM_ARB) {
			programName += ".fp";
		}

		strncpy(progs[progIndex].name, programName.c_str(), sizeof(progs[progIndex].name) - 1);
		progs[progIndex].name[sizeof(progs[progIndex].name) - 1] = 0;
	}

	idStr	fullPath = "glprogs/";
	fullPath += progs[progIndex].name;

	common->Printf("%s", fullPath.c_str());

	// load the program even if we don't support it, so
	// fs_copyfiles can generate cross-platform data dumps
	fileSystem->ReadFile(fullPath.c_str(), (void**)&fileBuffer, NULL);
	if (!fileBuffer) {
		common->Printf(": File not found\n");
		RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
		return;
	}

	// copy to stack memory and free
	buffer = (char*)_alloca(strlen(fileBuffer) + 1);
	strcpy(buffer, fileBuffer);
	fileSystem->FreeFile(fileBuffer);

	if (!glConfig.isInitialized) {
		RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
		return;
	}

	//
	// submit the program string at start to GL
	//
	if (progs[progIndex].ident == 0) {
		// allocate a new identifier for this program
		progs[progIndex].ident = PROG_USER + progIndex;
	}

	// vertex and fragment programs can both be present in a single file, so
	// scan for the proper header to be the start point, and stamp a 0 in after the end
	if (progs[progIndex].target == GL_VERTEX_PROGRAM_ARB) {
		if (!glConfig.ARBVertexProgramAvailable) {
			common->Printf(": GL_VERTEX_PROGRAM_ARB not available\n");
			RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
			return;
		}
		expecting = "!!ARBvp";
	}
	else if (progs[progIndex].target == GL_FRAGMENT_PROGRAM_ARB) {
		if (!glConfig.ARBFragmentProgramAvailable) {
			common->Printf(": GL_FRAGMENT_PROGRAM_ARB not available\n");
			RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
			return;
		}
		expecting = "!!ARBfp";
#if defined( Q4_ARB2_ENABLE_NV_FRAGMENT_PROGRAM )
	}
	else if (progs[progIndex].target == GL_FRAGMENT_PROGRAM_NV) {
		if (!glConfig.nvProgramsAvailable) {
			common->Printf(": GL_FRAGMENT_PROGRAM_NV not available\n");
			RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
			return;
		}
		expecting = "!!FP";
#endif
	}
	else {
		common->Printf(": unsupported program target 0x%x\n", progs[progIndex].target);
		RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
		return;
	}

	start = strstr((char*)buffer, expecting);
	if (!start) {
		common->Printf(": %s not found\n", expecting);
		RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
		return;
	}
	end = strstr(start, "END");

	if (!end) {
		common->Printf(": END not found\n");
		RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
		return;
	}
	end[3] = 0;

#if defined( Q4_ARB2_ENABLE_NV_FRAGMENT_PROGRAM )
	if (progs[progIndex].target == GL_FRAGMENT_PROGRAM_NV) {
		glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[progIndex].ident, strlen(start), (unsigned char*)start);
	}
	else
#endif
	{
		glBindProgramARB(progs[progIndex].target, progs[progIndex].ident);
		glGetError();

		glProgramStringARB(progs[progIndex].target, GL_PROGRAM_FORMAT_ASCII_ARB,
			strlen(start), (unsigned char*)start);
	}

	err = glGetError();
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, (GLint*)&ofs);
	if (err == GL_INVALID_OPERATION) {
		const GLubyte* str = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
		common->Printf("\nGL_PROGRAM_ERROR_STRING_ARB: %s\n", str);
		if (ofs < 0) {
			common->Printf("GL_PROGRAM_ERROR_POSITION_ARB < 0 with error\n");
		}
		else if (ofs >= (int)strlen((char*)start)) {
			common->Printf("error at end of program\n");
		}
		else {
			common->Printf("error at %i:\n%s", ofs, start + ofs);
		}
		RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
		return;
	}
	if (ofs != -1) {
		common->Printf("\nGL_PROGRAM_ERROR_POSITION_ARB != -1 without error\n");
		RB_Q4_MarkProgramLoaded(progs[progIndex].ident, false);
		return;
	}

	RB_Q4_MarkProgramLoaded(progs[progIndex].ident, true);

	common->Printf("\n");
}

/*
==================
R_FindARBProgram

Returns a GL identifier that can be bound to the given target, parsing
a text file if it hasn't already been loaded.
==================
*/
int R_FindARBProgram(GLenum target, const char* program) {
	int		i;
	idStr	stripped = program;

	stripped.StripFileExtension();

	// see if it is already loaded
	for (i = 0; progs[i].name[0]; i++) {
		if (progs[i].target != target) {
			continue;
		}

		idStr	compare = progs[i].name;
		compare.StripFileExtension();

		if (!idStr::Icmp(stripped.c_str(), compare.c_str())) {
			return progs[i].ident;
		}
	}

	if (i == MAX_GLPROGS) {
		common->Error("R_FindARBProgram: MAX_GLPROGS");
	}

	// add it to the list and load it
	progs[i].ident = (program_t)0;	// will be gen'd by R_LoadARBProgram
	progs[i].target = target;
	strncpy(progs[i].name, program, sizeof(progs[i].name) - 1);

	R_LoadARBProgram(i);

	return progs[i].ident;
}

/*
==================
R_ReloadARBPrograms_f
==================
*/
void R_ReloadARBPrograms_f(const idCmdArgs& args) {
	int		i;

	common->Printf("----- R_ReloadARBPrograms -----\n");
	for (i = 0; progs[i].name[0]; i++) {
		R_LoadARBProgram(i);
	}
	common->Printf("-------------------------------\n");
}

/*
==================
R_ARB2_Init

==================
*/
void R_ARB2_Init(void) {
	glConfig.allowARB2Path = false;

	common->Printf("---------- R_ARB2_Init ----------\n");

	if (!glConfig.ARBVertexProgramAvailable || !glConfig.ARBFragmentProgramAvailable) {
		common->Printf("Not available.\n");
		return;
	}

	common->Printf("Available.\n");

	glConfig.allowARB2Path = true;

	// Quake 4 added renderer-string preference hints.  Doom 3 GPL's glconfig_t
	// has allowNV20Path but not Raven's preferNV20Path/preferSimpleLighting
	// fields, so keep NV20 as a diagnostic hint and drive simple lighting via
	// the r_useSimpleInteraction cvar.
	if (glConfig.renderer_string) {
		if (glConfig.allowNV20Path &&
			RB_Q4_ContainsNoCase(glConfig.renderer_string, "GeForce") &&
			(RB_Q4_ContainsNoCase(glConfig.renderer_string, "5200") ||
				RB_Q4_ContainsNoCase(glConfig.renderer_string, "5600"))) {
			common->Printf("%s: Quake 4 ARB2 would prefer the NV20 path on this renderer\n",
				glConfig.renderer_string);
		}

		if (r_q4AutoSimpleLighting.GetBool() && RB_Q4_RendererPrefersSimpleLighting()) {
			common->Printf("%s: Quake 4 ARB2 prefers simple lighting\n", glConfig.renderer_string);
			r_useSimpleInteraction.SetBool(true);
		}
	}

	common->Printf("---------------------------------\n");
}

