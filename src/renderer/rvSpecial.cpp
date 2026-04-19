/*
===========================================================================

Quake4Doom

Copyright (C) 2026 Justin Marshall
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Quake4Doom Project by Justin Marshall
This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Quake4Doom is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Quake4Doom is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake4Doom.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Quake4Doom is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"

#include "tr_local.h"

rvBlurTexture* DepthTexture = NULL;
rvAL* ptr = NULL;
const idMaterial* ALMaterial = NULL;
int numDrawn = 0;

static bool RB_SurfaceHasActiveStages(const drawSurf_s* surf) {
	const idMaterial* material = surf->material;
	const float* shaderRegisters = surf->shaderRegisters;

	if (material->GetNumStages() <= 0) {
		return false;
	}

	for (int i = 0; i < material->GetNumStages(); ++i) {
		const shaderStage_t* stage = material->GetStage(i);
		if (shaderRegisters[stage->conditionRegister] != 0.0f) {
			return true;
		}
	}

	return false;
}

static void RB_SetupDepthTextureSurface(const drawSurf_s* surf, idDrawVert*& ac) {
	ac = reinterpret_cast<idDrawVert*>(vertexCache.Position(surf->geo->ambientCache));
	glVertexPointer(3, GL_FLOAT, sizeof(idDrawVert), ac);
	glTexCoordPointer(2, GL_FLOAT, sizeof(idDrawVert), &ac->st);
}

static void RB_DrawPerforatedSurface(const drawSurf_s* surf, idDrawVert* ac, float color[4], bool& didDraw) {
	const idMaterial* material = surf->material;
	const float* shaderRegisters = surf->shaderRegisters;
	const srfTriangles_s* tri = surf->geo;

	didDraw = false;

	glEnable(GL_ALPHA_TEST);

	for (int i = 0; i < material->GetNumStages(); ++i) {
		const shaderStage_t* stage = material->GetStage(i);

		if (stage->texture.dynamic == DI_REFLECTION_RENDER || stage->texture.dynamic == DI_REFRACTION_RENDER) {
			continue;
		}

		if (!stage->hasAlphaTest) {
			continue;
		}

		if (shaderRegisters[stage->conditionRegister] == 0.0f) {
			continue;
		}

		color[3] = shaderRegisters[stage->color.registers[3]];
		if (color[3] <= 0.0f) {
			continue;
		}

		didDraw = true;
		glColor4fv(color);
		glAlphaFunc(stage->alphaTestMode, shaderRegisters[stage->alphaTestRegister]);
		stage->texture.image->Bind();
		RB_PrepareStageTexturing(const_cast<shaderStage_t*>(stage), surf, ac, true);
		RB_DrawElementsWithCounters(tri);
		RB_FinishStageTexturing(const_cast<shaderStage_t*>(stage), surf, ac);
	}

	glDisable(GL_ALPHA_TEST);
}

bool rvBlurTexture::CreateBuffer(const char* name, idPBufferImage*& image) {
	image = globalImages->AllocPBufferImage(name);
	if (!image) {
		return false;
	}

	image->useCount = 1;
	image->type = TT_2D;
	image->uploadWidth = 256;
	image->uploadHeight = 256;

	image->mRenderTarget->Init(
		image->uploadWidth,
		image->uploadHeight,
		32, 8, 8, 8, 8,
		24, 0, 9
	);
	image->mRenderTarget->DefaultD3GL();
	return true;
}

bool rvAL::CreateBuffer(const char* name, idPBufferImage*& image) {
	image = globalImages->AllocPBufferImage(name);
	if (!image) {
		return false;
	}

	image->useCount = 1;
	image->type = TT_2D;
	image->uploadWidth = 512;
	image->uploadHeight = 512;

	image->mRenderTarget->Init(
		image->uploadWidth,
		image->uploadHeight,
		32, 8, 8, 8, 8,
		24, 0, 9
	);
	image->mRenderTarget->DefaultD3GL();
	return true;
}

rvBlurTexture::rvBlurTexture() {
	memset(this, 0, sizeof(*this));

	CreateBuffer("DepthTexture", mDepthImage);
	CreateBuffer("BlurTexture1", mBlurImage[0]);

	mDepthMaterial = declManager->FindMaterial("hs/MedLabs", true);
	regs = static_cast<float*>(Mem_ClearedAlloc(sizeof(float) * mDepthMaterial->GetNumRegisters()));

	shaderParms[0] = 0.694f;
	shaderParms[1] = 0.694f;
	shaderParms[2] = 0.694f;
	shaderParms[3] = 1.0f;
	shaderParms[4] = 4.0f;
	shaderParms[5] = 0.31f;
	shaderParms[6] = 0.5f;
	shaderParms[7] = 500.0f;
}

rvAL::rvAL() {
	memset(this, 0, sizeof(*this));

	CreateBuffer("DepthTexture", mDepthImage);
	CreateBuffer("BlurTexture1", mBlurImage[0]);

	mDepthMaterial = declManager->FindMaterial("hs/ALSetup", true);
	ALMaterial = mDepthMaterial;
	regs = static_cast<float*>(Mem_ClearedAlloc(sizeof(float) * mDepthMaterial->GetNumRegisters()));

	offset = 0.0f;
	count = 0;

	if (!tr.primaryWorld) {
		return;
	}

	for (int i = 0; i < tr.primaryWorld->lightDefs.Num() && count < MAX_LIGHTS; ++i) {
		idRenderLightLocal* light = tr.primaryWorld->lightDefs[i];
		if (!light) {
			continue;
		}

		lOrigin[count] = light->globalLightOrigin;
		lColor[count].x = light->parms.shaderParms[0];
		lColor[count].y = light->parms.shaderParms[1];
		lColor[count].z = light->parms.shaderParms[2];
		lColor[count].Normalize();
		lSize[count] = 300.0f;
		++count;
	}
}

void RB_RestoreDrawingView() {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(backEnd.viewDef->projectionMatrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	backEnd.currentSpace = NULL;

	glViewport(
		tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
		tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
		backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1,
		backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1
	);

	glScissor(
		tr.viewportOffset[0] + backEnd.viewDef->scissor.x1 + backEnd.viewDef->viewport.x1,
		tr.viewportOffset[1] + backEnd.viewDef->scissor.y1 + backEnd.viewDef->viewport.y1,
		backEnd.viewDef->scissor.x2 - backEnd.viewDef->scissor.x1 + 1,
		backEnd.viewDef->scissor.y2 - backEnd.viewDef->scissor.y1 + 1
	);

	backEnd.currentScissor = backEnd.viewDef->scissor;

	GL_State(GLS_DEFAULT);

	if (backEnd.viewDef->viewEntitys) {
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);
	}
	else {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
	}

	backEnd.glState.faceCulling = -1;
	GL_Cull(CT_FRONT_SIDED);
}

void R_AddSpecialEffects(viewDef_s* parms) {
	if ((tr.specialEffectsEnabled & (SPECIAL_EFFECT_BLUR | SPECIAL_EFFECT_AL)) == 0) {
		return;
	}

	emptyCommand_t* cmd = R_GetCommandBuffer(sizeof(drawSurfsCommand_t));
	if (!cmd) {
		return;
	}

	cmd->commandId = RC_DRAW_DEPTH_TEXTURE;
	reinterpret_cast<drawSurfsCommand_t*>(cmd)->viewDef = parms;
}

void idRenderSystemLocal::SetSpecialEffectParm(ESpecialEffectType which, int parm, float value) {
	if (which == SPECIAL_EFFECT_BLUR) {
		if (DepthTexture) {
			DepthTexture->shaderParms[parm] = value;
		}
	}
	else if (which == SPECIAL_EFFECT_AL) {
		if (ptr) {
			ptr->shaderParms[parm] = value;
		}
	}
}

void RB_T_FillDepthTexture(const drawSurf_s* surf) {
	const idMaterial* material = surf->material;
	const srfTriangles_s* tri = surf->geo;

	if ((material->GetNumStages() <= 0 && !material->GlobalGui() && !material->HasGui()) ||
		!tri->numIndexes ||
		material->GetCoverage() == MC_TRANSLUCENT) {
		return;
	}

	if (!tri->ambientCache) {
		common->Printf("RB_T_FillDepthTexture: !tri->ambientCache\n");
		return;
	}

	if (!RB_SurfaceHasActiveStages(surf)) {
		return;
	}

	GL_PolygonOffset(material, 1);

	float color[4] = { 0, 0, 0, 1 };

	if (material->GetSort() == SS_SUBVIEW) {
		GL_State(GLS_DEPTHFUNC_EQUAL | GLS_COLORMASK);
		color[0] = color[1] = color[2] = 1.0f / backEnd.overBright;
	}

	idDrawVert* ac = NULL;
	RB_SetupDepthTextureSurface(surf, ac);

	const bool drawSolid = (material->GetCoverage() == MC_OPAQUE);

	if (material->GetCoverage() == MC_PERFORATED) {
		bool didDraw = false;
		RB_DrawPerforatedSurface(surf, ac, color, didDraw);

		if (!didDraw && drawSolid) {
			glColor4fv(color);
			globalImages->whiteImage->Bind();
			RB_DrawElementsWithCounters(tri);
		}
	}
	else if (drawSolid) {
		glColor4fv(color);
		globalImages->whiteImage->Bind();
		RB_DrawElementsWithCounters(tri);
	}

	GL_PolygonOffset(material, 0);

	if (material->GetSort() == SS_SUBVIEW) {
		GL_State(GLS_DEFAULT);
	}
}

void rvBlurTexture::Render(const drawSurfsCommand_t* drawSurfs) {
	idVec3 randomizer;

	backEnd.currentRenderNeeded = true;

	rvTexRenderTarget::BeginRender(mDepthImage->mRenderTarget, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glDepthRange(0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	backEnd.viewDef = drawSurfs->viewDef;
	RB_BeginDrawingView();

	glViewport(0, 0, mDepthImage->uploadWidth, mDepthImage->uploadHeight);
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);

	GL_SelectTexture(1);
	GL_SelectTexture(0);
	globalImages->GetDefaultImage()->Bind();
	globalImages->GetWhiteImage()->Bind();
	GL_SelectTexture(0);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 255);

	const_cast<idMaterial*>(mDepthMaterial)->EvaluateRegisters(
		regs,
		shaderParms,
		drawSurfs->viewDef,
		NULL,
		&randomizer
	);

	const shaderStage_t* stage0 = mDepthMaterial->GetStage(0);
	stage0->newShaderStage->Bind(regs, 0);

	RB_RenderDrawSurfListWithFunction(
		drawSurfs->viewDef->drawSurfs,
		drawSurfs->viewDef->numDrawSurfs,
		RB_T_FillDepthTexture
	);

	stage0->newShaderStage->UnBind();

	rvTexRenderTarget::EndRender(mDepthImage->mRenderTarget, true);

	backEnd.glState.forceGlState = true;

	for (int i = 0; i < backEnd.glState.currenttmu; ++i) {
		GL_SelectTexture(i);
		glDisable(GL_TEXTURE_CUBE_MAP_EXT);
		glDisable(GL_TEXTURE_3D);
		glDisable(GL_TEXTURE_2D);

		backEnd.glState.tmu[i].textureType = TT_DISABLED;
		backEnd.glState.tmu[i].current2DMap = -1;
		backEnd.glState.tmu[i].current3DMap = -1;
		backEnd.glState.tmu[i].currentCubeMap = -1;

		globalImages->GetWhiteImage()->Bind();
	}
}

void rvBlurTexture::Display(const viewEntity_s* viewEnts, bool prePass) {
	if (!prePass || viewEnts) {
		return;
	}

	mBlurImage[0]->mRenderTarget->BeginRender(0);

	RB_SetGL2D();
	glViewport(0, 0, mBlurImage[0]->uploadWidth, mBlurImage[0]->uploadHeight);
	glColor4f(1, 1, 1, 1);
	glClearColor(0, 0, 0, 1);
	glClearDepth(1.0f);
	glDepthRange(0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	GL_SelectTexture(1);
	GL_SelectTexture(0);
	globalImages->GetDefaultImage()->Bind();
	globalImages->GetWhiteImage()->Bind();

	const shaderStage_t* stage1 = mDepthMaterial->GetStage(1);

	stage1->newShaderStage->Bind(regs, 0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(640.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(640.0f, 480.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 480.0f);
	glEnd();
	stage1->newShaderStage->UnBind();

	mBlurImage[0]->mRenderTarget->EndRender(true);

	RB_SetGL2D();
	glBlendFunc(GL_ONE, GL_ZERO);
	glColor4f(1, 1, 1, 1);

	const shaderStage_t* stage2 = mDepthMaterial->GetStage(2);
	stage2->newShaderStage->Bind(regs, 0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(640.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(640.0f, 480.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 480.0f);
	glEnd();

	stage2->newShaderStage->UnBind();

	backEnd.glState.forceGlState = true;
}

void RB_T_FillDepthTextureAL(const drawSurf_s* surf) {
	const idMaterial* material = surf->material;
	const srfTriangles_s* tri = surf->geo;

	if ((material->GetNumStages() <= 0 && !material->GlobalGui() && !material->HasGui()) ||
		!tri->numIndexes ||
		material->GetCoverage() == MC_TRANSLUCENT) {
		return;
	}

	if (!tri->ambientCache) {
		common->Printf("RB_T_FillDepthTexture: !tri->ambientCache\n");
		return;
	}

	if (!RB_SurfaceHasActiveStages(surf)) {
		return;
	}

	GL_PolygonOffset(material, 1);

	float color[4] = { 0, 0, 0, 1 };

	if (material->GetSort() == SS_SUBVIEW) {
		GL_State(GLS_DEPTHFUNC_EQUAL | GLS_COLORMASK);
		color[0] = color[1] = color[2] = 1.0f / backEnd.overBright;
	}

	idDrawVert* ac = NULL;
	RB_SetupDepthTextureSurface(surf, ac);

	const bool drawSolid = (material->GetCoverage() == MC_OPAQUE);

	if (material->GetCoverage() == MC_PERFORATED) {
		bool didDraw = false;
		RB_DrawPerforatedSurface(surf, ac, color, didDraw);

		if (!didDraw && drawSolid) {
			for (int i = 0; i < material->GetNumStages(); ++i) {
				const shaderStage_t* stage = material->GetStage(i);
				if (stage->lighting == SL_DIFFUSE) {
					ALMaterial->GetStage(0)->newShaderStage->SetTextureParm(
						"Image",
						stage->texture.image
					);
					ALMaterial->GetStage(0)->newShaderStage->Bind(ptr->regs, 0);
					break;
				}
			}

			glColor4fv(color);
			RB_DrawElementsWithCounters(tri);
		}
	}
	else if (drawSolid) {
		for (int i = 0; i < material->GetNumStages(); ++i) {
			const shaderStage_t* stage = material->GetStage(i);
			if (stage->lighting == SL_DIFFUSE) {
				ALMaterial->GetStage(0)->newShaderStage->SetTextureParm(
					"Image",
					stage->texture.image
				);
				ALMaterial->GetStage(0)->newShaderStage->Bind(ptr->regs, 0);
				break;
			}
		}

		glColor4fv(color);
		RB_DrawElementsWithCounters(tri);
	}

	GL_PolygonOffset(material, 0);

	if (material->GetSort() == SS_SUBVIEW) {
		GL_State(GLS_DEFAULT);
	}
}

void rvAL::Render(const drawSurfsCommand_t* drawSurfs) {
	idVec3 randomizer;

	mDepthImage->mRenderTarget->BeginRender(0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glDepthRange(0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	backEnd.viewDef = drawSurfs->viewDef;
	RB_BeginDrawingView();
	memcpy(backEnd.projectionMatrix, backEnd.viewDef->projectionMatrix, sizeof(backEnd.projectionMatrix));

	glViewport(0, 0, mDepthImage->uploadWidth, mDepthImage->uploadHeight);
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);

	GL_SelectTexture(1);
	globalImages->defaultImage->Bind();
	GL_SelectTexture(0);
	globalImages->whiteImage->Bind();
	GL_SelectTexture(0);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 255);

	const_cast<idMaterial*>(mDepthMaterial)->EvaluateRegisters(
		regs,
		shaderParms,
		drawSurfs->viewDef,
		NULL,
		&randomizer
	);

	const shaderStage_t* stage0 = mDepthMaterial->GetStage(0);
	stage0->newShaderStage->Bind(regs, 0);

	RB_RenderDrawSurfListWithFunction(
		drawSurfs->viewDef->drawSurfs,
		drawSurfs->viewDef->numDrawSurfs,
		RB_T_FillDepthTextureAL
	);

	stage0->newShaderStage->UnBind();

	mDepthImage->mRenderTarget->EndRender(true);

	backEnd.glState.forceGlState = true;

	for (int i = 0; i < backEnd.glState.currenttmu; ++i) {
		GL_SelectTexture(i);
		glDisable(GL_TEXTURE_CUBE_MAP_EXT);
		glDisable(GL_TEXTURE_3D);
		glDisable(GL_TEXTURE_2D);

		backEnd.glState.tmu[i].textureType = TT_DISABLED;
		backEnd.glState.tmu[i].current2DMap = -1;
		backEnd.glState.tmu[i].current3DMap = -1;
		backEnd.glState.tmu[i].currentCubeMap = -1;

		globalImages->GetWhiteImage()->Bind();
	}
}

void RB_SetGL2D2() {
	glViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);

	if (r_useScissor.GetBool()) {
		glScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 640.0, 480.0, 0.0, 0.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK);
	GL_Cull(CT_TWO_SIDED);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
}

void R_ShutdownSpecialEffects() {
	if (DepthTexture) {
		Mem_Free(DepthTexture->regs);
		delete DepthTexture;
		DepthTexture = NULL;
	}

	if (ptr) {
		Mem_Free(ptr->regs);
		delete ptr;
		ptr = NULL;
	}

	tr.specialEffectsEnabled = 0;
}

void RB_DrawDepthTexture(const drawSurfsCommand_t* data) {
	backEnd.viewDef = data->viewDef;

	if (!backEnd.viewDef->numDrawSurfs) {
		return;
	}

	if (DepthTexture) {
		DepthTexture->Render(data);
	}
	if (ptr) {
		ptr->Render(data);
	}
}

void idRenderSystemLocal::SetSpecialEffect(ESpecialEffectType which, bool enabled) {
	if (enabled) {
		specialEffectsEnabled |= which;

		if (which == SPECIAL_EFFECT_BLUR) {
			if (!DepthTexture && glConfig.allowARB2Path) {
				DepthTexture = new rvBlurTexture();
			}
		}
		else if (which == SPECIAL_EFFECT_AL) {
			if (!ptr && glConfig.allowARB2Path) {
				ptr = new rvAL();
			}
		}
	}
	else {
		specialEffectsEnabled &= ~which;
	}
}

void idRenderSystemLocal::ShutdownSpecialEffects() {
	R_ShutdownSpecialEffects();
}

void rvAL::DrawLight(idVec3& origin, float size, idVec3& color) {
	idVec3 temp;
	idVec3 ndc;
	idVec3 points[4];
	idPlane plane;
	float x1, y1, x2, y2;
	float minDistance;

	// Early frustum reject.
	for (int i = 0; i < 5; ++i) {
		const idPlane& p = backEnd.viewDef->frustum[i];
		if (p.Distance(origin) > size) {
			return;
		}
	}

	temp = tr.primaryRenderView.viewaxis[1] * size;
	idVec3 up = tr.primaryRenderView.viewaxis[2] * size;

	points[0] = origin + temp + up;
	points[1] = origin - temp + up;
	points[2] = origin - temp - up;
	points[3] = origin + temp - up;

	R_GlobalToNormalizedDeviceCoordinates(
		tr.primaryView->worldSpace.modelViewMatrix,
		tr.primaryView->projectionMatrix,
		&points[0],
		&ndc
	);
	x1 = (backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1) * (ndc.x + 1.0f) * 0.5f;
	y1 = backEnd.viewDef->viewport.y2 -
		(backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1) * (ndc.y + 1.0f) * 0.5f;

	R_GlobalToNormalizedDeviceCoordinates(
		tr.primaryView->worldSpace.modelViewMatrix,
		tr.primaryView->projectionMatrix,
		&points[2],
		&ndc
	);
	x2 = (backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1) * (ndc.x + 1.0f) * 0.5f;
	y2 = backEnd.viewDef->viewport.y2 -
		(backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1) * (ndc.y + 1.0f) * 0.5f;

	if (x1 > x2) {
		idSwap(x1, x2);
	}
	if (y1 > y2) {
		idSwap(y1, y2);
	}

	R_LocalPointToGlobal(tr.primaryView->worldSpace.modelViewMatrix, &origin, &temp);

	idVec3 normal = tr.primaryRenderView.viewaxis[2].Cross(tr.primaryRenderView.viewaxis[1]);
	normal.Normalize();

	plane.SetNormal(normal);
	plane.FitThroughPoint(tr.primaryView->renderView.vieworg);

	minDistance = plane.Distance(origin);

	mainStage->SetShaderParameter(mLightLocParm, regs, &temp.x, 3);
	mainStage->SetShaderParameter(mLightColorParm, regs, &color.x, 3);
	mainStage->SetShaderParameter(mLightSizeParm, regs, &size, 1);
	mainStage->SetShaderParameter(mLightMinDistanceParm, regs, &minDistance, 1);
	mainStage->UpdateShaderParms(regs, 0);

	glBegin(GL_QUADS);
	glTexCoord2f(x1 / 639.0f, 1.0f - y1 / 479.0f); glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0f, 0.0f); glVertex2f(x1, y1);
	glTexCoord2f(x2 / 639.0f, 1.0f - y1 / 479.0f); glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0f, 0.0f); glVertex2f(x2, y1);
	glTexCoord2f(x2 / 639.0f, 1.0f - y2 / 479.0f); glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0f, 1.0f); glVertex2f(x2, y2);
	glTexCoord2f(x1 / 639.0f, 1.0f - y2 / 479.0f); glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0f, 1.0f); glVertex2f(x1, y2);
	glEnd();

	++numDrawn;
}

void rvAL::Display(const viewEntity_s* viewEnts, bool prePass) {
	if (prePass) {
		return;
	}

	mainStage = ALMaterial->GetStage(1)->newShaderStage;
	mLightLocParm = mainStage->FindShaderParameter("LightLoc");
	mLightColorParm = mainStage->FindShaderParameter("LightColor");
	mLightSizeParm = mainStage->FindShaderParameter("LightSize");
	mLightMinDistanceParm = mainStage->FindShaderParameter("LightMinDistance");

	RB_SetGL2D2();
	glColor4f(1, 1, 1, 1);
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	numDrawn = 0;

	if (viewEnts && !cvarSystem->GetCVarBool("rj")) {
		mainStage->Bind(regs, 0);

		idVec3 origin(0.0f, 0.0f, 0.0f);
		idVec3 color(1.0f, 1.0f, 0.0f);
		DrawLight(origin, 200.0f, color);

		offset += 0.005f;

		for (int i = 0; i < count; ++i) {
			idVec3 lightOrigin = lOrigin[i];
			DrawLight(lightOrigin, lSize[i], lColor[i]);
		}

		mainStage->UnBind();
	}

	backEnd.glState.forceGlState = true;
	RB_RestoreDrawingView();
}

void RB_DisplaySpecialEffects(const viewEntity_s* viewEnts, bool prePass) {
	if ((tr.specialEffectsEnabled & SPECIAL_EFFECT_BLUR) && DepthTexture) {
		DepthTexture->Display(viewEnts, prePass);
	}

	if ((tr.specialEffectsEnabled & SPECIAL_EFFECT_AL) && ptr) {
		ptr->Display(viewEnts, prePass);
	}
}