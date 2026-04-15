
// FUNC: void __cdecl RB_SetProgramEnvironmentSpace(void)
void __cdecl RB_SetProgramEnvironmentSpace()
{
  const viewEntity_s *currentSpace; // esi
  float *modelMatrix; // edi
  float parm[4]; // [esp+0h] [ebp-10h] BYREF

  if ( glConfig.ARBVertexProgramAvailable )
  {
    currentSpace = backEnd.currentSpace;
    modelMatrix = backEnd.currentSpace->modelMatrix;
    R_GlobalPointToLocal(backEnd.currentSpace->modelMatrix, &backEnd.viewDef->renderView.vieworg, (idVec3 *)parm);
    parm[3] = 1.0;
    qglProgramEnvParameter4fvARB(0x8620u, 5u, parm);
    parm[0] = *modelMatrix;
    parm[1] = currentSpace->modelMatrix[4];
    parm[2] = currentSpace->modelMatrix[8];
    parm[3] = currentSpace->modelMatrix[12];
    qglProgramEnvParameter4fvARB(0x8620u, 6u, parm);
    parm[0] = currentSpace->modelMatrix[1];
    parm[1] = currentSpace->modelMatrix[5];
    parm[2] = currentSpace->modelMatrix[9];
    parm[3] = currentSpace->modelMatrix[13];
    qglProgramEnvParameter4fvARB(0x8620u, 7u, parm);
    parm[0] = currentSpace->modelMatrix[2];
    parm[1] = currentSpace->modelMatrix[6];
    parm[2] = currentSpace->modelMatrix[10];
    parm[3] = currentSpace->modelMatrix[14];
    qglProgramEnvParameter4fvARB(0x8620u, 8u, parm);
  }
}

// FUNC: void __cdecl RB_BakeTextureMatrixIntoTexgen(class idPlane * const,float const * const)
void __cdecl RB_BakeTextureMatrixIntoTexgen(idPlane *lightProject)
{
  float b; // ecx
  float c; // edx
  float d; // eax
  float a; // ecx
  float v5; // eax
  float v6; // ecx
  float v7; // eax
  float v8; // edx
  float v9; // ecx
  float v10; // edx
  float v11; // edx
  float v12; // eax
  float v13; // ecx
  float v14; // edx
  float v15; // eax
  float v16; // ecx
  float v17; // edx
  float v18; // eax
  float genMatrix[16]; // [esp+4h] [ebp-80h] BYREF
  float final[16]; // [esp+44h] [ebp-40h] BYREF

  b = lightProject->b;
  c = lightProject->c;
  genMatrix[0] = lightProject->a;
  d = lightProject->d;
  genMatrix[4] = b;
  a = lightProject[1].a;
  genMatrix[12] = d;
  v5 = lightProject[1].c;
  genMatrix[1] = a;
  v6 = lightProject[1].d;
  genMatrix[9] = v5;
  v7 = lightProject[2].b;
  genMatrix[8] = c;
  v8 = lightProject[1].b;
  genMatrix[13] = v6;
  v9 = lightProject[2].c;
  genMatrix[7] = v7;
  genMatrix[5] = v8;
  v10 = lightProject[2].a;
  genMatrix[11] = v9;
  genMatrix[3] = v10;
  v11 = lightProject[2].d;
  genMatrix[2] = 0.0;
  genMatrix[6] = 0.0;
  genMatrix[10] = 0.0;
  genMatrix[14] = 0.0;
  genMatrix[15] = v11;
  myGlMultMatrix(genMatrix, backEnd.lightTextureMatrix, final);
  v12 = final[4];
  v13 = final[8];
  lightProject->a = final[0];
  v14 = final[12];
  lightProject->b = v12;
  v15 = final[1];
  lightProject->c = v13;
  v16 = final[5];
  lightProject->d = v14;
  v17 = final[9];
  lightProject[1].a = v15;
  v18 = final[13];
  lightProject[1].b = v16;
  lightProject[1].c = v17;
  lightProject[1].d = v18;
}

// FUNC: void __cdecl RB_PrepareStageTexturing(struct shaderStage_t const *,struct drawSurf_s const *,class idDrawVert *,bool)
void __cdecl RB_PrepareStageTexturing(
        const shaderStage_t *pStage,
        const drawSurf_s *surf,
        idDrawVert *ac,
        bool fillingDepth)
{
  texgen_t texgen; // eax
  char *v5; // eax
  idImage *v6; // eax
  idImage *v7; // eax
  const shaderStage_t *BumpStage; // esi
  float units; // [esp+0h] [ebp-60h]
  float plane[4]; // [esp+10h] [ebp-50h] BYREF
  float mat[16]; // [esp+20h] [ebp-40h] BYREF

  if ( surf->geo->primBatchMesh )
  {
    if ( tr.backEndRenderer == BE_ARB2 )
      RB_ARB2_PrepareStageTexturing(pStage, surf, fillingDepth);
  }
  else
  {
    if ( pStage->privatePolygonOffset != 0.0 )
    {
      units = r_offsetUnits.internalVar->floatValue * pStage->privatePolygonOffset;
      GL_PolygonOffsetState(1, r_offsetFactor.internalVar->floatValue, units);
    }
    if ( pStage->texture.hasMatrix )
      RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture);
    if ( pStage->texture.texgen == TG_DIFFUSE_CUBE )
      qglTexCoordPointer(3, 0x1406u, 64, &ac->normal);
    texgen = pStage->texture.texgen;
    if ( texgen == TG_SKYBOX_CUBE || texgen == TG_WOBBLESKY_CUBE )
    {
      v5 = idVertexCache::Position(&vertexCache, surf->dynamicTexCoords);
      qglTexCoordPointer(3, 0x1406u, 0, v5);
    }
    if ( pStage->texture.texgen == TG_SCREEN )
    {
      qglEnable(0xC60u);
      qglEnable(0xC61u);
      qglEnable(0xC63u);
      myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
      plane[1] = mat[4];
      plane[0] = mat[0];
      plane[2] = mat[8];
      plane[3] = mat[12];
      qglTexGenfv(0x2000u, 0x2501u, plane);
      plane[1] = mat[5];
      plane[0] = mat[1];
      plane[2] = mat[9];
      plane[3] = mat[13];
      qglTexGenfv(0x2001u, 0x2501u, plane);
      plane[1] = mat[7];
      plane[0] = mat[3];
      plane[2] = mat[11];
      plane[3] = mat[15];
      qglTexGenfv(0x2003u, 0x2501u, plane);
    }
    if ( pStage->texture.texgen == TG_SCREEN2 )
    {
      qglEnable(0xC60u);
      qglEnable(0xC61u);
      qglEnable(0xC63u);
      myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
      plane[1] = mat[4];
      plane[0] = mat[0];
      plane[2] = mat[8];
      plane[3] = mat[12];
      qglTexGenfv(0x2000u, 0x2501u, plane);
      plane[1] = mat[5];
      plane[0] = mat[1];
      plane[2] = mat[9];
      plane[3] = mat[13];
      qglTexGenfv(0x2001u, 0x2501u, plane);
      plane[1] = mat[7];
      plane[0] = mat[3];
      plane[2] = mat[11];
      plane[3] = mat[15];
      qglTexGenfv(0x2003u, 0x2501u, plane);
    }
    if ( pStage->texture.texgen == TG_GLASSWARP && tr.backEndRenderer == BE_ARB2 )
    {
      qglBindProgramARB(0x8804u, 0x12u);
      qglEnable(0x8804u);
      GL_SelectTexture(2);
      v6 = globalImages->GetScratchImage(globalImages);
      v6->Bind(v6);
      GL_SelectTexture(1);
      v7 = globalImages->GetScratchImage2(globalImages);
      v7->Bind(v7);
      qglEnable(0xC60u);
      qglEnable(0xC61u);
      qglEnable(0xC63u);
      myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
      plane[1] = mat[4];
      plane[0] = mat[0];
      plane[2] = mat[8];
      plane[3] = mat[12];
      qglTexGenfv(0x2000u, 0x2501u, plane);
      plane[1] = mat[5];
      plane[0] = mat[1];
      plane[2] = mat[9];
      plane[3] = mat[13];
      qglTexGenfv(0x2001u, 0x2501u, plane);
      plane[1] = mat[7];
      plane[0] = mat[3];
      plane[2] = mat[11];
      plane[3] = mat[15];
      qglTexGenfv(0x2003u, 0x2501u, plane);
      GL_SelectTexture(0);
    }
    if ( pStage->texture.texgen == TG_REFLECT_CUBE )
    {
      if ( tr.backEndRenderer == BE_ARB2 )
      {
        BumpStage = idMaterial::GetBumpStage((idMaterial *)surf->material);
        if ( BumpStage )
        {
          GL_SelectTexture(1);
          BumpStage->texture.image->Bind(BumpStage->texture.image);
          GL_SelectTexture(0);
          qglNormalPointer(0x1406u, 64, &ac->normal);
          qglVertexAttribPointerARB(0xAu, 3, 0x1406u, 0, 64, &ac->tangents[1]);
          qglVertexAttribPointerARB(9u, 3, 0x1406u, 0, 64, ac->tangents);
          GL_EnableVertexAttribState(0x6000002u);
          qglBindProgramARB(0x8804u, 0xDu);
          qglEnable(0x8804u);
          qglBindProgramARB(0x8620u, 3u);
        }
        else
        {
          qglNormalPointer(0x1406u, 64, &ac->normal);
          GL_EnableVertexAttribState(2u);
          qglBindProgramARB(0x8804u, 0xCu);
          qglEnable(0x8804u);
          qglBindProgramARB(0x8620u, 2u);
        }
        qglEnable(0x8620u);
      }
      else
      {
        qglEnable(0xC60u);
        qglEnable(0xC61u);
        qglEnable(0xC62u);
        qglTexGenf(0x2000u, 0x2500u, 34066.0);
        qglTexGenf(0x2001u, 0x2500u, 34066.0);
        qglTexGenf(0x2002u, 0x2500u, 34066.0);
        GL_EnableVertexAttribState(2u);
        qglNormalPointer(0x1406u, 64, &ac->normal);
        qglMatrixMode(0x1702u);
        R_TransposeGLMatrix(backEnd.viewDef->worldSpace.modelViewMatrix, mat);
        qglLoadMatrixf(mat);
        qglMatrixMode(0x1700u);
      }
    }
  }
}

