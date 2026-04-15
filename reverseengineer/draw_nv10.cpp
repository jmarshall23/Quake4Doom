
// FUNC: void __cdecl R_NV10_Init(void)
void __cdecl R_NV10_Init()
{
  glConfig.allowNV10Path = glConfig.registerCombinersAvailable;
}

// FUNC: RB_RenderInteraction
void __usercall RB_RenderInteraction(const drawSurf_s *surf@<eax>)
{
  const idMaterial *material; // edi
  const viewEntity_s *space; // eax
  int v4; // ebx
  const srfTriangles_s *geo; // ebp
  int weaponDepthHackInViewID; // eax
  const viewEntity_s *v7; // ecx
  char *v8; // ebx
  shaderStage_t *v9; // ebx
  stageLighting_t lighting; // ecx
  idDeclBase *base; // ecx
  int v12; // edi
  int v13; // eax
  int depthFunc; // eax
  const textureStage_t *p_texture; // ebx
  idImage *v16; // eax
  int v17; // ecx
  idImage *v18; // eax
  const void *v19; // eax
  int v20; // ebx
  int v21; // eax
  int v22; // ebx
  int v23; // eax
  idImage *v24; // eax
  idImage *v25; // eax
  const void *v26; // eax
  stageLighting_t v27; // eax
  const textureStage_t *v28; // edi
  stageLighting_t v29; // eax
  idImage *v30; // eax
  stageVertexColor_t vertexColor; // eax
  shaderStage_t *v32; // edi
  int v33; // edx
  int v34; // eax
  double v35; // st7
  const textureStage_t *v36; // edi
  const viewEntity_s *v37; // esi
  int v38; // eax
  bool diffuseOnly; // [esp+Bh] [ebp-3Dh]
  const float *surfaceRegs; // [esp+Ch] [ebp-3Ch]
  const shaderStage_t *lastBumpStage; // [esp+10h] [ebp-38h]
  const idMaterial *lightShader; // [esp+14h] [ebp-34h]
  const idMaterial *surfaceShader; // [esp+18h] [ebp-30h]
  int v44; // [esp+1Ch] [ebp-2Ch]
  int v45; // [esp+20h] [ebp-28h]
  viewLight_s *vLight; // [esp+24h] [ebp-24h]
  const float *lightRegs; // [esp+28h] [ebp-20h]
  int j; // [esp+2Ch] [ebp-1Ch]
  int i; // [esp+30h] [ebp-18h]
  float color[4]; // [esp+38h] [ebp-10h] BYREF

  surfaceRegs = surf->shaderRegisters;
  material = surf->material;
  surfaceShader = material;
  vLight = backEnd.vLight;
  lightShader = backEnd.vLight->lightShader;
  lightRegs = backEnd.vLight->shaderRegisters;
  if ( (_S3 & 1) == 0 )
    _S3 |= 1u;
  space = surf->space;
  v4 = 0;
  geo = surf->geo;
  lastBumpStage = 0;
  if ( space != backEnd.currentSpace )
  {
    backEnd.currentSpace = space;
    qglLoadMatrixf(surf->space->modelViewMatrix);
    do
    {
      R_GlobalPlaneToLocal(surf->space->modelMatrix, &backEnd.vLight->lightProject[v4], &lightProject[v4]);
      ++v4;
    }
    while ( v4 < 4 );
  }
  if ( r_useScissor.internalVar->integerValue && !idScreenRect::Equals(&backEnd.currentScissor, &surf->scissorRect) )
  {
    backEnd.currentScissor = surf->scissorRect;
    qglScissor(
      backEnd.currentScissor.x1 + backEnd.viewDef->viewport.x1,
      (*(int *)&backEnd.currentScissor.x1 >> 16) + backEnd.viewDef->viewport.y1,
      backEnd.currentScissor.x2 - backEnd.currentScissor.x1 + 1,
      (*(int *)&backEnd.currentScissor.x2 >> 16) - (*(int *)&backEnd.currentScissor.x1 >> 16) + 1);
  }
  weaponDepthHackInViewID = surf->space->weaponDepthHackInViewID;
  if ( weaponDepthHackInViewID && weaponDepthHackInViewID == backEnd.viewDef->renderView.viewID )
    RB_EnterWeaponDepthHack();
  v7 = surf->space;
  if ( v7->modelDepthHack != 0.0 )
    RB_EnterModelDepthHack(v7->modelDepthHack);
  v8 = (char *)idVertexCache::Position(&vertexCache, geo->ambientCache);
  qglVertexPointer(3, 0x1406u, 64, v8);
  GL_SelectTexture(0);
  qglTexCoordPointer(2, 0x1406u, 64, v8 + 56);
  qglColorPointer(4, 0x1401u, 64, v8 + 12);
  GL_PolygonOffset(surf->material, 1);
  i = 0;
  diffuseOnly = r_forceDiffuseOnly.internalVar->integerValue != 0;
  if ( material->numStages > 0 )
  {
    v45 = 0;
    do
    {
      v9 = &material->stages[v45];
      lighting = v9->lighting;
      if ( lighting == SL_AMBIENT || surfaceRegs[v9->conditionRegister] == 0.0 )
        goto LABEL_65;
      if ( lighting == SL_BUMP )
      {
        if ( !diffuseOnly )
        {
          if ( v9->vertexColor )
          {
            base = material->base;
            v12 = *(_DWORD *)common.type;
            v13 = (int)base->GetName(base);
            (*(void (**)(netadrtype_t, const char *, ...))(v12 + 124))(
              common.type,
              "shader %s: vertexColor on a bump stage\n",
              v13);
            material = surfaceShader;
          }
          lastBumpStage = v9;
          if ( lightShader->ambientLight )
          {
            depthFunc = backEnd.depthFunc;
            BYTE1(depthFunc) = BYTE1(backEnd.depthFunc) | 0xF;
            GL_State(depthFunc);
            GL_SelectTexture(0);
            GL_EnableVertexAttribState(8u);
            p_texture = &v9->texture;
            RB_BindStageTexture(surfaceRegs, p_texture, surf);
            if ( r_skipBump.internalVar->integerValue )
            {
              v16 = globalImages->GetFlatNormalMap(globalImages);
              v16->Bind(v16);
            }
            GL_SelectTexture(1);
            qglEnable(0xC60u);
            qglTexGenfv(0x2000u, 0x2501u, &lightProject[3].a);
            qglTexCoord2f(0.0, 0.5);
            vLight->falloffImage->Bind(vLight->falloffImage);
            qglCombinerParameteriNV(0x854Eu, 2);
            qglCombinerParameterfvNV(0x852Au, &tr.ambientLightVector.x);
            qglCombinerInputNV(0x8550u, 0x1907u, 0x8523u, 0x852Au, 0x8538u, 0x1907u);
            qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x84C0u, 0x8538u, 0x1907u);
            qglCombinerOutputNV(0x8550u, 0x1907u, 0x852Cu, 0x8530u, 0x8530u, 0, 0, 1u, 0, 0);
            qglCombinerInputNV(0x8551u, 0x1906u, 0x8523u, 0x852Cu, 0x8536u, 0x1905u);
            qglCombinerInputNV(0x8551u, 0x1906u, 0x8524u, 0x84C1u, 0x8536u, 0x1906u);
            qglCombinerOutputNV(0x8551u, 0x1906u, 0x852Cu, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
            qglFinalCombinerInputNV(0x8523u, 0, 0x8536u, 0x1907u);
            qglFinalCombinerInputNV(0x8524u, 0, 0x8536u, 0x1907u);
            qglFinalCombinerInputNV(0x8525u, 0, 0x8536u, 0x1907u);
            qglFinalCombinerInputNV(0x8526u, 0, 0x8536u, 0x1907u);
            qglFinalCombinerInputNV(0x8529u, 0x852Cu, 0x8536u, 0x1906u);
            RB_DrawElementsWithCounters(geo);
            globalImages->BindNull(globalImages);
            qglDisable(0xC60u);
            GL_SelectTexture(0);
            RB_FinishStageTexture(p_texture, surf);
          }
          else
          {
            v17 = backEnd.depthFunc;
            BYTE1(v17) = BYTE1(backEnd.depthFunc) | 0xF;
            GL_State(v17);
            GL_DisableVertexAttribState(0xCu);
            qglEnable(0xC60u);
            qglTexGenfv(0x2000u, 0x2501u, &lightProject[3].a);
            qglTexCoord2f(0.0, 0.5);
            vLight->falloffImage->Bind(vLight->falloffImage);
            qglCombinerParameteriNV(0x854Eu, 1);
            qglCombinerOutputNV(0x8550u, 0x1906u, 0x8530u, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
            qglFinalCombinerInputNV(0x8523u, 0, 0x8536u, 0x1907u);
            qglFinalCombinerInputNV(0x8524u, 0, 0x8536u, 0x1907u);
            qglFinalCombinerInputNV(0x8525u, 0, 0x8536u, 0x1907u);
            qglFinalCombinerInputNV(0x8526u, 0, 0x8536u, 0x1907u);
            qglFinalCombinerInputNV(0x8529u, 0x84C0u, 0x8536u, 0x1906u);
            RB_DrawElementsWithCounters(geo);
            qglDisable(0xC60u);
            GL_State(backEnd.depthFunc | 0xF07);
            GL_SelectTexture(0);
            GL_EnableVertexAttribState(8u);
            RB_BindStageTexture(surfaceRegs, &v9->texture, surf);
            GL_SelectTexture(1);
            v18 = globalImages->GetNormalCubeMapImage(globalImages);
            v18->Bind(v18);
            GL_EnableVertexAttribState(0x10u);
            v19 = idVertexCache::Position(&vertexCache, geo->lightingCache);
            qglTexCoordPointer(3, 0x1406u, 12, v19);
            GL_DisableVertexAttribState(4u);
            qglCombinerParameteriNV(0x854Eu, 1);
            qglCombinerInputNV(0x8550u, 0x1907u, 0x8523u, 0x84C1u, 0x8538u, 0x1907u);
            qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x84C0u, 0x8538u, 0x1907u);
            qglCombinerOutputNV(0x8550u, 0x1907u, 0x852Eu, 0x8530u, 0x8530u, 0, 0, 1u, 0, 0);
            qglFinalCombinerInputNV(0x8529u, 0x852Eu, 0x8536u, 0x1905u);
            RB_DrawElementsWithCounters(geo);
            globalImages->BindNull(globalImages);
            GL_DisableVertexAttribState(0x10u);
            GL_SelectTexture(0);
            RB_FinishStageTexture(&v9->texture, surf);
            material = surfaceShader;
          }
        }
        goto LABEL_65;
      }
      if ( lighting != SL_DIFFUSE || lastBumpStage || diffuseOnly )
      {
        if ( lighting == SL_SPECULAR )
        {
          if ( diffuseOnly || r_skipSpecular.internalVar->integerValue || lightShader->ambientLight )
            goto LABEL_65;
          if ( !lastBumpStage )
          {
            v22 = *(_DWORD *)common.type;
            v23 = (int)material->base->GetName(material->base);
            (*(void (**)(netadrtype_t, const char *, ...))(v22 + 124))(
              common.type,
              "shader %s: specular stage without a preceeding bumpmap stage\n",
              v23);
            goto LABEL_65;
          }
          GL_State(backEnd.depthFunc | 0xF57);
          GL_SelectTexture(0);
          GL_EnableVertexAttribState(8u);
          RB_BindStageTexture(surfaceRegs, &lastBumpStage->texture, surf);
          if ( r_skipBump.internalVar->integerValue )
          {
            v24 = globalImages->GetFlatNormalMap(globalImages);
            v24->Bind(v24);
          }
          GL_SelectTexture(1);
          v25 = globalImages->GetNormalCubeMapImage(globalImages);
          v25->Bind(v25);
          GL_EnableVertexAttribState(0x10u);
          v26 = idVertexCache::Position(&vertexCache, surf->dynamicTexCoords);
          qglTexCoordPointer(4, 0x1406u, 0, v26);
          qglCombinerParameteriNV(0x854Eu, 2);
          qglCombinerInputNV(0x8550u, 0x1907u, 0x8523u, 0x84C1u, 0x8538u, 0x1907u);
          qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x84C0u, 0x8538u, 0x1907u);
          qglCombinerOutputNV(0x8550u, 0x1907u, 0x852Cu, 0x8530u, 0x8530u, 0x853Eu, 0x8541u, 1u, 0, 0);
          qglCombinerOutputNV(0x8550u, 0x1906u, 0x8530u, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
          qglCombinerOutputNV(0x8551u, 0x1907u, 0x852Cu, 0x8530u, 0x8530u, 0, 0, 0, 0, 0);
          qglCombinerInputNV(0x8551u, 0x1906u, 0x8523u, 0x852Cu, 0x8536u, 0x1905u);
          qglCombinerInputNV(0x8551u, 0x1906u, 0x8524u, 0x852Cu, 0x8536u, 0x1905u);
          qglCombinerOutputNV(0x8551u, 0x1906u, 0x852Cu, 0x8530u, 0x8530u, 0x853Eu, 0x8541u, 0, 0, 0);
          qglFinalCombinerInputNV(0x8526u, 0x852Cu, 0x8536u, 0x1907u);
          qglFinalCombinerInputNV(0x8523u, 0, 0x8536u, 0x1907u);
          qglFinalCombinerInputNV(0x8524u, 0, 0x8536u, 0x1907u);
          qglFinalCombinerInputNV(0x8525u, 0, 0x8536u, 0x1907u);
          qglFinalCombinerInputNV(0x8529u, 0x852Cu, 0x8536u, 0x1906u);
          RB_DrawElementsWithCounters(geo);
          globalImages->BindNull(globalImages);
          GL_DisableVertexAttribState(0x10u);
          GL_SelectTexture(0);
          RB_FinishStageTexture(&lastBumpStage->texture, surf);
          material = surfaceShader;
          lastBumpStage = 0;
        }
        v27 = v9->lighting;
        if ( v27 == SL_DIFFUSE || v27 == SL_SPECULAR )
        {
          GL_State(backEnd.depthFunc | 0x1127);
          GL_SelectTexture(0);
          GL_EnableVertexAttribState(8u);
          v28 = &v9->texture;
          RB_BindStageTexture(surfaceRegs, &v9->texture, surf);
          v29 = v9->lighting;
          if ( v29 == SL_DIFFUSE && r_skipDiffuse.internalVar->integerValue
            || v29 == SL_SPECULAR && r_skipSpecular.internalVar->integerValue )
          {
            v30 = globalImages->GetBlackImage(globalImages);
            v30->Bind(v30);
          }
          GL_SelectTexture(1);
          GL_DisableVertexAttribState(0x10u);
          qglEnable(0xC60u);
          qglEnable(0xC61u);
          qglEnable(0xC63u);
          qglTexGenfv(0x2000u, 0x2501u, &lightProject[0].a);
          qglTexGenfv(0x2001u, 0x2501u, &lightProject[1].a);
          qglTexGenfv(0x2003u, 0x2501u, &lightProject[2].a);
          qglCombinerParameteriNV(0x854Eu, 1);
          qglCombinerInputNV(0x8550u, 0x1907u, 0x8523u, 0x84C0u, 0x8536u, 0x1907u);
          if ( !diffuseOnly )
          {
            vertexColor = v9->vertexColor;
            if ( vertexColor == SVC_MODULATE )
            {
              qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x852Cu, 0x8536u, 0x1907u);
              GL_EnableVertexAttribState(4u);
LABEL_53:
              qglCombinerInputNV(0x8550u, 0x1907u, 0x8525u, 0x84C1u, 0x8536u, 0x1907u);
              qglCombinerInputNV(0x8550u, 0x1907u, 0x8526u, 0x852Bu, 0x8536u, 0x1907u);
              qglCombinerOutputNV(0x8550u, 0x1907u, 0x852Eu, 0x852Fu, 0x8530u, 0, 0, 0, 0, 0);
              qglFinalCombinerInputNV(0x8523u, 0x852Fu, 0x8536u, 0x1907u);
              qglFinalCombinerInputNV(0x8524u, 0x852Eu, 0x8536u, 0x1907u);
              qglFinalCombinerInputNV(0x8525u, 0, 0x8536u, 0x1907u);
              qglFinalCombinerInputNV(0x8526u, 0, 0x8536u, 0x1907u);
              qglFinalCombinerInputNV(0x8529u, 0, 0x8536u, 0x1906u);
              j = 0;
              if ( lightShader->numStages > 0 )
              {
                v44 = 0;
                do
                {
                  v32 = &lightShader->stages[v44];
                  if ( lightRegs[v32->conditionRegister] != 0.0 )
                  {
                    v33 = v9->color.registers[1];
                    color[0] = lightRegs[v32->color.registers[0]]
                             * surfaceRegs[v9->color.registers[0]]
                             * backEnd.lightScale;
                    v34 = v9->color.registers[2];
                    color[1] = lightRegs[v32->color.registers[1]] * surfaceRegs[v33] * backEnd.lightScale;
                    v35 = lightRegs[v32->color.registers[2]] * surfaceRegs[v34];
                    color[3] = 1.0;
                    color[2] = v35 * backEnd.lightScale;
                    if ( color[0] != 0.0 || color[1] != 0.0 || color[2] != 0.0 )
                    {
                      qglCombinerParameterfvNV(0x852Bu, color);
                      v36 = &v32->texture;
                      RB_BindStageTexture(lightRegs, v36, surf);
                      RB_DrawElementsWithCounters(geo);
                      RB_FinishStageTexture(v36, surf);
                    }
                  }
                  ++v44;
                  ++j;
                }
                while ( j < lightShader->numStages );
                v28 = &v9->texture;
              }
              if ( v9->vertexColor )
                GL_DisableVertexAttribState(4u);
              qglDisable(0xC60u);
              qglDisable(0xC61u);
              qglDisable(0xC63u);
              globalImages->BindNull(globalImages);
              GL_SelectTexture(0);
              RB_FinishStageTexture(v28, surf);
              material = surfaceShader;
              goto LABEL_65;
            }
            if ( vertexColor == SVC_INVERSE_MODULATE )
            {
              qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0x852Cu, 0x8537u, 0x1907u);
              GL_EnableVertexAttribState(4u);
              goto LABEL_53;
            }
          }
          qglCombinerInputNV(0x8550u, 0x1907u, 0x8524u, 0, 0x8537u, 0x1907u);
          GL_DisableVertexAttribState(4u);
          goto LABEL_53;
        }
      }
      else
      {
        v20 = *(_DWORD *)common.type;
        v21 = (int)material->base->GetName(material->base);
        (*(void (**)(netadrtype_t, const char *, ...))(v20 + 124))(
          common.type,
          "shader %s: diffuse stage without a preceeding bumpmap stage\n",
          v21);
      }
LABEL_65:
      ++v45;
      ++i;
    }
    while ( i < material->numStages );
  }
  GL_PolygonOffset(surf->material, 0);
  v37 = surf->space;
  v38 = v37->weaponDepthHackInViewID;
  if ( v38 && v38 == backEnd.viewDef->renderView.viewID || v37->modelDepthHack != 0.0 )
    RB_LeaveDepthHack();
}

// FUNC: RB_RenderInteractionList
void __usercall RB_RenderInteractionList(const drawSurf_s *surf@<eax>)
{
  const drawSurf_s *v1; // esi

  v1 = surf;
  if ( surf )
  {
    qglEnable(0x8522u);
    backEnd.currentSpace = 0;
    do
    {
      RB_RenderInteraction(v1);
      v1 = v1->nextOnLight;
    }
    while ( v1 );
    qglDisable(0x8522u);
  }
}

// FUNC: void __cdecl RB_NV10_DrawInteractions(void)
void __cdecl RB_NV10_DrawInteractions()
{
  viewLight_s *i; // esi

  qglEnable(0xB90u);
  for ( i = backEnd.viewDef->viewLights; i; i = i->next )
    RB_RenderViewLight_0(i);
}
