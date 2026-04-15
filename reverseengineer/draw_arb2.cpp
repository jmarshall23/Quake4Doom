
// FUNC: void __cdecl RB_ARB2_LoadMVPMatrixIntoVPParams(struct drawSurf_s const *)
void __cdecl RB_ARB2_LoadMVPMatrixIntoVPParams(const drawSurf_s *surf)
{
  const viewEntity_s *space; // eax
  float v2; // [esp+4h] [ebp-10h] BYREF
  float v3; // [esp+8h] [ebp-Ch]
  float v4; // [esp+Ch] [ebp-8h]
  float v5; // [esp+10h] [ebp-4h]

  space = surf->space;
  if ( backEnd.mvpSpace != space )
  {
    myGlMultMatrix(space->modelViewMatrix, backEnd.projectionMatrix, backEnd.modelViewProjection);
    backEnd.mvpSpace = surf->space;
  }
  v3 = backEnd.modelViewProjection[4];
  v2 = backEnd.modelViewProjection[0];
  v4 = backEnd.modelViewProjection[8];
  v5 = backEnd.modelViewProjection[12];
  qglProgramEnvParameter4fvARB(0x8620u, 0x4Bu, &v2);
  v3 = backEnd.modelViewProjection[5];
  v2 = backEnd.modelViewProjection[1];
  v4 = backEnd.modelViewProjection[9];
  v5 = backEnd.modelViewProjection[13];
  qglProgramEnvParameter4fvARB(0x8620u, 0x4Cu, &v2);
  v3 = backEnd.modelViewProjection[6];
  v2 = backEnd.modelViewProjection[2];
  v4 = backEnd.modelViewProjection[10];
  v5 = backEnd.modelViewProjection[14];
  qglProgramEnvParameter4fvARB(0x8620u, 0x4Du, &v2);
  v3 = backEnd.modelViewProjection[7];
  v2 = backEnd.modelViewProjection[3];
  v4 = backEnd.modelViewProjection[11];
  v5 = backEnd.modelViewProjection[15];
  qglProgramEnvParameter4fvARB(0x8620u, 0x4Eu, &v2);
}

// FUNC: void __cdecl RB_ARB2_LoadProjectionMatrixIntoVPParams(struct drawSurf_s const *)
void __cdecl RB_ARB2_LoadProjectionMatrixIntoVPParams()
{
  float v0; // [esp+0h] [ebp-10h] BYREF
  float v1; // [esp+4h] [ebp-Ch]
  float v2; // [esp+8h] [ebp-8h]
  float v3; // [esp+Ch] [ebp-4h]

  v1 = backEnd.projectionMatrix[4];
  v0 = backEnd.projectionMatrix[0];
  v2 = backEnd.projectionMatrix[8];
  v3 = backEnd.projectionMatrix[12];
  qglProgramEnvParameter4fvARB(0x8620u, 0x51u, &v0);
  v1 = backEnd.projectionMatrix[5];
  v0 = backEnd.projectionMatrix[1];
  v2 = backEnd.projectionMatrix[9];
  v3 = backEnd.projectionMatrix[13];
  qglProgramEnvParameter4fvARB(0x8620u, 0x52u, &v0);
  v1 = backEnd.projectionMatrix[6];
  v0 = backEnd.projectionMatrix[2];
  v2 = backEnd.projectionMatrix[10];
  v3 = backEnd.projectionMatrix[14];
  qglProgramEnvParameter4fvARB(0x8620u, 0x53u, &v0);
  v1 = backEnd.projectionMatrix[7];
  v0 = backEnd.projectionMatrix[3];
  v2 = backEnd.projectionMatrix[11];
  v3 = backEnd.projectionMatrix[15];
  qglProgramEnvParameter4fvARB(0x8620u, 0x54u, &v0);
}

// FUNC: void __cdecl RB_ARB2_LoadShaderTextureMatrixIntoVPParams(float const *,struct textureStage_t const *)
void __cdecl RB_ARB2_LoadShaderTextureMatrixIntoVPParams(const float *shaderRegisters, const textureStage_t *texture)
{
  float v2; // [esp+0h] [ebp-50h] BYREF
  float v3; // [esp+4h] [ebp-4Ch]
  float v4; // [esp+8h] [ebp-48h]
  float v5; // [esp+Ch] [ebp-44h]
  float matrix[16]; // [esp+10h] [ebp-40h] BYREF

  RB_GetShaderTextureMatrix(shaderRegisters, texture, matrix);
  v3 = matrix[4];
  v2 = matrix[0];
  v4 = matrix[8];
  v5 = matrix[12];
  qglProgramEnvParameter4fvARB(0x8620u, 0x55u, &v2);
  v3 = matrix[5];
  v2 = matrix[1];
  v4 = matrix[9];
  v5 = matrix[13];
  qglProgramEnvParameter4fvARB(0x8620u, 0x56u, &v2);
}

