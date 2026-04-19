#include "precompiled.h"
#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"
#include "BSE_API.h"

namespace {
rvBSEManagerLocal bseLocal;
}

idDecl* OpenQ4_AllocIntegratedBSEDeclEffect( void ) {
	return new rvDeclEffect();
}

rvBSEManager* OpenQ4_GetIntegratedBSEManager( void ) {
	return &bseLocal;
}