// FUNC: void __cdecl RB_FinishStageTexturing(struct shaderStage_t const *,struct drawSurf_s const *,class idDrawVert *,bool)
void __cdecl RB_FinishStageTexturing(const shaderStage_t *pStage, const drawSurf_s *surf, idDrawVert *ac)
{
  texgen_t texgen; // eax

  if ( surf->geo->primBatchMesh )
  {
    RB_ARB2_DisableStageTexturing(pStage, surf);
  }
  else
  {
    if ( pStage->privatePolygonOffset != 0.0 && (surf->material->materialFlags & 2) == 0 )
      GL_PolygonOffsetState(0, 0.0, 0.0);
    texgen = pStage->texture.texgen;
    if ( texgen == TG_DIFFUSE_CUBE || texgen == TG_SKYBOX_CUBE || texgen == TG_WOBBLESKY_CUBE )
      qglTexCoordPointer(2, 0x1406u, 64, &ac->st);
    if ( pStage->texture.texgen == TG_SCREEN )
    {
      qglDisable(0xC60u);
      qglDisable(0xC61u);
      qglDisable(0xC63u);
    }
    if ( pStage->texture.texgen == TG_SCREEN2 )
    {
      qglDisable(0xC60u);
      qglDisable(0xC61u);
      qglDisable(0xC63u);
    }
    if ( pStage->texture.texgen == TG_GLASSWARP && tr.backEndRenderer == BE_ARB2 )
    {
      GL_SelectTexture(2);
      globalImages->BindNull(globalImages);
      GL_SelectTexture(1);
      if ( pStage->texture.hasMatrix )
        RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture);
      qglDisable(0xC60u);
      qglDisable(0xC61u);
      qglDisable(0xC63u);
      qglDisable(0x8804u);
      globalImages->BindNull(globalImages);
      GL_SelectTexture(0);
    }
    if ( pStage->texture.texgen == TG_REFLECT_CUBE )
    {
      if ( tr.backEndRenderer == BE_ARB2 )
      {
        if ( idMaterial::GetBumpStage((idMaterial *)surf->material) )
        {
          GL_SelectTexture(1);
          globalImages->BindNull(globalImages);
          GL_SelectTexture(0);
          GL_DisableVertexAttribState(0x6000000u);
        }
        GL_DisableVertexAttribState(2u);
        qglDisable(0x8804u);
        qglDisable(0x8620u);
        qglBindProgramARB(0x8620u, 0);
      }
      else
      {
        qglDisable(0xC60u);
        qglDisable(0xC61u);
        qglDisable(0xC62u);
        qglTexGenf(0x2000u, 0x2500u, 9217.0);
        qglTexGenf(0x2001u, 0x2500u, 9217.0);
        qglTexGenf(0x2002u, 0x2500u, 9217.0);
        GL_DisableVertexAttribState(2u);
        qglMatrixMode(0x1702u);
        qglLoadIdentity();
        qglMatrixMode(0x1700u);
      }
    }
    if ( pStage->texture.hasMatrix )
    {
      qglMatrixMode(0x1702u);
      qglLoadIdentity();
      qglMatrixMode(0x1700u);
    }
  }
}

// FUNC: void __cdecl RB_T_FillDepthBuffer(struct drawSurf_s const *)
void __cdecl RB_T_FillDepthBuffer(const drawSurf_s *surf)
{
  const srfTriangles_s *geo; // ebp
  const idMaterial *material; // edi
  int numStages; // esi
  const float *shaderRegisters; // ebx
  int v5; // ecx
  shaderStage_t *v6; // edx
  bool v7; // zf
  shaderStage_t *v8; // edx
  materialCoverage_t coverage; // eax
  char v10; // bl
  int v11; // ebp
  int v12; // edi
  shaderStage_t *stages; // esi
  dynamicidImage_t dynamic; // eax
  const shaderStage_t *v15; // esi
  double v16; // st7
  int alphaTestMode; // eax
  idImage *v18; // eax
  bool useAlphaToCoverage; // [esp+11h] [ebp-57h]
  bool didDraw; // [esp+12h] [ebp-56h]
  bool drawSolid; // [esp+13h] [ebp-55h]
  const idMaterial *shader; // [esp+14h] [ebp-54h]
  float alphaRef; // [esp+18h] [ebp-50h]
  idDrawVert *ac; // [esp+1Ch] [ebp-4Ch]
  const srfTriangles_s *tri; // [esp+20h] [ebp-48h]
  const float *regs; // [esp+24h] [ebp-44h]
  float color[4]; // [esp+28h] [ebp-40h] BYREF
  idPlane plane; // [esp+38h] [ebp-30h] BYREF
  long double clipPlane[4]; // [esp+48h] [ebp-20h] BYREF

  geo = surf->geo;
  material = surf->material;
  ac = 0;
  tri = surf->geo;
  shader = material;
  if ( backEnd.viewDef->numClipPlanes && surf->space != backEnd.currentSpace )
  {
    GL_SelectTexture(1);
    R_GlobalPlaneToLocal(surf->space->modelMatrix, backEnd.viewDef->clipPlanes, &plane);
    plane.d = plane.d + 0.5;
    qglTexGenfv(0x2000u, 0x2501u, &plane.a);
    GL_SelectTexture(0);
    qglEnable(0x3000u);
    clipPlane[0] = plane.a;
    clipPlane[1] = plane.b;
    clipPlane[2] = plane.c;
    clipPlane[3] = plane.d;
    qglClipPlane(0x3000u, clipPlane);
  }
  numStages = material->numStages;
  if ( (numStages > 0 || material->entityGui || material->gui)
    && geo->numIndexes
    && material->coverage != MC_TRANSLUCENT )
  {
    if ( !geo->primBatchMesh && !geo->ambientCache )
    {
      (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
        common.type,
        "RB_T_FillDepthBuffer: !tri->ambientCache\n");
      return;
    }
    shaderRegisters = surf->shaderRegisters;
    v5 = 0;
    regs = shaderRegisters;
    if ( numStages < 4 )
    {
LABEL_19:
      v7 = v5 == numStages;
      if ( v5 >= numStages )
        goto LABEL_28;
      v8 = &material->stages[v5];
      do
      {
        if ( shaderRegisters[v8->conditionRegister] != 0.0 )
          break;
        ++v5;
        ++v8;
      }
      while ( v5 < numStages );
    }
    else
    {
      v6 = material->stages + 2;
      while ( shaderRegisters[v6[-2].conditionRegister] == 0.0 )
      {
        if ( shaderRegisters[v6[-1].conditionRegister] != 0.0 )
        {
          ++v5;
          break;
        }
        if ( shaderRegisters[v6->conditionRegister] != 0.0 )
        {
          v5 += 2;
          break;
        }
        if ( shaderRegisters[v6[1].conditionRegister] != 0.0 )
        {
          v5 += 3;
          break;
        }
        v5 += 4;
        v6 += 4;
        if ( v5 >= numStages - 3 )
          goto LABEL_19;
      }
    }
    v7 = v5 == numStages;
LABEL_28:
    if ( v7 )
      return;
    GL_PolygonOffset(material, 1);
    if ( LODWORD(material->sort) == -1065353216 )
    {
      GL_State(3);
      color[2] = 1.0 / backEnd.overBright;
      color[1] = color[2];
      color[0] = color[2];
    }
    else
    {
      memset(color, 0, 12);
    }
    color[3] = 1.0;
    if ( !geo->primBatchMesh )
    {
      ac = (idDrawVert *)idVertexCache::Position(&vertexCache, geo->ambientCache);
      qglVertexPointer(3, 0x1406u, 64, ac);
      qglTexCoordPointer(2, 0x1406u, 64, &ac->st);
    }
    coverage = material->coverage;
    drawSolid = coverage == MC_OPAQUE;
    if ( coverage != MC_PERFORATED )
    {
LABEL_62:
      if ( !drawSolid )
      {
LABEL_66:
        GL_PolygonOffset(material, 0);
        if ( LODWORD(material->sort) == -1065353216 )
          GL_State(0);
        return;
      }
LABEL_63:
      qglColor4fv(color);
      v18 = globalImages->GetWhiteImage(globalImages);
      v18->Bind(v18);
      if ( geo->primBatchMesh )
      {
        RB_ARB2_MD5R_DrawDepthElements(surf);
      }
      else
      {
        GL_VertexAttribState(9u);
        RB_DrawElementsWithCounters(geo);
      }
      goto LABEL_66;
    }
    didDraw = 0;
    if ( !r_alphaToCoverage.internalVar->integerValue
      || (useAlphaToCoverage = 1, r_multiSamples.internalVar->integerValue <= 0) )
    {
      useAlphaToCoverage = 0;
    }
    v10 = 1;
    qglEnable(0xBC0u);
    v11 = 0;
    if ( material->numStages > 0 )
    {
      v12 = 0;
      do
      {
        stages = shader->stages;
        dynamic = stages[v12].texture.dynamic;
        v15 = &stages[v12];
        if ( dynamic != DI_REFLECTION_RENDER
          && dynamic != DI_REFRACTION_RENDER
          && v15->hasAlphaTest
          && regs[v15->conditionRegister] != 0.0 )
        {
          v16 = regs[v15->color.registers[3]];
          didDraw = 1;
          color[3] = regs[v15->color.registers[3]];
          if ( v16 > 0.0 )
          {
            qglColor4fv(color);
            alphaRef = regs[v15->alphaTestRegister];
            if ( useAlphaToCoverage
              && ((alphaTestMode = v15->alphaTestMode, alphaTestMode == 516) || alphaTestMode == 518)
              && alphaRef >= 0.40000001
              && alphaRef < 0.60000002 )
            {
              if ( v10 )
              {
                qglDisable(0xBC0u);
                qglEnable(0x809Eu);
                v10 = 0;
              }
            }
            else
            {
              if ( !v10 )
              {
                qglDisable(0x809Eu);
                qglEnable(0xBC0u);
                v10 = 1;
              }
              qglAlphaFunc(v15->alphaTestMode, alphaRef);
            }
            v15->texture.image->Bind(v15->texture.image);
            GL_VertexAttribState(9u);
            RB_PrepareStageTexturing(v15, surf, ac, 1);
            RB_DrawElementsWithCounters(tri);
            RB_FinishStageTexturing(v15, surf, ac);
          }
        }
        ++v11;
        ++v12;
      }
      while ( v11 < shader->numStages );
      if ( !v10 )
      {
        qglDisable(0x809Eu);
        material = shader;
LABEL_61:
        geo = tri;
        if ( !didDraw )
          goto LABEL_63;
        goto LABEL_62;
      }
      material = shader;
    }
    qglDisable(0xBC0u);
    goto LABEL_61;
  }
}