// FUNC: void __cdecl RB_ARB2_LoadModelMatrixIntoVPParams(struct drawSurf_s const *)
void __cdecl RB_ARB2_LoadModelMatrixIntoVPParams(const drawSurf_s *surf)
{
  const viewEntity_s *space; // esi
  int v2; // edx
  float v3; // ecx
  int v4; // eax
  int v5; // ecx
  float v6; // eax
  int v7; // edx
  int v8; // eax
  float v9; // edx
  int v10; // ecx
  int v11; // edx
  float v12; // [esp+4h] [ebp-10h] BYREF
  float zmax; // [esp+8h] [ebp-Ch]
  int v14; // [esp+Ch] [ebp-8h]
  int v15; // [esp+10h] [ebp-4h]

  space = surf->space;
  v2 = LODWORD(space->modelMatrix[4]);
  v3 = space->modelMatrix[0];
  v4 = LODWORD(space->modelMatrix[8]);
  space = (const viewEntity_s *)((char *)space + 32);
  zmax = *(float *)&v2;
  v12 = v3;
  v5 = LODWORD(space->modelMatrix[4]);
  v14 = v4;
  v15 = v5;
  qglProgramEnvParameter4fvARB(0x8620u, 0x57u, &v12);
  v6 = *(float *)&space->entityDef;
  v7 = LODWORD(space->modelMatrix[1]);
  zmax = space->scissorRect.zmax;
  v12 = v6;
  v8 = LODWORD(space->modelMatrix[5]);
  v14 = v7;
  v15 = v8;
  qglProgramEnvParameter4fvARB(0x8620u, 0x58u, &v12);
  v9 = *(float *)&space->scissorRect.x1;
  v10 = LODWORD(space->modelMatrix[2]);
  zmax = *(float *)&space->weaponDepthHackInViewID;
  v12 = v9;
  v11 = LODWORD(space->modelMatrix[6]);
  v14 = v10;
  v15 = v11;
  qglProgramEnvParameter4fvARB(0x8620u, 0x59u, &v12);
}

// FUNC: void __cdecl RB_ARB2_LoadModelViewMatrixIntoVPParams(struct drawSurf_s const *)
void __cdecl RB_ARB2_LoadModelViewMatrixIntoVPParams(const drawSurf_s *surf)
{
  const viewEntity_s *space; // esi
  int v2; // edx
  float v3; // ecx
  int v4; // eax
  int v5; // ecx
  float v6; // eax
  int v7; // edx
  int v8; // eax
  float v9; // edx
  int v10; // ecx
  int v11; // edx
  float v12; // [esp+4h] [ebp-10h] BYREF
  float zmax; // [esp+8h] [ebp-Ch]
  int v14; // [esp+Ch] [ebp-8h]
  int v15; // [esp+10h] [ebp-4h]

  space = surf->space;
  v2 = LODWORD(space->modelViewMatrix[4]);
  v3 = space->modelViewMatrix[0];
  v4 = LODWORD(space->modelViewMatrix[8]);
  space = (const viewEntity_s *)((char *)space + 96);
  zmax = *(float *)&v2;
  v12 = v3;
  v5 = LODWORD(space->modelMatrix[4]);
  v14 = v4;
  v15 = v5;
  qglProgramEnvParameter4fvARB(0x8620u, 0x57u, &v12);
  v6 = *(float *)&space->entityDef;
  v7 = LODWORD(space->modelMatrix[1]);
  zmax = space->scissorRect.zmax;
  v12 = v6;
  v8 = LODWORD(space->modelMatrix[5]);
  v14 = v7;
  v15 = v8;
  qglProgramEnvParameter4fvARB(0x8620u, 0x58u, &v12);
  v9 = *(float *)&space->scissorRect.x1;
  v10 = LODWORD(space->modelMatrix[2]);
  zmax = *(float *)&space->weaponDepthHackInViewID;
  v12 = v9;
  v11 = LODWORD(space->modelMatrix[6]);
  v14 = v10;
  v15 = v11;
  qglProgramEnvParameter4fvARB(0x8620u, 0x59u, &v12);
}

// FUNC: void __cdecl RB_ARB2_MD5R_DrawDepthElements(struct drawSurf_s const *)
void __cdecl RB_ARB2_MD5R_DrawDepthElements(const drawSurf_s *surf)
{
  rvMesh *primBatchMesh; // ebx
  int v3; // esi
  const rvVertexFormat *v4; // edi
  const rvVertexFormat *v5; // esi
  rvVertexFormat *surfa; // [esp+14h] [ebp+4h]

  primBatchMesh = surf->geo->primBatchMesh;
  surfa = (rvVertexFormat *)rvMesh::GetDrawVertexBufferFormat(primBatchMesh);
  v3 = 0;
  v4 = supportedMD5RDepthVertexFormats;
  while ( !rvVertexFormat::HasSameComponents(surfa, v4) )
  {
    ++v4;
    ++v3;
    if ( (int)v4 >= (int)supportedMD5RShadowVolVertexFormats )
      return;
  }
  if ( v3 < 3 )
  {
    qglBindProgramARB(0x8620u, v3 + 24);
    qglEnable(0x8620u);
    RB_ARB2_LoadMVPMatrixIntoVPParams(surf);
    v5 = &supportedMD5RDepthVertexFormats[v3];
    rvMesh::SetupForDrawRender(primBatchMesh, v5);
    GL_DisableVertexAttribState(4u);
    rvMesh::Draw(primBatchMesh, surf->geo->skinToModelTransforms, v5);
    qglBindProgramARB(0x8620u, 0);
    qglDisable(0x8620u);
    qglBindBufferARB(0x8892u, 0);
    qglBindBufferARB(0x8893u, 0);
  }
}

