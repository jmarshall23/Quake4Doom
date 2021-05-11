// BSE_Manager.cpp
//

#include "precompiled.h"
#include "BSE.h"

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

bool rvBSEManagerLocal::Init(void) {
	return true;
}
bool rvBSEManagerLocal::Shutdown(void) {
	return true;
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

}

void rvBSEManagerLocal::EndFrame(void) {

}

bool rvBSEManagerLocal::Filtered(const char* name, effectCategory_t category) {
	return true;
}

void rvBSEManagerLocal::UpdateRateTimes(void) {

}

bool rvBSEManagerLocal::CanPlayRateLimited(effectCategory_t category) {
	return true;
}