// FUNC: void __cdecl RB_STD_FillDepthBuffer(struct drawSurf_s * *,int)
void __cdecl RB_STD_FillDepthBuffer(drawSurf_s **drawSurfs, int numDrawSurfs)
{
  idImage *v2; // eax

  if ( backEnd.viewDef->viewEntitys )
  {
    if ( backEnd.viewDef->numClipPlanes )
    {
      GL_SelectTexture(1);
      v2 = globalImages->GetAlphaNotchImage(globalImages);
      v2->Bind(v2);
      GL_DisableVertexAttribState(0x10u);
      qglEnable(0xC60u);
      qglTexCoord2f(1.0, 0.5);
    }
    GL_SelectTexture(0);
    GL_EnableVertexAttribState(8u);
    GL_State(0);
    qglEnable(0xB90u);
    qglStencilFunc(0x207u, 1, 0xFFu);
    RB_RenderDrawSurfListWithFunction(drawSurfs, numDrawSurfs, RB_T_FillDepthBuffer);
    if ( backEnd.viewDef->numClipPlanes )
    {
      GL_SelectTexture(1);
      globalImages->BindNull(globalImages);
      qglDisable(0xC60u);
      GL_SelectTexture(0);
      qglDisable(0x3000u);
    }
  }
}

// FUNC: void __cdecl RB_SetProgramEnvironment(void)
void __cdecl RB_SetProgramEnvironment()
{
  double v0; // st7
  int w; // [esp+0h] [ebp-1Ch]
  float ha; // [esp+4h] [ebp-18h]
  int h; // [esp+4h] [ebp-18h]
  float v4; // [esp+8h] [ebp-14h]
  float parm[4]; // [esp+Ch] [ebp-10h] BYREF

  if ( glConfig.ARBVertexProgramAvailable )
  {
    w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
    ha = (float)w;
    parm[0] = ha / (double)globalImages->GetCurrentRenderImage(globalImages)->uploadWidth;
    h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
    v4 = (float)h;
    v0 = v4 / (double)globalImages->GetCurrentRenderImage(globalImages)->uploadHeight;
    parm[2] = 0.0;
    parm[3] = 1.0;
    parm[1] = v0;
    qglProgramEnvParameter4fvARB(0x8620u, 0, parm);
    if ( glConfig.ARBFragmentProgramAvailable )
    {
      qglProgramEnvParameter4fvARB(0x8804u, 0, parm);
      parm[2] = 0.0;
      parm[3] = 1.0;
      parm[0] = 1.0 / (double)w;
      parm[1] = 1.0 / (double)h;
      qglProgramEnvParameter4fvARB(0x8804u, 1u, parm);
      *(idVec3 *)parm = backEnd.viewDef->renderView.vieworg;
      parm[3] = 1.0;
      qglProgramEnvParameter4fvARB(0x8620u, 1u, parm);
    }
  }
}

// FUNC: void __cdecl RB_GetCurrentRender(bool)
void __cdecl RB_GetCurrentRender(bool Force)
{
  idImage *v1; // eax
  int x1; // [esp-18h] [ebp-18h]
  int y1; // [esp-14h] [ebp-14h]
  int v4; // [esp-10h] [ebp-10h]
  int v5; // [esp-Ch] [ebp-Ch]

  backEnd.currentRenderNeeded = 0;
  if ( !r_skipPostProcess.internalVar->integerValue
    && glConfig.allowARB2Path
    && (SLOBYTE(backEnd.viewDef->renderFlags) < 0 || Force)
    && tr.backEndRenderer == BE_ARB2 )
  {
    v5 = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
    v4 = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
    y1 = backEnd.viewDef->viewport.y1;
    x1 = backEnd.viewDef->viewport.x1;
    v1 = globalImages->GetCurrentRenderImage(globalImages);
    idImage::CopyFramebuffer(v1, x1, y1, v4, v5, (void *)1);
    backEnd.currentRenderCopied = 1;
  }
}