// FUNC: void __cdecl RB_ARB2_DrawInteraction(struct drawInteraction_t const *)
void __cdecl RB_ARB2_DrawInteraction(const drawInteraction_t *din)
{
  int v1; // edi
  stageVertexColor_t vertexColor; // eax
  __int32 v3; // eax
  float specularColor_x2[4]; // [esp+8h] [ebp-10h] BYREF

  if ( din->surf->geo->primBatchMesh )
  {
    RB_ARB2_LoadMVPMatrixIntoVPParams(din->surf);
    v1 = 75;
  }
  else
  {
    v1 = 0;
  }
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 4, &din->localLightOrigin.x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 5, &din->localViewOrigin.x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 6, &din->lightProjection[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 7, &din->lightProjection[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 8, &din->lightProjection[2].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 9, &din->lightProjection[3].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 10, &din->bumpMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 11, &din->bumpMatrix[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 12, &din->diffuseMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 13, &din->diffuseMatrix[1].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 14, &din->specularMatrix[0].x);
  qglProgramEnvParameter4fvARB(0x8620u, v1 + 15, &din->specularMatrix[1].x);
  if ( r_testARBProgram.internalVar->integerValue )
  {
    qglProgramEnvParameter4fvARB(0x8804u, 2u, &din->localLightOrigin.x);
    qglProgramEnvParameter4fvARB(0x8804u, 3u, &din->localViewOrigin.x);
  }
  vertexColor = din->vertexColor;
  if ( vertexColor )
  {
    v3 = vertexColor - 1;
    if ( v3 )
    {
      if ( v3 == 1 )
        qglProgramEnvParameter4fvARB(0x8620u, v1 + 16, negOneOne);
    }
    else
    {
      qglProgramEnvParameter4fvARB(0x8620u, v1 + 16, oneZero);
    }
  }
  else
  {
    qglProgramEnvParameter4fvARB(0x8620u, v1 + 16, zeroOne);
  }
  qglProgramEnvParameter4fvARB(0x8804u, 0, &din->diffuseColor.x);
  specularColor_x2[0] = din->specularColor.x + din->specularColor.x;
  specularColor_x2[1] = din->specularColor.y + din->specularColor.y;
  specularColor_x2[2] = din->specularColor.z + din->specularColor.z;
  specularColor_x2[3] = din->specularColor.w + din->specularColor.w;
  qglProgramEnvParameter4fvARB(0x8804u, 1u, specularColor_x2);
  GL_PolygonOffset(din->surf->material, 1);
  backEnd.glState.currenttmu = 1;
  qglActiveTextureARB(0x84C1u);
  din->bumpImage->Bind(din->bumpImage);
  backEnd.glState.currenttmu = 2;
  qglActiveTextureARB(0x84C2u);
  din->lightFalloffImage->Bind(din->lightFalloffImage);
  backEnd.glState.currenttmu = 3;
  qglActiveTextureARB(0x84C3u);
  din->lightImage->Bind(din->lightImage);
  backEnd.glState.currenttmu = 4;
  qglActiveTextureARB(0x84C4u);
  din->diffuseImage->Bind(din->diffuseImage);
  backEnd.glState.currenttmu = 5;
  qglActiveTextureARB(0x84C5u);
  din->specularImage->Bind(din->specularImage);
  RB_DrawElementsWithCounters(din->surf->geo);
  GL_PolygonOffset(din->surf->material, 0);
}

// FUNC: void __cdecl RB_ARB2_CreateDrawInteractions(struct drawSurf_s const *)
void __cdecl RB_ARB2_CreateDrawInteractions(const drawSurf_s *surf)
{
  const drawSurf_s *v1; // ebx
  rvMesh *v2; // edi
  unsigned int v3; // ebp
  idImage *v4; // eax
  rvMesh *primBatchMesh; // esi
  const rvVertexFormat *DrawVertexBufferFormat; // eax
  int v7; // edx
  const rvVertexFormat *v8; // ecx
  char *v9; // esi
  program_t defaultVertexProgram; // [esp+8h] [ebp-4h]
  unsigned int surfa; // [esp+10h] [ebp+4h]

  v1 = surf;
  v2 = 0;
  if ( surf )
  {
    GL_State(backEnd.depthFunc | 0x120);
    if ( r_testARBProgram.internalVar->integerValue )
    {
      v3 = 10;
      defaultVertexProgram = VPROG_TEST;
      surfa = 10;
      qglBindProgramARB(0x8620u, 0xAu);
      qglBindProgramARB(0x8804u, 0xEu);
    }
    else if ( r_useSimpleInteraction.internalVar->integerValue )
    {
      v3 = 19;
      defaultVertexProgram = VPROG_SIMPLE_INTERACTION;
      surfa = 19;
      qglBindProgramARB(0x8620u, 0x13u);
      qglBindProgramARB(0x8804u, 0x14u);
    }
    else
    {
      v3 = 1;
      defaultVertexProgram = VPROG_INTERACTION;
      surfa = 1;
      qglBindProgramARB(0x8620u, 1u);
      qglBindProgramARB(0x8804u, 0xBu);
    }
    qglEnable(0x8620u);
    qglEnable(0x8804u);
    backEnd.glState.currenttmu = 0;
    qglActiveTextureARB(0x84C0u);
    if ( backEnd.vLight->lightShader->ambientLight )
      v4 = globalImages->GetAmbientNormalMap(globalImages);
    else
      v4 = globalImages->GetNormalCubeMapImage(globalImages);
    v4->Bind(v4);
    do
    {
      primBatchMesh = v1->geo->primBatchMesh;
      if ( primBatchMesh )
      {
        DrawVertexBufferFormat = rvMesh::GetDrawVertexBufferFormat(v1->geo->primBatchMesh);
        v7 = 0;
        v8 = supportedMD5RVertexFormats;
        while ( DrawVertexBufferFormat->m_flags != v8->m_flags
             || DrawVertexBufferFormat->m_dimensions != v8->m_dimensions )
        {
          ++v8;
          ++v7;
          if ( (int)v8 >= (int)supportedMD5RDepthVertexFormats )
            goto LABEL_18;
        }
        surfa = v7 + 21;
LABEL_18:
        qglBindProgramARB(0x8620u, surfa);
        v3 = defaultVertexProgram;
        v2 = primBatchMesh;
      }
      else
      {
        if ( v2 )
        {
          qglBindBufferARB(0x8892u, 0);
          qglBindBufferARB(0x8893u, 0);
          surfa = v3;
          qglBindProgramARB(0x8620u, v3);
          v2 = 0;
        }
        v9 = idVertexCache::Position(&vertexCache, v1->geo->ambientCache);
        qglColorPointer(4, 0x1401u, 64, v9 + 12);
        qglVertexAttribPointerARB(0xBu, 3, 0x1406u, 0, 64, v9 + 16);
        qglVertexAttribPointerARB(0xAu, 3, 0x1406u, 0, 64, v9 + 44);
        qglVertexAttribPointerARB(9u, 3, 0x1406u, 0, 64, v9 + 32);
        qglVertexAttribPointerARB(8u, 2, 0x1406u, 0, 64, v9 + 56);
        qglVertexPointer(3, 0x1406u, 64, v9);
        GL_VertexAttribState(0xF000005u);
      }
      RB_CreateSingleDrawInteractions(v1, RB_ARB2_DrawInteraction);
      v1 = v1->nextOnLight;
    }
    while ( v1 );
    GL_VertexAttribState(1u);
    if ( v2 )
    {
      qglBindBufferARB(0x8892u, 0);
      qglBindBufferARB(0x8893u, 0);
    }
    backEnd.glState.currenttmu = 5;
    qglActiveTextureARB(0x84C5u);
    globalImages->BindNull(globalImages);
    backEnd.glState.currenttmu = 4;
    qglActiveTextureARB(0x84C4u);
    globalImages->BindNull(globalImages);
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
    qglDisable(0x8620u);
    qglDisable(0x8804u);
  }
}

// FUNC: void __cdecl RB_ARB2_DrawInteractions(void)
void __cdecl RB_ARB2_DrawInteractions()
{
  viewLight_s *i; // esi
  const idMaterial *lightShader; // eax
  int v2; // ecx
  int v3; // edx

  GL_SelectTexture(0);
  GL_DisableVertexAttribState(8u);
  for ( i = backEnd.viewDef->viewLights; i; i = i->next )
  {
    backEnd.vLight = i;
    lightShader = i->lightShader;
    if ( !lightShader->fogLight
      && !lightShader->blendLight
      && (i->localInteractions || i->globalInteractions || i->translucentInteractions) )
    {
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
      if ( r_useShadowVertexProgram.internalVar->integerValue )
      {
        qglEnable(0x8620u);
        qglBindProgramARB(0x8620u, 5u);
        RB_StencilShadowPass(i->globalShadows);
        RB_ARB2_CreateDrawInteractions(i->localInteractions);
        qglEnable(0x8620u);
        qglBindProgramARB(0x8620u, 5u);
        RB_StencilShadowPass(i->localShadows);
        RB_ARB2_CreateDrawInteractions(i->globalInteractions);
        qglDisable(0x8620u);
      }
      else
      {
        RB_StencilShadowPass(i->globalShadows);
        RB_ARB2_CreateDrawInteractions(i->localInteractions);
        RB_StencilShadowPass(i->localShadows);
        RB_ARB2_CreateDrawInteractions(i->globalInteractions);
      }
      if ( !r_skipTranslucent.internalVar->integerValue )
      {
        qglStencilFunc(0x207u, 128, 0xFFu);
        backEnd.depthFunc = 0;
        RB_ARB2_CreateDrawInteractions(i->translucentInteractions);
        backEnd.depthFunc = 0x20000;
      }
    }
  }
  qglStencilFunc(0x207u, 128, 0xFFu);
  GL_SelectTexture(0);
  GL_VertexAttribState(9u);
}

// FUNC: void __cdecl R_ARB2_Init(void)
void __cdecl R_ARB2_Init()
{
  int v0; // ecx
  const char *renderer_string; // edi
  const char *v2; // eax
  int v3; // esi
  int v4; // eax
  char *data; // edx
  const char *v6; // ecx
  char v7; // al
  idStr renderer; // [esp+4h] [ebp-2Ch] BYREF
  int v9; // [esp+2Ch] [ebp-4h]

  glConfig.allowARB2Path = 0;
  glConfig.preferNV20Path = 0;
  (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
    common.type,
    "---------------- R_ARB2_Init ----------------\n");
  if ( glConfig.ARBVertexProgramAvailable && glConfig.ARBFragmentProgramAvailable )
  {
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, "Available.\n");
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
      common.type,
      "---------------------------------------------\n");
    v0 = 0;
    glConfig.allowARB2Path = 1;
    renderer_string = glConfig.renderer_string;
    renderer.len = 0;
    renderer.alloced = 20;
    renderer.data = renderer.baseBuffer;
    renderer.baseBuffer[0] = 0;
    if ( glConfig.renderer_string )
    {
      v2 = &glConfig.renderer_string[strlen(glConfig.renderer_string) + 1];
      v3 = v2 - (glConfig.renderer_string + 1);
      v4 = v2 - glConfig.renderer_string;
      if ( v4 > 20 )
        idStr::ReAllocate(&renderer, v4, 1);
      data = renderer.data;
      v6 = renderer_string;
      do
      {
        v7 = *v6;
        *data++ = *v6++;
      }
      while ( v7 );
      v0 = v3;
      renderer.len = v3;
    }
    v9 = 0;
    glConfig.preferSimpleLighting = 0;
    if ( idStr::FindText(renderer.data, "GeForce", 0, 0, v0) <= -1
      || idStr::FindText(renderer.data, "5200", 0, 0, renderer.len) <= -1
      && idStr::FindText(renderer.data, "5600", 0, 0, renderer.len) <= -1 )
    {
      if ( idStr::FindText(renderer.data, "RADEON", 0, 0, renderer.len) > -1
        && (idStr::FindText(renderer.data, "9700", 0, 0, renderer.len) > -1
         || idStr::FindText(renderer.data, "9600", 0, 0, renderer.len) > -1) )
      {
        (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
          common.type,
          "%s: prefers simple lighting\n",
          renderer.data);
        glConfig.preferSimpleLighting = 1;
      }
    }
    else if ( glConfig.allowNV20Path )
    {
      glConfig.preferNV20Path = 1;
    }
    v9 = -1;
    idStr::FreeData(&renderer);
  }
  else
  {
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, "Not available.\n");
  }
}

