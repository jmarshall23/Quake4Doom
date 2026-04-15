
// FUNC: RB_ARB_DrawInteraction
void __cdecl RB_ARB_DrawInteraction(const drawInteraction_t *din)
{
  const srfTriangles_s *geo; // edi
  char *v2; // ebx
  int depthFunc; // eax
  idImageManager_vtbl *v4; // eax
  int v5; // eax
  const void *v6; // eax
  idVec4 plane; // [esp+Ch] [ebp-10h] BYREF

  geo = din->surf->geo;
  v2 = (char *)idVertexCache::Position(&vertexCache, geo->ambientCache);
  qglVertexPointer(3, 0x1406u, 64, v2);
  GL_SelectTexture(0);
  qglTexCoordPointer(2, 0x1406u, 64, v2 + 56);
  depthFunc = backEnd.depthFunc;
  BYTE1(depthFunc) = BYTE1(backEnd.depthFunc) | 0xF;
  GL_State(depthFunc);
  GL_PolygonOffset(din->surf->material, 1);
  qglColor3f(1.0, 1.0, 1.0);
  GL_DisableVertexAttribState(8u);
  qglEnable(0xC60u);
  qglTexGenfv(0x2000u, 0x2501u, &din->lightProjection[3].x);
  qglTexCoord2f(0.0, 0.5);
  memset(&plane, 0, 12);
  plane.w = 0.5;
  qglEnable(0xC61u);
  qglTexGenfv(0x2001u, 0x2501u, &plane.x);
  memset(&plane, 0, 12);
  plane.w = 1.0;
  qglEnable(0xC63u);
  qglTexGenfv(0x2003u, 0x2501u, &plane.x);
  din->lightFalloffImage->Bind(din->lightFalloffImage);
  RB_DrawElementsWithCounters(geo);
  qglDisable(0xC60u);
  qglDisable(0xC61u);
  qglDisable(0xC63u);
  if ( glConfig.envDot3Available && glConfig.cubeMapAvailable )
  {
    GL_State(backEnd.depthFunc | 0xF07);
    GL_SelectTexture(0);
    GL_EnableVertexAttribState(8u);
    din->bumpImage->Bind(din->bumpImage);
    GL_SelectTexture(1);
    v4 = globalImages->__vftable;
    if ( din->ambientLight )
      v5 = ((int (*)(void))v4->GetAmbientNormalMap)();
    else
      v5 = ((int (*)(void))v4->GetNormalCubeMapImage)();
    (*(void (__thiscall **)(int))(*(_DWORD *)v5 + 4))(v5);
    GL_EnableVertexAttribState(0x10u);
    v6 = idVertexCache::Position(&vertexCache, geo->lightingCache);
    qglTexCoordPointer(3, 0x1406u, 12, v6);
    GL_TexEnv(34160);
    qglTexEnvi(0x2300u, 0x8571u, 34479);
    qglTexEnvi(0x2300u, 0x8580u, 5890);
    qglTexEnvi(0x2300u, 0x8581u, 34168);
    qglTexEnvi(0x2300u, 0x8590u, 768);
    qglTexEnvi(0x2300u, 0x8591u, 768);
    qglTexEnvi(0x2300u, 0x8573u, 1);
    qglTexEnvi(0x2300u, 0xD1Cu, 1);
    RB_DrawElementsWithCounters(geo);
    GL_TexEnv(8448);
    globalImages->BindNull(globalImages);
    GL_DisableVertexAttribState(0x10u);
    GL_SelectTexture(0);
  }
  GL_State(backEnd.depthFunc | 0x1127);
  GL_SelectTexture(0);
  idVertexCache::Position(&vertexCache, geo->ambientCache);
  if ( din->vertexColor )
  {
    qglColorPointer(4, 0x1401u, 64, v2 + 12);
    GL_EnableVertexAttribState(4u);
    if ( din->vertexColor == SVC_INVERSE_MODULATE )
    {
      GL_TexEnv(34160);
      qglTexEnvi(0x2300u, 0x8571u, 8448);
      qglTexEnvi(0x2300u, 0x8580u, 5890);
      qglTexEnvi(0x2300u, 0x8581u, 34167);
      qglTexEnvi(0x2300u, 0x8590u, 768);
      qglTexEnvi(0x2300u, 0x8591u, 769);
      qglTexEnvi(0x2300u, 0x8573u, 1);
    }
  }
  else
  {
    qglColor4fv(&din->diffuseColor.x);
  }
  GL_EnableVertexAttribState(8u);
  din->diffuseImage->Bind(din->diffuseImage);
  GL_SelectTexture(1);
  GL_DisableVertexAttribState(0x10u);
  qglEnable(0xC60u);
  qglEnable(0xC61u);
  qglEnable(0xC63u);
  qglTexGenfv(0x2000u, 0x2501u, &din->lightProjection[0].x);
  qglTexGenfv(0x2001u, 0x2501u, &din->lightProjection[1].x);
  qglTexGenfv(0x2003u, 0x2501u, &din->lightProjection[2].x);
  din->lightImage->Bind(din->lightImage);
  RB_DrawElementsWithCounters(geo);
  qglDisable(0xC60u);
  qglDisable(0xC61u);
  qglDisable(0xC63u);
  globalImages->BindNull(globalImages);
  GL_SelectTexture(0);
  if ( din->vertexColor )
  {
    GL_DisableVertexAttribState(4u);
    GL_TexEnv(8448);
  }
  GL_PolygonOffset(din->surf->material, 0);
}

