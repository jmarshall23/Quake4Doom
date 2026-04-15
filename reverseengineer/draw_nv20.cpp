
// FUNC: RB_NV20_BumpAndLightFragment
void RB_NV20_BumpAndLightFragment()
{
  if ( r_useCombinerDisplayLists.internalVar->integerValue )
  {
    qglCallList(fragmentDisplayListBase);
  }
  else
  {
    qglCombinerParameteriNV(0x854Eu, 3);
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8523u, 0x84C1u, 0x8538u, 0x1907u);
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x84C0u, 0x8538u, 0x1907u);
    qglCombinerOutputNV(0x8550u, 0x1907u, 0x852Eu, 0x8530u, 0x8530u, 0, 0, 1u, 0, 0);
    qglCombinerInputNV(0x8551u, 0x1907u, 0x8523u, 0x84C2u, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8551u, 0x1907u, 0x8524u, 0x84C3u, 0x8536u, 0x1907u);
    qglCombinerOutputNV(0x8551u, 0x1907u, 0x852Fu, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8523u, 0x852Eu, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8524u, 0x852Fu, 0x8536u, 0x1907u);
    qglCombinerOutputNV(0x8552u, 0x1907u, 0x852Eu, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
    qglCombinerInputNV(0x8552u, 0x1906u, 0x8523u, 0x852Eu, 0x8536u, 0x1905u);
    qglCombinerInputNV(0x8552u, 0x1906u, 0x8524u, 0x852Fu, 0x8536u, 0x1905u);
    qglCombinerOutputNV(0x8552u, 0x1906u, 0x852Eu, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
    qglFinalCombinerInputNV(0x8526u, 0x852Eu, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8523u, 0, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8524u, 0, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8525u, 0, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8529u, 0x852Eu, 0x8536u, 0x1906u);
  }
}

// FUNC: RB_NV20_DI_BumpAndLightPass
void __usercall RB_NV20_DI_BumpAndLightPass(const drawInteraction_t *din@<esi>, bool monoLightShader)
{
  int depthFunc; // eax
  idImage *v3; // eax
  idImage *v4; // eax

  depthFunc = backEnd.depthFunc;
  BYTE1(depthFunc) = BYTE1(backEnd.depthFunc) | 0xF;
  GL_State(depthFunc);
  backEnd.glState.currenttmu = 0;
  qglActiveTextureARB(0x84C0u);
  if ( din->ambientLight )
    v3 = globalImages->GetAmbientNormalMap(globalImages);
  else
    v3 = globalImages->GetNormalCubeMapImage(globalImages);
  v3->Bind(v3);
  backEnd.glState.currenttmu = 1;
  qglActiveTextureARB(0x84C1u);
  din->bumpImage->Bind(din->bumpImage);
  backEnd.glState.currenttmu = 2;
  qglActiveTextureARB(0x84C2u);
  din->lightFalloffImage->Bind(din->lightFalloffImage);
  backEnd.glState.currenttmu = 3;
  qglActiveTextureARB(0x84C3u);
  if ( monoLightShader )
  {
    din->lightImage->Bind(din->lightImage);
  }
  else
  {
    v4 = globalImages->GetWhiteImage(globalImages);
    v4->Bind(v4);
  }
  RB_NV20_BumpAndLightFragment();
  qglBindProgramARB(0x8620u, 6u);
  RB_DrawElementsWithCounters(din->surf->geo);
}