// FUNC: void __cdecl RB_ARB2_LoadLocalViewOriginIntoVPParams(struct drawSurf_s const *)
void __cdecl RB_ARB2_LoadLocalViewOriginIntoVPParams(const drawSurf_s *surf)
{
  idVec3 localPos; // [esp+0h] [ebp-1Ch] BYREF
  float parm[4]; // [esp+Ch] [ebp-10h] BYREF

  R_GlobalPointToLocal(surf->space->modelMatrix, &backEnd.viewDef->renderView.vieworg, &localPos);
  *(idVec3 *)parm = localPos;
  parm[3] = 1.0;
  qglProgramEnvParameter4fvARB(0x8620u, 0x50u, parm);
}

// FUNC: void __cdecl RB_ARB2_MD5R_DrawShadowElements(struct drawSurf_s const *,int)
void __cdecl RB_ARB2_MD5R_DrawShadowElements(const drawSurf_s *surf, int numIndices)
{
  const srfTriangles_s *geo; // edi
  rvMesh *primBatchMesh; // ebp
  int v4; // esi
  const rvVertexFormat *v5; // ebx
  const rvVertexFormat *v6; // esi
  rvVertexFormat *drawVertexFormat; // [esp+10h] [ebp-14h]
  idVec4 localLight; // [esp+14h] [ebp-10h] BYREF

  geo = surf->geo;
  primBatchMesh = surf->geo->primBatchMesh;
  drawVertexFormat = (rvVertexFormat *)rvMesh::GetShadowVolVertexBufferFormat(primBatchMesh);
  v4 = 0;
  v5 = supportedMD5RShadowVolVertexFormats;
  while ( !rvVertexFormat::HasSameComponents(drawVertexFormat, v5) )
  {
    ++v5;
    ++v4;
    if ( (int)v5 >= (int)&colorMatrix )
      return;
  }
  if ( v4 < 3 )
  {
    qglBindProgramARB(0x8620u, v4 + 42);
    R_GlobalPointToLocal(surf->space->modelMatrix, &backEnd.vLight->globalLightOrigin, (idVec3 *)&localLight);
    localLight.w = 0.0;
    qglProgramEnvParameter4fvARB(0x8620u, 0x4Fu, &localLight.x);
    RB_ARB2_LoadMVPMatrixIntoVPParams(surf);
    v6 = &supportedMD5RShadowVolVertexFormats[v4];
    rvMesh::SetupForShadowVolRender(primBatchMesh, v6);
    rvMesh::DrawShadowVolume(primBatchMesh, geo->skinToModelTransforms, geo->indexes, numIndices == geo->numIndexes, v6);
    qglBindProgramARB(0x8620u, 5u);
    qglBindBufferARB(0x8892u, 0);
    qglBindBufferARB(0x8893u, 0);
  }
}

