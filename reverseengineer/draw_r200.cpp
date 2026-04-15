
// FUNC: R_BuildSurfaceFragmentProgram
void __usercall R_BuildSurfaceFragmentProgram(unsigned int programNum@<eax>)
{
  float data[4]; // [esp+0h] [ebp-10h] BYREF

  qglBindFragmentShaderATI(programNum);
  qglBeginFragmentShaderATI();
  qglSampleMapATI(0x8921u, 0x84C0u, 0x8979u);
  qglSampleMapATI(0x8922u, 0x84C1u, 0x8976u);
  qglSampleMapATI(0x8925u, 0x84C2u, 0x8976u);
  qglSampleMapATI(0x8926u, 0x84C5u, 0x8976u);
  if ( image_useNormalCompression.internalVar->integerValue == 2 )
    qglColorFragmentOp1ATI(0x8961u, 0x8925u, 1u, 0, 0x8925u, 0x1906u, 0);
  qglColorFragmentOp2ATI(0x8964u, 0x8921u, 0, 0, 0x8921u, 0, 0, 0x8922u, 0, 0);
  qglColorFragmentOp2ATI(0x8966u, 0x8922u, 0, 0x40u, 0x8925u, 0, 9u, 0x8926u, 0, 9u);
  qglColorFragmentOp2ATI(0x8964u, 0x8921u, 0, 0, 0x8921u, 0, 0, 0x8922u, 0, 0);
  qglPassTexCoordATI(0x8921u, 0x8921u, 0x8976u);
  qglSampleMapATI(0x8923u, 0x84C3u, 0x8976u);
  qglSampleMapATI(0x8924u, 0x84C3u, 0x8976u);
  qglPassTexCoordATI(0x8925u, 0x8925u, 0x8976u);
  qglSampleMapATI(0x8926u, 0x84C4u, 0x8976u);
  qglColorFragmentOp2ATI(0x8966u, 0x8922u, 0, 0x40u, 0x8925u, 0, 9u, 0x8926u, 0, 9u);
  data[0] = 0.75;
  data[1] = 0.75;
  data[2] = 0.75;
  data[3] = 0.75;
  qglSetFragmentShaderConstantATI(0x8946u, data);
  qglColorFragmentOp2ATI(0x8965u, 0x8922u, 0, 0x42u, 0x8922u, 0, 0, 0x8946u, 0, 0);
  qglColorFragmentOp2ATI(0x8964u, 0x8922u, 0, 0x40u, 0x8922u, 0, 0, 0x8922u, 0, 0);
  qglColorFragmentOp2ATI(0x8964u, 0x8922u, 0, 0x41u, 0x8922u, 0, 0, 0x8924u, 0, 0);
  qglColorFragmentOp2ATI(0x8964u, 0x8923u, 0, 0x40u, 0x8923u, 0, 0, 0x8941u, 0, 0);
  qglColorFragmentOp3ATI(0x8968u, 0x8923u, 0, 0x40u, 0x8922u, 0, 0, 0x8942u, 0, 0, 0x8923u, 0, 0);
  qglColorFragmentOp2ATI(0x8964u, 0x8921u, 0, 0x40u, 0x8921u, 0, 0, 0x8923u, 0, 0);
  qglColorFragmentOp2ATI(0x8964u, 0x8921u, 0, 0, 0x8921u, 0, 0, 0x8577u, 0, 0);
  qglAlphaFragmentOp1ATI(0x8961u, 0x8921u, 0, 0, 0, 0);
  qglEndFragmentShaderATI();
  GL_CheckErrors();
}

// FUNC: void __cdecl R_R200_Init(void)
void __cdecl R_R200_Init()
{
  glConfig.allowR200Path = 0;
  (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
    common.type,
    "----------------- R200_Init -----------------\n");
  if ( glConfig.atiFragmentShaderAvailable
    && glConfig.ARBVertexProgramAvailable
    && glConfig.ARBVertexBufferObjectAvailable )
  {
    GL_CheckErrors();
    qglGetIntegerv(0x896Eu, &fsi.numFragmentRegisters);
    qglGetIntegerv(0x896Fu, &fsi.numFragmentConstants);
    qglGetIntegerv(0x8970u, &fsi.numPasses);
    qglGetIntegerv(0x8971u, &fsi.numInstructionsPerPass);
    qglGetIntegerv(0x8972u, &fsi.numInstructionsTotal);
    qglGetIntegerv(0x8975u, &fsi.colorAlphaPairing);
    qglGetIntegerv(0x8974u, &fsi.numLoopbackComponenets);
    qglGetIntegerv(0x8973u, &fsi.numInputInterpolatorComponents);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "GL_NUM_FRAGMENT_REGISTERS_ATI: %i\n",
      fsi.numFragmentRegisters);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "GL_NUM_FRAGMENT_CONSTANTS_ATI: %i\n",
      fsi.numFragmentConstants);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "GL_NUM_PASSES_ATI: %i\n",
      fsi.numPasses);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "GL_NUM_INSTRUCTIONS_PER_PASS_ATI: %i\n",
      fsi.numInstructionsPerPass);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "GL_NUM_INSTRUCTIONS_TOTAL_ATI: %i\n",
      fsi.numInstructionsTotal);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "GL_COLOR_ALPHA_PAIRING_ATI: %i\n",
      fsi.colorAlphaPairing);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "GL_NUM_LOOPBACK_COMPONENTS_ATI: %i\n",
      fsi.numLoopbackComponenets);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI: %i\n",
      fsi.numInputInterpolatorComponents);
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, "FPROG_FAST_PATH\n");
    R_BuildSurfaceFragmentProgram(FPROG_FAST_PATH);
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
      common.type,
      "---------------------\n");
    glConfig.allowR200Path = 1;
  }
  else
  {
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, "Not available.\n");
  }
}