// FUNC: RB_T_Shadow
void __cdecl RB_T_Shadow(const drawSurf_s *surf)
{
  const viewEntity_s *space; // eax
  const srfTriangles_s *geo; // edi
  const void *v4; // eax
  int integerValue; // eax
  char v6; // bl
  int numShadowIndexesNoCaps; // esi
  int shadowCapPlaneBits; // eax
  int v9; // eax
  double overBright; // st7
  double v11; // st7
  float v12; // eax
  double v13; // st7
  double v14; // st7
  float v15; // [esp+4h] [ebp-2Ch]
  float v16; // [esp+8h] [ebp-28h]
  float v17; // [esp+8h] [ebp-28h]
  float v18; // [esp+Ch] [ebp-24h]
  idVec4 localLight; // [esp+20h] [ebp-10h] BYREF
  float surfa; // [esp+34h] [ebp+4h]
  float surfb; // [esp+34h] [ebp+4h]
  float surfc; // [esp+34h] [ebp+4h]

  if ( tr.backEndRendererHasVertexPrograms )
  {
    if ( r_useShadowVertexProgram.internalVar->integerValue )
    {
      space = surf->space;
      if ( space != backEnd.currentSpace && !surf->geo->primBatchMesh )
      {
        R_GlobalPointToLocal(space->modelMatrix, &backEnd.vLight->globalLightOrigin, (idVec3 *)&localLight);
        localLight.w = 0.0;
        qglProgramEnvParameter4fvARB(0x8620u, 4u, &localLight.x);
      }
    }
  }
  geo = surf->geo;
  if ( !surf->geo->primBatchMesh )
  {
    if ( !geo->shadowCache )
      return;
    v4 = idVertexCache::Position(&vertexCache, geo->shadowCache);
    qglVertexPointer(4, 0x1406u, 16, v4);
  }
  integerValue = r_useExternalShadows.internalVar->integerValue;
  v6 = 0;
  if ( !integerValue )
    goto LABEL_18;
  if ( integerValue != 2 )
  {
    if ( (surf->dsFlags & 1) == 0 )
    {
LABEL_13:
      numShadowIndexesNoCaps = geo->numShadowIndexesNoCaps;
      v6 = 1;
      goto LABEL_19;
    }
    if ( !backEnd.vLight->viewInsideLight )
    {
      shadowCapPlaneBits = surf->geo->shadowCapPlaneBits;
      if ( (shadowCapPlaneBits & 0x40) == 0 )
      {
        if ( (shadowCapPlaneBits & backEnd.vLight->viewSeesShadowPlaneBits) != 0 )
        {
          numShadowIndexesNoCaps = geo->numShadowIndexesNoFrontCaps;
          v6 = 1;
          goto LABEL_19;
        }
        goto LABEL_13;
      }
    }
LABEL_18:
    numShadowIndexesNoCaps = geo->numIndexes;
    goto LABEL_19;
  }
  numShadowIndexesNoCaps = geo->numShadowIndexesNoCaps;
LABEL_19:
  if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.internalVar->integerValue )
    qglDepthBoundsEXT(surf->scissorRect.zmin, surf->scissorRect.zmax);
  v9 = r_showShadows.internalVar->integerValue;
  if ( v9 )
  {
    overBright = backEnd.overBright;
    if ( v9 == 3 )
    {
      surfa = 0.1 / overBright;
      v11 = 1.0 / backEnd.overBright;
      v12 = surfa;
      v18 = surfa;
      if ( !v6 )
      {
        v16 = surfa;
LABEL_36:
        v15 = v11;
        qglColor3f(v15, v16, v18);
        goto LABEL_37;
      }
    }
    else
    {
      if ( (surf->geo->shadowCapPlaneBits & 0x40) != 0 )
      {
        if ( numShadowIndexesNoCaps == geo->numIndexes )
        {
          surfb = 0.1 / overBright;
          v11 = 1.0 / backEnd.overBright;
          v18 = surfb;
          v16 = surfb;
        }
        else
        {
          v13 = 1.0 / overBright;
          v18 = 0.1 * v13;
          v16 = v13 * 0.4;
          v11 = 1.0 / backEnd.overBright;
        }
        goto LABEL_36;
      }
      if ( numShadowIndexesNoCaps != geo->numIndexes )
      {
        v14 = 1.0 / overBright;
        if ( numShadowIndexesNoCaps == geo->numShadowIndexesNoFrontCaps )
        {
          v18 = 0.6 * v14;
          v16 = 1.0 / backEnd.overBright;
          v11 = v14 * 0.1;
        }
        else
        {
          v18 = 0.1 * v14;
          v16 = 1.0 / backEnd.overBright;
          v11 = v14 * 0.6;
        }
        goto LABEL_36;
      }
      surfc = 0.1 / overBright;
      v11 = 1.0 / backEnd.overBright;
      v12 = surfc;
      v18 = surfc;
    }
    v17 = v11;
    qglColor3f(v12, v17, v18);
LABEL_37:
    qglStencilOp(0x1E00u, 0x1E00u, 0x1E00u);
    qglDisable(0xB90u);
    GL_Cull(2);
    RB_DrawShadowElementsWithCounters(surf, numShadowIndexesNoCaps);
    GL_Cull(0);
    qglEnable(0xB90u);
    return;
  }
  if ( !r_useTwoSidedStencil.internalVar->integerValue )
  {
LABEL_64:
    if ( v6 )
      qglStencilOp(0x1E00u, 0x1E00u, tr.stencilIncr);
    else
      qglStencilOp(0x1E00u, tr.stencilDecr, 0x1E00u);
    GL_Cull(0);
    RB_DrawShadowElementsWithCounters(surf, numShadowIndexesNoCaps);
    if ( v6 )
      qglStencilOp(0x1E00u, 0x1E00u, tr.stencilDecr);
    else
      qglStencilOp(0x1E00u, tr.stencilIncr, 0x1E00u);
    GL_Cull(1);
    RB_DrawShadowElementsWithCounters(surf, numShadowIndexesNoCaps);
    return;
  }
  if ( !glConfig.twoSidedStencilAvailable )
  {
    if ( glConfig.atiTwoSidedStencilAvailable )
    {
      if ( backEnd.viewDef->isMirror )
      {
        if ( v6 )
        {
          qglStencilOpSeparateATI(0x404u, 0x1E00u, 0x1E00u, tr.stencilIncr);
          qglStencilOpSeparateATI(0x405u, 0x1E00u, 0x1E00u, tr.stencilDecr);
        }
        else
        {
          qglStencilOpSeparateATI(0x404u, 0x1E00u, tr.stencilDecr, 0x1E00u);
          qglStencilOpSeparateATI(0x405u, 0x1E00u, tr.stencilIncr, 0x1E00u);
        }
      }
      else if ( v6 )
      {
        qglStencilOpSeparateATI(0x405u, 0x1E00u, 0x1E00u, tr.stencilIncr);
        qglStencilOpSeparateATI(0x404u, 0x1E00u, 0x1E00u, tr.stencilDecr);
      }
      else
      {
        qglStencilOpSeparateATI(0x405u, 0x1E00u, tr.stencilDecr, 0x1E00u);
        qglStencilOpSeparateATI(0x404u, 0x1E00u, tr.stencilIncr, 0x1E00u);
      }
      GL_Cull(2);
      RB_DrawShadowElementsWithCounters(surf, numShadowIndexesNoCaps);
      GL_Cull(0);
      return;
    }
    goto LABEL_64;
  }
  if ( backEnd.viewDef->isMirror )
  {
    qglActiveStencilFaceEXT(0x405u);
    if ( v6 )
      qglStencilOp(0x1E00u, 0x1E00u, tr.stencilDecr);
    else
      qglStencilOp(0x1E00u, tr.stencilIncr, 0x1E00u);
    qglActiveStencilFaceEXT(0x404u);
    if ( v6 )
      qglStencilOp(0x1E00u, 0x1E00u, tr.stencilIncr);
    else
      qglStencilOp(0x1E00u, tr.stencilDecr, 0x1E00u);
  }
  else
  {
    qglActiveStencilFaceEXT(0x405u);
    if ( v6 )
      qglStencilOp(0x1E00u, 0x1E00u, tr.stencilIncr);
    else
      qglStencilOp(0x1E00u, tr.stencilDecr, 0x1E00u);
    qglActiveStencilFaceEXT(0x404u);
    if ( v6 )
      qglStencilOp(0x1E00u, 0x1E00u, tr.stencilDecr);
    else
      qglStencilOp(0x1E00u, tr.stencilIncr, 0x1E00u);
  }
  qglEnable(0x8910u);
  GL_Cull(2);
  RB_DrawShadowElementsWithCounters(surf, numShadowIndexesNoCaps);
  GL_Cull(0);
  qglDisable(0x8910u);
}

// FUNC: void __cdecl RB_StencilShadowPass(struct drawSurf_s const *)
void __cdecl RB_StencilShadowPass(const drawSurf_s *drawSurfs)
{
  int integerValue; // eax
  float units; // [esp+0h] [ebp-8h]

  if ( r_shadows.internalVar->integerValue && drawSurfs )
  {
    globalImages->BindNull(globalImages);
    GL_VertexAttribState(1u);
    integerValue = r_showShadows.internalVar->integerValue;
    if ( integerValue )
    {
      if ( integerValue == 2 )
        GL_State(288);
      else
        GL_State(73728);
    }
    else
    {
      GL_State(7936);
    }
    if ( r_shadowPolygonFactor.internalVar->floatValue != 0.0 || r_shadowPolygonOffset.internalVar->floatValue != 0.0 )
    {
      units = -r_shadowPolygonOffset.internalVar->floatValue;
      GL_PolygonOffsetState(1, r_shadowPolygonFactor.internalVar->floatValue, units);
    }
    qglStencilFunc(0x207u, 1, 0xFFu);
    if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.internalVar->integerValue )
      qglEnable(0x8890u);
    RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_Shadow);
    GL_Cull(0);
    if ( r_shadowPolygonFactor.internalVar->floatValue != 0.0 || r_shadowPolygonOffset.internalVar->floatValue != 0.0 )
      GL_PolygonOffsetState(0, 0.0, 0.0);
    if ( glConfig.depthBoundsTestAvailable )
    {
      if ( r_useDepthBoundsTest.internalVar->integerValue )
        qglDisable(0x8890u);
    }
    GL_VertexAttribState(9u);
    qglStencilFunc(0x206u, 128, 0xFFu);
    qglStencilOp(0x1E00u, 0x1E00u, 0x1E00u);
  }
}

// FUNC: RB_T_BlendLight
void __cdecl RB_T_BlendLight(const drawSurf_s *surf)
{
  const srfTriangles_s *geo; // ebx
  int i; // esi
  const void *v3; // [esp-4h] [ebp-4Ch]
  const void *v4; // [esp-4h] [ebp-4Ch]
  idPlane lightProject[4]; // [esp+8h] [ebp-40h] BYREF

  geo = surf->geo;
  if ( backEnd.currentSpace != surf->space )
  {
    for ( i = 0; i < 4; ++i )
      R_GlobalPlaneToLocal(surf->space->modelMatrix, &backEnd.vLight->lightProject[i], &lightProject[i]);
    GL_SelectTexture(0);
    qglTexGenfv(0x2000u, 0x2501u, &lightProject[0].a);
    qglTexGenfv(0x2001u, 0x2501u, &lightProject[1].a);
    qglTexGenfv(0x2003u, 0x2501u, &lightProject[2].a);
    GL_SelectTexture(1);
    qglTexGenfv(0x2000u, 0x2501u, &lightProject[3].a);
  }
  if ( geo->ambientCache )
  {
    v3 = idVertexCache::Position(&vertexCache, geo->ambientCache);
    qglVertexPointer(3, 0x1406u, 64, v3);
  }
  else if ( geo->shadowCache )
  {
    v4 = idVertexCache::Position(&vertexCache, geo->shadowCache);
    qglVertexPointer(3, 0x1406u, 16, v4);
  }
  RB_DrawElementsWithCounters(geo);
}