// FUNC: void __cdecl RB_ARB2_PrepareStageTexturing(struct shaderStage_t const *,struct drawSurf_s const *,bool)
void __cdecl RB_ARB2_PrepareStageTexturing(const shaderStage_t *pStage, const drawSurf_s *surf, bool fillingDepth)
{
  rvMesh *primBatchMesh; // ebx
  const rvVertexFormat *DrawVertexBufferFormat; // eax
  int v5; // edi
  const rvVertexFormat *v6; // ecx
  unsigned int v7; // edi
  const shaderStage_t *BumpStage; // ebx
  const float *shaderRegisters; // eax
  int v10; // ecx
  int v11; // edx
  double v12; // st7
  int v13; // ecx
  float units; // [esp+0h] [ebp-24h]
  float color[4]; // [esp+14h] [ebp-10h] BYREF

  primBatchMesh = surf->geo->primBatchMesh;
  DrawVertexBufferFormat = rvMesh::GetDrawVertexBufferFormat(primBatchMesh);
  v5 = 0;
  v6 = supportedMD5RVertexFormats;
  while ( DrawVertexBufferFormat->m_flags != v6->m_flags || DrawVertexBufferFormat->m_dimensions != v6->m_dimensions )
  {
    ++v6;
    ++v5;
    if ( (int)v6 >= (int)supportedMD5RDepthVertexFormats )
      return;
  }
  if ( v5 < 3 )
  {
    if ( pStage->privatePolygonOffset != 0.0 )
    {
      units = r_offsetUnits.internalVar->floatValue * pStage->privatePolygonOffset;
      GL_PolygonOffsetState(1, r_offsetFactor.internalVar->floatValue, units);
    }
    if ( !primBatchMesh->m_drawSetUp )
      rvMesh::SetupForDrawRender(primBatchMesh, 0);
    switch ( pStage->texture.texgen )
    {
      case TG_DIFFUSE_CUBE:
        v7 = v5 + 33;
        goto LABEL_19;
      case TG_REFLECT_CUBE:
        BumpStage = idMaterial::GetBumpStage((idMaterial *)surf->material);
        if ( BumpStage )
        {
          GL_SelectTexture(1);
          BumpStage->texture.image->Bind(BumpStage->texture.image);
          GL_SelectTexture(0);
          qglBindProgramARB(0x8804u, 0xDu);
          qglEnable(0x8804u);
          qglBindProgramARB(0x8620u, v5 + 39);
          qglEnable(0x8620u);
          RB_ARB2_LoadModelMatrixIntoVPParams(surf);
        }
        else
        {
          qglBindProgramARB(0x8620u, v5 + 36);
          qglEnable(0x8620u);
        }
        RB_ARB2_LoadLocalViewOriginIntoVPParams(surf);
        goto LABEL_20;
      case TG_SKYBOX_CUBE:
      case TG_WOBBLESKY_CUBE:
        qglBindProgramARB(0x8620u, v5 + 30);
        qglEnable(0x8620u);
        qglProgramEnvParameter4fvARB(0x8620u, 0x50u, (const float *)surf->texGenTransformAndViewOrg + 12);
        qglProgramEnvParameter4fvARB(0x8620u, 0x51u, surf->texGenTransformAndViewOrg);
        qglProgramEnvParameter4fvARB(0x8620u, 0x52u, (const float *)surf->texGenTransformAndViewOrg + 4);
        qglProgramEnvParameter4fvARB(0x8620u, 0x53u, (const float *)surf->texGenTransformAndViewOrg + 8);
        goto LABEL_20;
      case TG_SCREEN:
        return;
      default:
        v7 = v5 + 27;
LABEL_19:
        qglBindProgramARB(0x8620u, v7);
        qglEnable(0x8620u);
LABEL_20:
        if ( fillingDepth )
        {
          memset(color, 0, sizeof(color));
          qglProgramEnvParameter4fvARB(0x8620u, 0x5Bu, color);
          color[3] = surf->shaderRegisters[pStage->color.registers[3]];
          qglProgramEnvParameter4fvARB(0x8620u, 0x5Cu, color);
        }
        else if ( pStage->vertexColor )
        {
          color[3] = 1.0;
          color[2] = 1.0;
          color[1] = 1.0;
          color[0] = 1.0;
          qglProgramEnvParameter4fvARB(0x8620u, 0x5Bu, color);
          memset(color, 0, sizeof(color));
          qglProgramEnvParameter4fvARB(0x8620u, 0x5Cu, color);
        }
        else
        {
          memset(color, 0, sizeof(color));
          qglProgramEnvParameter4fvARB(0x8620u, 0x5Bu, color);
          if ( (surf->mFlags & 1) != 0 )
          {
            color[3] = 1.0;
            color[2] = 1.0;
            color[1] = 1.0;
            color[0] = 1.0;
          }
          else
          {
            shaderRegisters = surf->shaderRegisters;
            v10 = pStage->color.registers[1];
            v11 = pStage->color.registers[2];
            color[0] = shaderRegisters[pStage->color.registers[0]];
            v12 = shaderRegisters[v10];
            v13 = pStage->color.registers[3];
            color[1] = v12;
            color[2] = shaderRegisters[v11];
            color[3] = shaderRegisters[v13];
          }
          qglProgramEnvParameter4fvARB(0x8620u, 0x5Cu, color);
        }
        RB_ARB2_LoadMVPMatrixIntoVPParams(surf);
        if ( pStage->texture.hasMatrix )
        {
          RB_ARB2_LoadShaderTextureMatrixIntoVPParams(surf->shaderRegisters, &pStage->texture);
        }
        else
        {
          qglProgramEnvParameter4fvARB(0x8620u, 0x55u, envParam[0]);
          qglProgramEnvParameter4fvARB(0x8620u, 0x56u, envParam[1]);
        }
        break;
    }
  }
}

