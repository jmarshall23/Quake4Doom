#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"

/*
====================
RB_DXDrawInteractions
====================
*/
void RB_DXDrawInteractions(void) {
	viewLight_t* vLight;
	bool hasLight = false;

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

		hasLight = true;

		glRaytracingLight_t light = {};
		light.color.x = vLight->lightDef->parms.shaderParms[SHADERPARM_RED];
		light.color.y = vLight->lightDef->parms.shaderParms[SHADERPARM_GREEN];
		light.color.z = vLight->lightDef->parms.shaderParms[SHADERPARM_BLUE];
		light.type = GL_RAYTRACING_LIGHT_TYPE_POINT;
		light.pointRadius.x = vLight->lightDef->parms.lightRadius[0] * 1.4f;
		light.pointRadius.y = vLight->lightDef->parms.lightRadius[1] * 1.4f;
		light.pointRadius.z = vLight->lightDef->parms.lightRadius[2] * 1.4f;
		light.intensity = 2;
		light.position.x = vLight->globalLightOrigin.x;
		light.position.y = vLight->globalLightOrigin.y;
		light.position.z = vLight->globalLightOrigin.z;
		glRaytracingLightingAddLight(&light);
	}

	if (!hasLight)
		return;


	glFinish();
	glLightScene();
	glRaytracingLightingClearLights(false);
}