// FUNC: RB_BlendLight
void __cdecl RB_BlendLight(const drawSurf_s *drawSurfs, const drawSurf_s *drawSurfs2)
{
  int v2; // ebp
  const idMaterial *lightShader; // ebx
  const float *shaderRegisters; // edi
  shaderStage_t *v5; // esi
  int v6; // [esp+4h] [ebp-4h]

  v2 = 0;
  if ( drawSurfs && !r_skipBlendLights.internalVar->integerValue )
  {
    lightShader = backEnd.vLight->lightShader;
    shaderRegisters = backEnd.vLight->shaderRegisters;
    GL_SelectTexture(1);
    GL_DisableVertexAttribState(0x10u);
    qglEnable(0xC60u);
    qglTexCoord2f(0.0, 0.5);
    backEnd.vLight->falloffImage->Bind(backEnd.vLight->falloffImage);
    GL_SelectTexture(0);
    GL_DisableVertexAttribState(8u);
    qglEnable(0xC60u);
    qglEnable(0xC61u);
    qglEnable(0xC63u);
    if ( lightShader->numStages > 0 )
    {
      v6 = 0;
      do
      {
        v5 = &lightShader->stages[v6];
        if ( shaderRegisters[v5->conditionRegister] != 0.0 )
        {
          GL_State(v5->drawStateBits | 0x20100);
          GL_SelectTexture(0);
          v5->texture.image->Bind(v5->texture.image);
          if ( v5->texture.hasMatrix )
            RB_LoadShaderTextureMatrix(shaderRegisters, &v5->texture);
          backEnd.lightColor[0] = shaderRegisters[v5->color.registers[0]];
          backEnd.lightColor[1] = shaderRegisters[v5->color.registers[1]];
          backEnd.lightColor[2] = shaderRegisters[v5->color.registers[2]];
          backEnd.lightColor[3] = shaderRegisters[v5->color.registers[3]];
          qglColor4fv(backEnd.lightColor);
          RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_BlendLight);
          RB_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_BlendLight);
          if ( v5->texture.hasMatrix )
          {
            GL_SelectTexture(0);
            qglMatrixMode(0x1702u);
            qglLoadIdentity();
            qglMatrixMode(0x1700u);
          }
        }
        ++v6;
        ++v2;
      }
      while ( v2 < lightShader->numStages );
    }
    GL_SelectTexture(1);
    qglDisable(0xC60u);
    globalImages->BindNull(globalImages);
    GL_SelectTexture(0);
    qglDisable(0xC60u);
    qglDisable(0xC61u);
    qglDisable(0xC63u);
  }
}

// FUNC: RB_T_BasicFog
void __cdecl RB_T_BasicFog(const drawSurf_s *surf)
{
  rvMesh *v2; // esi
  const rvVertexFormat *DrawVertexBufferFormat; // eax
  int v4; // esi
  const rvVertexFormat *v5; // ebp
  unsigned int v6; // edi
  bool fixedFunctionVertex; // [esp+Bh] [ebp-19h]
  rvVertexFormat *drawVertexFormat; // [esp+Ch] [ebp-18h]
  rvMesh *primBatchMesh; // [esp+10h] [ebp-14h]
  idPlane local; // [esp+14h] [ebp-10h] BYREF
  bool surfa; // [esp+28h] [ebp+4h]

  v2 = surf->geo->primBatchMesh;
  surfa = v2 != 0;
  primBatchMesh = v2;
  fixedFunctionVertex = v2 == 0;
  if ( v2 )
  {
    qglEnable(0x8620u);
    DrawVertexBufferFormat = rvMesh::GetDrawVertexBufferFormat(v2);
    v4 = 0;
    drawVertexFormat = (rvVertexFormat *)DrawVertexBufferFormat;
    v5 = supportedMD5RBasicFogVertexFormats;
    v6 = 0;
    while ( !rvVertexFormat::HasSameComponents(drawVertexFormat, v5) )
    {
      v6 += 184;
      ++v4;
      ++v5;
      if ( v6 >= 0x228 )
        goto LABEL_10;
    }
    rvMesh::SetupForDrawRender(primBatchMesh, &supportedMD5RBasicFogVertexFormats[v4]);
    qglBindProgramARB(0x8620u, v4 + 45);
LABEL_10:
    RB_ARB2_LoadMVPMatrixIntoVPParams(surf);
    R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes, &local);
    local.d = local.d + 0.5;
    qglProgramEnvParameter4fvARB(0x8620u, 0x5Du, &local.a);
    memset(&local, 0, 12);
    local.d = 0.5;
    qglProgramEnvParameter4fvARB(0x8620u, 0x5Eu, &local.a);
    R_GlobalPlaneToLocal(surf->space->modelMatrix, &fogPlanes[2], &local);
    local.d = local.d + 0.5078125;
    qglProgramEnvParameter4fvARB(0x8620u, 0x5Fu, &local.a);
    R_GlobalPlaneToLocal(surf->space->modelMatrix, &fogPlanes[3], &local);
    qglProgramEnvParameter4fvARB(0x8620u, 0x60u, &local.a);
  }
  else if ( backEnd.currentSpace != surf->space )
  {
    GL_SelectTexture(0);
    R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes, &local);
    local.d = local.d + 0.5;
    qglTexGenfv(0x2000u, 0x2501u, &local.a);
    memset(&local, 0, 12);
    local.d = 0.5;
    qglTexGenfv(0x2001u, 0x2501u, &local.a);
    GL_SelectTexture(1);
    R_GlobalPlaneToLocal(surf->space->modelMatrix, &fogPlanes[2], &local);
    local.d = local.d + 0.5078125;
    qglTexGenfv(0x2001u, 0x2501u, &local.a);
    R_GlobalPlaneToLocal(surf->space->modelMatrix, &fogPlanes[3], &local);
    qglTexGenfv(0x2000u, 0x2501u, &local.a);
  }
  if ( surfa )
    RB_DrawElementsWithCounters(surf->geo);
  else
    RB_T_RenderTriangleSurface(surf);
  if ( !fixedFunctionVertex )
    qglDisable(0x8620u);
}

// FUNC: RB_FogPass
void __cdecl RB_FogPass(const drawSurf_s *drawSurfs, const drawSurf_s *drawSurfs2)
{
  const srfTriangles_s *frustumTris; // esi
  const float *shaderRegisters; // eax
  shaderStage_t *stages; // ecx
  idImage *v5; // eax
  idImage *v6; // eax
  double v7; // st7
  double v8; // st6
  float a; // [esp+4h] [ebp-3Ch]
  float aa; // [esp+4h] [ebp-3Ch]
  drawSurf_s v11; // [esp+8h] [ebp-38h] BYREF

  frustumTris = backEnd.vLight->frustumTris;
  if ( frustumTris->ambientCache )
  {
    memset(&v11, 0, sizeof(v11));
    v11.geo = frustumTris;
    v11.space = &backEnd.viewDef->worldSpace;
    v11.scissorRect = backEnd.viewDef->scissor;
    shaderRegisters = backEnd.vLight->shaderRegisters;
    stages = backEnd.vLight->lightShader->stages;
    backEnd.lightColor[0] = shaderRegisters[stages->color.registers[0]];
    backEnd.lightColor[1] = shaderRegisters[stages->color.registers[1]];
    backEnd.lightColor[2] = shaderRegisters[stages->color.registers[2]];
    backEnd.lightColor[3] = shaderRegisters[stages->color.registers[3]];
    qglColor3fv(backEnd.lightColor);
    if ( backEnd.lightColor[3] > 1.0 )
      a = -0.5 / backEnd.lightColor[3];
    else
      a = -0.001;
    GL_State(131429);
    GL_SelectTexture(0);
    v5 = globalImages->GetFogImage(globalImages);
    v5->Bind(v5);
    GL_DisableVertexAttribState(8u);
    qglEnable(0xC60u);
    qglEnable(0xC61u);
    qglTexCoord2f(0.5, 0.5);
    fogPlanes[0].a = a * backEnd.viewDef->worldSpace.modelViewMatrix[2];
    fogPlanes[0].b = a * backEnd.viewDef->worldSpace.modelViewMatrix[6];
    fogPlanes[0].c = a * backEnd.viewDef->worldSpace.modelViewMatrix[10];
    fogPlanes[0].d = a * backEnd.viewDef->worldSpace.modelViewMatrix[14];
    fogPlanes[1].a = a * backEnd.viewDef->worldSpace.modelViewMatrix[0];
    fogPlanes[1].b = a * backEnd.viewDef->worldSpace.modelViewMatrix[4];
    fogPlanes[1].c = a * backEnd.viewDef->worldSpace.modelViewMatrix[8];
    fogPlanes[1].d = a * backEnd.viewDef->worldSpace.modelViewMatrix[12];
    GL_SelectTexture(1);
    v6 = globalImages->GetFogEnterImage(globalImages);
    v6->Bind(v6);
    GL_DisableVertexAttribState(0x10u);
    qglEnable(0xC60u);
    qglEnable(0xC61u);
    fogPlanes[2].a = backEnd.vLight->fogPlane.a * 0.001;
    fogPlanes[2].b = backEnd.vLight->fogPlane.b * 0.001;
    fogPlanes[2].c = backEnd.vLight->fogPlane.c * 0.001;
    fogPlanes[2].d = backEnd.vLight->fogPlane.d * 0.001;
    v7 = fogPlanes[2].c * backEnd.viewDef->renderView.vieworg.z + fogPlanes[2].b * backEnd.viewDef->renderView.vieworg.y;
    v8 = fogPlanes[2].a * backEnd.viewDef->renderView.vieworg.x;
    fogPlanes[3].a = 0.0;
    fogPlanes[3].b = 0.0;
    fogPlanes[3].c = 0.0;
    aa = v7 + v8 + fogPlanes[2].d + 0.5078125;
    fogPlanes[3].d = aa;
    qglTexCoord2f(aa, 0.5078125);
    RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_BasicFog);
    RB_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_BasicFog);
    GL_State(357);
    GL_Cull(1);
    RB_RenderDrawSurfChainWithFunction(&v11, RB_T_BasicFog);
    GL_Cull(0);
    GL_SelectTexture(1);
    qglDisable(0xC60u);
    qglDisable(0xC61u);
    globalImages->BindNull(globalImages);
    GL_SelectTexture(0);
    qglDisable(0xC60u);
    qglDisable(0xC61u);
  }
}