// FUNC: void __cdecl RB_ARB2_DisableStageTexturing(struct shaderStage_t const *,struct drawSurf_s const *)
void __cdecl RB_ARB2_DisableStageTexturing(const shaderStage_t *pStage, const drawSurf_s *surf)
{
  if ( pStage->privatePolygonOffset != 0.0 && (surf->material->materialFlags & 2) == 0 )
    GL_PolygonOffsetState(0, 0.0, 0.0);
  qglDisable(0x8804u);
  qglDisable(0x8620u);
  qglBindProgramARB(0x8620u, 0);
  qglBindBufferARB(0x8892u, 0);
  qglBindBufferARB(0x8893u, 0);
  if ( pStage->texture.texgen == TG_REFLECT_CUBE )
  {
    if ( idMaterial::GetBumpStage((idMaterial *)surf->material) )
    {
      GL_SelectTexture(1);
      globalImages->BindNull(globalImages);
      GL_SelectTexture(0);
    }
  }
}

// FUNC: void __cdecl R_LoadARBProgram(int)
void __cdecl R_LoadARBProgram(int progIndex)
{
  int v1; // eax
  char v2; // cl
  int v3; // ebx
  char *name; // eax
  int v5; // edi
  char *v6; // ecx
  char *data; // edx
  char v8; // al
  unsigned int target; // esi
  int v10; // esi
  int v11; // eax
  char v12; // cl
  char *v13; // edx
  int v14; // eax
  char v15; // cl
  char *v16; // edx
  char *v17; // ebx
  char *v18; // esi
  char *v19; // eax
  int v20; // edi
  char v21; // cl
  int v22; // eax
  char *v23; // edx
  int v24; // eax
  void *v25; // esp
  char *v26; // ecx
  char *v27; // edx
  char v28; // al
  int v29; // eax
  char *v30; // eax
  const char *v31; // esi
  char *v32; // eax
  unsigned int v33; // eax
  unsigned int Error; // edi
  const unsigned __int8 *String; // eax
  char v36[12]; // [esp+0h] [ebp-68h] BYREF
  idStr newName; // [esp+Ch] [ebp-5Ch] BYREF
  idStr fullPath; // [esp+2Ch] [ebp-3Ch] BYREF
  int ofs; // [esp+4Ch] [ebp-1Ch] BYREF
  char *fileBuffer; // [esp+50h] [ebp-18h] BYREF
  char *Destination; // [esp+54h] [ebp-14h]
  const char *expecting; // [esp+58h] [ebp-10h]
  int v43; // [esp+64h] [ebp-4h]

  fullPath.data = fullPath.baseBuffer;
  fullPath.alloced = 20;
  fullPath.baseBuffer[0] = 0;
  v1 = 0;
  do
  {
    v2 = aGlprogs[v1];
    fullPath.baseBuffer[v1++] = v2;
  }
  while ( v2 );
  fullPath.len = 8;
  v43 = 0;
  expecting = 0;
  v3 = progIndex;
  if ( strstr(fullPath.data, ".cg") )
  {
    name = progs[progIndex].name;
    Destination = name;
    newName.len = 0;
    newName.alloced = 20;
    newName.data = newName.baseBuffer;
    newName.baseBuffer[0] = 0;
    if ( name )
    {
      v5 = strlen(name);
      if ( v5 + 1 > 20 )
        idStr::ReAllocate(&newName, v5 + 1, 1);
      v6 = Destination;
      data = newName.data;
      do
      {
        v8 = *v6;
        *data++ = *v6++;
      }
      while ( v8 );
      newName.len = v5;
    }
    LOBYTE(v43) = 1;
    idStr::StripFileExtension(&newName);
    target = progs[progIndex].target;
    if ( target == 34336 )
    {
      v10 = newName.len + 3;
      if ( newName.len + 4 > newName.alloced )
        idStr::ReAllocate(&newName, newName.len + 4, 1);
      v11 = 0;
      v12 = 46;
      do
      {
        v13 = &newName.data[v11++];
        v13[newName.len] = v12;
        v12 = aVp[v11];
      }
      while ( v12 );
    }
    else
    {
      if ( target != 34820 )
      {
LABEL_22:
        strncpy(Destination, newName.data, 0x3Fu);
        LOBYTE(v43) = 0;
        idStr::FreeData(&newName);
        goto LABEL_23;
      }
      v10 = newName.len + 3;
      if ( newName.len + 4 > newName.alloced )
        idStr::ReAllocate(&newName, newName.len + 4, 1);
      v14 = 0;
      v15 = 46;
      do
      {
        v16 = &newName.data[v14++];
        v16[newName.len] = v15;
        v15 = aFp_1[v14];
      }
      while ( v15 );
    }
    v3 = progIndex;
    newName.len = v10;
    newName.data[v10] = 0;
    goto LABEL_22;
  }
LABEL_23:
  v17 = (char *)(72 * v3);
  v18 = &progs[0].name[(_DWORD)v17];
  Destination = v17;
  if ( &progs[0].name[(_DWORD)v17] )
  {
    v19 = &progs[0].name[(_DWORD)v17 + 1 + strlen(&progs[0].name[(_DWORD)v17])];
    v20 = v19 - (v18 + 1) + fullPath.len;
    if ( v19 - v18 + fullPath.len > fullPath.alloced )
      idStr::ReAllocate(&fullPath, v19 - v18 + fullPath.len, 1);
    v21 = *v18;
    v22 = 0;
    if ( *v18 )
    {
      do
      {
        v23 = &fullPath.data[v22++];
        v23[fullPath.len] = v21;
        v21 = v18[v22];
      }
      while ( v21 );
      v17 = Destination;
    }
    fullPath.len = v20;
    fullPath.data[v20] = 0;
  }
  (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(common.type, "%s", fullPath.data);
  fileSystem->ReadFile(fileSystem, fullPath.data, (void **)&fileBuffer, 0);
  if ( !fileBuffer )
  {
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, ": File not found\n");
    *(unsigned int *)((char *)&progs[0].ident + (_DWORD)v17) = 0;
    v43 = -1;
    goto LABEL_67;
  }
  v24 = strlen(fileBuffer) + 4;
  LOBYTE(v24) = v24 & 0xFC;
  v25 = alloca(v24);
  v26 = fileBuffer;
  v27 = v36;
  do
  {
    v28 = *v26;
    *v27++ = *v26++;
  }
  while ( v28 );
  fileSystem->FreeFile(fileSystem, fileBuffer);
  if ( !glConfig.isInitialized )
  {
LABEL_62:
    v43 = -1;
    goto LABEL_67;
  }
  if ( !*(unsigned int *)((char *)&progs[0].ident + (_DWORD)v17) )
    *(unsigned int *)((char *)&progs[0].ident + (_DWORD)v17) = progIndex + 51;
  v29 = *(unsigned int *)((char *)&progs[0].target + (_DWORD)v17);
  if ( v29 == 34336 )
  {
    if ( !glConfig.ARBVertexProgramAvailable )
    {
      (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
        common.type,
        ": GL_VERTEX_PROGRAM_ARB not available\n");
      v43 = -1;
      goto LABEL_67;
    }
    expecting = "!!ARBvp";
  }
  if ( v29 == 34820 )
  {
    if ( !glConfig.ARBFragmentProgramAvailable )
    {
      (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
        common.type,
        ": GL_FRAGMENT_PROGRAM_ARB not available\n");
      v43 = -1;
      goto LABEL_67;
    }
    expecting = "!!ARBfp";
  }
  if ( v29 == 34928 )
  {
    if ( !glConfig.nvProgramsAvailable )
    {
      (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
        common.type,
        ": GL_FRAGMENT_PROGRAM_NV not available\n");
      v43 = -1;
      goto LABEL_67;
    }
    expecting = "!!FP";
  }
  v30 = strstr(v36, expecting);
  v31 = v30;
  if ( !v30 )
  {
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      ": %s not found\n",
      expecting);
    v43 = -1;
    goto LABEL_67;
  }
  v32 = strstr(v30, "END");
  if ( !v32 )
  {
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, ": END not found\n");
    v43 = -1;
    goto LABEL_67;
  }
  v32[3] = 0;
  v33 = *(unsigned int *)((char *)&progs[0].target + (_DWORD)v17);
  if ( v33 == 34928 )
  {
    qglLoadProgramNV(
      0x8870u,
      *(unsigned int *)((char *)&progs[0].ident + (_DWORD)v17),
      strlen(v31),
      (const unsigned __int8 *)v31);
  }
  else
  {
    qglBindProgramARB(v33, *(unsigned int *)((char *)&progs[0].ident + (_DWORD)v17));
    qglGetError();
    qglProgramStringARB(*(unsigned int *)((char *)&progs[0].target + (_DWORD)v17), 0x8875u, strlen(v31), v31);
  }
  Error = qglGetError();
  qglGetIntegerv(0x864Bu, &ofs);
  if ( Error == 1282 )
  {
    String = qglGetString(0x8874u);
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "\nGL_PROGRAM_ERROR_STRING_ARB: %s\n",
      String);
    if ( ofs < 0 )
    {
      (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
        common.type,
        "GL_PROGRAM_ERROR_POSITION_ARB < 0 with error\n");
      v43 = -1;
      goto LABEL_67;
    }
    if ( ofs >= (int)strlen(v31) )
    {
      (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
        common.type,
        "error at end of program\n");
      v43 = -1;
      goto LABEL_67;
    }
    (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
      common.type,
      "error at %i:\n%s",
      ofs,
      &v31[ofs]);
    goto LABEL_62;
  }
  if ( ofs == -1 )
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, "\n");
  else
    (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
      common.type,
      "\nGL_PROGRAM_ERROR_POSITION_ARB != -1 without error\n");
  v43 = -1;