// FUNC: RB_NV20_DiffuseColorFragment
void RB_NV20_DiffuseColorFragment()
{
  if ( r_useCombinerDisplayLists.internalVar->integerValue )
  {
    qglCallList(fragmentDisplayListBase + 1);
  }
  else
  {
    qglCombinerParameteriNV(0x854Eu, 1);
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8523u, 0x84C0u, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x852Cu, 0x8536u, 0x1907u);
    qglCombinerOutputNV(0x8550u, 0x1907u, 0x84C0u, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
    qglCombinerOutputNV(0x8550u, 0x1906u, 0x8530u, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
    qglFinalCombinerInputNV(0x8523u, 0x852Au, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8524u, 0x8531u, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8525u, 0, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8526u, 0, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8527u, 0x84C0u, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8528u, 0x84C1u, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8529u, 0, 0x8536u, 0x1906u);
  }
}

// FUNC: RB_NV20_DI_DiffuseColorPass
void __usercall RB_NV20_DI_DiffuseColorPass(const drawInteraction_t *din@<esi>)
{
  GL_State(backEnd.depthFunc | 0x1127);
  backEnd.glState.currenttmu = 0;
  qglActiveTextureARB(0x84C0u);
  din->diffuseImage->Bind(din->diffuseImage);
  backEnd.glState.currenttmu = 1;
  qglActiveTextureARB(0x84C1u);
  din->lightImage->Bind(din->lightImage);
  backEnd.glState.currenttmu = 2;
  qglActiveTextureARB(0x84C2u);
  globalImages->BindNull(globalImages);
  backEnd.glState.currenttmu = 3;
  qglActiveTextureARB(0x84C3u);
  globalImages->BindNull(globalImages);
  RB_NV20_DiffuseColorFragment();
  if ( din->vertexColor == SVC_INVERSE_MODULATE )
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x852Cu, 0x8537u, 0x1907u);
  qglBindProgramARB(0x8620u, 7u);
  RB_DrawElementsWithCounters(din->surf->geo);
}

// FUNC: RB_NV20_SpecularColorFragment
void RB_NV20_SpecularColorFragment()
{
  if ( r_useCombinerDisplayLists.internalVar->integerValue )
  {
    qglCallList(fragmentDisplayListBase + 2);
  }
  else
  {
    qglCombinerParameteriNV(0x854Eu, 4);
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8523u, 0x84C1u, 0x8538u, 0x1907u);
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x84C0u, 0x8538u, 0x1907u);
    qglCombinerOutputNV(0x8550u, 0x1907u, 0x852Eu, 0x8530u, 0x8530u, 0x853Eu, 0x8541u, 1u, 0, 0);
    qglCombinerInputNV(0x8551u, 0x1907u, 0x8523u, 0x852Eu, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8551u, 0x1907u, 0x8524u, 0x852Eu, 0x8536u, 0x1907u);
    qglCombinerOutputNV(0x8551u, 0x1907u, 0x852Eu, 0x8530u, 0x8530u, 0x853Eu, 0x8541u, 0, 0, 0);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8523u, 0x852Eu, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8524u, 0x84C3u, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8525u, 0x852Bu, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8526u, 0x84C2u, 0x8536u, 0x1907u);
    qglCombinerOutputNV(0x8552u, 0x1907u, 0x852Eu, 0x852Du, 0x8530u, 0, 0, 0, 0, 0);
    qglCombinerInputNV(0x8553u, 0x1907u, 0x8523u, 0x852Du, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8553u, 0x1907u, 0x8524u, 0x852Cu, 0x8536u, 0x1907u);
    qglCombinerOutputNV(0x8553u, 0x1907u, 0x852Du, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
    qglFinalCombinerInputNV(0x8523u, 0x852Eu, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8524u, 0x852Du, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8525u, 0, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8526u, 0x8531u, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8527u, 0x852Eu, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8528u, 0x852Du, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8529u, 0, 0x8536u, 0x1906u);
  }
}

// FUNC: RB_NV20_DI_SpecularColorPass
void __usercall RB_NV20_DI_SpecularColorPass(const drawInteraction_t *din@<esi>)
{
  idImage *v1; // eax

  GL_State(backEnd.depthFunc | 0x1127);
  backEnd.glState.currenttmu = 0;
  qglActiveTextureARB(0x84C0u);
  v1 = globalImages->GetNormalCubeMapImage(globalImages);
  v1->Bind(v1);
  backEnd.glState.currenttmu = 1;
  qglActiveTextureARB(0x84C1u);
  din->bumpImage->Bind(din->bumpImage);
  backEnd.glState.currenttmu = 2;
  qglActiveTextureARB(0x84C2u);
  din->specularImage->Bind(din->specularImage);
  backEnd.glState.currenttmu = 3;
  qglActiveTextureARB(0x84C3u);
  din->lightImage->Bind(din->lightImage);
  RB_NV20_SpecularColorFragment();
  if ( din->vertexColor == SVC_INVERSE_MODULATE )
    qglCombinerInputNV(0x8553u, 0x1907u, 0x8524u, 0x852Cu, 0x8537u, 0x1907u);
  qglBindProgramARB(0x8620u, 8u);
  RB_DrawElementsWithCounters(din->surf->geo);
}