// FUNC: void __cdecl RB_STD_FogAllLights(void)
void __cdecl RB_STD_FogAllLights()
{
  viewLight_s *viewLights; // esi
  const idMaterial *lightShader; // eax

  if ( !r_showOverDraw.internalVar->integerValue )
  {
    qglDisable(0xB90u);
    viewLights = backEnd.viewDef->viewLights;
    if ( viewLights )
    {
      while ( 1 )
      {
        backEnd.vLight = viewLights;
        lightShader = viewLights->lightShader;
        if ( lightShader->fogLight )
          break;
        if ( lightShader->blendLight )
        {
          if ( lightShader->blendLight )
            RB_BlendLight(viewLights->globalInteractions, viewLights->localInteractions);
          goto LABEL_9;
        }
LABEL_10:
        viewLights = viewLights->next;
        if ( !viewLights )
          goto LABEL_11;
      }
      RB_FogPass(viewLights->globalInteractions, viewLights->localInteractions);
LABEL_9:
      qglDisable(0xB90u);
      goto LABEL_10;
    }
LABEL_11:
    qglEnable(0xB90u);
  }
}

// FUNC: void __cdecl RB_STD_LightScale(void)
void __cdecl RB_STD_LightScale()
{
  double v0; // st7
  float v1; // [esp+34h] [ebp-Ch]
  float v2; // [esp+34h] [ebp-Ch]
  float v; // [esp+38h] [ebp-8h]
  float f; // [esp+3Ch] [ebp-4h]

  if ( backEnd.overBright != 1.0 && !r_skipLightScale.internalVar->integerValue )
  {
    if ( r_useScissor.internalVar->integerValue )
    {
      qglScissor(
        backEnd.viewDef->scissor.x1 + backEnd.viewDef->viewport.x1,
        backEnd.viewDef->scissor.y1 + backEnd.viewDef->viewport.y1,
        backEnd.viewDef->scissor.x2 - backEnd.viewDef->scissor.x1 + 1,
        backEnd.viewDef->scissor.y2 - backEnd.viewDef->scissor.y1 + 1);
      backEnd.currentScissor = backEnd.viewDef->scissor;
    }
    qglLoadIdentity();
    qglMatrixMode(0x1701u);
    qglPushMatrix();
    qglLoadIdentity();
    qglOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    GL_State(51);
    GL_Cull(2);
    globalImages->BindNull(globalImages);
    qglDisable(0xB71u);
    qglDisable(0xB90u);
    v = 1.0;
    v1 = 1.0 - backEnd.overBright;
    if ( COERCE_FLOAT(LODWORD(v1) & 0x7FFFFFFF) > 0.01 )
    {
      do
      {
        v0 = backEnd.overBright / v * 0.5;
        f = v0;
        if ( v0 > 1.0 )
          f = 1.0;
        qglColor3f(f, f, f);
        v = f * v + f * v;
        qglBegin(7u);
        qglVertex2f(0.0, 0.0);
        qglVertex2f(0.0, 1.0);
        qglVertex2f(1.0, 1.0);
        qglVertex2f(1.0, 0.0);
        qglEnd();
        v2 = v - backEnd.overBright;
      }
      while ( COERCE_FLOAT(LODWORD(v2) & 0x7FFFFFFF) > 0.01 );
    }
    qglPopMatrix();
    qglEnable(0xB71u);
    qglMatrixMode(0x1700u);
    GL_Cull(0);
  }
}