LABEL_67:
  idStr::FreeData(&fullPath);
}

// FUNC: int __cdecl R_FindARBProgram(unsigned int,char const *)
unsigned int __cdecl R_FindARBProgram(unsigned int target, const char *program)
{
  const char *v2; // edi
  const char *v3; // eax
  int v4; // esi
  int v5; // eax
  char *data; // edx
  const char *v7; // ecx
  char v8; // al
  int v9; // ebp
  char *name; // esi
  unsigned int v11; // eax
  int v12; // edi
  int v13; // eax
  char *v14; // edx
  char *v15; // ecx
  char v16; // al
  unsigned int v17; // eax
  int v18; // esi
  unsigned int ident; // esi
  idStr compare; // [esp+10h] [ebp-4Ch] BYREF
  idStr stripped; // [esp+30h] [ebp-2Ch] BYREF
  int v23; // [esp+58h] [ebp-4h]

  v2 = program;
  stripped.len = 0;
  stripped.alloced = 20;
  stripped.data = stripped.baseBuffer;
  stripped.baseBuffer[0] = 0;
  if ( program )
  {
    v3 = &program[strlen(program) + 1];
    v4 = v3 - (program + 1);
    v5 = v3 - program;
    if ( v5 > 20 )
      idStr::ReAllocate(&stripped, v5, 1);
    data = stripped.data;
    v7 = program;
    do
    {
      v8 = *v7;
      *data++ = *v7++;
    }
    while ( v8 );
    stripped.len = v4;
  }
  v23 = 0;
  idStr::StripFileExtension(&stripped);
  v9 = 0;
  if ( progs[0].name[0] )
  {
    name = progs[0].name;
    do
    {
      if ( *((_DWORD *)name - 2) == target )
      {
        compare.len = 0;
        compare.alloced = 20;
        compare.data = compare.baseBuffer;
        compare.baseBuffer[0] = 0;
        if ( name )
        {
          v11 = (unsigned int)&name[strlen(name) + 1];
          v12 = v11 - (_DWORD)(name + 1);
          v13 = v11 - (_DWORD)name;
          if ( v13 > 20 )
            idStr::ReAllocate(&compare, v13, 1);
          v14 = compare.data;
          v15 = name;
          do
          {
            v16 = *v15;
            *v14++ = *v15++;
          }
          while ( v16 );
          compare.len = v12;
          v2 = program;
        }
        LOBYTE(v23) = 1;
        idStr::StripFileExtension(&compare);
        v17 = idStr::Icmp(stripped.data, compare.data);
        LOBYTE(v23) = 0;
        if ( !v17 )
        {
          ident = progs[v9].ident;
          idStr::FreeData(&compare);
          goto LABEL_22;
        }
        idStr::FreeData(&compare);
      }
      name += 72;
      ++v9;
    }
    while ( *name );
    if ( v9 == 200 )
      (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 152))(
        common.type,
        "R_FindARBProgram: MAX_GLPROGS");
  }
  v18 = v9;
  progs[v18].ident = 0;
  progs[v18].target = target;
  strncpy(progs[v9].name, v2, 0x3Fu);
  R_LoadARBProgram(v9);
  ident = progs[v9].ident;
LABEL_22:
  v23 = -1;
  idStr::FreeData(&stripped);
  return ident;
}

// FUNC: void __cdecl R_ReloadARBPrograms_f(class idCmdArgs const &)
void __cdecl R_ReloadARBPrograms_f()
{
  int v0; // esi
  char *name; // edi

  fileSystem->SetIsFileLoadingAllowed(fileSystem, 1);
  (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
    common.type,
    "------------ R_ReloadARBPrograms ------------\n");
  v0 = 0;
  if ( progs[0].name[0] )
  {
    name = progs[0].name;
    do
    {
      R_LoadARBProgram(v0);
      name += 72;
      ++v0;
    }
    while ( *name );
  }
  (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
    common.type,
    "---------------------------------------------\n");
  fileSystem->SetIsFileLoadingAllowed(fileSystem, 0);
}