// FUNC: RB_NV20_DiffuseAndSpecularColorFragment
void RB_NV20_DiffuseAndSpecularColorFragment()
{
  if ( r_useCombinerDisplayLists.internalVar->integerValue )
  {
    qglCallList(fragmentDisplayListBase + 3);
  }
  else
  {
    qglCombinerParameteriNV(0x854Eu, 3);
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8523u, 0x84C1u, 0x8538u, 0x1907u);
    qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x84C0u, 0x8538u, 0x1907u);
    qglCombinerOutputNV(0x8550u, 0x1907u, 0x852Du, 0x8530u, 0x8530u, 0x853Eu, 0x8541u, 1u, 0, 0);
    qglCombinerInputNV(0x8551u, 0x1907u, 0x8523u, 0x852Du, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8551u, 0x1907u, 0x8524u, 0x852Du, 0x8536u, 0x1907u);
    qglCombinerOutputNV(0x8551u, 0x1907u, 0x852Du, 0x8530u, 0x8530u, 0x853Eu, 0x8541u, 0, 0, 0);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8523u, 0x852Du, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8524u, 0x84C3u, 0x8536u, 0x1907u);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8525u, 0, 0x8537u, 0x1907u);
    qglCombinerInputNV(0x8552u, 0x1907u, 0x8526u, 0, 0x8537u, 0x1907u);
    qglCombinerOutputNV(0x8552u, 0x1907u, 0x852Du, 0x852Eu, 0x8530u, 0x853Eu, 0, 0, 0, 0);
    qglFinalCombinerInputNV(0x8523u, 0x852Bu, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8524u, 0x852Du, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8525u, 0, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8526u, 0x8531u, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8527u, 0x84C2u, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8528u, 0x852Au, 0x8536u, 0x1907u);
    qglFinalCombinerInputNV(0x8529u, 0, 0x8536u, 0x1906u);
  }
}

// FUNC: RB_NV20_DI_DiffuseAndSpecularColorPass
void __usercall RB_NV20_DI_DiffuseAndSpecularColorPass(const drawInteraction_t *din@<esi>)
{
  GL_State(backEnd.depthFunc | 0x127);
  backEnd.glState.currenttmu = 2;
  qglActiveTextureARB(0x84C2u);
  din->diffuseImage->Bind(din->diffuseImage);
  backEnd.glState.currenttmu = 3;
  qglActiveTextureARB(0x84C3u);
  din->specularImage->Bind(din->specularImage);
  RB_NV20_DiffuseAndSpecularColorFragment();
  qglBindProgramARB(0x8620u, 9u);
  RB_DrawElementsWithCounters(din->surf->geo);
}