// FUNC: RB_R200_ARB_DrawInteraction
void __cdecl RB_R200_ARB_DrawInteraction(drawInteraction_t *din)
{
  drawInteraction_t *p_d; // ebx
  const srfTriangles_s *geo; // edi
  char *v3; // esi
  stageVertexColor_t vertexColor; // eax
  __int32 v5; // eax
  idImage *v6; // eax
  drawInteraction_t d; // [esp+10h] [ebp-100h] BYREF

  p_d = din;
  GL_PolygonOffset(din->surf->material, 1);
  if ( din->diffuseImage != globalImages->GetBlackImage(globalImages)
    && din->specularImage != globalImages->GetBlackImage(globalImages)
    && memcmp(din->specularMatrix, din->diffuseMatrix, 0x20u) )
  {
    qmemcpy(&d, din, sizeof(d));
    d.diffuseImage = globalImages->GetBlackImage(globalImages);
    SIMDProcessor->Memcpy(SIMDProcessor, d.diffuseMatrix, d.specularMatrix, 32);
    RB_R200_ARB_DrawInteraction(&d);
    qmemcpy(&d, din, sizeof(d));
    p_d = &d;
    d.specularImage = globalImages->GetBlackImage(globalImages);
  }
  qglProgramEnvParameter4fvARB(0x8620u, 4u, &p_d->localLightOrigin.x);
  qglProgramEnvParameter4fvARB(0x8620u, 5u, &p_d->localViewOrigin.x);
  qglProgramEnvParameter4fvARB(0x8620u, 6u, &p_d->lightProjection[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, 7u, &p_d->lightProjection[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, 8u, &p_d->lightProjection[2].x);
  qglProgramEnvParameter4fvARB(0x8620u, 9u, &p_d->lightProjection[3].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xAu, &p_d->bumpMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xBu, &p_d->bumpMatrix[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xCu, &p_d->diffuseMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xDu, &p_d->diffuseMatrix[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xEu, &p_d->diffuseMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, 0xFu, &p_d->diffuseMatrix[1].x);
  geo = p_d->surf->geo;
  v3 = (char *)idVertexCache::Position(&vertexCache, geo->ambientCache);
  qglVertexPointer(3, 0x1406u, 64, v3);
  vertexColor = p_d->vertexColor;
  if ( vertexColor )
  {
    v5 = vertexColor - 1;
    if ( v5 )
    {
      if ( v5 == 1 )
        qglProgramEnvParameter4fvARB(0x8620u, 0x10u, negOneOne);
    }
    else
    {
      qglProgramEnvParameter4fvARB(0x8620u, 0x10u, oneZero);
    }
  }
  else
  {
    qglProgramEnvParameter4fvARB(0x8620u, 0x10u, zeroOne);
  }
  GL_SelectTexture(5);
  if ( p_d->ambientLight )
    v6 = globalImages->GetAmbientNormalMap(globalImages);
  else
    v6 = globalImages->GetNormalCubeMapImage(globalImages);
  v6->Bind(v6);
  GL_SelectTexture(4);
  p_d->bumpImage->Bind(p_d->bumpImage);
  GL_SelectTexture(3);
  p_d->specularImage->Bind(p_d->specularImage);
  qglTexCoordPointer(3, 0x1406u, 64, v3 + 16);
  GL_SelectTexture(2);
  p_d->diffuseImage->Bind(p_d->diffuseImage);
  qglTexCoordPointer(3, 0x1406u, 64, v3 + 44);
  GL_SelectTexture(1);
  p_d->lightFalloffImage->Bind(p_d->lightFalloffImage);
  qglTexCoordPointer(3, 0x1406u, 64, v3 + 32);
  GL_SelectTexture(0);
  p_d->lightImage->Bind(p_d->lightImage);
  qglTexCoordPointer(2, 0x1406u, 64, v3 + 56);
  qglSetFragmentShaderConstantATI(0x8941u, &p_d->diffuseColor.x);
  qglSetFragmentShaderConstantATI(0x8942u, &p_d->specularColor.x);
  if ( p_d->vertexColor )
  {
    qglColorPointer(4, 0x1401u, 64, v3 + 12);
    GL_EnableVertexAttribState(4u);
    RB_DrawElementsWithCounters(geo);
    GL_DisableVertexAttribState(4u);
    qglColor4f(1.0, 1.0, 1.0, 1.0);
  }
  else
  {
    RB_DrawElementsWithCounters(geo);
  }
  GL_PolygonOffset(p_d->surf->material, 0);
}

// FUNC: RB_R200_ARB_CreateDrawInteractions
void __usercall RB_R200_ARB_CreateDrawInteractions(const drawSurf_s *surf@<eax>)
{
  const drawSurf_s *v1; // esi

  v1 = surf;
  if ( surf )
  {
    backEnd.currentSpace = 0;
    if ( surf->material->coverage == MC_TRANSLUCENT )
      GL_State(288);
    else
      GL_State(131360);
    qglBindProgramARB(0x8620u, 4u);
    qglEnable(0x8620u);
    qglBindFragmentShaderATI(FPROG_FAST_PATH);
    qglEnable(0x8920u);
    qglColor4f(1.0, 1.0, 1.0, 1.0);
    GL_EnableVertexAttribState(0x30u);
    GL_SelectTexture(3);
    GL_EnableVertexAttribState(0x40u);
    do
    {
      RB_CreateSingleDrawInteractions(v1, (void (__cdecl *)(const drawInteraction_t *))RB_R200_ARB_DrawInteraction);
      v1 = v1->nextOnLight;
    }
    while ( v1 );
    GL_SelectTexture(5);
    globalImages->BindNull(globalImages);
    GL_SelectTexture(4);
    globalImages->BindNull(globalImages);
    GL_SelectTexture(3);
    globalImages->BindNull(globalImages);
    GL_DisableVertexAttribState(0x40u);
    GL_SelectTexture(2);
    globalImages->BindNull(globalImages);
    GL_DisableVertexAttribState(0x20u);
    GL_SelectTexture(1);
    globalImages->BindNull(globalImages);
    GL_DisableVertexAttribState(0x10u);
    GL_SelectTexture(0);
    qglDisable(0x8620u);
    qglDisable(0x8920u);
  }
}

// FUNC: void __cdecl RB_R200_DrawInteractions(void)
void __cdecl RB_R200_DrawInteractions()
{
  const viewDef_s *viewDef; // edi
  viewLight_s *viewLights; // esi
  const idMaterial *lightShader; // eax
  int v3; // ecx
  int v4; // edx

  qglEnable(0xB90u);
  viewDef = backEnd.viewDef;
  viewLights = backEnd.viewDef->viewLights;
  if ( viewLights )
  {
    while ( 1 )
    {
      lightShader = viewLights->lightShader;
      if ( !lightShader->fogLight && !lightShader->blendLight )
      {
        backEnd.vLight = viewLights;
        if ( viewLights->globalShadows || viewLights->localShadows )
        {
          v3 = *(_DWORD *)&viewLights->scissorRect.x1;
          *(_DWORD *)&backEnd.currentScissor.x1 = v3;
          v4 = *(_DWORD *)&viewLights->scissorRect.x2;
          *(_DWORD *)&backEnd.currentScissor.x2 = v4;
          backEnd.currentScissor.zmin = viewLights->scissorRect.zmin;
          backEnd.currentScissor.zmax = viewLights->scissorRect.zmax;
          if ( r_useScissor.internalVar->integerValue )
            qglScissor(
              (__int16)v3 + viewDef->viewport.x1,
              (v3 >> 16) + viewDef->viewport.y1,
              (__int16)v4 - (__int16)v3 + 1,
              (v4 >> 16) - (v3 >> 16) + 1);
          qglClear(0x400u);
        }
        else
        {
          qglStencilFunc(0x207u, 128, 0xFFu);
        }
        if ( r_useShadowVertexProgram.internalVar->integerValue )
        {
          qglEnable(0x8620u);
          qglBindProgramARB(0x8620u, 5u);
          RB_StencilShadowPass(viewLights->globalShadows);
          RB_R200_ARB_CreateDrawInteractions(viewLights->localInteractions);
          qglEnable(0x8620u);
          qglBindProgramARB(0x8620u, 5u);
          RB_StencilShadowPass(viewLights->localShadows);
          RB_R200_ARB_CreateDrawInteractions(viewLights->globalInteractions);
          qglDisable(0x8620u);
        }
        else
        {
          RB_StencilShadowPass(viewLights->globalShadows);
          RB_R200_ARB_CreateDrawInteractions(viewLights->localInteractions);
          RB_StencilShadowPass(viewLights->localShadows);
          RB_R200_ARB_CreateDrawInteractions(viewLights->globalInteractions);
        }
        if ( !r_skipTranslucent.internalVar->integerValue )
        {
          qglStencilFunc(0x207u, 128, 0xFFu);
          RB_R200_ARB_CreateDrawInteractions(viewLights->translucentInteractions);
        }
      }
      viewLights = viewLights->next;
      if ( !viewLights )
        break;
      viewDef = backEnd.viewDef;
    }
  }
}