// FUNC: void __cdecl RB_STD_T_RenderShaderPasses(struct drawSurf_s const *)
void __cdecl RB_STD_T_RenderShaderPasses(const drawSurf_s *surf)
{
  const idMaterial *material; // esi
  bool v2; // cc
  const srfTriangles_s *geo; // ebx
  const viewEntity_s *space; // eax
  int weaponDepthHackInViewID; // eax
  const viewEntity_s *v6; // edi
  idDrawVert *v7; // edi
  shaderStage_t *v8; // ebx
  dynamicidImage_t dynamic; // eax
  char *v10; // eax
  newShaderStage_t *newStage; // esi
  bool v12; // zf
  const drawSurf_s *v13; // edi
  rvMesh *primBatchMesh; // ecx
  idMegaTexture *megaTexture; // ecx
  const float *v16; // ebx
  int *v17; // edi
  int v18; // ecx
  int v19; // edx
  int v20; // eax
  int *v21; // edi
  int v22; // ecx
  int v23; // edx
  int v24; // eax
  int v25; // edi
  idImage **fragmentProgramImages; // ebx
  int v27; // edi
  idImage **v28; // ebx
  idMegaTexture *v29; // esi
  const float *v30; // edi
  const srfTriangles_s *v31; // esi
  rvNewShaderStage *v32; // ebx
  int drawStateBits; // ecx
  idImage *v34; // eax
  const viewEntity_s *v35; // ecx
  int v36; // eax
  int cullType; // [esp-4h] [ebp-274h]
  bool resetTexCoords; // [esp+13h] [ebp-25Dh]
  idImage *image; // [esp+14h] [ebp-25Ch] BYREF
  const srfTriangles_s *tri; // [esp+18h] [ebp-258h]
  float color[4]; // [esp+1Ch] [ebp-254h] BYREF
  const float *regs; // [esp+2Ch] [ebp-244h]
  idDrawVert *ac; // [esp+30h] [ebp-240h]
  const idMaterial *shader; // [esp+34h] [ebp-23Ch]
  int v45; // [esp+38h] [ebp-238h]
  unsigned int v46; // [esp+3Ch] [ebp-234h]
  rvNewShaderStage *newShaderStage; // [esp+40h] [ebp-230h]
  idVec3 localViewer; // [esp+44h] [ebp-22Ch] BYREF
  float parm[4]; // [esp+50h] [ebp-220h] BYREF
  float v50[4]; // [esp+60h] [ebp-210h] BYREF
  _OWORD v51[16]; // [esp+70h] [ebp-200h] BYREF
  drawInteraction_t inter; // [esp+170h] [ebp-100h] BYREF

  material = surf->material;
  v2 = material->numAmbientStages <= 0;
  geo = surf->geo;
  resetTexCoords = 0;
  tri = surf->geo;
  shader = material;
  if ( !v2 && !material->portalSky )
  {
    space = surf->space;
    if ( space != backEnd.currentSpace )
    {
      qglLoadMatrixf(space->modelViewMatrix);
      backEnd.currentSpace = surf->space;
      RB_SetProgramEnvironmentSpace();
    }
    if ( geo->primBatchMesh )
      backEnd.currentSpace = 0;
    if ( r_useScissor.internalVar->integerValue )
    {
      if ( !idScreenRect::Equals(&backEnd.currentScissor, &surf->scissorRect) )
      {
        backEnd.currentScissor = surf->scissorRect;
        qglScissor(
          backEnd.currentScissor.x1 + backEnd.viewDef->viewport.x1,
          (*(int *)&backEnd.currentScissor.x1 >> 16) + backEnd.viewDef->viewport.y1,
          backEnd.currentScissor.x2 - backEnd.currentScissor.x1 + 1,
          (*(int *)&backEnd.currentScissor.x2 >> 16) - (*(int *)&backEnd.currentScissor.x1 >> 16) + 1);
      }
      material = shader;
    }
    if ( geo->numIndexes )
    {
      if ( !geo->primBatchMesh && !geo->ambientCache )
      {
        (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
          common.type,
          "RB_T_RenderShaderPasses: !tri->ambientCache\n");
        return;
      }
      cullType = material->cullType;
      regs = surf->shaderRegisters;
      GL_Cull(cullType);
      GL_PolygonOffset(material, 1);
      weaponDepthHackInViewID = surf->space->weaponDepthHackInViewID;
      if ( weaponDepthHackInViewID && weaponDepthHackInViewID == backEnd.viewDef->renderView.viewID )
        RB_EnterWeaponDepthHack();
      v6 = surf->space;
      if ( v6->modelDepthHack != 0.0 )
        RB_EnterModelDepthHack(v6->modelDepthHack);
      if ( geo->primBatchMesh )
      {
        v7 = 0;
        ac = 0;
      }
      else
      {
        v7 = (idDrawVert *)idVertexCache::Position(&vertexCache, geo->ambientCache);
        ac = v7;
        qglVertexPointer(3, 0x1406u, 64, v7);
        qglTexCoordPointer(2, 0x1406u, 64, &v7->st);
        GL_VertexAttribState(9u);
      }
      if ( material->numStages > 0 )
      {
        v45 = 16;
        v46 = 0;
        do
        {
          v8 = &shader->stages[v46 / 0x84];
          dynamic = v8->texture.dynamic;
          if ( dynamic != DI_REFLECTION_RENDER
            && dynamic != DI_REFRACTION_RENDER
            && regs[v8->conditionRegister] != 0.0
            && v8->lighting == SL_AMBIENT
            && LOBYTE(v8->drawStateBits) != 33 )
          {
            if ( resetTexCoords )
            {
              qglTexCoordPointer(2, 0x1406u, 64, &v7->st);
              resetTexCoords = 0;
            }
            if ( v8->texture.texgen == TG_POT_CORRECTION )
            {
              v10 = idVertexCache::Position(&vertexCache, surf->dynamicTexCoords);
              qglTexCoordPointer(2, 0x1406u, 0, v10);
              resetTexCoords = 1;
            }
            newStage = v8->newStage;
            if ( newStage )
            {
              v12 = !v8->texture.hasMatrix;
              inter.surf = surf;
              if ( !v12 )
              {
                image = 0;
                R_SetDrawInteraction(v8, regs, &image, inter.diffuseMatrix, 0);
              }
              R_GlobalPointToLocal(
                surf->space->modelMatrix,
                &backEnd.viewDef->renderView.vieworg,
                (idVec3 *)&inter.localViewOrigin);
              if ( tr.backEndRenderer == BE_ARB2
                && glConfig.allowARB2Path
                && !r_skipNewAmbient.internalVar->integerValue )
              {
                if ( tri->primBatchMesh )
                {
                  if ( newStage->md5rVertexProgram <= 0 )
                  {
                    qglBindProgramARB(0x8620u, newStage->vertexProgram);
                    v13 = surf;
                  }
                  else
                  {
                    qglBindProgramARB(0x8620u, newStage->md5rVertexProgram);
                    v13 = surf;
                    RB_ARB2_LoadLocalViewOriginIntoVPParams(surf);
                    RB_ARB2_LoadMVPMatrixIntoVPParams(surf);
                    RB_ARB2_LoadProjectionMatrixIntoVPParams();
                    RB_ARB2_LoadModelViewMatrixIntoVPParams(surf);
                  }
                  GL_State(v8->drawStateBits);
                  primBatchMesh = tri->primBatchMesh;
                  if ( !primBatchMesh->m_drawSetUp )
                    rvMesh::SetupForDrawRender(primBatchMesh, 0);
                }
                else
                {
                  qglColorPointer(4, 0x1401u, 64, v7->color);
                  qglVertexAttribPointerARB(9u, 3, 0x1406u, 0, 64, v7->tangents);
                  qglVertexAttribPointerARB(0xAu, 3, 0x1406u, 0, 64, &v7->tangents[1]);
                  qglNormalPointer(0x1406u, 64, &v7->normal);
                  GL_VertexAttribState(0x600000Fu);
                  GL_State(v8->drawStateBits);
                  qglBindProgramARB(0x8620u, newStage->vertexProgram);
                  v13 = surf;
                }
                qglEnable(0x8620u);
                megaTexture = newStage->megaTexture;
                if ( megaTexture )
                {
                  idMegaTexture::SetMappingForSurface(megaTexture, tri);
                  R_GlobalPointToLocal(v13->space->modelMatrix, &backEnd.viewDef->renderView.vieworg, &localViewer);
                  idMegaTexture::BindForViewOrigin(newStage->megaTexture, localViewer);
                }
                v2 = newStage->numVertexParms <= 0;
                v16 = regs;
                image = 0;
                if ( !v2 )
                {
                  v17 = &newStage->vertexParms[0][1];
                  do
                  {
                    v18 = *v17;
                    v19 = v17[1];
                    parm[0] = v16[*(v17 - 1)];
                    v20 = v17[2];
                    parm[1] = v16[v18];
                    parm[2] = v16[v19];
                    parm[3] = v16[v20];
                    qglProgramLocalParameter4fvARB(0x8620u, (unsigned int)image, parm);
                    v17 += 4;
                    v2 = (int)&image->__vftable + 1 < newStage->numVertexParms;
                    image = (idImage *)((char *)image + 1);
                  }
                  while ( v2 );
                }
                v2 = newStage->numFragmentParms <= 0;
                image = 0;
                if ( !v2 )
                {
                  v21 = &newStage->fragmentParms[0][1];
                  do
                  {
                    v22 = *v21;
                    v23 = v21[1];
                    v50[0] = v16[*(v21 - 1)];
                    v24 = v21[2];
                    v50[1] = v16[v22];
                    v50[2] = v16[v23];
                    v50[3] = v16[v24];
                    qglProgramLocalParameter4fvARB(0x8804u, (unsigned int)image, v50);
                    v21 += 4;
                    v2 = (int)&image->__vftable + 1 < newStage->numFragmentParms;
                    image = (idImage *)((char *)image + 1);
                  }
                  while ( v2 );
                }
                v25 = 0;
                if ( newStage->numFragmentProgramImages > 0 )
                {
                  fragmentProgramImages = newStage->fragmentProgramImages;
                  do
                  {
                    if ( *fragmentProgramImages )
                    {
                      GL_SelectTexture(v25);
                      (*fragmentProgramImages)->Bind(*fragmentProgramImages);
                    }
                    ++v25;
                    ++fragmentProgramImages;
                  }
                  while ( v25 < newStage->numFragmentProgramImages );
                }
                qglBindProgramARB(0x8804u, newStage->fragmentProgram);
                qglEnable(0x8804u);
                RB_DrawElementsWithCounters(tri);
                v27 = 1;
                if ( newStage->numFragmentProgramImages > 1 )
                {
                  v28 = &newStage->fragmentProgramImages[1];
                  do
                  {
                    if ( *v28 )
                    {
                      GL_SelectTexture(v27);
                      globalImages->BindNull(globalImages);
                    }
                    ++v27;
                    ++v28;
                  }
                  while ( v27 < newStage->numFragmentProgramImages );
                }
                v29 = newStage->megaTexture;
                if ( v29 )
                  idMegaTexture::Unbind(v29);
                GL_SelectTexture(0);
                qglDisable(0x8620u);
                qglDisable(0x8804u);
                qglBindProgramARB(0x8620u, 0);
                v7 = ac;
              }
            }
            else
            {
              newShaderStage = v8->newShaderStage;
              if ( newShaderStage )
              {
                if ( newShaderStage->IsValid(newShaderStage) && !r_skipNewAmbient.internalVar->integerValue )
                {
                  v12 = !v8->texture.hasMatrix;
                  memset(v51, 0, sizeof(v51));
                  v30 = regs;
                  LODWORD(v51[0]) = surf;
                  if ( !v12 )
                  {
                    image = 0;
                    R_SetDrawInteraction(v8, regs, &image, (idVec4 *)&v51[12], 0);
                  }
                  R_GlobalPointToLocal(
                    surf->space->modelMatrix,
                    &backEnd.viewDef->renderView.vieworg,
                    (idVec3 *)&v51[5]);
                  if ( (surf->mFlags & 1) != 0 )
                  {
                    color[3] = 1.0;
                    color[2] = 1.0;
                    color[1] = 1.0;
                    color[0] = 1.0;
                  }
                  else
                  {
                    color[0] = v30[v8->color.registers[0]];
                    color[1] = v30[v8->color.registers[1]];
                    color[2] = v30[v8->color.registers[2]];
                    color[3] = v30[v8->color.registers[3]];
                  }
                  v31 = tri;
                  if ( tri->primBatchMesh )
                  {
                    if ( v8->vertexColor == SVC_IGNORE )
                      qglColor4f(color[0], color[1], color[2], color[3]);
                  }
                  else
                  {
                    GL_VertexAttribState(0x600000Bu);
                    if ( v8->vertexColor )
                    {
                      if ( (surf->mFlags & 1) != 0 )
                        qglColorPointer(4, 0x1401u, 4, &ac->xyz.x + v45 * v31->numVerts);
                      else
                        qglColorPointer(4, 0x1401u, 64, ac->color);
                      GL_EnableVertexAttribState(4u);
                    }
                    else
                    {
                      qglColor4f(color[0], color[1], color[2], color[3]);
                    }
                    qglVertexAttribPointerARB(9u, 3, 0x1406u, 0, 64, ac->tangents);
                    qglVertexAttribPointerARB(0xAu, 3, 0x1406u, 0, 64, &ac->tangents[1]);
                    qglNormalPointer(0x1406u, 64, &ac->normal);
                  }
                  GL_State(v8->drawStateBits);
                  v32 = newShaderStage;
                  newShaderStage->Bind(newShaderStage, v30, (const drawInteraction_t *)v51);
                  RB_DrawElementsWithCounters(v31);
                  v32->UnBind(v32);
                  GL_SelectTexture(0);
                  v7 = ac;
                }
              }
              else
              {
                if ( (surf->mFlags & 1) != 0 )
                {
                  color[3] = 1.0;
                  color[2] = 1.0;
                  color[1] = 1.0;
                  color[0] = 1.0;
                }
                else
                {
                  color[0] = regs[v8->color.registers[0]];
                  color[1] = regs[v8->color.registers[1]];
                  color[2] = regs[v8->color.registers[2]];
                  color[3] = regs[v8->color.registers[3]];
                }
                drawStateBits = (unsigned __int8)v8->drawStateBits;
                if ( (drawStateBits != 32 || color[0] > 0.0 || color[1] > 0.0 || color[2] > 0.0)
                  && (drawStateBits != 101 || color[3] > 0.0) )
                {
                  if ( !tri->primBatchMesh )
                    GL_VertexAttribState(9u);
                  if ( v8->vertexColor == SVC_IGNORE )
                  {
                    qglColor4f(color[0], color[1], color[2], color[3]);
LABEL_113:
                    RB_BindVariableStageImage(&v8->texture, regs);
                    GL_State(v8->drawStateBits);
                    RB_PrepareStageTexturing(v8, surf, v7, 0);
                    RB_DrawElementsWithCounters(tri);
                    RB_FinishStageTexturing(v8, surf, v7);
                    if ( v8->vertexColor )
                    {
                      GL_SelectTexture(1);
                      GL_TexEnv(8448);
                      globalImages->BindNull(globalImages);
                      GL_SelectTexture(0);
                      GL_TexEnv(8448);
                    }
                    goto LABEL_115;
                  }
                  if ( (surf->mFlags & 1) != 0 )
                  {
                    qglColorPointer(4, 0x1401u, 4, &v7->xyz.x + v45 * tri->numVerts);
                    goto LABEL_104;
                  }
                  if ( !tri->primBatchMesh )
                  {
                    qglColorPointer(4, 0x1401u, 64, v7->color);
LABEL_104:
                    GL_EnableVertexAttribState(4u);
                  }
                  if ( v8->vertexColor == SVC_INVERSE_MODULATE )
                  {
                    GL_TexEnv(34160);
                    qglTexEnvi(0x2300u, 0x8571u, 8448);
                    qglTexEnvi(0x2300u, 0x8580u, 5890);
                    qglTexEnvi(0x2300u, 0x8581u, 34167);
                    qglTexEnvi(0x2300u, 0x8590u, 768);
                    qglTexEnvi(0x2300u, 0x8591u, 769);
                    qglTexEnvi(0x2300u, 0x8573u, 1);
                  }
                  if ( color[0] != 1.0
                    || color[1] != 1.0
                    || color[2] != 1.0
                    || color[3] != 1.0
                    || (surf->mFlags & 1) != 0 )
                  {
                    GL_SelectTexture(1);
                    v34 = globalImages->GetWhiteImage(globalImages);
                    v34->Bind(v34);
                    GL_TexEnv(34160);
                    qglTexEnvfv(0x2300u, 0x2201u, color);
                    qglTexEnvi(0x2300u, 0x8571u, 8448);
                    qglTexEnvi(0x2300u, 0x8580u, 34168);
                    qglTexEnvi(0x2300u, 0x8581u, 34166);
                    qglTexEnvi(0x2300u, 0x8590u, 768);
                    qglTexEnvi(0x2300u, 0x8591u, 768);
                    qglTexEnvi(0x2300u, 0x8573u, 1);
                    qglTexEnvi(0x2300u, 0x8572u, 8448);
                    qglTexEnvi(0x2300u, 0x8588u, 34168);
                    qglTexEnvi(0x2300u, 0x8589u, 34166);
                    qglTexEnvi(0x2300u, 0x8598u, 770);
                    qglTexEnvi(0x2300u, 0x8599u, 770);
                    qglTexEnvi(0x2300u, 0xD1Cu, 1);
                    GL_SelectTexture(0);
                  }
                  goto LABEL_113;
                }
              }
            }
          }
LABEL_115:
          v46 += 132;
          ++v45;
        }
        while ( v45 - 16 < shader->numStages );
      }
      GL_PolygonOffset(shader, 0);
      v35 = surf->space;
      v36 = v35->weaponDepthHackInViewID;
      if ( v36 && v36 == backEnd.viewDef->renderView.viewID || v35->modelDepthHack != 0.0 )
        RB_LeaveDepthHack();
      qglBindBufferARB(0x8893u, 0);
    }
  }
}