// FUNC: RB_NV20_DrawInteraction
void __cdecl RB_NV20_DrawInteraction(const drawInteraction_t *din)
{
  bool v1; // bl
  int integerValue; // eax

  qglProgramEnvParameter4fvARB(0x8620u, 4u, &din->localLightOrigin.x);
  qglProgramEnvParameter4fvARB(0x8620u, 5u, &din->localViewOrigin.x);
  qglProgramEnvParameter4fvARB(0x8620u, 6u, &din->lightProjection[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, 7u, &din->lightProjection[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, 8u, &din->lightProjection[2].x);
  qglProgramEnvParameter4fvARB(0x8620u, 9u, &din->lightProjection[3].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xAu, &din->bumpMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xBu, &din->bumpMatrix[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xCu, &din->diffuseMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xDu, &din->diffuseMatrix[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xEu, &din->specularMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xFu, &din->specularMatrix[1].x);
  qglCombinerParameterfvNV(0x852Au, &din->diffuseColor.x);
  qglCombinerParameterfvNV(0x852Bu, &din->specularColor.x);
  GL_PolygonOffset(din->surf->material, 1);
  v1 = r_forceDiffuseOnly.internalVar->integerValue != 0;
  if ( din->vertexColor )
  {
    GL_EnableVertexAttribState(4u);
    if ( !v1 )
      RB_NV20_DI_BumpAndLightPass(din, 0);
    RB_NV20_DI_DiffuseColorPass(din);
    if ( !v1 )
      RB_NV20_DI_SpecularColorPass(din);
    GL_DisableVertexAttribState(4u);
  }
  else
  {
    qglColor3f(1.0, 1.0, 1.0);
    integerValue = r_useNV20MonoLights.internalVar->integerValue;
    if ( integerValue == 2 || din->lightImage->isMonochrome && integerValue )
    {
      if ( !v1 )
      {
        RB_NV20_DI_BumpAndLightPass(din, 1);
        RB_NV20_DI_DiffuseAndSpecularColorPass(din);
        GL_PolygonOffset(din->surf->material, 0);
        return;
      }
      RB_NV20_DI_DiffuseColorPass(din);
    }
    else
    {
      if ( !v1 )
        RB_NV20_DI_BumpAndLightPass(din, 0);
      RB_NV20_DI_DiffuseColorPass(din);
      if ( !v1 )
      {
        RB_NV20_DI_SpecularColorPass(din);
        GL_PolygonOffset(din->surf->material, 0);
        return;
      }
    }
    GL_PolygonOffset(din->surf->material, 0);
  }
}

// FUNC: RB_NV20_CreateDrawInteractions
void __usercall RB_NV20_CreateDrawInteractions(const drawSurf_s *surf@<eax>)
{
  const drawSurf_s *v1; // edi
  char *v2; // esi

  v1 = surf;
  if ( surf )
  {
    qglEnable(0x8620u);
    qglEnable(0x8522u);
    GL_VertexAttribState(0xF000001u);
    do
    {
      v2 = (char *)idVertexCache::Position(&vertexCache, v1->geo->ambientCache);
      qglColorPointer(4, 0x1401u, 64, v2 + 12);
      qglVertexAttribPointerARB(0xBu, 3, 0x1406u, 0, 64, v2 + 16);
      qglVertexAttribPointerARB(0xAu, 3, 0x1406u, 0, 64, v2 + 44);
      qglVertexAttribPointerARB(9u, 3, 0x1406u, 0, 64, v2 + 32);
      qglVertexAttribPointerARB(8u, 2, 0x1406u, 0, 64, v2 + 56);
      qglVertexPointer(3, 0x1406u, 64, v2);
      RB_CreateSingleDrawInteractions(v1, RB_NV20_DrawInteraction);
      v1 = v1->nextOnLight;
    }
    while ( v1 );
    GL_VertexAttribState(1u);
    backEnd.glState.currenttmu = 3;
    qglActiveTextureARB(0x84C3u);
    globalImages->BindNull(globalImages);
    backEnd.glState.currenttmu = 2;
    qglActiveTextureARB(0x84C2u);
    globalImages->BindNull(globalImages);
    backEnd.glState.currenttmu = 1;
    qglActiveTextureARB(0x84C1u);
    globalImages->BindNull(globalImages);
    backEnd.glState.currenttmu = -1;
    GL_SelectTexture(0);
    GL_EnableVertexAttribState(8u);
    qglDisable(0x8620u);
    qglDisable(0x8522u);
  }
}

// FUNC: void __cdecl RB_NV20_DrawInteractions(void)
void __cdecl RB_NV20_DrawInteractions()
{
  viewLight_s *i; // esi
  const idMaterial *lightShader; // eax
  int v2; // ecx
  int v3; // edx

  for ( i = backEnd.viewDef->viewLights; i; i = i->next )
  {
    lightShader = i->lightShader;
    if ( !lightShader->fogLight
      && !lightShader->blendLight
      && (i->localInteractions || i->globalInteractions || i->translucentInteractions) )
    {
      backEnd.vLight = i;
      if ( i->globalShadows || i->localShadows )
      {
        v2 = *(_DWORD *)&i->scissorRect.x1;
        *(_DWORD *)&backEnd.currentScissor.x1 = v2;
        v3 = *(_DWORD *)&i->scissorRect.x2;
        *(_DWORD *)&backEnd.currentScissor.x2 = v3;
        backEnd.currentScissor.zmin = i->scissorRect.zmin;
        backEnd.currentScissor.zmax = i->scissorRect.zmax;
        if ( r_useScissor.internalVar->integerValue )
          qglScissor(
            (__int16)v2 + backEnd.viewDef->viewport.x1,
            (v2 >> 16) + backEnd.viewDef->viewport.y1,
            (__int16)v3 - (__int16)v2 + 1,
            (v3 >> 16) - (v2 >> 16) + 1);
        qglClear(0x400u);
      }
      else
      {
        qglStencilFunc(0x207u, 128, 0xFFu);
      }
      backEnd.depthFunc = 0x20000;
      if ( r_useShadowVertexProgram.internalVar->integerValue )
      {
        qglEnable(0x8620u);
        qglBindProgramARB(0x8620u, 5u);
        RB_StencilShadowPass(i->globalShadows);
        RB_NV20_CreateDrawInteractions(i->localInteractions);
        qglEnable(0x8620u);
        qglBindProgramARB(0x8620u, 5u);
        RB_StencilShadowPass(i->localShadows);
        RB_NV20_CreateDrawInteractions(i->globalInteractions);
        qglDisable(0x8620u);
      }
      else
      {
        RB_StencilShadowPass(i->globalShadows);
        RB_NV20_CreateDrawInteractions(i->localInteractions);
        RB_StencilShadowPass(i->localShadows);
        RB_NV20_CreateDrawInteractions(i->globalInteractions);
      }
      if ( !r_skipTranslucent.internalVar->integerValue )
      {
        qglStencilFunc(0x207u, 128, 0xFFu);
        backEnd.depthFunc = 0;
        RB_NV20_CreateDrawInteractions(i->translucentInteractions);
        backEnd.depthFunc = 0x20000;
      }
    }
  }
}

// FUNC: void __cdecl R_NV20_Init(void)
void __thiscall R_NV20_Init(void *this)
{
  BOOL retaddr; // [esp+8h] [ebp+0h]

  glConfig.allowNV20Path = 0;
  (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
    common.type,
    "---------------- R_NV20_Init ----------------\n");
  if ( glConfig.registerCombinersAvailable && glConfig.ARBVertexProgramAvailable && glConfig.maxTextureUnits >= 4 )
  {
    GL_CheckErrors();
    fragmentDisplayListBase = qglGenLists(4);
    LOBYTE(this) = r_useCombinerDisplayLists.internalVar->integerValue != 0;
    ((void (__stdcall *)(_DWORD, void *))r_useCombinerDisplayLists.internalVar->InternalSetBool)(0, this);
    qglNewList(fragmentDisplayListBase, 0x1300u);
    RB_NV20_BumpAndLightFragment();
    qglEndList();
    qglNewList(fragmentDisplayListBase + 1, 0x1300u);
    RB_NV20_DiffuseColorFragment();
    qglEndList();
    qglNewList(fragmentDisplayListBase + 2, 0x1300u);
    RB_NV20_SpecularColorFragment();
    qglEndList();
    qglNewList(fragmentDisplayListBase + 3, 0x1300u);
    RB_NV20_DiffuseAndSpecularColorFragment();
    qglEndList();
    r_useCombinerDisplayLists.internalVar->InternalSetBool(r_useCombinerDisplayLists.internalVar, retaddr);
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
      common.type,
      "---------------------------------------------\n");
    glConfig.allowNV20Path = 1;
  }
  else
  {
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, "Not available.\n");
  }
}
