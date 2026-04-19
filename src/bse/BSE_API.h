#ifndef _BSE_API_H_INC_
#define _BSE_API_H_INC_

#include "BSEInterface.h"

class idDecl;

typedef idDecl* (*BSE_AllocDeclEffect_t)(void);

rvBSEManager*				OpenQ4_GetIntegratedBSEManager( void );
idDecl*						OpenQ4_AllocIntegratedBSEDeclEffect( void );

extern BSE_AllocDeclEffect_t	bseAllocDeclEffect;

#endif // _BSE_API_H_INC_