// FUNC: int __cdecl RB_STD_DrawShaderPasses(struct drawSurf_s * *,int,enum materialSort_t,enum materialSort_t)
int __cdecl RB_STD_DrawShaderPasses(
        drawSurf_s **drawSurfs,
        int numDrawSurfs,
        materialSort_t minSort,
        materialSort_t maxSort)
{
  idImage *v4; // eax
  int v5; // edi
  drawSurf_s *v6; // esi
  const idMaterial *material; // ecx
  int x1; // [esp-14h] [ebp-20h]
  int y1; // [esp-10h] [ebp-1Ch]
  int v11; // [esp-Ch] [ebp-18h]
  int v12; // [esp-8h] [ebp-14h]

  if ( minSort == SS_POST_PROCESS && !backEnd.viewDef->isSubview && backEnd.currentRenderNeeded )
  {
    backEnd.currentRenderNeeded = 0;
    if ( !r_skipPostProcess.internalVar->integerValue
      && glConfig.allowARB2Path
      && SLOBYTE(backEnd.viewDef->renderFlags) < 0
      && tr.backEndRenderer == BE_ARB2 )
    {
      v12 = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
      v11 = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
      y1 = backEnd.viewDef->viewport.y1;
      x1 = backEnd.viewDef->viewport.x1;
      v4 = globalImages->GetCurrentRenderImage(globalImages);
      idImage::CopyFramebuffer(v4, x1, y1, v11, v12, (void *)1);
      backEnd.currentRenderCopied = 1;
    }
  }
  GL_SelectTexture(1);
  globalImages->BindNull(globalImages);
  v5 = 0;
  GL_SelectTexture(0);
  GL_EnableVertexAttribState(8u);
  RB_SetProgramEnvironment();
  backEnd.currentSpace = 0;
  if ( numDrawSurfs > 0 )
  {
    while ( 1 )
    {
      v6 = drawSurfs[v5];
      material = v6->material;
      if ( !material->suppressInSubview )
        break;
LABEL_21:
      if ( ++v5 >= numDrawSurfs )
        goto LABEL_22;
    }
    if ( (double)(int)minSort > material->sort || (double)(int)maxSort < material->sort )
    {
LABEL_19:
      if ( v6->material->sort >= 100.0 )
        backEnd.currentRenderNeeded = 1;
      goto LABEL_21;
    }
    if ( minSort != SS_POST_PROCESS )
    {
      if ( (material->materialFlags & 0x100) == 0 || backEnd.currentRenderCopied )
      {
LABEL_18:
        RB_STD_T_RenderShaderPasses(v6);
        goto LABEL_19;
      }
      RB_GetCurrentRender(1);
    }
    if ( !backEnd.currentRenderCopied )
      goto LABEL_19;
    goto LABEL_18;
  }
LABEL_22:
  GL_Cull(0);
  qglColor4f(1.0, 1.0, 1.0, 1.0);
  GL_VertexAttribState(9u);
  return 0;
}

// FUNC: void __cdecl RB_STD_DrawView(void)
void __cdecl RB_STD_DrawView()
{
  drawSurf_s **drawSurfs; // esi
  int numDrawSurfs; // edi

  backEnd.depthFunc = 0x20000;
  drawSurfs = backEnd.viewDef->drawSurfs;
  numDrawSurfs = backEnd.viewDef->numDrawSurfs;
  RB_DisplaySpecialEffects(backEnd.viewDef->viewEntitys, 1);
  RB_BeginDrawingView();
  if ( r_showEditorImages.internalVar->integerValue )
  {
    RB_RenderDrawSurfListWithFunction(drawSurfs, numDrawSurfs, RB_T_RenderSimpleSurface);
  }
  else
  {
    RB_DetermineLightScale();
    RB_STD_FillDepthBuffer(drawSurfs, numDrawSurfs);
    RB_DisplaySpecialEffects(backEnd.viewDef->viewEntitys, 0);
    switch ( tr.backEndRenderer )
    {
      case BE_ARB:
        RB_ARB_DrawInteractions();
        break;
      case BE_NV10:
        RB_NV10_DrawInteractions();
        break;
      case BE_NV20:
        RB_NV20_DrawInteractions();
        break;
      case BE_R200:
        RB_R200_DrawInteractions();
        break;
      case BE_ARB2:
        RB_ARB2_DrawInteractions();
        break;
      default:
        break;
    }
  }
  qglStencilFunc(0x207u, 128, 0xFFu);
  RB_STD_LightScale();
  if ( r_portalsDistanceCull.internalVar->integerValue && backEnd.viewDef->viewEntitys )
    backEnd.viewDef->renderWorld->RenderPortalFades(backEnd.viewDef->renderWorld);
  if ( !r_skipDecals.internalVar->integerValue )
    RB_STD_DrawShaderPasses(drawSurfs, numDrawSurfs, SS_MIN, SS_FAR);
  if ( !r_skipFogLights.internalVar->integerValue )
    RB_STD_FogAllLights();
  if ( !r_skipAmbient.internalVar->integerValue || !backEnd.viewDef->viewEntitys )
    RB_STD_DrawShaderPasses(drawSurfs, numDrawSurfs, SS_MEDIUM, SS_NEAREST);
  if ( !r_skipPostProcess.internalVar->integerValue )
    RB_STD_DrawShaderPasses(drawSurfs, numDrawSurfs, SS_POST_PROCESS, SS_MAX);
  RB_RenderDebugTools(drawSurfs, numDrawSurfs);
}