// FUNC: RB_ARB_DrawThreeTextureInteraction
void __cdecl RB_ARB_DrawThreeTextureInteraction(const drawInteraction_t *din)
{
  const srfTriangles_s *geo; // edi
  char *v2; // ebx
  int depthFunc; // eax
  idImage *v4; // eax
  const void *v5; // eax
  idVec4 plane; // [esp+Ch] [ebp-10h] BYREF

  geo = din->surf->geo;
  v2 = (char *)idVertexCache::Position(&vertexCache, geo->ambientCache);
  qglVertexPointer(3, 0x1406u, 64, v2);
  GL_SelectTexture(0);
  qglTexCoordPointer(2, 0x1406u, 64, v2 + 56);
  qglColor3f(1.0, 1.0, 1.0);
  depthFunc = backEnd.depthFunc;
  BYTE1(depthFunc) = BYTE1(backEnd.depthFunc) | 0xF;
  GL_State(depthFunc);
  GL_PolygonOffset(din->surf->material, 1);
  GL_SelectTexture(0);
  GL_EnableVertexAttribState(8u);
  din->bumpImage->Bind(din->bumpImage);
  GL_SelectTexture(1);
  if ( din->ambientLight )
    v4 = globalImages->GetAmbientNormalMap(globalImages);
  else
    v4 = globalImages->GetNormalCubeMapImage(globalImages);
  v4->Bind(v4);
  GL_EnableVertexAttribState(0x10u);
  v5 = idVertexCache::Position(&vertexCache, geo->lightingCache);
  qglTexCoordPointer(3, 0x1406u, 12, v5);
  GL_TexEnv(34160);
  qglTexEnvi(0x2300u, 0x8571u, 34479);
  qglTexEnvi(0x2300u, 0x8580u, 5890);
  qglTexEnvi(0x2300u, 0x8581u, 34168);
  qglTexEnvi(0x2300u, 0x8590u, 768);
  qglTexEnvi(0x2300u, 0x8591u, 768);
  qglTexEnvi(0x2300u, 0x8573u, 1);
  qglTexEnvi(0x2300u, 0xD1Cu, 1);
  RB_DrawElementsWithCounters(geo);
  GL_TexEnv(8448);
  globalImages->BindNull(globalImages);
  GL_DisableVertexAttribState(0x10u);
  GL_SelectTexture(0);
  GL_State(backEnd.depthFunc | 0x1127);
  GL_SelectTexture(0);
  idVertexCache::Position(&vertexCache, geo->ambientCache);
  if ( din->vertexColor )
  {
    qglColorPointer(4, 0x1401u, 64, v2 + 12);
    GL_EnableVertexAttribState(4u);
    if ( din->vertexColor == SVC_INVERSE_MODULATE )
    {
      GL_TexEnv(34160);
      qglTexEnvi(0x2300u, 0x8571u, 8448);
      qglTexEnvi(0x2300u, 0x8580u, 5890);
      qglTexEnvi(0x2300u, 0x8581u, 34167);
      qglTexEnvi(0x2300u, 0x8590u, 768);
      qglTexEnvi(0x2300u, 0x8591u, 769);
      qglTexEnvi(0x2300u, 0x8573u, 1);
    }
  }
  else
  {
    qglColor4fv(&din->diffuseColor.x);
  }
  GL_EnableVertexAttribState(8u);
  din->diffuseImage->Bind(din->diffuseImage);
  GL_SelectTexture(1);
  GL_DisableVertexAttribState(0x10u);
  qglEnable(0xC60u);
  qglEnable(0xC61u);
  qglEnable(0xC63u);
  qglTexGenfv(0x2000u, 0x2501u, &din->lightProjection[0].x);
  qglTexGenfv(0x2001u, 0x2501u, &din->lightProjection[1].x);
  qglTexGenfv(0x2003u, 0x2501u, &din->lightProjection[2].x);
  din->lightImage->Bind(din->lightImage);
  GL_SelectTexture(2);
  GL_DisableVertexAttribState(0x20u);
  qglEnable(0xC60u);
  qglEnable(0xC61u);
  qglEnable(0xC63u);
  qglTexGenfv(0x2000u, 0x2501u, &din->lightProjection[3].x);
  memset(&plane, 0, 12);
  plane.w = 0.5;
  qglTexGenfv(0x2001u, 0x2501u, &plane.x);
  memset(&plane, 0, 12);
  plane.w = 1.0;
  qglTexGenfv(0x2003u, 0x2501u, &plane.x);
  din->lightFalloffImage->Bind(din->lightFalloffImage);
  RB_DrawElementsWithCounters(geo);
  qglDisable(0xC60u);
  qglDisable(0xC61u);
  qglDisable(0xC63u);
  globalImages->BindNull(globalImages);
  GL_SelectTexture(1);
  qglDisable(0xC60u);
  qglDisable(0xC61u);
  qglDisable(0xC63u);
  globalImages->BindNull(globalImages);
  GL_SelectTexture(0);
  if ( din->vertexColor )
  {
    GL_DisableVertexAttribState(4u);
    GL_TexEnv(8448);
  }
  GL_PolygonOffset(din->surf->material, 0);
}

// FUNC: RB_CreateDrawInteractions
void __usercall RB_CreateDrawInteractions(const drawSurf_s *surf@<eax>)
{
  const drawSurf_s *v1; // esi

  v1 = surf;
  if ( surf )
  {
    backEnd.currentSpace = 0;
    if ( r_useTripleTextureARB.internalVar->integerValue && glConfig.maxTextureUnits >= 3 )
    {
      do
      {
        RB_CreateSingleDrawInteractions(v1, RB_ARB_DrawThreeTextureInteraction);
        v1 = v1->nextOnLight;
      }
      while ( v1 );
    }
    else
    {
      do
      {
        RB_CreateSingleDrawInteractions(v1, RB_ARB_DrawInteraction);
        v1 = v1->nextOnLight;
      }
      while ( v1 );
    }
  }
}

// FUNC: void __cdecl RB_ARB_DrawInteractions(void)
void __cdecl RB_ARB_DrawInteractions()
{
  viewLight_s *i; // esi

  qglEnable(0xB90u);
  for ( i = backEnd.viewDef->viewLights; i; i = i->next )
    RB_RenderViewLight(i);
}
