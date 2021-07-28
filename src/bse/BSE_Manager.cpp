// BSE_Manager.cpp
//

#include "precompiled.h"
#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

rvBSEManagerLocal bseLocal;
rvBSEManager* bse = &bseLocal;

idCVar bse_speeds("bse_speeds", "0", CVAR_INTEGER, "print bse frame statistics");
idCVar bse_enabled("bse_enabled", "1", CVAR_BOOL, "set to false to disable all effects");
idCVar bse_render("bse_render", "1", CVAR_BOOL, "disable effect rendering");
idCVar bse_debug("bse_debug", "0", CVAR_INTEGER, "display debug info about effect");
idCVar bse_showBounds("bse_showbounds", "0", CVAR_BOOL, "display debug bounding boxes effect");
idCVar bse_physics("bse_physics", "1", CVAR_BOOL, "disable effect physics");
idCVar bse_debris("bse_debris", "1", CVAR_BOOL, "disable effect debris");
idCVar bse_singleEffect("bse_singleEffect", "", 0, "set to the name of the effect that is only played");
idCVar bse_rateLimit("bse_rateLimit", "1", CVAR_FLOAT, "rate limit for spawned effects");
idCVar bse_rateCost("bse_rateCost", "1", CVAR_FLOAT, "rate cost multiplier for spawned effects");

float effectCosts[EC_MAX] = { 0, 2, 0.1 }; // dd 0.0, 2 dup(0.1)

idBlockAlloc<rvBSE, 256, 0>	rvBSEManagerLocal::effects;
idVec3						rvBSEManagerLocal::mCubeNormals[6];
idMat3						rvBSEManagerLocal::mModelToBSE;
idList<idTraceModel*>		rvBSEManagerLocal::mTraceModels;
const char* rvBSEManagerLocal::mSegmentNames[SEG_COUNT];
int							rvBSEManagerLocal::mPerfCounters[NUM_PERF_COUNTERS];
float						rvBSEManagerLocal::mEffectRates[EC_MAX];

bool rvBSEManagerLocal::Init(void) {
	common->Printf("----------------- BSE Init ------------------\n");

	renderModelManager->FindModel("_default");

	common->Printf("--------- BSE Created Successfully ----------\n");
	return true;
}

bool rvBSEManagerLocal::Shutdown(void) {
	return true;
}

/*
=======================
rvBSEManagerLocal::AddTraceModel
=======================
*/
int rvBSEManagerLocal::AddTraceModel(idTraceModel* model)
{
	mTraceModels.Append(model);
	return mTraceModels.Num() - 1;
}

/*
=======================
rvBSEManagerLocal::GetTraceModel
=======================
*/
idTraceModel* rvBSEManagerLocal::GetTraceModel(int index)
{
	idTraceModel* result; // eax

	if (index < 0 || index >= mTraceModels.Num())
		result = 0;
	else
		result = mTraceModels[index];
	return result;
}

/*
=======================
rvBSEManagerLocal::FreeTraceModel
=======================
*/
void rvBSEManagerLocal::FreeTraceModel(int index)
{
	if (index >= 0 && index < mTraceModels.Num())
	{
		delete mTraceModels[index];
		mTraceModels[index] = NULL;
	}
}

bool rvBSEManagerLocal::PlayEffect(class rvRenderEffectLocal* def, float time) {
	return true;
}

bool rvBSEManagerLocal::ServiceEffect(class rvRenderEffectLocal* def, float time) {
	return true;
}

void rvBSEManagerLocal::StopEffect(rvRenderEffectLocal* def) {

}

void rvBSEManagerLocal::FreeEffect(rvRenderEffectLocal* def) {

}

float rvBSEManagerLocal::EffectDuration(const rvRenderEffectLocal* def) {
	return 0;
}

bool rvBSEManagerLocal::CheckDefForSound(const renderEffect_t* def) {
	return true;
}

void rvBSEManagerLocal::BeginLevelLoad(void) {

}

void rvBSEManagerLocal::EndLevelLoad(void) {

}

void rvBSEManagerLocal::StartFrame(void) {
	if (bse_speeds.GetInteger())
	{
		rvBSEManagerLocal::mPerfCounters[0] = 0;
		rvBSEManagerLocal::mPerfCounters[1] = 0;
		rvBSEManagerLocal::mPerfCounters[2] = 0;
		rvBSEManagerLocal::mPerfCounters[3] = 0;
		rvBSEManagerLocal::mPerfCounters[4] = 0;
	}
}

void rvBSEManagerLocal::EndFrame(void) {
	common->Printf("bse_active: %i particles: %i traces: %i texels: %i\n",
		rvBSEManagerLocal::mPerfCounters[0],
		rvBSEManagerLocal::mPerfCounters[2],
		rvBSEManagerLocal::mPerfCounters[1],
		(double)rvBSEManagerLocal::mPerfCounters[3] * 0.00000095367431640625);
}

bool rvBSEManagerLocal::Filtered(const char* name, effectCategory_t category) {
	return true;
}

void rvBSEManagerLocal::UpdateRateTimes(void) {

}

bool rvBSEManagerLocal::CanPlayRateLimited(effectCategory_t category) {
	return true;
}