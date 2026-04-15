
// FUNC: protected: void __thiscall rvRenderModelMD5R::ResetValues(void)
void __thiscall rvRenderModelMD5R::ResetValues(rvRenderModelMD5R *this)
{
  this->purged = 1;
  this->m_joints = 0;
  this->m_skinSpaceToLocalMats = 0;
  this->m_defaultPose = 0;
  this->m_numJoints = 0;
  this->m_vertexBuffers = 0;
  this->m_allocVertexBuffers = 0;
  this->m_numVertexBuffers = 0;
  this->m_indexBuffers = 0;
  this->m_allocIndexBuffers = 0;
  this->m_numIndexBuffers = 0;
  this->m_silEdges = 0;
  this->m_allocSilEdges = 0;
  this->m_numSilEdges = 0;
  this->m_numSilEdgesAdded = 0;
  this->m_meshes = 0;
  this->m_numMeshes = 0;
  this->m_numLODs = 0;
  this->m_lods = 0;
  this->m_allLODMeshes = 0;
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::ParseVertexBuffers(class Lexer &)
void __thiscall rvRenderModelMD5R::ParseVertexBuffers(rvRenderModelMD5R *this, Lexer *lex)
{
  int m_numVertexBuffers; // ebx
  int *v4; // eax
  rvVertexBuffer *v5; // ebp
  rvVertexBuffer *v6; // eax
  int v7; // ebx
  int v8; // ebp

  Lexer::ExpectTokenString(lex, "[");
  this->m_numVertexBuffers = Lexer::ParseInt(lex);
  Lexer::ExpectTokenString(lex, "]");
  Lexer::ExpectTokenString(lex, "{");
  m_numVertexBuffers = this->m_numVertexBuffers;
  v4 = (int *)Memory::Allocate(472 * m_numVertexBuffers + 4);
  if ( v4 )
  {
    v5 = (rvVertexBuffer *)(v4 + 1);
    *v4 = m_numVertexBuffers;
    `eh vector constructor iterator'(
      v4 + 1,
      0x1D8u,
      m_numVertexBuffers,
      (void (__thiscall *)(void *))rvVertexBuffer::rvVertexBuffer,
      (void (__thiscall *)(void *))rvVertexBuffer::~rvVertexBuffer);
    v6 = v5;
  }
  else
  {
    v6 = 0;
  }
  this->m_allocVertexBuffers = v6;
  this->m_vertexBuffers = v6;
  if ( !v6 )
    Lexer::Error(lex, "Out of memory");
  v7 = 0;
  if ( this->m_numVertexBuffers > 0 )
  {
    v8 = 0;
    do
    {
      Lexer::ExpectTokenString(lex, "VertexBuffer");
      rvVertexBuffer::Init(&this->m_vertexBuffers[v8], lex);
      ++v7;
      ++v8;
    }
    while ( v7 < this->m_numVertexBuffers );
  }
  Lexer::ExpectTokenString(lex, "}");
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::ParseIndexBuffers(class Lexer &)
void __thiscall rvRenderModelMD5R::ParseIndexBuffers(rvRenderModelMD5R *this, Lexer *lex)
{
  int m_numIndexBuffers; // ebx
  int *v4; // eax
  rvIndexBuffer *v5; // ebp
  rvIndexBuffer *v6; // eax
  int v7; // ebx
  int v8; // ebp

  Lexer::ExpectTokenString(lex, "[");
  this->m_numIndexBuffers = Lexer::ParseInt(lex);
  Lexer::ExpectTokenString(lex, "]");
  Lexer::ExpectTokenString(lex, "{");
  m_numIndexBuffers = this->m_numIndexBuffers;
  v4 = (int *)Memory::Allocate(36 * m_numIndexBuffers + 4);
  if ( v4 )
  {
    v5 = (rvIndexBuffer *)(v4 + 1);
    *v4 = m_numIndexBuffers;
    `eh vector constructor iterator'(
      v4 + 1,
      0x24u,
      m_numIndexBuffers,
      (void (__thiscall *)(void *))rvIndexBuffer::rvIndexBuffer,
      (void (__thiscall *)(void *))rvIndexBuffer::~rvIndexBuffer);
    v6 = v5;
  }
  else
  {
    v6 = 0;
  }
  this->m_allocIndexBuffers = v6;
  this->m_indexBuffers = v6;
  if ( !v6 )
    Lexer::Error(lex, "Out of memory");
  v7 = 0;
  if ( this->m_numIndexBuffers > 0 )
  {
    v8 = 0;
    do
    {
      Lexer::ExpectTokenString(lex, "IndexBuffer");
      rvIndexBuffer::Init(&this->m_indexBuffers[v8], lex);
      ++v7;
      ++v8;
    }
    while ( v7 < this->m_numIndexBuffers );
  }
  Lexer::ExpectTokenString(lex, "}");
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::ParseSilhouetteEdges(class Lexer &)
void __thiscall rvRenderModelMD5R::ParseSilhouetteEdges(rvRenderModelMD5R *this, Lexer *lex)
{
  int v3; // eax
  silEdge_t *v4; // eax
  int v5; // ebp
  int v6; // ebx

  Lexer::ExpectTokenString(lex, "[");
  v3 = Lexer::ParseInt(lex);
  this->m_numSilEdges = v3;
  this->m_numSilEdgesAdded = v3;
  Lexer::ExpectTokenString(lex, "]");
  Lexer::ExpectTokenString(lex, "{");
  v4 = (silEdge_t *)Memory::Allocate(16 * this->m_numSilEdges);
  this->m_allocSilEdges = v4;
  this->m_silEdges = v4;
  if ( !v4 )
    Lexer::Error(lex, "Out of memory");
  v5 = 0;
  if ( this->m_numSilEdges > 0 )
  {
    v6 = 0;
    do
    {
      this->m_silEdges[v6].p1 = Lexer::ParseInt(lex);
      this->m_silEdges[v6].p2 = Lexer::ParseInt(lex);
      this->m_silEdges[v6].v1 = Lexer::ParseInt(lex);
      this->m_silEdges[v6].v2 = Lexer::ParseInt(lex);
      ++v5;
      ++v6;
    }
    while ( v5 < this->m_numSilEdges );
  }
  Lexer::ExpectTokenString(lex, "}");
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::ParseLevelOfDetail(class Lexer &)
void __thiscall rvRenderModelMD5R::ParseLevelOfDetail(rvRenderModelMD5R *this, Lexer *lex)
{
  levelOfDetailMD5R_s *v3; // eax
  int v4; // ebp
  int v5; // edi
  levelOfDetailMD5R_s *m_lods; // edx
  double m_rangeEnd; // st7
  levelOfDetailMD5R_s *v8; // eax

  Lexer::ExpectTokenString(lex, "[");
  this->m_numLODs = Lexer::ParseInt(lex);
  Lexer::ExpectTokenString(lex, "]");
  Lexer::ExpectTokenString(lex, "{");
  v3 = (levelOfDetailMD5R_s *)Memory::Allocate(12 * this->m_numLODs);
  this->m_lods = v3;
  if ( !v3 )
    Lexer::Error(lex, "Out of memory");
  v4 = 0;
  if ( this->m_numLODs > 0 )
  {
    v5 = 0;
    do
    {
      this->m_lods[v5].m_rangeEnd = Lexer::ParseFloat(lex, 0);
      m_lods = this->m_lods;
      m_rangeEnd = m_lods[v5].m_rangeEnd;
      v8 = &m_lods[v5];
      ++v4;
      ++v5;
      v8->m_rangeEndSquared = m_rangeEnd * m_rangeEnd;
      this->m_lods[v5 - 1].m_meshList = 0;
    }
    while ( v4 < this->m_numLODs );
  }
  Lexer::ExpectTokenString(lex, "}");
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::ParseMeshes(class Lexer &)
void __thiscall rvRenderModelMD5R::ParseMeshes(rvRenderModelMD5R *this, Lexer *lex)
{
  int m_numMeshes; // ebx
  int *v4; // eax
  rvMesh *v5; // ebp
  int v6; // ebx
  int v7; // ebp

  Lexer::ExpectTokenString(lex, "[");
  this->m_numMeshes = Lexer::ParseInt(lex);
  Lexer::ExpectTokenString(lex, "]");
  Lexer::ExpectTokenString(lex, "{");
  m_numMeshes = this->m_numMeshes;
  v4 = (int *)Memory::Allocate(96 * m_numMeshes + 4);
  v5 = 0;
  if ( v4 )
  {
    v5 = (rvMesh *)(v4 + 1);
    *v4 = m_numMeshes;
    `eh vector constructor iterator'(
      v4 + 1,
      0x60u,
      m_numMeshes,
      (void (__thiscall *)(void *))rvMesh::rvMesh,
      (void (__thiscall *)(void *))rvMesh::~rvMesh);
  }
  this->m_meshes = v5;
  if ( !v5 )
    Lexer::Error(lex, "Out of memory");
  v6 = 0;
  if ( this->m_numMeshes > 0 )
  {
    v7 = 0;
    do
    {
      Lexer::ExpectTokenString(lex, "Mesh");
      rvMesh::Init(&this->m_meshes[v7], this, lex);
      this->m_meshes[v7++].m_meshIdentifier = v6++;
    }
    while ( v6 < this->m_numMeshes );
  }
  Lexer::ExpectTokenString(lex, "}");
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::BuildLevelsOfDetail(void)
void __thiscall rvRenderModelMD5R::BuildLevelsOfDetail(rvRenderModelMD5R *this)
{
  int v2; // eax
  int v3; // edx
  __int16 *p_m_levelOfDetail; // ecx
  levelOfDetailMD5R_s *v5; // eax
  int v6; // edx
  int v7; // ecx
  levelOfDetailMD5R_s *m_lods; // eax
  levelOfDetailMD5R_s *v9; // eax
  double m_rangeEnd; // st7
  levelOfDetailMD5R_s *v11; // eax
  int v12; // edi
  int v13; // ecx
  rvMesh *m_meshes; // edx
  int m_levelOfDetail; // eax
  rvMesh *v16; // edx
  int v17; // eax

  v2 = 0;
  v3 = -1;
  if ( this->m_numMeshes > 0 )
  {
    p_m_levelOfDetail = &this->m_meshes->m_levelOfDetail;
    do
    {
      if ( *p_m_levelOfDetail > v3 )
        v3 = v2;
      ++v2;
      p_m_levelOfDetail += 48;
    }
    while ( v2 < this->m_numMeshes );
    if ( v3 >= 0 && !this->m_numLODs )
    {
      this->m_numLODs = v3 + 1;
      v5 = (levelOfDetailMD5R_s *)Memory::Allocate(12 * (v3 + 1));
      this->m_lods = v5;
      if ( !v5 )
        idLib::common->FatalError(idLib::common, "Out of memory");
      v6 = 0;
      if ( this->m_numLODs > 0 )
      {
        v7 = 0;
        do
        {
          m_lods = this->m_lods;
          if ( v6 >= 3 )
            m_lods[v7].m_rangeEnd = m_lods[v7 - 1].m_rangeEnd + m_lods[v7 - 1].m_rangeEnd;
          else
            m_lods[v7].m_rangeEnd = MD5R_DefaultLODRanges[v6];
          v9 = this->m_lods;
          m_rangeEnd = v9[v7].m_rangeEnd;
          v11 = &v9[v7];
          ++v6;
          ++v7;
          v11->m_rangeEndSquared = m_rangeEnd * m_rangeEnd;
        }
        while ( v6 < this->m_numLODs );
      }
    }
  }
  v12 = 0;
  if ( this->m_numMeshes > 0 )
  {
    v13 = 0;
    do
    {
      m_meshes = this->m_meshes;
      m_levelOfDetail = m_meshes[v13].m_levelOfDetail;
      v16 = &m_meshes[v13];
      if ( m_levelOfDetail < 0 || m_levelOfDetail >= this->m_numLODs )
      {
        v16->m_nextInLOD = this->m_allLODMeshes;
        this->m_allLODMeshes = &this->m_meshes[v13];
      }
      else
      {
        v17 = m_levelOfDetail;
        v16->m_nextInLOD = this->m_lods[v17].m_meshList;
        this->m_lods[v17].m_meshList = &this->m_meshes[v13];
      }
      ++v12;
      ++v13;
    }
    while ( v12 < this->m_numMeshes );
  }
}

// FUNC: public: virtual enum dynamicModel_t __thiscall rvRenderModelMD5R::IsDynamicModel(void)const
BOOL __thiscall rvRenderModelMD5R::IsDynamicModel(rvRenderModelMD5R *this)
{
  return this->m_numJoints != 0;
}

// FUNC: public: virtual class idBounds __thiscall rvRenderModelMD5R::Bounds(struct renderEntity_s const *)const
idBounds *__thiscall rvRenderModelMD5R::Bounds(rvRenderModelMD5R *this, idBounds *result, const renderEntity_s *ent)
{
  idBounds *p_bounds; // ecx
  idBounds *v4; // eax

  if ( ent && this->m_numJoints )
    p_bounds = &ent->bounds;
  else
    p_bounds = &this->bounds;
  v4 = result;
  *result = *p_bounds;
  return v4;
}

// FUNC: public: virtual struct srfTriangles_s * __thiscall rvRenderModelMD5R::GenerateStaticTriSurface(int)
srfTriangles_s *__thiscall rvRenderModelMD5R::GenerateStaticTriSurface(rvRenderModelMD5R *this, int meshOffset)
{
  rvMesh *v2; // edi
  srfTriangles_s *v3; // esi
  int m_numDrawVertices; // eax

  v2 = &this->m_meshes[meshOffset];
  v3 = R_AllocStaticTriSurf();
  m_numDrawVertices = v2->m_numDrawVertices;
  v3->numVerts = m_numDrawVertices;
  v3->numIndexes = v2->m_numDrawIndices;
  R_AllocStaticTriSurfVerts(v3, m_numDrawVertices);
  R_AllocStaticTriSurfIndexes(v3, v3->numIndexes);
  rvMesh::CopyTriangles(v2, v3->verts, v3->indexes, 0);
  return v3;
}

// FUNC: public: virtual int __thiscall rvRenderModelMD5R::NumJoints(void)const
const idMD5Joint *__thiscall rvRenderModelMD5R::NumJoints(idRenderModelMD5 *this)
{
  return this->joints.list;
}

// FUNC: public: virtual class idMD5Joint const * __thiscall rvRenderModelMD5R::GetJoints(void)const
int __thiscall rvRenderModelMD5R::GetJoints(idRenderModelMD5 *this)
{
  return this->joints.num;
}

// FUNC: public: virtual class idJointQuat const * __thiscall rvRenderModelMD5R::GetDefaultPose(void)const
const idJointQuat *__thiscall rvRenderModelMD5R::GetDefaultPose(rvRenderModelMD5R *this)
{
  return this->m_defaultPose;
}

// FUNC: protected: int __thiscall rvRenderModelMD5R::CalcMaxBonesPerVertex(struct jointWeight_t const *,int,struct srfTriangles_s const &)
int __thiscall rvRenderModelMD5R::CalcMaxBonesPerVertex(
        rvRenderModelMD5R *this,
        const jointWeight_t *jointWeights,
        int numVerts,
        const srfTriangles_s *tri)
{
  int v4; // edx
  int v5; // edi
  int v6; // esi
  int v8; // eax
  int *p_nextVertexOffset; // ecx
  int v10; // ebp
  idCommon_vtbl *v11; // ebp
  int v12; // eax
  int maxBonesPerVertex; // [esp+Ch] [ebp-4h]

  v4 = 0;
  v5 = 0;
  v6 = 0;
  for ( maxBonesPerVertex = 0; v5 < numVerts; ++v5 )
  {
    v8 = 0;
    p_nextVertexOffset = &jointWeights[v6].nextVertexOffset;
    do
    {
      v10 = *p_nextVertexOffset;
      ++v8;
      ++v6;
      p_nextVertexOffset += 3;
    }
    while ( v10 != 12 );
    if ( v8 > v4 )
    {
      v4 = v8;
      maxBonesPerVertex = v8;
    }
    if ( v8 > 4 )
    {
      v11 = idLib::common->__vftable;
      v12 = ((int (__thiscall *)(rvRenderModelMD5R *, int))this->Name)(this, v8);
      v11->Warning(idLib::common, "Vertex %d in %s is weighted to %d transforms", v5, v12);
      v4 = maxBonesPerVertex;
    }
  }
  return v4;
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::FixBadTangentSpaces(struct srfTriangles_s &)
void __thiscall rvRenderModelMD5R::FixBadTangentSpaces(rvRenderModelMD5R *this, srfTriangles_s *tri)
{
  int v3; // ebx
  int v4; // edx
  idDrawVert *v5; // eax
  idDrawVert *verts; // ecx
  float z; // eax
  idDrawVert *v8; // ecx
  double v9; // st7
  double y; // st6
  long double v11; // st7
  long double v12; // st6
  long double v13; // st7
  long double v14; // st6
  srfTriangles_s *tria; // [esp+10h] [ebp+4h]

  v3 = 0;
  if ( tri->numVerts > 0 )
  {
    v4 = 0;
    do
    {
      v5 = &tri->verts[v4];
      if ( LODWORD(v5->tangents[0].x) == -4194304
        || LODWORD(v5->tangents[0].y) == -4194304
        || LODWORD(v5->tangents[0].z) == -4194304
        || LODWORD(v5->tangents[1].x) == -4194304
        || LODWORD(v5->tangents[1].y) == -4194304
        || LODWORD(v5->tangents[1].z) == -4194304 )
      {
        verts = tri->verts;
        z = verts[v4].normal.z;
        v8 = &verts[v4];
        *(float *)&tria = z;
        HIBYTE(tria) = HIBYTE(z) & 0x7F;
        if ( *(float *)&tria <= 0.69999999 )
        {
          v13 = v8->normal.y * v8->normal.y + v8->normal.x * v8->normal.x;
          v14 = 1.0 / sqrt(v13);
          v8->tangents[0].x = -(v14 * v8->normal.y);
          v8->tangents[0].y = v14 * v8->normal.x;
          v8->tangents[0].z = 0.0;
          v8->tangents[1].x = -(v8->tangents[0].y * v8->normal.z);
          v8->tangents[1].y = v8->normal.z * v8->tangents[0].x;
          v8->tangents[1].z = v14 * v13;
        }
        else
        {
          v9 = v8->normal.z;
          y = v8->normal.y;
          v8->tangents[1].x = 0.0;
          v11 = y * y + v9 * v9;
          v12 = 1.0 / sqrt(v11);
          v8->tangents[1].y = v12 * v8->normal.z;
          v8->tangents[1].z = -(v12 * v8->normal.y);
          v8->tangents[0].x = v12 * v11;
          v8->tangents[0].y = -(v8->tangents[1].z * v8->normal.x);
          v8->tangents[0].z = v8->normal.x * v8->tangents[1].y;
        }
      }
      ++v3;
      ++v4;
    }
    while ( v3 < tri->numVerts );
  }
}

// FUNC: protected: static void __cdecl rvRenderModelMD5R::RemoveFromList(class rvRenderModelMD5R &)
void __cdecl rvRenderModelMD5R::RemoveFromList(rvRenderModelMD5R *model)
{
  rvRenderModelMD5R *v1; // eax
  rvRenderModelMD5R *v2; // ecx

  v1 = rvRenderModelMD5R::ms_modelList;
  v2 = 0;
  if ( rvRenderModelMD5R::ms_modelList )
  {
    while ( v1 != model )
    {
      v2 = v1;
      v1 = v1->m_next;
      if ( !v1 )
        return;
    }
    if ( v2 )
      v2->m_next = model->m_next;
    else
      rvRenderModelMD5R::ms_modelList = model->m_next;
    model->m_next = 0;
  }
}

// FUNC: public: static void __cdecl rvRenderModelMD5R::CompressVertexFormat(class rvVertexFormat &,class rvVertexFormat const &)
void __cdecl rvRenderModelMD5R::CompressVertexFormat(rvVertexFormat *compressedFormat, const rvVertexFormat *format)
{
  rvVertexFormat::Init(compressedFormat, format);
  if ( (compressedFormat->m_flags & 0x10) != 0 )
  {
    compressedFormat->m_dataTypes[4] = 19;
    rvVertexFormat::CalcSize(compressedFormat);
  }
  if ( (compressedFormat->m_flags & 0x20) != 0 )
  {
    compressedFormat->m_dataTypes[5] = 19;
    rvVertexFormat::CalcSize(compressedFormat);
  }
  if ( (compressedFormat->m_flags & 0x40) != 0 )
  {
    compressedFormat->m_dataTypes[6] = 19;
    rvVertexFormat::CalcSize(compressedFormat);
  }
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::WriteSilhouetteEdges(class idFile &,char const *,char const *)
void __thiscall rvRenderModelMD5R::WriteSilhouetteEdges(
        rvRenderModelMD5R *this,
        idFile *outFile,
        const char *prepend,
        const char *prependPlusTab)
{
  int m_numSilEdgesAdded; // eax
  const char *v6; // ebp
  int v7; // ebx
  int v8; // ebp

  m_numSilEdgesAdded = this->m_numSilEdgesAdded;
  if ( m_numSilEdgesAdded )
  {
    v6 = prepend;
    outFile->WriteFloatString(outFile, "%sSilhouetteEdge[ %d ]\n", prepend, m_numSilEdgesAdded);
    outFile->WriteFloatString(outFile, "%s{\n", prepend);
    v7 = 0;
    if ( this->m_numSilEdgesAdded > 0 )
    {
      v8 = 0;
      do
      {
        outFile->WriteFloatString(
          outFile,
          "%s%d %d %d %d\n",
          prependPlusTab,
          this->m_silEdges[v8].p1,
          this->m_silEdges[v8].p2,
          this->m_silEdges[v8].v1,
          this->m_silEdges[v8].v2);
        ++v7;
        ++v8;
      }
      while ( v7 < this->m_numSilEdgesAdded );
      v6 = prepend;
    }
    outFile->WriteFloatString(outFile, "%s}\n", v6);
  }
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::WriteLevelOfDetail(class idFile &,char const *,char const *)
void __thiscall rvRenderModelMD5R::WriteLevelOfDetail(
        rvRenderModelMD5R *this,
        idFile *outFile,
        const char *prepend,
        const char *prependPlusTab)
{
  int m_numLODs; // ebx
  int v6; // ecx
  int v7; // edx
  levelOfDetailMD5R_s *m_lods; // eax
  double v9; // st7
  const char *v10; // ebp
  int v11; // ebx
  int v12; // ebp

  m_numLODs = this->m_numLODs;
  if ( m_numLODs )
  {
    if ( m_numLODs > 3 )
      goto LABEL_11;
    v6 = 0;
    if ( m_numLODs > 0 )
    {
      v7 = 0;
      do
      {
        m_lods = this->m_lods;
        if ( v6 >= 3 )
          v9 = m_lods[v7 - 1].m_rangeEnd + m_lods[v7 - 1].m_rangeEnd;
        else
          v9 = MD5R_DefaultLODRanges[v6];
        if ( m_lods[v7].m_rangeEnd != v9 )
          break;
        ++v6;
        ++v7;
      }
      while ( v6 < m_numLODs );
      if ( v6 < m_numLODs )
      {
LABEL_11:
        v10 = prepend;
        outFile->WriteFloatString(outFile, "%sLevelOfDetail[ %d ]\n", prepend, m_numLODs);
        outFile->WriteFloatString(outFile, "%s{\n", prepend);
        v11 = 0;
        if ( this->m_numLODs > 0 )
        {
          v12 = 0;
          do
          {
            outFile->WriteFloatString(outFile, "%s%f\n", prependPlusTab, this->m_lods[v12].m_rangeEnd);
            ++v11;
            ++v12;
          }
          while ( v11 < this->m_numLODs );
          v10 = prepend;
        }
        outFile->WriteFloatString(outFile, "%s}\n", v10);
      }
    }
  }
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::WriteMeshes(class idFile &,char const *)
void __thiscall rvRenderModelMD5R::WriteMeshes(rvRenderModelMD5R *this, idFile *outFile, const char *prepend)
{
  int m_numMeshes; // eax
  const char *v4; // ebx
  unsigned int v5; // esi
  int v6; // eax
  void *v7; // esp
  rvRenderModelMD5R *v8; // eax
  int v9; // esi
  int v10; // ebx
  char v11[12]; // [esp+0h] [ebp-10h] BYREF
  rvRenderModelMD5R *v12; // [esp+Ch] [ebp-4h]

  m_numMeshes = this->m_numMeshes;
  v12 = this;
  if ( m_numMeshes )
  {
    v4 = prepend;
    outFile->WriteFloatString(outFile, "%sMesh[ %d ]\n", prepend, m_numMeshes);
    outFile->WriteFloatString(outFile, "%s{\n", prepend);
    v5 = strlen(prepend);
    v6 = v5 + 6;
    LOBYTE(v6) = (v5 + 6) & 0xFC;
    v7 = alloca(v6);
    strcpy(v11, prepend);
    v8 = v12;
    v11[v5] = 9;
    v11[v5 + 1] = 0;
    v9 = 0;
    if ( v8->m_numMeshes > 0 )
    {
      v10 = 0;
      do
      {
        rvMesh::Write(&v12->m_meshes[v10], outFile, v11);
        ++v9;
        ++v10;
      }
      while ( v9 < v12->m_numMeshes );
      v4 = prepend;
    }
    outFile->WriteFloatString(outFile, "%s}\n", v4);
  }
}

// FUNC: public: __thiscall rvRenderModelMD5R::rvRenderModelMD5R(void)
void __thiscall rvRenderModelMD5R::rvRenderModelMD5R(rvRenderModelMD5R *this)
{
  idRenderModelStatic::idRenderModelStatic(this);
  this->__vftable = (rvRenderModelMD5R_vtbl *)&rvRenderModelMD5R::`vftable';
  rvRenderModelMD5R::ResetValues(this);
  this->m_source = MD5R_SOURCE_NONE;
  this->m_next = rvRenderModelMD5R::ms_modelList;
  rvRenderModelMD5R::ms_modelList = this;
}

// FUNC: public: virtual void __thiscall rvRenderModelMD5R::Print(void)const
void __thiscall rvRenderModelMD5R::Print(rvRenderModelMD5R *this)
{
  rvRenderModelMD5R *v1; // ebx
  int v2; // edi
  int *p_m_numDrawPrimitives; // esi
  int v4; // ebx
  int v5; // ebp
  int v6; // eax
  int totalVerts; // [esp+Ch] [ebp-10h]
  int totalTris; // [esp+10h] [ebp-Ch]
  int v9; // [esp+14h] [ebp-8h]

  v1 = this;
  v2 = 0;
  totalVerts = 0;
  totalTris = 0;
  (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(common.type, "%s\n", this->name.data);
  (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, "Dynamic MD5R model.\n");
  (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(
    common.type,
    "    verts  tris material\n");
  if ( v1->m_numMeshes > 0 )
  {
    p_m_numDrawPrimitives = &v1->m_meshes->m_numDrawPrimitives;
    do
    {
      totalTris += *p_m_numDrawPrimitives;
      v4 = *(p_m_numDrawPrimitives - 2);
      totalVerts += v4;
      v9 = *p_m_numDrawPrimitives;
      v5 = *(_DWORD *)common.type;
      v6 = (*(int (__thiscall **)(_DWORD))(**(_DWORD **)(*(p_m_numDrawPrimitives - 10) + 4) + 4))(*(_DWORD *)(*(p_m_numDrawPrimitives - 10) + 4));
      (*(void (**)(netadrtype_t, const char *, ...))(v5 + 124))(common.type, "%2i: %5i %5i %s\n", v2, v4, v9, v6);
      v1 = this;
      ++v2;
      p_m_numDrawPrimitives += 24;
    }
    while ( v2 < this->m_numMeshes );
  }
  (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 124))(common.type, "-----\n");
  (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(common.type, "%4i verts.\n", totalVerts);
  (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(common.type, "%4i tris.\n", totalTris);
  (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
    common.type,
    "%4i joints.\n",
    v1->m_numJoints);
}

// FUNC: public: virtual void __thiscall rvRenderModelMD5R::TouchData(void)
void __thiscall rvRenderModelMD5R::TouchData(rvRenderModelMD5R *this)
{
  int v2; // edi
  int v3; // ebx
  idDeclBase *base; // ecx
  idDeclManager_vtbl *v5; // ebp
  int v6; // eax

  v2 = 0;
  if ( this->m_numMeshes > 0 )
  {
    v3 = 0;
    do
    {
      base = this->m_meshes[v3].m_material->base;
      v5 = declManager->__vftable;
      v6 = ((int (__thiscall *)(idDeclBase *, int))base->GetName)(base, 1);
      ((void (__thiscall *)(idDeclManager *, int))v5->FindMaterial)(declManager, v6);
      ++v2;
      ++v3;
    }
    while ( v2 < this->m_numMeshes );
  }
}

// FUNC: public: virtual enum jointHandle_t __thiscall rvRenderModelMD5R::GetJointHandle(char const *)const
int __thiscall rvRenderModelMD5R::GetJointHandle(rvRenderModelMD5R *this, const char *name)
{
  int v3; // esi
  char **i; // edi

  v3 = 0;
  if ( this->m_numJoints <= 0 )
    return -1;
  for ( i = &this->m_joints->name.data; idStr::Icmp(*i, name); i += 9 )
  {
    if ( ++v3 >= this->m_numJoints )
      return -1;
  }
  return v3;
}

// FUNC: public: virtual char const * __thiscall rvRenderModelMD5R::GetJointName(enum jointHandle_t)const
char *__thiscall rvRenderModelMD5R::GetJointName(rvRenderModelMD5R *this, int handle)
{
  if ( handle < 0 || handle >= this->m_numJoints )
    return "<invalid joint>";
  else
    return this->m_joints[handle].name.data;
}

// FUNC: public: virtual int __thiscall rvRenderModelMD5R::GetSurfaceMask(char const *)const
int __thiscall rvRenderModelMD5R::GetSurfaceMask(rvRenderModelMD5R *this, const char *surface)
{
  int v3; // ebx
  int v4; // eax
  int v5; // esi
  idDeclBase *base; // ecx
  const char *v7; // eax
  int m_numMeshes; // [esp+8h] [ebp-4h]

  v3 = 0;
  v4 = this->m_numMeshes - 1;
  if ( v4 >= 0 )
  {
    v5 = v4;
    m_numMeshes = this->m_numMeshes;
    do
    {
      base = this->m_meshes[v5].m_material->base;
      v7 = base->GetName(base);
      if ( !idStr::Icmp(v7, surface) )
        v3 |= 1 << this->m_meshes[v5].m_meshIdentifier;
      --v5;
      --m_numMeshes;
    }
    while ( m_numMeshes );
  }
  return v3;
}

// FUNC: protected: int __thiscall rvRenderModelMD5R::BuildBlend4VertArray(class idMD5Mesh &,struct modelSurface_s &,class rvBlend4DrawVert *)
int __thiscall rvRenderModelMD5R::BuildBlend4VertArray(
        rvRenderModelMD5R *this,
        idMD5Mesh *md5Mesh,
        modelSurface_s *surface,
        rvBlend4DrawVert *drawVertArray)
{
  srfTriangles_s *geometry; // ebx
  int v6; // edx
  int v7; // eax
  int v8; // esi
  float *p_z; // ebp
  int v10; // ecx
  int *p_nextVertexOffset; // edi
  double v12; // st7
  unsigned int v13; // kr00_4
  bool v14; // zf
  int v15; // eax
  double v16; // st6
  double v17; // st7
  float v18; // eax
  int v19; // eax
  int v20; // edx
  int v21; // eax
  float *v22; // ecx
  idDrawVert *verts; // eax
  idDrawVert *v24; // eax
  idDrawVert *v25; // ecx
  idDrawVert *v26; // eax
  idDrawVert *v27; // eax
  bool v28; // cc
  int v29; // esi
  int v30; // ebp
  float *v31; // eax
  idDrawVert *v32; // ecx
  int v33; // edx
  float *v34; // ecx
  float *v35; // ecx
  float *v36; // ecx
  float *v37; // ecx
  idDrawVert *v38; // edx
  int i; // esi
  int v40; // eax
  int curVert; // [esp+10h] [ebp-38h]
  int v43; // [esp+14h] [ebp-34h]
  int maxBonesPerVertex; // [esp+18h] [ebp-30h]
  int curWeight; // [esp+1Ch] [ebp-2Ch]
  int numVertBlendTransforms; // [esp+20h] [ebp-28h]
  jointWeight_t *jointWeights; // [esp+24h] [ebp-24h]
  float vertBlendWeights[4]; // [esp+28h] [ebp-20h] BYREF
  int vertBlendIndices[4]; // [esp+38h] [ebp-10h] BYREF
  modelSurface_s *surfacea; // [esp+50h] [ebp+8h]

  geometry = surface->geometry;
  maxBonesPerVertex = 0;
  R_DeriveTangents(geometry, 1);
  rvRenderModelMD5R::FixBadTangentSpaces(this, geometry);
  jointWeights = md5Mesh->weights;
  v6 = 0;
  v7 = 0;
  curWeight = 0;
  curVert = 0;
  if ( md5Mesh->texCoords.num > 0 )
  {
    v8 = 0;
    v43 = 0;
    surfacea = 0;
    p_z = &drawVertArray->normal.z;
    while ( 1 )
    {
      v10 = 0;
      p_nextVertexOffset = &jointWeights[v6].nextVertexOffset;
      do
      {
        v12 = *((float *)p_nextVertexOffset - 2);
        v13 = *(p_nextVertexOffset - 1);
        vertBlendWeights[v10] = *((float *)p_nextVertexOffset - 2);
        v14 = v12 > vertBlendWeights[0];
        vertBlendIndices[v10] = v13 / 0x30;
        if ( v14 )
        {
          v15 = vertBlendIndices[0];
          v16 = v12;
          v17 = vertBlendWeights[0];
          vertBlendIndices[0] = v13 / 0x30;
          vertBlendWeights[0] = v16;
          vertBlendIndices[v10] = v15;
          vertBlendWeights[v10] = v17;
        }
        v18 = *(float *)p_nextVertexOffset;
        ++curWeight;
        ++v10;
        p_nextVertexOffset += 3;
      }
      while ( LODWORD(v18) != 12 );
      numVertBlendTransforms = v10;
      if ( v10 > maxBonesPerVertex )
        maxBonesPerVertex = v10;
      v19 = 0;
      if ( v10 <= 0 || (v19 = v10, qmemcpy(p_z - 10, vertBlendIndices, 4 * v10), v8 = v43, v10 < 4) )
        memset(&drawVertArray->blendIndex[(_DWORD)surfacea + v19], 0, 4 * (4 - v19));
      v20 = v10;
      v21 = 0;
      if ( v10 >= 4 )
      {
        v22 = p_z - 5;
        do
        {
          *(v22 - 1) = vertBlendWeights[v21];
          *v22 = vertBlendWeights[v21 + 1];
          v22[1] = vertBlendWeights[v21 + 2];
          v22[2] = vertBlendWeights[v21 + 3];
          v21 += 4;
          v22 += 4;
        }
        while ( v21 < numVertBlendTransforms - 3 );
      }
      if ( v21 < numVertBlendTransforms )
      {
        v20 = numVertBlendTransforms;
        qmemcpy(
          &drawVertArray->blendWeight[(_DWORD)surfacea + v21],
          &vertBlendWeights[v21],
          4 * (numVertBlendTransforms - v21));
        v8 = v43;
      }
      if ( v20 < 4 )
        memset(&drawVertArray->blendWeight[(_DWORD)surfacea + v20], 0, 4 * (4 - v20));
      verts = geometry->verts;
      *(p_z - 13) = *(float *)((char *)&verts->xyz.x + v8);
      *(p_z - 12) = *(float *)((char *)&verts->xyz.y + v8);
      *(p_z - 11) = *(float *)((char *)&verts->xyz.z + v8);
      v24 = geometry->verts;
      *(p_z - 2) = *(float *)((char *)&v24->normal.x + v8);
      *(p_z - 1) = *(float *)((char *)&v24->normal.y + v8);
      *p_z = *(float *)((char *)&v24->normal.z + v8);
      v25 = geometry->verts;
      p_z[1] = *(float *)((char *)&v25->tangents[0].x + v8);
      p_z[2] = *(float *)((char *)&v25->tangents[0].y + v8);
      p_z[3] = *(float *)((char *)&v25->tangents[0].z + v8);
      v26 = geometry->verts;
      p_z[4] = *(float *)((char *)&v26->tangents[1].x + v8);
      p_z[5] = *(float *)((char *)&v26->tangents[1].y + v8);
      p_z[6] = *(float *)((char *)&v26->tangents[1].z + v8);
      *((_BYTE *)p_z + 28) = geometry->verts->color[v8];
      surfacea = (modelSurface_s *)((char *)surfacea + 23);
      *((_BYTE *)p_z + 29) = geometry->verts->color[v8 + 1];
      *((_BYTE *)p_z + 30) = geometry->verts->color[v8 + 2];
      *((_BYTE *)p_z + 31) = geometry->verts->color[v8 + 3];
      v27 = geometry->verts;
      p_z[8] = *(float *)((char *)&v27->st.x + v8);
      p_z[9] = *(float *)((char *)&v27->st.y + v8);
      v7 = curVert + 1;
      v8 += 64;
      p_z += 23;
      v28 = ++curVert < md5Mesh->texCoords.num;
      v43 = v8;
      if ( !v28 )
        break;
      v6 = curWeight;
    }
  }
  if ( v7 < geometry->numVerts )
  {
    v29 = v7 << 6;
    v30 = 0;
    v31 = &drawVertArray[v7].normal.z;
    do
    {
      *(v31 - 10) = *(float *)drawVertArray[geometry->mirroredVerts[v30]].blendIndex;
      *(v31 - 9) = *(float *)&drawVertArray[geometry->mirroredVerts[v30]].blendIndex[1];
      *(v31 - 8) = *(float *)&drawVertArray[geometry->mirroredVerts[v30]].blendIndex[2];
      *(v31 - 7) = *(float *)&drawVertArray[geometry->mirroredVerts[v30]].blendIndex[3];
      *(v31 - 6) = drawVertArray[geometry->mirroredVerts[v30]].blendWeight[0];
      *(v31 - 5) = drawVertArray[geometry->mirroredVerts[v30]].blendWeight[1];
      *(v31 - 4) = drawVertArray[geometry->mirroredVerts[v30]].blendWeight[2];
      *(v31 - 3) = drawVertArray[geometry->mirroredVerts[v30]].blendWeight[3];
      v32 = geometry->verts;
      v33 = *(_DWORD *)((char *)&v32->xyz.x + v29);
      v34 = (float *)((char *)&v32->xyz.x + v29);
      *((_DWORD *)v31 - 13) = v33;
      *(v31 - 12) = v34[1];
      *(v31 - 11) = v34[2];
      v35 = (float *)((char *)&geometry->verts->normal.x + v29);
      *(v31 - 2) = *v35;
      *(v31 - 1) = v35[1];
      *v31 = v35[2];
      v36 = (float *)((char *)&geometry->verts->tangents[0].x + v29);
      v31[1] = *v36;
      v31[2] = v36[1];
      v31[3] = v36[2];
      v37 = (float *)((char *)&geometry->verts->tangents[1].x + v29);
      v31[4] = *v37;
      v31[5] = v37[1];
      v31[6] = v37[2];
      *((_BYTE *)v31 + 28) = geometry->verts->color[v29];
      *((_BYTE *)v31 + 29) = geometry->verts->color[v29 + 1];
      *((_BYTE *)v31 + 30) = geometry->verts->color[v29 + 2];
      *((_BYTE *)v31 + 31) = geometry->verts->color[v29 + 3];
      v38 = geometry->verts;
      v31[8] = *(float *)((char *)&v38->st.x + v29);
      v31[9] = *(float *)((char *)&v38->st.y + v29);
      v29 += 64;
      v31 += 23;
      ++v30;
      ++curVert;
    }
    while ( curVert < geometry->numVerts );
  }
  for ( i = 0; i < geometry->numIndexes; i += 3 )
  {
    if ( R_FaceNegativePolarity(geometry, i) )
    {
      drawVertArray[geometry->indexes[i]].blendWeight[0] = -fabs(drawVertArray[geometry->indexes[i]].blendWeight[0]);
      drawVertArray[geometry->indexes[i + 1]].blendWeight[0] = -fabs(drawVertArray[geometry->indexes[i + 1]].blendWeight[0]);
      v40 = geometry->indexes[i + 2];
      drawVertArray[v40].blendWeight[0] = -fabs(drawVertArray[v40].blendWeight[0]);
    }
  }
  return maxBonesPerVertex;
}

// FUNC: public: void __thiscall rvRenderModelMD5R::GenerateCollisionModel(class idCollisionModelManagerLocal &,class idCollisionModelLocal &)
void __thiscall rvRenderModelMD5R::GenerateCollisionModel(
        rvRenderModelMD5R *this,
        idCollisionModelManagerLocal *modelManager,
        idCollisionModelLocal *collisionModel)
{
  int m_numMeshes; // ecx
  int v5; // eax
  const idMaterial **p_m_material; // edx
  int v7; // ebp
  int v8; // edi
  rvMesh *v9; // ecx
  const idMaterial *m_material; // eax
  bool hasCollisionMesh; // [esp+Fh] [ebp-1h]

  m_numMeshes = this->m_numMeshes;
  v5 = 0;
  hasCollisionMesh = 0;
  if ( m_numMeshes > 0 )
  {
    p_m_material = &this->m_meshes->m_material;
    while ( ((*p_m_material)->surfaceFlags & 0x40) == 0 )
    {
      ++v5;
      p_m_material += 24;
      if ( v5 >= m_numMeshes )
        goto LABEL_7;
    }
    hasCollisionMesh = 1;
  }
LABEL_7:
  v7 = 0;
  if ( m_numMeshes > 0 )
  {
    v8 = 0;
    do
    {
      v9 = &this->m_meshes[v8];
      m_material = v9->m_material;
      if ( (m_material->contentFlags & 0xFFCFFFFF) != 0 && (!hasCollisionMesh || (m_material->surfaceFlags & 0x40) != 0) )
        rvMesh::GenerateCollisionPolys(v9, modelManager, collisionModel);
      ++v7;
      ++v8;
    }
    while ( v7 < this->m_numMeshes );
  }
}

// FUNC: public: void __thiscall rvRenderModelMD5R::WriteSansBuffers(class idFile &,char const *)
void __thiscall rvRenderModelMD5R::WriteSansBuffers(rvRenderModelMD5R *this, idFile *outFile, const char *prepend)
{
  if ( this->m_numMeshes )
  {
    rvRenderModelMD5R::WriteMeshes(this, outFile, prepend);
    outFile->WriteFloatString(
      outFile,
      "%sBounds %f %f %f  %f %f %f\n",
      prepend,
      this->bounds.b[0].x,
      this->bounds.b[0].y,
      this->bounds.b[0].z,
      this->bounds.b[1].x,
      this->bounds.b[1].y,
      this->bounds.b[1].z);
    if ( this->GetHasSky(this) )
      outFile->WriteFloatString(outFile, "%sHasSky\n", prepend);
  }
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::WriteJoints(class idFile &,char const *,char const *)
void __thiscall rvRenderModelMD5R::WriteJoints(
        rvRenderModelMD5R *this,
        idFile *outFile,
        const char *prepend,
        const char *prependPlusTab)
{
  int m_numJoints; // eax
  const char *v6; // edi
  idFile *v7; // esi
  int v8; // ecx
  double v9; // st7
  double v10; // st6
  double v11; // st4
  double v12; // st7
  double v13; // st6
  idJointQuat *v14; // eax
  idCQuat *v15; // eax
  bool v16; // cc
  int v17; // [esp+3Ch] [ebp-7Ch]
  int v18; // [esp+40h] [ebp-78h]
  int parentOffset; // [esp+44h] [ebp-74h]
  int curJoint; // [esp+48h] [ebp-70h]
  float v21; // [esp+4Ch] [ebp-6Ch]
  idCQuat v22; // [esp+5Ch] [ebp-5Ch] BYREF
  idJointQuat result; // [esp+68h] [ebp-50h] BYREF
  idJointMat jointMat; // [esp+88h] [ebp-30h] BYREF

  m_numJoints = this->m_numJoints;
  if ( m_numJoints )
  {
    v6 = prepend;
    v7 = outFile;
    outFile->WriteFloatString(outFile, "%sJoint[ %d ]\n", prepend, m_numJoints);
    v7->WriteFloatString(v7, "%s{\n", prepend);
    v8 = 0;
    curJoint = 0;
    if ( this->m_numJoints > 0 )
    {
      v18 = 0;
      v17 = 0;
      while ( 1 )
      {
        parentOffset = *(const idMD5Joint **)((char *)&this->m_joints->parent + v8)
                     ? *(const idMD5Joint **)((char *)&this->m_joints->parent + v8) - this->m_joints
                     : -1;
        qmemcpy(&jointMat, &this->m_skinSpaceToLocalMats[v17], sizeof(jointMat));
        v9 = jointMat.mat[3];
        v10 = jointMat.mat[7];
        v21 = jointMat.mat[6];
        jointMat.mat[3] = -(jointMat.mat[8] * jointMat.mat[11]
                          + jointMat.mat[4] * jointMat.mat[7]
                          + jointMat.mat[3] * jointMat.mat[0]);
        jointMat.mat[7] = -(jointMat.mat[9] * jointMat.mat[11] + jointMat.mat[5] * jointMat.mat[7] + jointMat.mat[1] * v9);
        v11 = jointMat.mat[6];
        jointMat.mat[6] = jointMat.mat[9];
        jointMat.mat[11] = -(jointMat.mat[10] * jointMat.mat[11] + v11 * v10 + jointMat.mat[2] * v9);
        v12 = jointMat.mat[1];
        jointMat.mat[1] = jointMat.mat[4];
        v13 = jointMat.mat[2];
        jointMat.mat[2] = jointMat.mat[8];
        jointMat.mat[4] = v12;
        jointMat.mat[8] = v13;
        jointMat.mat[9] = v21;
        v14 = idJointMat::ToJointQuat(&jointMat, &result);
        v15 = idQuat::ToCQuat(&v14->q, &v22);
        outFile->WriteFloatString(
          outFile,
          "%s\"%s\" %d  %f %f %f  %f %f %f\n",
          prependPlusTab,
          this->m_joints[v18].name.data,
          parentOffset,
          jointMat.mat[3],
          jointMat.mat[7],
          jointMat.mat[11],
          v15->x,
          v15->y,
          v15->z);
        ++v17;
        v16 = ++curJoint < this->m_numJoints;
        ++v18;
        if ( !v16 )
          break;
        v8 = v18 * 36;
      }
      v7 = outFile;
      v6 = prepend;
    }
    v7->WriteFloatString(v7, "%s}\n", v6);
  }
}

// FUNC: public: void __thiscall rvRenderModelMD5R::Shutdown(void)
void __thiscall rvRenderModelMD5R::Shutdown(rvRenderModelMD5R *this)
{
  idMD5Joint *m_joints; // eax
  const idMD5Joint **p_parent; // edi
  rvVertexBuffer *m_allocVertexBuffers; // eax
  float **v5; // edi
  rvIndexBuffer *m_allocIndexBuffers; // eax
  int *p_m_numIndicesWritten; // edi
  rvMesh *m_meshes; // eax
  bool *p_m_drawSetUp; // edi

  idRenderModelStatic::PurgeModel(this);
  m_joints = this->m_joints;
  if ( m_joints )
  {
    p_parent = &m_joints[-1].parent;
    `eh vector destructor iterator'(
      m_joints,
      0x24u,
      (int)m_joints[-1].parent,
      (void (__thiscall *)(void *))idHashTable<rvDeclGuide *>::hashnode_s::~hashnode_s);
    Memory::Free(p_parent);
  }
  if ( this->m_skinSpaceToLocalMats )
    Mem_Free16(this->m_skinSpaceToLocalMats);
  if ( this->m_defaultPose )
    Mem_Free16(this->m_defaultPose);
  m_allocVertexBuffers = this->m_allocVertexBuffers;
  if ( m_allocVertexBuffers )
  {
    v5 = &m_allocVertexBuffers[-1].m_texCoordArrays[6];
    `eh vector destructor iterator'(
      m_allocVertexBuffers,
      0x1D8u,
      (int)m_allocVertexBuffers[-1].m_texCoordArrays[6],
      (void (__thiscall *)(void *))rvVertexBuffer::~rvVertexBuffer);
    Memory::Free(v5);
  }
  m_allocIndexBuffers = this->m_allocIndexBuffers;
  if ( m_allocIndexBuffers )
  {
    p_m_numIndicesWritten = &m_allocIndexBuffers[-1].m_numIndicesWritten;
    `eh vector destructor iterator'(
      m_allocIndexBuffers,
      0x24u,
      m_allocIndexBuffers[-1].m_numIndicesWritten,
      (void (__thiscall *)(void *))rvIndexBuffer::~rvIndexBuffer);
    Memory::Free(p_m_numIndicesWritten);
  }
  if ( this->m_allocSilEdges )
    Memory::Free(this->m_allocSilEdges);
  m_meshes = this->m_meshes;
  if ( m_meshes )
  {
    p_m_drawSetUp = &m_meshes[-1].m_drawSetUp;
    `eh vector destructor iterator'(
      m_meshes,
      0x60u,
      *(_DWORD *)&m_meshes[-1].m_drawSetUp,
      (void (__thiscall *)(void *))rvMesh::~rvMesh);
    Memory::Free(p_m_drawSetUp);
  }
  if ( this->m_lods )
    Memory::Free(this->m_lods);
  rvRenderModelMD5R::ResetValues(this);
}

// FUNC: public: virtual void __thiscall rvRenderModelMD5R::PurgeModel(void)
// attributes: thunk
void __thiscall rvRenderModelMD5R::PurgeModel(rvRenderModelMD5R *this)
{
  rvRenderModelMD5R::Shutdown(this);
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::ParseJoints(class Lexer &)
void __thiscall rvRenderModelMD5R::ParseJoints(rvRenderModelMD5R *this, Lexer *lex)
{
  rvRenderModelMD5R *v2; // esi
  int m_numJoints; // edi
  int *v4; // eax
  idMD5Joint *v5; // ebx
  idMD5Joint *v6; // eax
  idJointMat *v7; // eax
  idJointQuat *v8; // ebx
  int v9; // eax
  void *v10; // esp
  bool v11; // cc
  idVec3 *v12; // ebx
  idMD5Joint *m_joints; // edi
  int len; // esi
  idStr *p_name; // edi
  int *v16; // eax
  int v17; // ecx
  double v18; // st7
  idMat3 *v19; // eax
  _DWORD *v20; // edi
  _DWORD *mat; // ecx
  _DWORD *v22; // eax
  int v23; // ecx
  int v24; // ecx
  int v25; // eax
  char *v26; // ecx
  char *v27; // eax
  char *v28; // eax
  int v29; // eax
  int v30; // ebx
  int v31; // edx
  double v32; // st7
  double v33; // st6
  double v34; // st5
  double v35; // st4
  double v36; // st3
  double v37; // st2
  double v38; // st1
  char *v39; // ecx
  int *v40; // eax
  double v41; // st7
  double v42; // st6
  idQuat *v43; // eax
  char *v44; // esi
  rvRenderModelMD5R *v45; // edx
  idJointMat *m_skinSpaceToLocalMats; // eax
  double v47; // st7
  double v48; // st6
  double v49; // st5
  double v50; // st4
  double v51; // st3
  double v52; // st2
  double v53; // st4
  double v54; // st7
  double v55; // st6
  float *v56; // eax
  int v57; // ecx
  int v58; // edx
  idJointMat *v59; // eax
  double v60; // st7
  idJointMat *v61; // eax
  double v62; // st6
  float v63; // edi
  double v64; // st7
  double v65; // st6
  float v66; // edi
  void *v67; // edi
  int v68; // [esp-14h] [ebp-108h]
  int v69; // [esp-Ch] [ebp-100h]
  idMat3 result; // [esp+8h] [ebp-ECh] BYREF
  idMat3 v71; // [esp+2Ch] [ebp-C8h] BYREF
  char v72[36]; // [esp+50h] [ebp-A4h] BYREF
  idQuat v73; // [esp+74h] [ebp-80h] BYREF
  idToken token; // [esp+84h] [ebp-70h] BYREF
  float v75; // [esp+D4h] [ebp-20h]
  float v76; // [esp+E0h] [ebp-14h]
  float v77; // [esp+ECh] [ebp-8h]
  int v78[8]; // [esp+F0h] [ebp-4h] BYREF
  float v79; // [esp+110h] [ebp+1Ch]
  float v80; // [esp+11Ch] [ebp+28h]
  int curJoint; // [esp+120h] [ebp+2Ch]
  int v82; // [esp+124h] [ebp+30h]
  float v83; // [esp+128h] [ebp+34h]
  float v84; // [esp+12Ch] [ebp+38h]
  idVec3 *v85; // [esp+130h] [ebp+3Ch]
  float v86; // [esp+134h] [ebp+40h]
  float v87; // [esp+138h] [ebp+44h]
  float v88; // [esp+13Ch] [ebp+48h]
  float v89; // [esp+140h] [ebp+4Ch]
  rvRenderModelMD5R *v90; // [esp+144h] [ebp+50h]
  unsigned int v91; // [esp+148h] [ebp+54h]
  unsigned int v92; // [esp+14Ch] [ebp+58h]
  void *ptr; // [esp+150h] [ebp+5Ch]
  int v94; // [esp+154h] [ebp+60h]
  int parentOffset; // [esp+158h] [ebp+64h]
  int v96; // [esp+164h] [ebp+70h]

  token.floatvalue = 0.0;
  v2 = this;
  v90 = this;
  token.len = 0;
  token.alloced = 20;
  token.data = token.baseBuffer;
  token.baseBuffer[0] = 0;
  v96 = 0;
  Lexer::ExpectTokenString(lex, "[");
  v2->m_numJoints = Lexer::ParseInt(lex);
  Lexer::ExpectTokenString(lex, "]");
  Lexer::ExpectTokenString(lex, "{");
  m_numJoints = v2->m_numJoints;
  *(float *)&v4 = COERCE_FLOAT(Memory::Allocate(36 * m_numJoints + 4));
  ptr = v4;
  LOBYTE(v96) = 1;
  if ( *(float *)&v4 == 0.0 )
  {
    v6 = 0;
  }
  else
  {
    v5 = (idMD5Joint *)(v4 + 1);
    *v4 = m_numJoints;
    `eh vector constructor iterator'(
      v4 + 1,
      0x24u,
      m_numJoints,
      (void (__thiscall *)(void *))idMD5Joint::idMD5Joint,
      (void (__thiscall *)(void *))idHashTable<rvDeclGuide *>::hashnode_s::~hashnode_s);
    v6 = v5;
  }
  v2->m_joints = v6;
  v69 = 48 * v2->m_numJoints;
  LOBYTE(v96) = 0;
  v7 = (idJointMat *)Mem_Alloc16(v69, 0xEu);
  v68 = 32 * v2->m_numJoints;
  v2->m_skinSpaceToLocalMats = v7;
  v8 = (idJointQuat *)Mem_Alloc16(v68, 0xEu);
  v9 = 32 * v2->m_numJoints + 18;
  LOBYTE(v9) = v9 & 0xFC;
  v2->m_defaultPose = v8;
  v10 = alloca(v9);
  if ( !v2->m_joints || !v2->m_skinSpaceToLocalMats || !v8 )
    Lexer::Error(lex, "Out of memory");
  v11 = v2->m_numJoints <= 0;
  curJoint = 0;
  if ( !v11 )
  {
    v92 = 0;
    v91 = 0;
    v12 = &result.mat[2];
    v85 = &result.mat[2];
    v82 = -24 - (_DWORD)&result;
    do
    {
      Lexer::ReadToken(lex, &token);
      m_joints = v2->m_joints;
      len = token.len;
      p_name = &m_joints[v91 / 0x24].name;
      v11 = token.len + 1 <= p_name->alloced;
      parentOffset = (int)p_name;
      v94 = token.len;
      if ( !v11 )
        idStr::ReAllocate(p_name, token.len + 1, 0);
      v16 = (int *)parentOffset;
      qmemcpy(p_name->data, token.data, len);
      v17 = v94;
      *(_BYTE *)(v94 + v16[1]) = 0;
      *v16 = v17;
      *(float *)&parentOffset = COERCE_FLOAT(Lexer::ParseInt(lex));
      v12[-1].y = Lexer::ParseFloat(lex, 0);
      v12[-1].z = Lexer::ParseFloat(lex, 0);
      v12->x = Lexer::ParseFloat(lex, 0);
      v12[-2].x = Lexer::ParseFloat(lex, 0);
      v12[-2].y = Lexer::ParseFloat(lex, 0);
      v18 = Lexer::ParseFloat(lex, 0);
      v12[-2].z = v18;
      v12[-1].x = sqrt(fabs(1.0 - (v18 * v18 + v12[-2].x * v12[-2].x + v12[-2].y * v12[-2].y)));
      v19 = idQuat::ToMat3((idQuat *)&v12[-2], &result);
      v20 = &v90->__vftable;
      mat = (_DWORD *)v90->m_skinSpaceToLocalMats[v92 / 0x30].mat;
      *mat = LODWORD(v19->mat[0].x);
      mat[1] = LODWORD(v19->mat[1].x);
      mat[2] = LODWORD(v19->mat[2].x);
      mat[4] = LODWORD(v19->mat[0].y);
      mat[5] = LODWORD(v19->mat[1].y);
      mat[6] = LODWORD(v19->mat[2].y);
      mat[8] = LODWORD(v19->mat[0].z);
      mat[9] = LODWORD(v19->mat[1].z);
      mat[10] = LODWORD(v19->mat[2].z);
      v22 = (_DWORD *)(v92 + v20[29]);
      v22[3] = LODWORD(v12[-1].y);
      v22[7] = LODWORD(v12[-1].z);
      v22[11] = LODWORD(v12->x);
      v23 = parentOffset;
      if ( parentOffset >= 0 )
      {
        if ( parentOffset >= v20[31] - 1 )
        {
          Lexer::Error(lex, "Invalid parent for joint '%s'", *(const char **)(v91 + v20[28] + 4));
          v23 = parentOffset;
        }
        *(_DWORD *)(v91 + v20[28] + 32) = v20[28] + 36 * v23;
        v29 = v20[29];
        v30 = v23;
        v31 = 3;
        v84 = *(float *)(48 * v23 + v29 + 40);
        ptr = *(void **)(48 * v23 + v29 + 24);
        v83 = *(float *)(48 * v23 + v29 + 8);
        v87 = *(float *)(48 * v23 + v29 + 36);
        v89 = *(float *)(48 * v23 + v29 + 20);
        v88 = *(float *)(48 * v23 + v29 + 4);
        v80 = *(float *)(48 * v23 + v29 + 32);
        v86 = *(float *)(48 * v23 + v29 + 16);
        v94 = *(int *)(48 * v23 + v29);
        v32 = *(float *)(v92 + v29 + 40);
        v33 = *(float *)(v92 + v29 + 24);
        v34 = *(float *)(v92 + v29 + 8);
        v35 = *(float *)(v92 + v29 + 36);
        v36 = *(float *)(v92 + v29 + 20);
        v37 = *(float *)(v92 + v29 + 4);
        v38 = *(float *)(v92 + v29 + 32);
        parentOffset = *(int *)(v92 + v29 + 16);
        v77 = *(float *)(v92 + v29);
        v78[0] = parentOffset;
        v39 = v72;
        *(float *)&v78[1] = v38;
        v40 = v78;
        *(float *)&v78[2] = v37;
        *(float *)&v78[3] = v36;
        *(float *)&v78[4] = v35;
        *(float *)&v78[5] = v34;
        *(float *)&v78[6] = v33;
        *(float *)&v78[7] = v32;
        do
        {
          v41 = *(float *)&v94 * *((float *)v40 - 1);
          v39 += 12;
          v42 = v80 * *((float *)v40 + 1);
          v40 += 3;
          --v31;
          *((float *)v39 - 3) = v41 + v42 + v86 * *((float *)v40 - 3);
          *((float *)v39 - 2) = v88 * *((float *)v40 - 4) + v87 * *((float *)v40 - 2) + v89 * *((float *)v40 - 3);
          *((float *)v39 - 1) = v83 * *((float *)v40 - 4)
                              + v84 * *((float *)v40 - 2)
                              + *(float *)&ptr * *((float *)v40 - 3);
        }
        while ( v31 );
        qmemcpy(&v71, v72, sizeof(v71));
        v43 = idMat3::ToQuat(&v71, &v73);
        v44 = (char *)v85 + v82;
        v45 = v90;
        *(idQuat *)((char *)&v90->m_defaultPose->q + (unsigned int)v44) = *v43;
        m_skinSpaceToLocalMats = v45->m_skinSpaceToLocalMats;
        v47 = m_skinSpaceToLocalMats[v30].mat[10];
        v48 = m_skinSpaceToLocalMats[v30].mat[6];
        v49 = m_skinSpaceToLocalMats[v30].mat[2];
        v89 = m_skinSpaceToLocalMats[v30].mat[9];
        v83 = m_skinSpaceToLocalMats[v30].mat[5];
        v84 = m_skinSpaceToLocalMats[v30].mat[1];
        v86 = m_skinSpaceToLocalMats[v30].mat[8];
        v88 = m_skinSpaceToLocalMats[v30].mat[4];
        v87 = m_skinSpaceToLocalMats[v30].mat[0];
        v50 = m_skinSpaceToLocalMats[v30].mat[11];
        v51 = m_skinSpaceToLocalMats[v30].mat[7];
        v75 = m_skinSpaceToLocalMats[v30].mat[3];
        v52 = m_skinSpaceToLocalMats[v92 / 0x30].mat[11];
        ptr = (void *)LODWORD(m_skinSpaceToLocalMats[v92 / 0x30].mat[7]);
        v76 = m_skinSpaceToLocalMats[v92 / 0x30].mat[3];
        *(float *)&parentOffset = v52 - v50;
        *(float *)&v94 = *(float *)&ptr - v51;
        v53 = v76 - v75;
        v79 = v53;
        v54 = v53 * v49 + *(float *)&v94 * v48 + *(float *)&parentOffset * v47;
        v55 = v79 * v84 + *(float *)&v94 * v83 + *(float *)&parentOffset * v89;
        v12 = v85;
        v56 = (float *)((char *)&v45->m_defaultPose->t.x + (unsigned int)v44);
        v20 = &v45->__vftable;
        *v56 = v79 * v87 + *(float *)&v94 * v88 + *(float *)&parentOffset * v86;
        v56[1] = v55;
        v56[2] = v54;
      }
      else
      {
        v24 = v82;
        *(_DWORD *)(v91 + v20[28] + 32) = 0;
        v25 = v20[30];
        v26 = (char *)v12 + v24;
        *(float *)&v26[v25] = v12[-2].x;
        v27 = &v26[v25];
        *((_DWORD *)v27 + 1) = LODWORD(v12[-2].y);
        *((_DWORD *)v27 + 2) = LODWORD(v12[-2].z);
        *((_DWORD *)v27 + 3) = LODWORD(v12[-1].x);
        v28 = &v26[v20[30] + 16];
        *(float *)v28 = v12[-1].y;
        *((_DWORD *)v28 + 1) = LODWORD(v12[-1].z);
        *((_DWORD *)v28 + 2) = LODWORD(v12->x);
      }
      v91 += 36;
      v92 += 48;
      v2 = v90;
      v12 = (idVec3 *)((char *)v12 + 32);
      v11 = ++curJoint < v20[31];
      v85 = v12;
    }
    while ( v11 );
  }
  v57 = 0;
  if ( v2->m_numJoints > 0 )
  {
    v58 = 0;
    do
    {
      v59 = v2->m_skinSpaceToLocalMats;
      v60 = v59[v58].mat[3];
      v61 = &v59[v58];
      v62 = v61->mat[7];
      ptr = (void *)LODWORD(v61->mat[6]);
      v63 = v61->mat[4];
      ++v57;
      ++v58;
      v61->mat[3] = -(v62 * v63 + v60 * v61->mat[0] + v61->mat[11] * v61->mat[8]);
      v61->mat[7] = -(v62 * v61->mat[5] + v60 * v61->mat[1] + v61->mat[11] * v61->mat[9]);
      v61->mat[11] = -(v61->mat[10] * v61->mat[11] + v60 * v61->mat[2] + v62 * v61->mat[6]);
      v64 = v61->mat[1];
      v61->mat[1] = v63;
      v65 = v61->mat[2];
      v61->mat[2] = v61->mat[8];
      v66 = v61->mat[9];
      v61->mat[4] = v64;
      v61->mat[6] = v66;
      v67 = ptr;
      v61->mat[8] = v65;
      LODWORD(v61->mat[9]) = v67;
    }
    while ( v57 < v2->m_numJoints );
  }
  Lexer::ExpectTokenString(lex, "}");
  v96 = -1;
  idStr::FreeData(&token);
}

// FUNC: public: virtual bool __thiscall rvRenderModelMD5R::HasCollisionSurface(struct renderEntity_s const *)const
char __thiscall rvRenderModelMD5R::HasCollisionSurface(rvRenderModelMD5R *this, const renderEntity_s *ent)
{
  int v3; // ebx
  int i; // edi
  const idMaterial *v5; // eax
  int materialFlags; // eax

  v3 = 0;
  if ( this->m_numMeshes <= 0 )
    return 0;
  for ( i = 0; ; ++i )
  {
    v5 = R_RemapShaderBySkin(this->m_meshes[i].m_material, (idDeclSkin *)ent->customSkin, ent->customShader);
    if ( (v5->surfaceFlags & 0x40) != 0 && v5->numStages <= 0 && !v5->entityGui && !v5->gui )
    {
      materialFlags = v5->materialFlags;
      if ( (materialFlags & 8) == 0 && (materialFlags & 4) != 0 )
        break;
    }
    if ( ++v3 >= this->m_numMeshes )
      return 0;
  }
  return 1;
}

// FUNC: public: void __thiscall rvRenderModelMD5R::Write(class idFile &,char const *,bool)
void __thiscall rvRenderModelMD5R::Write(
        rvRenderModelMD5R *this,
        idFile *outFile,
        const char *prepend,
        bool compressed)
{
  const char *v5; // ebx
  int v6; // eax
  void *v7; // esp
  int v8; // eax
  int v10; // ebx
  rvVertexBuffer *m_vertexBuffers; // eax
  int v12; // ecx
  unsigned int m_flags; // eax
  char v14[12]; // [esp+0h] [ebp-6Ch] BYREF
  rvVertexFormat compressedFormat; // [esp+Ch] [ebp-60h] BYREF
  int prependLen; // [esp+C4h] [ebp+58h]
  char *prependPlusTab; // [esp+C8h] [ebp+5Ch]
  int v18; // [esp+D4h] [ebp+68h]
  int curBuffer; // [esp+E0h] [ebp+74h]
  int curBuffera; // [esp+E0h] [ebp+74h]
  int compresseda; // [esp+E8h] [ebp+7Ch]

  if ( !this->IsDefaultModel(this) )
  {
    v5 = prepend;
    prependLen = strlen(prepend);
    v6 = prependLen + 6;
    LOBYTE(v6) = (prependLen + 6) & 0xFC;
    v7 = alloca(v6);
    prependPlusTab = strcpy(v14, prepend);
    v8 = prependLen;
    v14[prependLen + 1] = 0;
    v14[v8] = 9;
    rvRenderModelMD5R::WriteJoints(this, outFile, prepend, v14);
    if ( this->m_numVertexBuffers )
    {
      outFile->WriteFloatString(outFile, "%sVertexBuffer[ %d ]\n", prepend, this->m_numVertexBuffers);
      outFile->WriteFloatString(outFile, "%s{\n", prepend);
      curBuffer = 0;
      if ( this->m_numVertexBuffers > 0 )
      {
        v10 = 0;
        do
        {
          m_vertexBuffers = this->m_vertexBuffers;
          if ( compressed )
          {
            v12 = (int)&m_vertexBuffers[v10];
            m_flags = m_vertexBuffers[v10].m_flags;
            prependLen = v12;
            if ( (m_flags & 0x80) != 0 || (m_flags & 0x200) != 0 )
            {
              rvVertexFormat::rvVertexFormat(&compressedFormat);
              v18 = 0;
              rvRenderModelMD5R::CompressVertexFormat(&compressedFormat, (const rvVertexFormat *)prependLen);
              rvVertexBuffer::SetLoadFormat(&this->m_vertexBuffers[v10], &compressedFormat);
              this->m_vertexBuffers[v10].m_flags |= 0x20u;
              v18 = -1;
              rvVertexFormat::Shutdown(&compressedFormat);
            }
            else
            {
              *(_DWORD *)(v12 + 368) |= 0x10u;
            }
          }
          else
          {
            m_vertexBuffers[v10].m_flags &= ~0x10u;
            this->m_vertexBuffers[v10].m_flags &= ~0x20u;
          }
          rvVertexBuffer::Write(&this->m_vertexBuffers[v10++], outFile, prependPlusTab);
          ++curBuffer;
        }
        while ( curBuffer < this->m_numVertexBuffers );
        v5 = prepend;
      }
      outFile->WriteFloatString(outFile, "%s}\n", v5);
    }
    if ( this->m_numIndexBuffers )
    {
      outFile->WriteFloatString(outFile, "%sIndexBuffer[ %d ]\n", v5, this->m_numIndexBuffers);
      outFile->WriteFloatString(outFile, "%s{\n", v5);
      curBuffera = 0;
      if ( this->m_numIndexBuffers > 0 )
      {
        compresseda = 0;
        do
        {
          rvIndexBuffer::Write(&this->m_indexBuffers[compresseda++], outFile, prependPlusTab);
          ++curBuffera;
        }
        while ( curBuffera < this->m_numIndexBuffers );
      }
      outFile->WriteFloatString(outFile, "%s}\n", v5);
    }
    rvRenderModelMD5R::WriteSilhouetteEdges(this, outFile, v5, prependPlusTab);
    rvRenderModelMD5R::WriteLevelOfDetail(this, outFile, v5, prependPlusTab);
    rvRenderModelMD5R::WriteSansBuffers(this, outFile, v5);
  }
}

// FUNC: public: virtual __thiscall rvRenderModelMD5R::~rvRenderModelMD5R(void)
void __thiscall rvRenderModelMD5R::~rvRenderModelMD5R(rvRenderModelMD5R *this)
{
  this->__vftable = (rvRenderModelMD5R_vtbl *)&rvRenderModelMD5R::`vftable';
  rvRenderModelMD5R::RemoveFromList(this);
  rvRenderModelMD5R::Shutdown(this);
  idRenderModelStatic::~idRenderModelStatic(this);
}

// FUNC: public: bool __thiscall rvRenderModelMD5R::Init(class idRenderModelMD5 &)
char __thiscall rvRenderModelMD5R::Init(rvRenderModelMD5R *this, idRenderModelMD5 *renderModelMD5)
{
  rvRenderModelMD5R *v2; // esi
  bool v3; // zf
  idRenderModelMD5 *v4; // ebx
  rvRenderModelMD5R_vtbl *v5; // edi
  const char *v6; // eax
  int v7; // edi
  idMD5Mesh *v8; // eax
  int v9; // eax
  int v10; // eax
  void *v11; // esp
  idJointMat *v12; // eax
  const idJointQuat *v13; // eax
  idRenderModelMD5_vtbl *v14; // edx
  int v15; // eax
  _DWORD *v16; // edi
  float *v17; // ebx
  idMat3 *v18; // eax
  idRenderModelStatic *v19; // eax
  int v20; // ecx
  idJointMat *v21; // eax
  double v22; // st7
  double v23; // st6
  float v24; // edx
  double v25; // st7
  double v26; // st6
  float v27; // edx
  double v28; // st7
  double v29; // st6
  float v30; // edx
  double v31; // st7
  double v32; // st6
  double v33; // st5
  double v34; // st7
  char *v35; // eax
  idMD5Joint *m_joints; // ecx
  unsigned int v37; // esi
  idStr *v38; // ecx
  bool v39; // cc
  int v40; // edx
  int v41; // ecx
  int *v42; // eax
  idRenderModelStatic *v43; // edx
  int v44; // ecx
  char *v45; // eax
  char *v46; // eax
  int v47; // esi
  char *v48; // edi
  int v49; // eax
  idRenderModelMD5_vtbl *v50; // edx
  idRenderModelStatic *v51; // eax
  int v52; // ecx
  int num; // edx
  modelSurface_s *v54; // edx
  const srfTriangles_s *geometry; // edi
  int id; // eax
  int v57; // eax
  int numSilEdges; // eax
  int v59; // edi
  idCommon_vtbl *v60; // esi
  int v61; // eax
  int m_numVertexBuffers; // ebx
  char *v64; // eax
  idMD5Mesh *v65; // eax
  int v66; // ebx
  idMD5Mesh *v67; // eax
  int *p_size; // edi
  rvIndexBuffer *v69; // eax
  int v70; // edi
  idJointMat *v71; // eax
  silEdge_t *v72; // eax
  idRenderModelMD5 *v73; // edi
  int v74; // ebx
  char *v75; // eax
  idMD5Mesh *v76; // eax
  int v77; // ebx
  modelSurface_s *v78; // ebx
  const srfTriangles_s *v79; // edi
  __int16 v80; // cx
  int numIndexes; // edx
  int *indexes; // eax
  int numVerts; // ecx
  rvBlend4DrawVert *v84; // edi
  const idMaterial *shader; // eax
  idJointMat *v86; // ebx
  int v87; // ebx
  int v88; // edi
  int *v89; // [esp-28h] [ebp-1E8h]
  silEdge_t *silEdges; // [esp-20h] [ebp-1E0h]
  int v91; // [esp-1Ch] [ebp-1DCh]
  __int16 v92; // [esp-10h] [ebp-1D0h]
  __int16 v93; // [esp-Ch] [ebp-1CCh]
  idMD5Mesh *v94; // [esp-4h] [ebp-1C4h]
  idMat3 result; // [esp+Ch] [ebp-1B4h] BYREF
  rvVertexFormat vertexFormat; // [esp+30h] [ebp-190h] BYREF
  renderEntity_s renderEntity; // [esp+E8h] [ebp-D8h] BYREF
  int texDimArray[7]; // [esp+1C8h] [ebp+8h] BYREF
  int v99; // [esp+1E4h] [ebp+24h]
  float v100; // [esp+1F0h] [ebp+30h]
  int numJoints; // [esp+1F4h] [ebp+34h]
  int curSurface; // [esp+1F8h] [ebp+38h] BYREF
  rvBlend4DrawVert *drawVertArray; // [esp+1FCh] [ebp+3Ch]
  int bonesPerVertex; // [esp+200h] [ebp+40h]
  int max1BoneVerts; // [esp+204h] [ebp+44h]
  idRenderModelStatic *staticModel; // [esp+208h] [ebp+48h]
  int numIndices; // [esp+20Ch] [ebp+4Ch]
  int max4BoneVerts; // [esp+210h] [ebp+50h]
  int maxIndices; // [esp+214h] [ebp+54h]
  idMD5Mesh *md5Mesh; // [esp+218h] [ebp+58h]
  int num1BoneDrawVerts; // [esp+21Ch] [ebp+5Ch]
  int fourBoneVBStart; // [esp+220h] [ebp+60h]
  idJointMat *jointMats; // [esp+224h] [ebp+64h]
  int v114; // [esp+230h] [ebp+70h]

  v2 = this;
  num1BoneDrawVerts = (int)this;
  rvVertexFormat::rvVertexFormat(&vertexFormat);
  v3 = v2->m_meshes == 0;
  v114 = 0;
  texDimArray[0] = 2;
  memset(&texDimArray[1], 0, 24);
  if ( !v3 )
    rvRenderModelMD5R::Shutdown(v2);
  v4 = renderModelMD5;
  v5 = v2->__vftable;
  v6 = renderModelMD5->Name(renderModelMD5);
  v5->InitEmpty(v2, v6);
  v2->m_source = MD5R_SOURCE_MD5;
  v2->purged = 0;
  v2->reloadable = 1;
  v7 = v4->NumJoints(v4);
  numJoints = v7;
  v8 = (idMD5Mesh *)Memory::Allocate(36 * v7 + 4);
  md5Mesh = v8;
  LOBYTE(v114) = 1;
  if ( v8 )
  {
    v8->texCoords.num = v7;
    maxIndices = (int)&v8->texCoords.size;
    `eh vector constructor iterator'(
      &v8->texCoords.size,
      0x24u,
      v7,
      (void (__thiscall *)(void *))idMD5Joint::idMD5Joint,
      (void (__thiscall *)(void *))idHashTable<rvDeclGuide *>::hashnode_s::~hashnode_s);
    v9 = maxIndices;
  }
  else
  {
    v9 = 0;
  }
  v2->m_joints = (idMD5Joint *)v9;
  LOBYTE(v114) = 0;
  maxIndices = 48 * v7;
  v2->m_skinSpaceToLocalMats = (idJointMat *)Mem_Alloc16(48 * v7, 0xEu);
  v2->m_defaultPose = (idJointQuat *)Mem_Alloc16(32 * v7, 0xEu);
  v10 = maxIndices + 18;
  LOBYTE(v10) = (maxIndices + 18) & 0xFC;
  v11 = alloca(v10);
  v12 = (idJointMat *)((char *)&result.mat[0].x + 3);
  LOBYTE(v12) = ((unsigned __int8)&result.mat[0].x + 3) & 0xF0;
  v3 = v2->m_joints == 0;
  jointMats = v12;
  if ( v3 || !v2->m_skinSpaceToLocalMats )
    idLib::common->FatalError(idLib::common, "Out of memory");
  v2->m_numJoints = v7;
  fourBoneVBStart = (int)renderModelMD5->GetJoints(renderModelMD5);
  v13 = renderModelMD5->GetDefaultPose(renderModelMD5);
  v14 = renderModelMD5->__vftable;
  maxIndices = (int)v13;
  v15 = (int)v14->GetSkinSpaceToLocalMats(renderModelMD5);
  if ( v7 > 0 )
  {
    numIndices = v15;
    staticModel = (idRenderModelStatic *)(maxIndices + 24);
    drawVertArray = (rvBlend4DrawVert *)(-24 - maxIndices);
    v16 = (_DWORD *)(fourBoneVBStart + 32);
    max1BoneVerts = -32 - fourBoneVBStart;
    v17 = &jointMats->mat[8];
    max4BoneVerts = fourBoneVBStart + 32;
    md5Mesh = (idMD5Mesh *)(-32 - (_DWORD)jointMats);
    maxIndices = numJoints;
    while ( 1 )
    {
      v18 = idQuat::ToMat3((idQuat *)&staticModel[-1].name.baseBuffer[12], &result);
      *(v17 - 8) = v18->mat[0].x;
      *(v17 - 7) = v18->mat[1].x;
      *(v17 - 6) = v18->mat[2].x;
      *(v17 - 4) = v18->mat[0].y;
      *(v17 - 3) = v18->mat[1].y;
      *(v17 - 2) = v18->mat[2].y;
      *v17 = v18->mat[0].z;
      v17[1] = v18->mat[1].z;
      v17[2] = v18->mat[2].z;
      v19 = staticModel;
      *(v17 - 5) = *(float *)&staticModel[-1].reloadable;
      *(v17 - 1) = *(float *)&v19[-1].timeStamp;
      v17[3] = *(float *)&v19->__vftable;
      if ( *v16 )
      {
        v20 = (*v16 - fourBoneVBStart) / 36;
        v21 = &jointMats[v20];
        v22 = v21->mat[1] * *(v17 - 4) + v21->mat[2] * *v17 + *(v17 - 8) * v21->mat[0];
        v23 = *(v17 - 8) * v21->mat[4] + v21->mat[6] * *v17 + *(v17 - 4) * v21->mat[5];
        v100 = v21->mat[10] * *v17 + v21->mat[9] * *(v17 - 4) + *(v17 - 8) * v21->mat[8];
        v24 = v100;
        *(v17 - 8) = v22;
        *v17 = v24;
        *(v17 - 4) = v23;
        v25 = v21->mat[1] * *(v17 - 3) + *(v17 - 7) * v21->mat[0] + v17[1] * v21->mat[2];
        v26 = v17[1] * v21->mat[6] + v21->mat[5] * *(v17 - 3) + v21->mat[4] * *(v17 - 7);
        v100 = v21->mat[9] * *(v17 - 3) + *(v17 - 7) * v21->mat[8] + v21->mat[10] * v17[1];
        v27 = v100;
        *(v17 - 7) = v25;
        v17[1] = v27;
        *(v17 - 3) = v26;
        v28 = *(v17 - 6) * v21->mat[0] + v21->mat[2] * v17[2] + v21->mat[1] * *(v17 - 2);
        v29 = v21->mat[5] * *(v17 - 2) + v21->mat[4] * *(v17 - 6) + v21->mat[6] * v17[2];
        v100 = v21->mat[10] * v17[2] + *(v17 - 6) * v21->mat[8] + v21->mat[9] * *(v17 - 2);
        v30 = v100;
        *(v17 - 6) = v28;
        v17[2] = v30;
        *(v17 - 2) = v29;
        v31 = v17[3] * v21->mat[2] + *(v17 - 1) * v21->mat[1] + *(v17 - 5) * v21->mat[0];
        v32 = *(v17 - 5) * v21->mat[4] + *(v17 - 1) * v21->mat[5] + v17[3] * v21->mat[6];
        v33 = *(v17 - 5) * v21->mat[8] + v21->mat[10] * v17[3] + v21->mat[9] * *(v17 - 1);
        v100 = v33;
        *(v17 - 5) = v31;
        *(v17 - 1) = v32;
        v17[3] = v33;
        *(v17 - 5) = v31 + v21->mat[3];
        *(v17 - 1) = v32 + v21->mat[7];
        v34 = v100 + v21->mat[11];
        v35 = (char *)v16 + max1BoneVerts;
        v17[3] = v34;
        *(const idMD5Joint **)((char *)&v2->m_joints->parent + (unsigned int)v35) = &v2->m_joints[v20];
      }
      else
      {
        v35 = (char *)v16 + max1BoneVerts;
        *(_DWORD *)((char *)v16 + max1BoneVerts + (unsigned int)v2->m_joints + 32) = 0;
      }
      m_joints = v2->m_joints;
      v37 = *(v16 - 8);
      v38 = (idStr *)((char *)&m_joints->name + (_DWORD)v35);
      v39 = (int)(v37 + 1) <= v38->alloced;
      bonesPerVertex = (int)v38;
      v99 = v37;
      if ( !v39 )
        idStr::ReAllocate(v38, v37 + 1, 0);
      v40 = bonesPerVertex;
      qmemcpy(*(void **)(bonesPerVertex + 4), (const void *)*(v16 - 7), v37);
      v41 = v99;
      v42 = (int *)v40;
      *(_BYTE *)(v99 + *(_DWORD *)(v40 + 4)) = 0;
      v43 = staticModel;
      *v42 = v41;
      v44 = num1BoneDrawVerts;
      v45 = (char *)drawVertArray + *(_DWORD *)(num1BoneDrawVerts + 120);
      *(idRenderModelStatic_vtbl **)((char *)&v43->__vftable + (_DWORD)v45) = *(idRenderModelStatic_vtbl **)&v43[-1].name.baseBuffer[12];
      v46 = &v45[(_DWORD)v43];
      *((_DWORD *)v46 + 1) = *(_DWORD *)&v43[-1].name.baseBuffer[16];
      *((_DWORD *)v46 + 2) = v43[-1].shadowHull;
      v47 = *(_DWORD *)&v43[-1].isStaticWorldModel;
      max4BoneVerts += 36;
      *((_DWORD *)v46 + 3) = v47;
      *((_DWORD *)v46 + 4) = *(_DWORD *)&v43[-1].reloadable;
      *((_DWORD *)v46 + 5) = v43[-1].timeStamp;
      *((_DWORD *)v46 + 6) = v43->__vftable;
      *((_DWORD *)v46 + 7) = v43->callback;
      v48 = (char *)v17 + *(_DWORD *)(v44 + 116) + (_DWORD)md5Mesh;
      v49 = numIndices + 48;
      v17 += 12;
      v3 = maxIndices-- == 1;
      qmemcpy(v48, (const void *)numIndices, 0x30u);
      v2 = (rvRenderModelMD5R *)num1BoneDrawVerts;
      staticModel = (idRenderModelStatic *)&v43->bounds.b[0].z;
      numIndices = v49;
      if ( v3 )
        break;
      v16 = (_DWORD *)max4BoneVerts;
    }
    v4 = renderModelMD5;
  }
  memset(&renderEntity, 0, sizeof(renderEntity));
  renderEntity.bounds.b[1].z = -1.0e30;
  renderEntity.bounds.b[1].y = renderEntity.bounds.b[1].z;
  renderEntity.bounds.b[1].x = renderEntity.bounds.b[1].z;
  renderEntity.axis = mat3_identity;
  renderEntity.numJoints = numJoints;
  renderEntity.joints = jointMats;
  renderEntity.bounds.b[0].y = idMath::INFINITY;
  renderEntity.bounds.b[0].z = idMath::INFINITY;
  renderEntity.bounds.b[0].x = idMath::INFINITY;
  idRenderModelMD5::instantiateAllSurfaces = 1;
  v50 = v4->__vftable;
  renderEntity.hModel = v4;
  v51 = (idRenderModelStatic *)v50->InstantiateDynamicModel(v4, &renderEntity, 0, 0, -65u);
  v52 = 0;
  idRenderModelMD5::instantiateAllSurfaces = 0;
  curSurface = 0;
  num = v51->surfaces.num;
  staticModel = v51;
  num1BoneDrawVerts = 0;
  fourBoneVBStart = 0;
  numIndices = 0;
  jointMats = 0;
  if ( num > 0 )
  {
    do
    {
      v54 = &v51->surfaces.list[v52];
      geometry = v54->geometry;
      if ( geometry )
      {
        id = v54->id;
        if ( v54->id >= 1000 )
          id -= 1000;
        v57 = rvRenderModelMD5R::CalcMaxBonesPerVertex(
                v2,
                v4->meshes.list[id].weights,
                v4->meshes.list[id].texCoords.num,
                geometry);
        bonesPerVertex = v57;
        if ( v57 > 1 )
        {
          if ( v57 > 4 )
          {
            rvRenderModelMD5R::Shutdown(v2);
            v60 = idLib::common->__vftable;
            v61 = ((int (__thiscall *)(idRenderModelMD5 *, int))v4->Name)(v4, bonesPerVertex);
            v60->Warning(idLib::common, "Unable to convert MD5 mesh %s - too many bones per vertex: %d", v61);
            ((void (__thiscall *)(idRenderModelStatic *, int))staticModel->~idRenderModelStatic)(staticModel, 1);
            v114 = -1;
            rvVertexFormat::Shutdown(&vertexFormat);
            return 0;
          }
          fourBoneVBStart += geometry->numVerts;
        }
        else
        {
          num1BoneDrawVerts += geometry->numVerts;
        }
        numSilEdges = geometry->numSilEdges;
        numIndices += geometry->numIndexes;
        jointMats = (idJointMat *)((char *)jointMats + numSilEdges);
        v52 = curSurface;
        v51 = staticModel;
      }
      curSurface = ++v52;
    }
    while ( v52 < v51->surfaces.num );
  }
  v39 = v2->m_numJoints <= 25;
  v59 = num1BoneDrawVerts;
  max1BoneVerts = num1BoneDrawVerts;
  max4BoneVerts = fourBoneVBStart;
  maxIndices = numIndices;
  if ( !v39 )
  {
    max1BoneVerts = 2 * num1BoneDrawVerts;
    v59 = 2 * num1BoneDrawVerts;
    jointMats = (idJointMat *)(2 * (_DWORD)jointMats);
    max4BoneVerts = 2 * fourBoneVBStart;
    maxIndices = 2 * numIndices;
  }
  if ( v59 && max4BoneVerts )
    v2->m_numVertexBuffers = 6;
  else
    v2->m_numVertexBuffers = 3;
  m_numVertexBuffers = v2->m_numVertexBuffers;
  v64 = (char *)Memory::Allocate(472 * m_numVertexBuffers + 4);
  drawVertArray = (rvBlend4DrawVert *)v64;
  LOBYTE(v114) = 2;
  if ( v64 )
  {
    *(_DWORD *)v64 = m_numVertexBuffers;
    md5Mesh = (idMD5Mesh *)(v64 + 4);
    `eh vector constructor iterator'(
      v64 + 4,
      0x1D8u,
      m_numVertexBuffers,
      (void (__thiscall *)(void *))rvVertexBuffer::rvVertexBuffer,
      (void (__thiscall *)(void *))rvVertexBuffer::~rvVertexBuffer);
    v65 = md5Mesh;
  }
  else
  {
    v65 = 0;
  }
  LOBYTE(v114) = 0;
  v2->m_allocVertexBuffers = (rvVertexBuffer *)v65;
  v2->m_vertexBuffers = (rvVertexBuffer *)v65;
  if ( !v65 )
    idLib::common->FatalError(idLib::common, "Out of memory");
  v66 = 0;
  fourBoneVBStart = 0;
  if ( v59 )
  {
    rvVertexFormat::Init(&vertexFormat, 5u, 3, 0, 1, 0, 0);
    rvVertexBuffer::Init(v2->m_vertexBuffers, &vertexFormat, v59, 0x41u);
    rvVertexFormat::Init(&vertexFormat, 0x4F5u, 3, 0, 1, texDimArray, 0);
    rvVertexBuffer::Init(v2->m_vertexBuffers + 1, &vertexFormat, v59, 0x83u);
    rvVertexFormat::Init(&vertexFormat, 5u, 3, 0, 1, 0, 0);
    rvVertexBuffer::Init(v2->m_vertexBuffers + 2, &vertexFormat, 2 * v59, 0x102u);
    v66 = 3;
  }
  if ( max4BoneVerts )
  {
    fourBoneVBStart = v66;
    rvVertexFormat::Init(&vertexFormat, 0xDu, 3, 4, 4, 0, 0);
    rvVertexBuffer::Init(&v2->m_vertexBuffers[v66], &vertexFormat, max4BoneVerts, 0x41u);
    rvVertexFormat::Init(&vertexFormat, 0x4BDu, 3, 3, 4, texDimArray, 0);
    rvVertexBuffer::Init(&v2->m_vertexBuffers[v66 + 1], &vertexFormat, max4BoneVerts, 0x83u);
    rvVertexFormat::Init(&vertexFormat, 0xDu, 4, 3, 4, 0, 0);
    rvVertexBuffer::Init(&v2->m_vertexBuffers[v66 + 2], &vertexFormat, 2 * max4BoneVerts, 0x102u);
  }
  v2->m_numIndexBuffers = 2;
  v67 = (idMD5Mesh *)Memory::Allocate(0x4Cu);
  md5Mesh = v67;
  LOBYTE(v114) = 3;
  if ( v67 )
  {
    p_size = &v67->texCoords.size;
    v67->texCoords.num = 2;
    `eh vector constructor iterator'(
      &v67->texCoords.size,
      0x24u,
      2,
      (void (__thiscall *)(void *))rvIndexBuffer::rvIndexBuffer,
      (void (__thiscall *)(void *))rvIndexBuffer::~rvIndexBuffer);
    v69 = (rvIndexBuffer *)p_size;
  }
  else
  {
    v69 = 0;
  }
  LOBYTE(v114) = 0;
  v2->m_allocIndexBuffers = v69;
  v2->m_indexBuffers = v69;
  if ( !v69 )
    idLib::common->FatalError(idLib::common, "Out of memory");
  v70 = maxIndices;
  rvIndexBuffer::Init(v2->m_indexBuffers, maxIndices, 1u);
  rvIndexBuffer::Init(v2->m_indexBuffers + 1, v70, 3u);
  v71 = jointMats;
  v2->m_numSilEdges = (int)jointMats;
  v2->m_numSilEdgesAdded = 0;
  v72 = (silEdge_t *)Memory::Allocate(16 * (_DWORD)v71);
  v2->m_allocSilEdges = v72;
  v2->m_silEdges = v72;
  if ( !v72 )
    idLib::common->FatalError(idLib::common, "Out of memory");
  v73 = renderModelMD5;
  v74 = renderModelMD5->meshes.num;
  v2->m_numMeshes = v74;
  v75 = (char *)Memory::Allocate(96 * v74 + 4);
  drawVertArray = (rvBlend4DrawVert *)v75;
  LOBYTE(v114) = 4;
  if ( v75 )
  {
    *(_DWORD *)v75 = v74;
    md5Mesh = (idMD5Mesh *)(v75 + 4);
    `eh vector constructor iterator'(
      v75 + 4,
      0x60u,
      v74,
      (void (__thiscall *)(void *))rvMesh::rvMesh,
      (void (__thiscall *)(void *))rvMesh::~rvMesh);
    v76 = md5Mesh;
  }
  else
  {
    v76 = 0;
  }
  LOBYTE(v114) = 0;
  v2->m_meshes = (rvMesh *)v76;
  if ( !v76 )
    idLib::common->FatalError(idLib::common, "Out of memory");
  v77 = 0;
  v39 = v2->m_numMeshes <= 0;
  numIndices = 0;
  if ( !v39 )
  {
    jointMats = 0;
    num1BoneDrawVerts = 0;
    do
    {
      md5Mesh = (idMD5Mesh *)((char *)v73->meshes.list + num1BoneDrawVerts);
      if ( idRenderModelStatic::FindSurfaceWithId(staticModel, v77, &curSurface) )
      {
        v78 = &staticModel->surfaces.list[curSurface];
        v79 = v78->geometry;
        drawVertArray = (rvBlend4DrawVert *)Memory::Allocate(92 * v79->numVerts);
        bonesPerVertex = rvRenderModelMD5R::BuildBlend4VertArray(v2, md5Mesh, v78, drawVertArray);
        md5Mesh = (idMD5Mesh *)R_CreateSilRemap(v79);
        v80 = bonesPerVertex <= 1 ? 0 : fourBoneVBStart;
        v93 = v80 + 2;
        v92 = v80 + 1;
        numIndexes = v79->numIndexes;
        v91 = v79->numSilEdges;
        indexes = v79->indexes;
        silEdges = v79->silEdges;
        numVerts = v79->numVerts;
        v84 = drawVertArray;
        v89 = indexes;
        shader = v78->shader;
        v86 = jointMats;
        rvMesh::Init(
          (rvMesh *)((char *)jointMats + (unsigned int)v2->m_meshes),
          v2,
          shader,
          numJoints,
          drawVertArray,
          numVerts,
          v89,
          numIndexes,
          silEdges,
          v91,
          &md5Mesh->texCoords.num,
          bonesPerVertex <= 1 ? 0 : fourBoneVBStart,
          v92,
          v93,
          0,
          1);
        v94 = md5Mesh;
        *(_DWORD *)((char *)&v86->mat[10] + (unsigned int)v2->m_meshes) = numIndices;
        R_StaticFree(v94);
        Memory::Free(v84);
        v77 = numIndices;
        v73 = renderModelMD5;
      }
      num1BoneDrawVerts += 56;
      jointMats += 2;
      v39 = ++v77 < v2->m_numMeshes;
      numIndices = v77;
    }
    while ( v39 );
  }
  rvRenderModelMD5R::BuildLevelsOfDetail(v2);
  v3 = max1BoneVerts == 0;
  v2->bounds.b[0].x = v73->bounds.b[0].x;
  v2->bounds.b[0].y = v73->bounds.b[0].y;
  v2->bounds.b[0].z = v73->bounds.b[0].z;
  v2->bounds.b[1].x = v73->bounds.b[1].x;
  v2->bounds.b[1].y = v73->bounds.b[1].y;
  v2->bounds.b[1].z = v73->bounds.b[1].z;
  if ( !v3 )
  {
    rvVertexBuffer::Resize(v2->m_vertexBuffers, v2->m_vertexBuffers->m_numVerticesWritten);
    rvVertexBuffer::Resize(v2->m_vertexBuffers + 1, v2->m_vertexBuffers[1].m_numVerticesWritten);
    rvVertexBuffer::Resize(v2->m_vertexBuffers + 2, v2->m_vertexBuffers[2].m_numVerticesWritten);
  }
  if ( max4BoneVerts )
  {
    v87 = fourBoneVBStart;
    v88 = fourBoneVBStart;
    rvVertexBuffer::Resize(
      &v2->m_vertexBuffers[fourBoneVBStart],
      v2->m_vertexBuffers[fourBoneVBStart].m_numVerticesWritten);
    rvVertexBuffer::Resize(&v2->m_vertexBuffers[v88 + 1], v2->m_vertexBuffers[v88 + 1].m_numVerticesWritten);
    rvVertexBuffer::Resize(&v2->m_vertexBuffers[v87 + 2], v2->m_vertexBuffers[v88 + 2].m_numVerticesWritten);
  }
  rvIndexBuffer::Resize(v2->m_indexBuffers, v2->m_indexBuffers->m_numIndicesWritten);
  rvIndexBuffer::Resize(v2->m_indexBuffers + 1, v2->m_indexBuffers[1].m_numIndicesWritten);
  v2->SetHasSky(v2, 0);
  ((void (__thiscall *)(idRenderModelStatic *, int))staticModel->~idRenderModelStatic)(staticModel, 1);
  v114 = -1;
  rvVertexFormat::Shutdown(&vertexFormat);
  return 1;
}

// FUNC: public: bool __thiscall rvRenderModelMD5R::Init(class idRenderModelStatic &,class rvVertexBuffer *,int,int,class rvIndexBuffer *,int,int,struct silEdge_t *,int,int,enum rvMD5RSource_t)
char __thiscall rvRenderModelMD5R::Init(
        rvRenderModelMD5R *this,
        idRenderModelStatic *renderModelStatic,
        rvVertexBuffer *vertexBuffers,
        int vertexBufferStart,
        int numVertexBuffers,
        rvIndexBuffer *indexBuffers,
        int indexBufferStart,
        int numIndexBuffers,
        silEdge_t *silEdges,
        int silEdgeStart,
        int maxNumSilEdges,
        int source)
{
  idRenderModelStatic *v13; // edi
  rvRenderModelMD5R_vtbl *v14; // ebx
  const char *v15; // eax
  int num; // edx
  int v17; // ebx
  int v18; // eax
  srfTriangles_s **p_geometry; // ecx
  __int16 v21; // cx
  __int16 v22; // ax
  int *v23; // eax
  rvMesh *v24; // ebp
  rvMesh *v25; // eax
  int v26; // ebx
  modelSurface_s *v27; // ebp
  const srfTriangles_s *geometry; // edi
  int *SilRemap; // edi
  bool v30; // cc
  rvRenderModelMD5R_vtbl *v31; // ebx
  int v32; // eax
  __int16 vertexBuffersa; // [esp+24h] [ebp+8h]
  int silIBOffset; // [esp+28h] [ebp+Ch]
  int curMesh; // [esp+2Ch] [ebp+10h]
  __int16 indexBuffersa; // [esp+30h] [ebp+14h]
  __int16 shadowVBOffset; // [esp+34h] [ebp+18h]
  __int16 drawVBOffset; // [esp+38h] [ebp+1Ch]
  __int16 shadowIBOffset; // [esp+3Ch] [ebp+20h]
  int maxNumSilEdgesa; // [esp+44h] [ebp+28h]
  __int16 curSurface; // [esp+48h] [ebp+2Ch]

  if ( this->m_meshes )
    rvRenderModelMD5R::Shutdown(this);
  v13 = renderModelStatic;
  v14 = this->__vftable;
  v15 = renderModelStatic->Name(renderModelStatic);
  v14->InitEmpty(this, v15);
  this->m_source = source;
  this->purged = 0;
  num = renderModelStatic->surfaces.num;
  v17 = 0;
  v18 = 0;
  if ( num <= 0 )
    goto LABEL_9;
  p_geometry = &renderModelStatic->surfaces.list->geometry;
  do
  {
    if ( *p_geometry )
      ++v17;
    ++v18;
    p_geometry += 4;
  }
  while ( v18 < num );
  if ( !v17 )
  {
LABEL_9:
    if ( source != 5 )
      idRenderModelStatic::MakeDefaultModel(this);
    return 0;
  }
  this->m_vertexBuffers = vertexBuffers;
  v21 = vertexBufferStart;
  this->m_numVertexBuffers = vertexBufferStart + numVertexBuffers;
  this->m_indexBuffers = indexBuffers;
  v22 = indexBufferStart;
  this->m_numIndexBuffers = indexBufferStart + numIndexBuffers;
  this->m_silEdges = silEdges;
  this->m_numSilEdges = maxNumSilEdges;
  this->m_numSilEdgesAdded = silEdgeStart;
  if ( numVertexBuffers == 3 )
  {
    shadowIBOffset = vertexBufferStart;
    v21 = vertexBufferStart + 2;
    shadowVBOffset = vertexBufferStart + 1;
  }
  else
  {
    shadowIBOffset = -1;
    shadowVBOffset = -1;
  }
  indexBuffersa = v21;
  if ( numIndexBuffers == 3 )
  {
    vertexBuffersa = v22;
    curSurface = v22 + 1;
    v22 += 2;
LABEL_20:
    drawVBOffset = v22;
    goto LABEL_21;
  }
  if ( numIndexBuffers != 2 )
  {
    vertexBuffersa = -1;
    curSurface = -1;
    goto LABEL_20;
  }
  vertexBuffersa = v22;
  curSurface = v22 + 1;
  drawVBOffset = -1;
LABEL_21:
  this->m_numMeshes = v17;
  v23 = (int *)Memory::Allocate(96 * v17 + 4);
  if ( v23 )
  {
    v24 = (rvMesh *)(v23 + 1);
    *v23 = v17;
    `eh vector constructor iterator'(
      v23 + 1,
      0x60u,
      v17,
      (void (__thiscall *)(void *))rvMesh::rvMesh,
      (void (__thiscall *)(void *))rvMesh::~rvMesh);
    v25 = v24;
  }
  else
  {
    v25 = 0;
  }
  this->m_meshes = v25;
  if ( !v25 )
    idLib::common->FatalError(idLib::common, "Out of memory");
  silIBOffset = 0;
  maxNumSilEdgesa = 0;
  if ( renderModelStatic->surfaces.num > 0 )
  {
    v26 = 0;
    curMesh = 0;
    do
    {
      v27 = &v13->surfaces.list[curMesh];
      geometry = v27->geometry;
      if ( geometry )
      {
        if ( geometry->verts )
        {
          rvRenderModelMD5R::FixBadTangentSpaces(this, v27->geometry);
          SilRemap = R_CreateSilRemap(geometry);
        }
        else
        {
          SilRemap = 0;
        }
        rvMesh::Init(
          &this->m_meshes[v26],
          (silEdge_t *)this,
          v27,
          SilRemap,
          shadowIBOffset,
          shadowVBOffset,
          indexBuffersa,
          vertexBuffersa,
          curSurface,
          drawVBOffset);
        this->m_meshes[v26].m_meshIdentifier = silIBOffset;
        if ( SilRemap )
          R_StaticFree(SilRemap);
        ++silIBOffset;
        ++v26;
      }
      ++curMesh;
      v30 = ++maxNumSilEdgesa < renderModelStatic->surfaces.num;
      v13 = renderModelStatic;
    }
    while ( v30 );
  }
  rvRenderModelMD5R::BuildLevelsOfDetail(this);
  this->bounds.b[0].x = v13->bounds.b[0].x;
  this->bounds.b[0].y = v13->bounds.b[0].y;
  this->bounds.b[0].z = v13->bounds.b[0].z;
  this->bounds.b[1].x = v13->bounds.b[1].x;
  this->bounds.b[1].y = v13->bounds.b[1].y;
  this->bounds.b[1].z = v13->bounds.b[1].z;
  v31 = this->__vftable;
  v32 = ((int (__thiscall *)(idRenderModelStatic *))v13->GetHasSky)(v13);
  v31->SetHasSky(this, v32);
  return 1;
}

// FUNC: public: virtual void __thiscall rvRenderModelMD5R::InitFromFile(char const *)
void __thiscall rvRenderModelMD5R::InitFromFile(rvRenderModelMD5R *this, char *fileName)
{
  rvRenderModelMD5R_vtbl *v3; // edx

  if ( this->m_meshes )
    rvRenderModelMD5R::Shutdown(this);
  idStr::operator=(&this->name, fileName);
  v3 = this->__vftable;
  this->m_source = MD5R_SOURCE_FILE;
  v3->LoadModel(this);
  this->reloadable = 1;
}

// FUNC: protected: struct modelSurface_s * __thiscall rvRenderModelMD5R::GenerateSurface(class idRenderModelStatic &,class rvMesh &,struct renderEntity_s const &,unsigned int)
modelSurface_s *__thiscall rvRenderModelMD5R::GenerateSurface(
        rvRenderModelMD5R *this,
        idRenderModelStatic *staticModel,
        rvMesh *mesh,
        const renderEntity_s *ent,
        char surfMask)
{
  rvMesh *v5; // ebx
  const idMaterial *v7; // eax
  int materialFlags; // eax
  int v9; // edi
  modelSurface_s *v10; // edi
  int granularity; // eax
  bool v12; // cc
  modelSurface_s *v13; // eax
  int v14; // ecx
  int *v15; // eax
  modelSurface_s *v16; // edx
  int num; // edx
  int size; // ecx
  int v19; // eax
  modelSurface_s *list; // edi
  modelSurface_s *v21; // eax
  int v22; // ecx
  int v23; // eax
  modelSurface_s *v24; // ebx
  float *geometry; // ecx
  srfTriangles_s *v26; // ecx
  double x; // st7
  float *p_x; // ecx
  int surfaceNum; // [esp+4h] [ebp-8h] BYREF
  rvRenderModelMD5R *v30; // [esp+8h] [ebp-4h]

  v30 = this;
  v5 = mesh;
  if ( !(*(int (__thiscall **)(_DWORD, int))(**(_DWORD **)common.ip + 28))(*(_DWORD *)common.ip, 0x2000)
    && ((1 << mesh->m_meshIdentifier) & ent->suppressSurfaceMask) != 0 )
  {
    idRenderModelStatic::DeleteSurfaceWithId(staticModel, mesh->m_meshIdentifier);
    mesh->m_surfaceNum = -1;
    return 0;
  }
  v7 = R_RemapShaderBySkin(mesh->m_material, ent->customSkin, ent->customShader);
  if ( (surfMask & 0x40) != 0 )
  {
    if ( !v7 || (v7->surfaceFlags & 0x40) == 0 )
    {
LABEL_65:
      idRenderModelStatic::DeleteSurfaceWithId(staticModel, mesh->m_meshIdentifier);
      mesh->m_surfaceNum = -1;
      return 0;
    }
  }
  else
  {
    if ( !v7 )
      goto LABEL_65;
    if ( v7->numStages <= 0 && !v7->entityGui && !v7->gui )
    {
      materialFlags = v7->materialFlags;
      if ( (materialFlags & 8) == 0 && (materialFlags & 4) != 0 )
        goto LABEL_65;
    }
  }
  if ( idRenderModelStatic::FindSurfaceWithId(staticModel, mesh->m_meshIdentifier, &surfaceNum) )
  {
    v9 = surfaceNum;
    mesh->m_surfaceNum = surfaceNum;
    v10 = &staticModel->surfaces.list[v9];
  }
  else
  {
    mesh->m_surfaceNum = staticModel->NumSurfaces(staticModel);
    if ( !staticModel->surfaces.list )
    {
      granularity = staticModel->surfaces.granularity;
      if ( granularity > 0 )
      {
        if ( granularity != staticModel->surfaces.size )
        {
          v12 = granularity < staticModel->surfaces.num;
          staticModel->surfaces.size = granularity;
          if ( v12 )
            staticModel->surfaces.num = granularity;
          v13 = (modelSurface_s *)Memory::Allocate(16 * granularity);
          v14 = 0;
          v12 = staticModel->surfaces.num <= 0;
          staticModel->surfaces.list = v13;
          if ( !v12 )
          {
            v15 = 0;
            do
            {
              v16 = (modelSurface_s *)((char *)v15 + (unsigned int)staticModel->surfaces.list);
              v16->id = *v15;
              v16->shader = (const idMaterial *)v15[1];
              v16->geometry = (srfTriangles_s *)v15[2];
              ++v14;
              v16->mOriginalSurfaceName = (idStr *)v15[3];
              v15 += 4;
            }
            while ( v14 < staticModel->surfaces.num );
          }
        }
      }
      else
      {
        staticModel->surfaces.list = 0;
        staticModel->surfaces.num = 0;
        staticModel->surfaces.size = 0;
      }
    }
    num = staticModel->surfaces.num;
    size = staticModel->surfaces.size;
    if ( num == size )
    {
      v19 = size + staticModel->surfaces.granularity;
      if ( v19 > 0 )
      {
        if ( v19 != staticModel->surfaces.size )
        {
          list = staticModel->surfaces.list;
          staticModel->surfaces.size = v19;
          if ( v19 < num )
            staticModel->surfaces.num = v19;
          v21 = (modelSurface_s *)Memory::Allocate(16 * v19);
          v22 = 0;
          v12 = staticModel->surfaces.num <= 0;
          staticModel->surfaces.list = v21;
          if ( !v12 )
          {
            v23 = 0;
            do
            {
              v24 = &staticModel->surfaces.list[v23];
              v24->id = list[v23].id;
              v24->shader = list[v23].shader;
              v24->geometry = list[v23].geometry;
              ++v22;
              v24->mOriginalSurfaceName = list[v23++].mOriginalSurfaceName;
            }
            while ( v22 < staticModel->surfaces.num );
            v5 = mesh;
          }
          if ( list )
            Memory::Free(list);
        }
      }
      else
      {
        if ( staticModel->surfaces.list )
          Memory::Free(staticModel->surfaces.list);
        staticModel->surfaces.list = 0;
        staticModel->surfaces.num = 0;
        staticModel->surfaces.size = 0;
      }
    }
    v10 = &staticModel->surfaces.list[staticModel->surfaces.num++];
    v10->geometry = 0;
    v10->shader = 0;
    v10->id = v5->m_meshIdentifier;
  }
  rvMesh::UpdateSurface(v5, v10, ent, v30->m_skinSpaceToLocalMats);
  geometry = (float *)v10->geometry;
  if ( *geometry < (double)staticModel->bounds.b[0].x )
    staticModel->bounds.b[0].x = *geometry;
  if ( *geometry > (double)staticModel->bounds.b[1].x )
    staticModel->bounds.b[1].x = *geometry;
  if ( geometry[1] < (double)staticModel->bounds.b[0].y )
    staticModel->bounds.b[0].y = geometry[1];
  if ( geometry[1] > (double)staticModel->bounds.b[1].y )
    staticModel->bounds.b[1].y = geometry[1];
  if ( geometry[2] < (double)staticModel->bounds.b[0].z )
    staticModel->bounds.b[0].z = geometry[2];
  if ( geometry[2] > (double)staticModel->bounds.b[1].z )
    staticModel->bounds.b[1].z = geometry[2];
  v26 = v10->geometry;
  x = v26->bounds.b[1].x;
  p_x = &v26->bounds.b[1].x;
  if ( x < staticModel->bounds.b[0].x )
    staticModel->bounds.b[0].x = *p_x;
  if ( *p_x > (double)staticModel->bounds.b[1].x )
    staticModel->bounds.b[1].x = *p_x;
  if ( p_x[1] < (double)staticModel->bounds.b[0].y )
    staticModel->bounds.b[0].y = p_x[1];
  if ( p_x[1] > (double)staticModel->bounds.b[1].y )
    staticModel->bounds.b[1].y = p_x[1];
  if ( p_x[2] < (double)staticModel->bounds.b[0].z )
    staticModel->bounds.b[0].z = p_x[2];
  if ( p_x[2] > (double)staticModel->bounds.b[1].z )
    staticModel->bounds.b[1].z = p_x[2];
  return v10;
}

// FUNC: protected: void __thiscall rvRenderModelMD5R::GenerateSurface(class rvMesh &)
void __thiscall rvRenderModelMD5R::GenerateSurface(rvRenderModelMD5R *this, rvMesh *mesh)
{
  int granularity; // eax
  bool v4; // cc
  modelSurface_s *v5; // eax
  int v6; // ecx
  int *v7; // eax
  modelSurface_s *v8; // edx
  int num; // edx
  int size; // ecx
  int v11; // eax
  modelSurface_s *list; // edi
  modelSurface_s *v13; // eax
  int v14; // ecx
  int v15; // eax
  modelSurface_s *v16; // ebx
  modelSurface_s *v17; // eax

  mesh->m_surfaceNum = this->NumSurfaces(this);
  if ( !this->surfaces.list )
  {
    granularity = this->surfaces.granularity;
    if ( granularity > 0 )
    {
      if ( granularity != this->surfaces.size )
      {
        v4 = granularity < this->surfaces.num;
        this->surfaces.size = granularity;
        if ( v4 )
          this->surfaces.num = granularity;
        v5 = (modelSurface_s *)Memory::Allocate(16 * granularity);
        v6 = 0;
        v4 = this->surfaces.num <= 0;
        this->surfaces.list = v5;
        if ( !v4 )
        {
          v7 = 0;
          do
          {
            v8 = (modelSurface_s *)((char *)v7 + (unsigned int)this->surfaces.list);
            v8->id = *v7;
            v8->shader = (const idMaterial *)v7[1];
            v8->geometry = (srfTriangles_s *)v7[2];
            ++v6;
            v8->mOriginalSurfaceName = (idStr *)v7[3];
            v7 += 4;
          }
          while ( v6 < this->surfaces.num );
        }
      }
    }
    else
    {
      this->surfaces.list = 0;
      this->surfaces.num = 0;
      this->surfaces.size = 0;
    }
  }
  num = this->surfaces.num;
  size = this->surfaces.size;
  if ( num == size )
  {
    v11 = size + this->surfaces.granularity;
    if ( v11 > 0 )
    {
      if ( v11 != this->surfaces.size )
      {
        list = this->surfaces.list;
        this->surfaces.size = v11;
        if ( v11 < num )
          this->surfaces.num = v11;
        v13 = (modelSurface_s *)Memory::Allocate(16 * v11);
        v14 = 0;
        v4 = this->surfaces.num <= 0;
        this->surfaces.list = v13;
        if ( !v4 )
        {
          v15 = 0;
          do
          {
            v16 = &this->surfaces.list[v15];
            v16->id = list[v15].id;
            v16->shader = list[v15].shader;
            v16->geometry = list[v15].geometry;
            ++v14;
            v16->mOriginalSurfaceName = list[v15++].mOriginalSurfaceName;
          }
          while ( v14 < this->surfaces.num );
        }
        if ( list )
          Memory::Free(list);
      }
    }
    else
    {
      if ( this->surfaces.list )
        Memory::Free(this->surfaces.list);
      this->surfaces.list = 0;
      this->surfaces.num = 0;
      this->surfaces.size = 0;
    }
  }
  v17 = &this->surfaces.list[this->surfaces.num++];
  v17->geometry = 0;
  v17->shader = 0;
  v17->id = mesh->m_meshIdentifier;
  rvMesh::UpdateSurface(mesh, v17);
}

// FUNC: public: virtual class idRenderModel * __thiscall rvRenderModelMD5R::InstantiateDynamicModel(struct renderEntity_s const *,struct viewDef_s const *,class idRenderModel *,unsigned int)
idRenderModelStatic *__thiscall rvRenderModelMD5R::InstantiateDynamicModel(
        rvRenderModelMD5R *this,
        const renderEntity_s *ent,
        const viewDef_s *view,
        idRenderModelStatic *cachedModel,
        unsigned int surfMask)
{
  idRenderModelStatic *v7; // ebx
  int v8; // esi
  const char *v9; // eax
  int v10; // esi
  const char *v11; // eax
  int v12; // esi
  const char *v13; // eax
  const viewDef_s *v14; // ecx
  int suppressSurfaceInViewID; // eax
  int m_numLODs; // ebp
  int v17; // esi
  float *p_m_rangeEndSquared; // ecx
  rvMesh *i; // esi
  rvMesh *j; // esi
  float viewa; // [esp+10h] [ebp+8h]

  if ( !this->m_numJoints )
    return 0;
  v7 = cachedModel;
  if ( cachedModel && !r_useCachedDynamicModels.internalVar->integerValue )
  {
    ((void (__thiscall *)(idRenderModelStatic *, int))cachedModel->~idRenderModelStatic)(cachedModel, 1);
    v7 = 0;
  }
  if ( this->purged )
  {
    v8 = *(_DWORD *)common.type;
    v9 = this->Name(this);
    (*(void (**)(netadrtype_t, const char *, ...))(v8 + 140))(common.type, "model %s instantiated while purged", v9);
    this->LoadModel(this);
  }
  if ( !ent->joints )
  {
    v10 = *(_DWORD *)common.type;
    v11 = this->Name(this);
    (*(void (**)(netadrtype_t, const char *, ...))(v10 + 124))(
      common.type,
      "rvRenderModelMD5R::InstantiateDynamicModel: NULL joints on renderEntity for '%s'\n",
      v11);
    if ( v7 )
      ((void (__thiscall *)(idRenderModelStatic *, int))v7->~idRenderModelStatic)(v7, 1);
    return 0;
  }
  if ( ent->numJoints == this->m_numJoints )
  {
    ++tr.pc.c_generateMd5r;
    if ( !v7 )
    {
      v7 = NewRenderModel<idRenderModelStatic>(this);
      v7->InitEmpty(v7, MD5R_SnapshotName);
    }
    v7->bounds.b[0].z = idMath::INFINITY;
    v7->bounds.b[0].y = idMath::INFINITY;
    v7->bounds.b[0].x = idMath::INFINITY;
    v14 = view;
    v7->bounds.b[1].z = -1.0e30;
    v7->bounds.b[1].y = -1.0e30;
    v7->bounds.b[1].x = -1.0e30;
    if ( !r_showSkel.internalVar->integerValue )
      goto LABEL_25;
    if ( view )
    {
      if ( !r_skipSuppress.internalVar->integerValue
        || (suppressSurfaceInViewID = ent->suppressSurfaceInViewID) == 0
        || suppressSurfaceInViewID != view->renderView.viewID )
      {
        rvRenderModelMD5R::DrawJoints(this, ent, view);
        v14 = view;
      }
    }
    if ( r_showSkel.internalVar->integerValue > 1 )
    {
      v7->InitEmpty(v7, MD5R_SnapshotName);
      return v7;
    }
    else
    {
LABEL_25:
      m_numLODs = this->m_numLODs;
      v17 = 0;
      if ( m_numLODs > 0 )
      {
        viewa = idVec3::Dist2(&v14->renderView.vieworg, &ent->origin);
        p_m_rangeEndSquared = &this->m_lods->m_rangeEndSquared;
        do
        {
          if ( viewa < (double)*p_m_rangeEndSquared )
            break;
          ++v17;
          p_m_rangeEndSquared += 3;
        }
        while ( v17 < m_numLODs );
      }
      if ( v17 != v7->levelOfDetail )
      {
        v7->InitEmpty(v7, MD5R_SnapshotName);
        v7->levelOfDetail = v17;
      }
      if ( v17 < this->m_numLODs )
      {
        for ( i = this->m_lods[v17].m_meshList; i; i = i->m_nextInLOD )
          rvRenderModelMD5R::GenerateSurface(this, v7, i, ent, surfMask);
      }
      for ( j = this->m_allLODMeshes; j; j = j->m_nextInLOD )
        rvRenderModelMD5R::GenerateSurface(this, v7, j, ent, surfMask);
      return v7;
    }
  }
  else
  {
    v12 = *(_DWORD *)common.type;
    v13 = this->Name(this);
    (*(void (**)(netadrtype_t, const char *, ...))(v12 + 124))(
      common.type,
      "rvRenderModelMD5R::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n",
      v13);
    if ( !v7 )
      return 0;
    ((void (__thiscall *)(idRenderModelStatic *, int))v7->~idRenderModelStatic)(v7, 1);
    return 0;
  }
}

// FUNC: public: void __thiscall rvRenderModelMD5R::GenerateStaticSurfaces(void)
void __thiscall rvRenderModelMD5R::GenerateStaticSurfaces(rvRenderModelMD5R *this)
{
  int v2; // esi
  const char *v3; // eax
  rvMesh *m_meshes; // esi
  rvMesh *i; // edi

  if ( this->purged )
  {
    v2 = *(_DWORD *)common.type;
    v3 = this->Name(this);
    (*(void (**)(netadrtype_t, const char *, ...))(v2 + 140))(common.type, "model %s instantiated while purged", v3);
    this->LoadModel(this);
  }
  m_meshes = this->m_meshes;
  for ( i = &m_meshes[this->m_numMeshes]; m_meshes < i; ++m_meshes )
    rvRenderModelMD5R::GenerateSurface(this, m_meshes);
}

// FUNC: public: static void __cdecl rvRenderModelMD5R::WriteAll(bool)
void __usercall rvRenderModelMD5R::WriteAll(int a1@<ebp>, int a2@<esi>, char compressed)
{
  rvRenderModelMD5R *i; // edi
  rvMD5RSource_t m_source; // eax
  int len; // esi
  char *data; // ecx
  char *v7; // edx
  char v8; // al
  int v9; // esi
  int v10; // eax
  char v11; // cl
  char *v12; // edx
  int v13; // esi
  int v14; // eax
  char v15; // cl
  char *v16; // edx
  idFile *v17; // eax
  idFile *v18; // esi
  int v19; // esi
  int v20; // eax
  char *v21; // ecx
  _BYTE *v22; // edx
  char v23; // al
  int v24; // esi
  int v25; // eax
  char v26; // cl
  int v27; // edx
  int v28; // eax
  idFile *v29; // esi
  unsigned int v30; // ebp
  idStr *p_md5Name; // ecx
  char *v32; // ecx
  _BYTE *v33; // edx
  char v34; // al
  int v35; // esi
  int v36; // eax
  char v37; // cl
  int v38; // edx
  idFile *v39; // esi
  int v40; // esi
  int v41; // eax
  char v42; // cl
  char *v43; // edx
  int v44; // esi
  int v45; // eax
  char v46; // cl
  char *v47; // edx
  idFile *v48; // esi
  unsigned int md5rTimestamp; // [esp+44h] [ebp-74h]
  bool isFileLoadingAllowed; // [esp+48h] [ebp-70h]
  idStr modelName; // [esp+4Ch] [ebp-6Ch] BYREF
  idStr md5rName; // [esp+6Ch] [ebp-4Ch] BYREF
  idStr md5Name; // [esp+8Ch] [ebp-2Ch] BYREF
  int v56; // [esp+B4h] [ebp-4h]

  isFileLoadingAllowed = idLib::fileSystem->GetIsFileLoadingAllowed(idLib::fileSystem);
  idLib::fileSystem->SetIsFileLoadingAllowed(idLib::fileSystem, 1);
  for ( i = rvRenderModelMD5R::ms_modelList; i; i = i->m_next )
  {
    if ( i->m_meshes )
    {
      if ( !i->purged )
      {
        m_source = i->m_source;
        if ( m_source == MD5R_SOURCE_MD5 || m_source == MD5R_SOURCE_LWO_ASE_FLT )
        {
          len = i->name.len;
          md5rName.data = md5rName.baseBuffer;
          md5rName.len = 0;
          md5rName.alloced = 20;
          md5rName.baseBuffer[0] = 0;
          if ( len + 1 > 20 )
            idStr::ReAllocate(&md5rName, len + 1, 1);
          data = i->name.data;
          v7 = md5rName.data;
          do
          {
            v8 = *data;
            *v7++ = *data++;
          }
          while ( v8 );
          md5rName.len = len;
          v56 = 0;
          idStr::StripAbsoluteFileExtension(&md5rName);
          if ( i->m_source == MD5R_SOURCE_LWO_ASE_FLT )
          {
            v9 = md5rName.len + 7;
            if ( md5rName.len + 8 > md5rName.alloced )
              idStr::ReAllocate(&md5rName, md5rName.len + 8, 1);
            v10 = 0;
            v11 = 95;
            do
            {
              v12 = &md5rName.data[v10++];
              v12[md5rName.len] = v11;
              v11 = aStatic_0[v10];
            }
            while ( v11 );
            md5rName.len = v9;
            md5rName.data[v9] = 0;
          }
          v13 = md5rName.len + 5;
          if ( md5rName.len + 6 > md5rName.alloced )
            idStr::ReAllocate(&md5rName, md5rName.len + 6, 1);
          v14 = 0;
          v15 = 46;
          do
          {
            v16 = &md5rName.data[v14++];
            v16[md5rName.len] = v15;
            v15 = aMd5r_0[v14];
          }
          while ( v15 );
          md5rName.len = v13;
          md5rName.data[v13] = 0;
          v17 = idLib::fileSystem->OpenFileRead(idLib::fileSystem, md5rName.data, 1, 0);
          v18 = v17;
          if ( !v17 )
            goto LABEL_57;
          modelName.len = ((int (__thiscall *)(idFile *, int, int))v17->Timestamp)(v17, a2, a1);
          idLib::fileSystem->CloseFile(idLib::fileSystem, v18);
          v19 = i->name.len;
          v20 = v19 + 1;
          if ( i->m_source == MD5R_SOURCE_MD5 )
          {
            md5Name.alloced = 0;
            *(_DWORD *)&md5Name.baseBuffer[4] = 20;
            *(_DWORD *)md5Name.baseBuffer = &md5Name.baseBuffer[8];
            md5Name.baseBuffer[8] = 0;
            if ( v20 > 20 )
              idStr::ReAllocate((idStr *)&md5Name.alloced, v19 + 1, 1);
            v21 = i->name.data;
            v22 = *(_BYTE **)md5Name.baseBuffer;
            do
            {
              v23 = *v21;
              *v22++ = *v21++;
            }
            while ( v23 );
            md5Name.alloced = v19;
            compressed = 1;
            idStr::StripAbsoluteFileExtension((idStr *)&md5Name.alloced);
            v24 = md5Name.alloced + 8;
            if ( md5Name.alloced + 9 > *(int *)&md5Name.baseBuffer[4] )
              idStr::ReAllocate((idStr *)&md5Name.alloced, md5Name.alloced + 9, 1);
            v25 = 0;
            v26 = 46;
            do
            {
              v27 = v25 + *(_DWORD *)md5Name.baseBuffer;
              ++v25;
              *(_BYTE *)(v27 + md5Name.alloced) = v26;
              v26 = aMd5mesh_0[v25];
            }
            while ( v26 );
            md5Name.alloced = v24;
            *(_BYTE *)(*(_DWORD *)md5Name.baseBuffer + v24) = 0;
            a1 = 0;
            a2 = 1;
            v28 = ((int (__thiscall *)(idFileSystem *, _DWORD))idLib::fileSystem->OpenFileRead)(
                    idLib::fileSystem,
                    *(_DWORD *)md5Name.baseBuffer);
            v29 = (idFile *)v28;
            if ( v28 )
            {
              v30 = (*(int (__thiscall **)(int))(*(_DWORD *)v28 + 24))(v28);
              idLib::fileSystem->CloseFile(idLib::fileSystem, v29);
            }
            else
            {
              v30 = 0;
            }
            p_md5Name = &md5Name;
          }
          else
          {
            modelName.alloced = 0;
            *(_DWORD *)&modelName.baseBuffer[4] = 20;
            *(_DWORD *)modelName.baseBuffer = &modelName.baseBuffer[8];
            modelName.baseBuffer[8] = 0;
            if ( v20 > 20 )
              idStr::ReAllocate((idStr *)&modelName.alloced, v19 + 1, 1);
            v32 = i->name.data;
            v33 = *(_BYTE **)modelName.baseBuffer;
            do
            {
              v34 = *v32;
              *v33++ = *v32++;
            }
            while ( v34 );
            modelName.alloced = v19;
            compressed = 2;
            idStr::StripAbsoluteFileExtension((idStr *)&modelName.alloced);
            v35 = modelName.alloced + 4;
            if ( modelName.alloced + 5 > *(int *)&modelName.baseBuffer[4] )
              idStr::ReAllocate((idStr *)&modelName.alloced, modelName.alloced + 5, 1);
            v36 = 0;
            v37 = 46;
            do
            {
              v38 = v36 + *(_DWORD *)modelName.baseBuffer;
              ++v36;
              *(_BYTE *)(v38 + modelName.alloced) = v37;
              v37 = aLwo_0[v36];
            }
            while ( v37 );
            modelName.alloced = v35;
            *(_BYTE *)(*(_DWORD *)modelName.baseBuffer + v35) = 0;
            a1 = 0;
            a2 = 1;
            v39 = (idFile *)((int (__thiscall *)(idFileSystem *, _DWORD))idLib::fileSystem->OpenFileRead)(
                              idLib::fileSystem,
                              *(_DWORD *)modelName.baseBuffer);
            if ( v39 )
              goto LABEL_53;
            idStr::StripAbsoluteFileExtension(&modelName);
            v40 = modelName.len + 4;
            if ( modelName.len + 5 > modelName.alloced )
              idStr::ReAllocate(&modelName, modelName.len + 5, 1);
            v41 = 0;
            v42 = 46;
            do
            {
              v43 = &modelName.data[v41++];
              v43[modelName.len] = v42;
              v42 = aAse_0[v41];
            }
            while ( v42 );
            modelName.len = v40;
            modelName.data[v40] = 0;
            v39 = idLib::fileSystem->OpenFileRead(idLib::fileSystem, modelName.data, 1, 0);
            if ( v39 )
              goto LABEL_53;
            idStr::StripAbsoluteFileExtension(&modelName);
            v44 = modelName.len + 4;
            if ( modelName.len + 5 > modelName.alloced )
              idStr::ReAllocate(&modelName, modelName.len + 5, 1);
            v45 = 0;
            v46 = 46;
            do
            {
              v47 = &modelName.data[v45++];
              v47[modelName.len] = v46;
              v46 = aFlt_0[v45];
            }
            while ( v46 );
            modelName.len = v44;
            modelName.data[v44] = 0;
            v39 = idLib::fileSystem->OpenFileRead(idLib::fileSystem, modelName.data, 1, 0);
            if ( v39 )
            {
LABEL_53:
              v30 = v39->Timestamp(v39);
              idLib::fileSystem->CloseFile(idLib::fileSystem, v39);
            }
            else
            {
              v30 = 0;
            }
            p_md5Name = &modelName;
          }
          LOBYTE(v56) = 0;
          idStr::FreeData(p_md5Name);
          if ( md5rTimestamp <= v30 )
          {
LABEL_57:
            v48 = idLib::fileSystem->OpenFileWrite(idLib::fileSystem, md5rName.data, "fs_savepath", 0);
            if ( v48 )
            {
              (*(void (**)(netadrtype_t, const char *, ...))(*(_DWORD *)common.type + 124))(
                common.type,
                "writing %s\n",
                md5rName.data);
              v48->WriteFloatString(v48, "MD5RVersion %d\n", 1);
              rvRenderModelMD5R::Write(i, v48, &entityFilter, compressed);
              idLib::fileSystem->CloseFile(idLib::fileSystem, v48);
              idLexer::WriteBinaryFile(md5rName.data);
            }
          }
          v56 = -1;
          idStr::FreeData(&md5rName);
        }
      }
    }
  }
  idLib::fileSystem->SetIsFileLoadingAllowed(idLib::fileSystem, isFileLoadingAllowed);
}

// FUNC: public: bool __thiscall rvRenderModelMD5R::Init(class idRenderModelStatic &,enum rvMD5RSource_t)
char __thiscall rvRenderModelMD5R::Init(
        rvRenderModelMD5R *this,
        idRenderModelStatic *renderModelStatic,
        rvMD5RSource_t source)
{
  int v4; // ebp
  bool v5; // zf
  rvRenderModelMD5R_vtbl *v6; // edi
  const char *v7; // eax
  int num; // ecx
  int v9; // edi
  int v10; // edx
  const idMaterial **p_shader; // eax
  int v12; // edi
  const idMaterial **v13; // edx
  const idMaterial *v14; // eax
  bool v15; // cc
  _DWORD *v17; // eax
  rvVertexBuffer *v18; // eax
  _DWORD *v19; // eax
  rvIndexBuffer *v20; // edi
  rvIndexBuffer *v21; // eax
  rvMesh *v22; // edi
  silEdge_t *v23; // eax
  int *v24; // eax
  modelSurface_s *v25; // ebp
  const srfTriangles_s *geometry; // edi
  int *SilRemap; // edi
  rvRenderModelMD5R_vtbl *v28; // edi
  int v29; // eax
  const idMaterial **v30; // [esp+10h] [ebp-F4h]
  rvVertexBuffer *v31; // [esp+10h] [ebp-F4h]
  int v32; // [esp+10h] [ebp-F4h]
  int numIndices; // [esp+14h] [ebp-F0h]
  int numIndicesa; // [esp+14h] [ebp-F0h]
  int numMeshes; // [esp+18h] [ebp-ECh]
  int numMeshesa; // [esp+18h] [ebp-ECh]
  int curSurface; // [esp+1Ch] [ebp-E8h]
  int curSurfacea; // [esp+1Ch] [ebp-E8h]
  int numDrawSurfs; // [esp+20h] [ebp-E4h]
  int texDimArray[7]; // [esp+24h] [ebp-E0h] BYREF
  rvVertexFormat vertexFormat; // [esp+40h] [ebp-C4h] BYREF
  int v42; // [esp+100h] [ebp-4h]

  rvVertexFormat::rvVertexFormat(&vertexFormat);
  v4 = 0;
  v5 = this->m_meshes == 0;
  v42 = 0;
  texDimArray[0] = 2;
  memset(&texDimArray[1], 0, 24);
  if ( !v5 )
    rvRenderModelMD5R::Shutdown(this);
  v6 = this->__vftable;
  v7 = renderModelStatic->Name(renderModelStatic);
  v6->InitEmpty(this, v7);
  this->m_source = source;
  this->purged = 0;
  this->reloadable = 1;
  num = renderModelStatic->surfaces.num;
  v9 = 0;
  v10 = 0;
  numDrawSurfs = 0;
  if ( num > 0 )
  {
    p_shader = &renderModelStatic->surfaces.list->shader;
    do
    {
      if ( p_shader[1] )
      {
        if ( ((*p_shader)->surfaceFlags & 0x40) == 0 )
          ++v9;
        v4 = 0;
      }
      ++v10;
      p_shader += 4;
    }
    while ( v10 < num );
    numDrawSurfs = v9;
  }
  v12 = 0;
  numIndices = 0;
  numMeshes = 0;
  curSurface = 0;
  if ( num <= 0 )
    goto LABEL_20;
  v13 = &renderModelStatic->surfaces.list->shader;
  v30 = v13;
  do
  {
    v14 = v13[1];
    if ( v14 )
    {
      if ( ((*v13)->surfaceFlags & 0x40) == 0 || numDrawSurfs <= 0 )
      {
        v12 += *(_DWORD *)&v14->desc.baseBuffer[16];
        numIndices += *(_DWORD *)v14->renderBump.baseBuffer;
        v4 += (int)v14->gui;
        ++numMeshes;
      }
      v13 = v30;
    }
    v13 += 4;
    v15 = ++curSurface < num;
    v30 = v13;
  }
  while ( v15 );
  if ( numMeshes )
  {
    this->m_numVertexBuffers = 3;
    v17 = Memory::Allocate(0x58Cu);
    LOBYTE(v42) = 1;
    if ( v17 )
    {
      *v17 = 3;
      v31 = (rvVertexBuffer *)(v17 + 1);
      `eh vector constructor iterator'(
        v17 + 1,
        0x1D8u,
        3,
        (void (__thiscall *)(void *))rvVertexBuffer::rvVertexBuffer,
        (void (__thiscall *)(void *))rvVertexBuffer::~rvVertexBuffer);
      v18 = v31;
    }
    else
    {
      v18 = 0;
    }
    LOBYTE(v42) = 0;
    this->m_allocVertexBuffers = v18;
    this->m_vertexBuffers = v18;
    if ( !v18 )
      idLib::common->FatalError(idLib::common, "Out of memory");
    rvVertexFormat::Init(&vertexFormat, 1u, 4, 0, 1, 0, 0);
    rvVertexBuffer::Init(this->m_vertexBuffers, &vertexFormat, v12, 0x41u);
    rvVertexFormat::Init(&vertexFormat, 0x4F1u, 4, 0, 1, texDimArray, 0);
    rvVertexBuffer::Init(this->m_vertexBuffers + 1, &vertexFormat, v12, 0x83u);
    rvVertexFormat::Init(&vertexFormat, 1u, 4, 0, 1, 0, 0);
    rvVertexBuffer::Init(this->m_vertexBuffers + 2, &vertexFormat, 2 * v12, 0x102u);
    this->m_numIndexBuffers = 2;
    v19 = Memory::Allocate(0x4Cu);
    LOBYTE(v42) = 2;
    if ( v19 )
    {
      v20 = (rvIndexBuffer *)(v19 + 1);
      *v19 = 2;
      `eh vector constructor iterator'(
        v19 + 1,
        0x24u,
        2,
        (void (__thiscall *)(void *))rvIndexBuffer::rvIndexBuffer,
        (void (__thiscall *)(void *))rvIndexBuffer::~rvIndexBuffer);
      v21 = v20;
    }
    else
    {
      v21 = 0;
    }
    LOBYTE(v42) = 0;
    this->m_allocIndexBuffers = v21;
    this->m_indexBuffers = v21;
    if ( !v21 )
      idLib::common->FatalError(idLib::common, "Out of memory");
    rvIndexBuffer::Init(this->m_indexBuffers, numIndices, 1u);
    rvIndexBuffer::Init(this->m_indexBuffers + 1, numIndices, 3u);
    this->m_numSilEdges = v4;
    v22 = 0;
    this->m_numSilEdgesAdded = 0;
    v23 = (silEdge_t *)Memory::Allocate(16 * v4);
    this->m_allocSilEdges = v23;
    this->m_silEdges = v23;
    if ( !v23 )
      idLib::common->FatalError(idLib::common, "Out of memory");
    this->m_numMeshes = numMeshes;
    v24 = (int *)Memory::Allocate(96 * numMeshes + 4);
    LOBYTE(v42) = 3;
    if ( v24 )
    {
      v22 = (rvMesh *)(v24 + 1);
      *v24 = numMeshes;
      `eh vector constructor iterator'(
        v24 + 1,
        0x60u,
        numMeshes,
        (void (__thiscall *)(void *))rvMesh::rvMesh,
        (void (__thiscall *)(void *))rvMesh::~rvMesh);
    }
    LOBYTE(v42) = 0;
    this->m_meshes = v22;
    if ( !v22 )
      idLib::common->FatalError(idLib::common, "Out of memory");
    numIndicesa = 0;
    curSurfacea = 0;
    if ( renderModelStatic->surfaces.num > 0 )
    {
      numMeshesa = 0;
      v32 = 0;
      do
      {
        v25 = &renderModelStatic->surfaces.list[v32];
        geometry = v25->geometry;
        if ( geometry && ((v25->shader->surfaceFlags & 0x40) == 0 || numDrawSurfs <= 0) )
        {
          rvRenderModelMD5R::FixBadTangentSpaces(this, v25->geometry);
          SilRemap = R_CreateSilRemap(geometry);
          rvMesh::Init(&this->m_meshes[numMeshesa], (silEdge_t *)this, v25, SilRemap, 0, 1, 2, 0, 1, -1);
          this->m_meshes[numMeshesa].m_meshIdentifier = numIndicesa;
          R_StaticFree(SilRemap);
          ++numIndicesa;
          ++numMeshesa;
        }
        ++v32;
        ++curSurfacea;
      }
      while ( curSurfacea < renderModelStatic->surfaces.num );
    }
    rvRenderModelMD5R::BuildLevelsOfDetail(this);
    this->bounds = renderModelStatic->bounds;
    rvVertexBuffer::Resize(this->m_vertexBuffers, this->m_vertexBuffers->m_numVerticesWritten);
    rvVertexBuffer::Resize(this->m_vertexBuffers + 1, this->m_vertexBuffers[1].m_numVerticesWritten);
    rvVertexBuffer::Resize(this->m_vertexBuffers + 2, this->m_vertexBuffers[2].m_numVerticesWritten);
    rvIndexBuffer::Resize(this->m_indexBuffers, this->m_indexBuffers->m_numIndicesWritten);
    rvIndexBuffer::Resize(this->m_indexBuffers + 1, this->m_indexBuffers[1].m_numIndicesWritten);
    rvRenderModelMD5R::GenerateStaticSurfaces(this);
    if ( source == MD5R_SOURCE_PROC )
    {
      v28 = this->__vftable;
      v29 = ((int (__thiscall *)(idRenderModelStatic *))renderModelStatic->GetHasSky)(renderModelStatic);
      v28->SetHasSky(this, v29);
    }
    else
    {
      this->SetHasSky(this, 0);
    }
    v42 = -1;
    rvVertexFormat::Shutdown(&vertexFormat);
    return 1;
  }
  else
  {
LABEL_20:
    if ( source != MD5R_SOURCE_PROC )
      idRenderModelStatic::MakeDefaultModel(this);
    v42 = -1;
    rvVertexFormat::Shutdown(&vertexFormat);
    return 0;
  }
}

// FUNC: public: bool __thiscall rvRenderModelMD5R::Init(class Lexer &,class rvVertexBuffer *,int,class rvIndexBuffer *,int,struct silEdge_t *,int,enum rvMD5RSource_t)
bool __thiscall rvRenderModelMD5R::Init(
        rvRenderModelMD5R *this,
        Lexer *lex,
        rvVertexBuffer *vertexBuffers,
        int numVertexBuffers,
        rvIndexBuffer *indexBuffers,
        int numIndexBuffers,
        silEdge_t *silEdges,
        int maxNumSilEdges,
        rvMD5RSource_t source)
{
  bool v10; // zf
  double v11; // st7
  bool rtnValue; // [esp+Bh] [ebp-75h]
  float min; // [esp+Ch] [ebp-74h]
  float min_4; // [esp+10h] [ebp-70h]
  float min_8; // [esp+14h] [ebp-6Ch]
  float max; // [esp+18h] [ebp-68h]
  float max_4; // [esp+1Ch] [ebp-64h]
  idToken token; // [esp+24h] [ebp-5Ch] BYREF
  int v20; // [esp+7Ch] [ebp-4h]

  token.floatvalue = 0.0;
  token.len = 0;
  token.alloced = 20;
  token.data = token.baseBuffer;
  token.baseBuffer[0] = 0;
  v10 = this->m_meshes == 0;
  v20 = 0;
  rtnValue = 1;
  if ( !v10 )
    rvRenderModelMD5R::Shutdown(this);
  this->m_source = source;
  this->m_numVertexBuffers = numVertexBuffers;
  this->m_indexBuffers = indexBuffers;
  this->m_vertexBuffers = vertexBuffers;
  this->m_silEdges = silEdges;
  this->purged = 0;
  this->m_numIndexBuffers = numIndexBuffers;
  this->m_numSilEdges = maxNumSilEdges;
  this->m_numSilEdgesAdded = maxNumSilEdges;
  Lexer::ReadToken(lex, &token);
  if ( idStr::Icmp(token.data, "Mesh") )
  {
    rtnValue = 0;
  }
  else
  {
    rvRenderModelMD5R::ParseMeshes(this, lex);
    rvRenderModelMD5R::BuildLevelsOfDetail(this);
    Lexer::ReadToken(lex, &token);
  }
  if ( !idStr::Icmp(token.data, "Bounds") )
  {
    min = Lexer::ParseFloat(lex, 0);
    min_4 = Lexer::ParseFloat(lex, 0);
    min_8 = Lexer::ParseFloat(lex, 0);
    max = Lexer::ParseFloat(lex, 0);
    max_4 = Lexer::ParseFloat(lex, 0);
    v11 = Lexer::ParseFloat(lex, 0);
    this->bounds.b[0].z = min_8;
    this->bounds.b[1].z = v11;
    this->bounds.b[0].x = min;
    this->bounds.b[0].y = min_4;
    this->bounds.b[1].x = max;
    this->bounds.b[1].y = max_4;
    Lexer::ReadToken(lex, &token);
  }
  if ( rtnValue )
  {
    if ( idStr::Icmp(token.data, "HasSky") )
      Lexer::UnreadToken(lex, &token);
    else
      this->SetHasSky(this, 1);
    if ( !this->m_numJoints )
      rvRenderModelMD5R::GenerateStaticSurfaces(this);
  }
  else
  {
    Lexer::UnreadToken(lex, &token);
  }
  v20 = -1;
  idStr::FreeData(&token);
  return rtnValue;
}

// FUNC: public: virtual void __thiscall rvRenderModelMD5R::LoadModel(void)
void __usercall rvRenderModelMD5R::LoadModel(rvRenderModelMD5R *this@<ecx>, int a2@<edi>)
{
  Lexer *v3; // esi
  rvMD5RSource_t m_source; // eax
  idRenderModelMD5 *v5; // ebp
  bool v6; // zf
  bool v7; // al
  rvRenderModelMD5R *v8; // ecx
  bool v9; // al
  unsigned int v10; // eax
  float *v11; // ecx
  float *p_y; // eax
  char *data; // [esp-10h] [ebp-ACh]
  int v15; // [esp+8h] [ebp-94h]
  float min; // [esp+Ch] [ebp-90h]
  float min_4; // [esp+10h] [ebp-8Ch]
  int min_8; // [esp+14h] [ebp-88h]
  float v19; // [esp+18h] [ebp-84h] BYREF
  int v20[5]; // [esp+1Ch] [ebp-80h] BYREF
  idToken token; // [esp+30h] [ebp-6Ch] BYREF
  idVec3 max; // [esp+80h] [ebp-1Ch]
  idAutoPtr<Lexer> lexer; // [esp+8Ch] [ebp-10h]
  int v24; // [esp+98h] [ebp-4h]

  token.floatvalue = 0.0;
  token.len = 0;
  token.alloced = 20;
  token.data = token.baseBuffer;
  token.baseBuffer[0] = 0;
  v24 = 0;
  v3 = LexerFactory::MakeLexer(144);
  lexer.mPtr = v3;
  m_source = this->m_source;
  LOBYTE(v24) = 1;
  if ( m_source == MD5R_SOURCE_MD5 )
  {
    v5 = NewRenderModel<idRenderModelMD5>(RV_HEAP_ID_LEVEL);
    if ( !v5 )
    {
LABEL_3:
      (*(void (__cdecl **)(netadrtype_t, const char *))(*(_DWORD *)common.type + 152))(
        common.type,
        "rvRenderModelMD5R: load failed, out of memory");
      LOBYTE(v24) = 0;
      v6 = v3 == 0;
      goto LABEL_40;
    }
    ((void (__thiscall *)(idRenderModelMD5 *, char *, int))v5->InitFromFile)(v5, this->name.data, a2);
    v7 = v5->IsDefaultModel(v5);
    v8 = this;
    if ( !v7 )
    {
      rvRenderModelMD5R::Init(this, v5);
      goto LABEL_11;
    }
LABEL_5:
    idRenderModelStatic::MakeDefaultModel(v8);
LABEL_11:
    ((void (__thiscall *)(idRenderModelMD5 *, int))v5->~idRenderModelMD5)(v5, 1);
    LOBYTE(v24) = 0;
    v6 = v3 == 0;
    goto LABEL_40;
  }
  if ( m_source == MD5R_SOURCE_LWO_ASE_FLT )
  {
    v5 = (idRenderModelMD5 *)NewRenderModel<idRenderModelStatic>(RV_HEAP_ID_LEVEL);
    if ( !v5 )
      goto LABEL_3;
    ((void (__thiscall *)(idRenderModelMD5 *, char *, int))v5->InitFromFile)(v5, this->name.data, a2);
    v9 = v5->IsDefaultModel(v5);
    v8 = this;
    if ( !v9 )
    {
      rvRenderModelMD5R::Init(this, v5, MD5R_SOURCE_LWO_ASE_FLT);
      goto LABEL_11;
    }
    goto LABEL_5;
  }
  if ( this->m_meshes )
    rvRenderModelMD5R::Shutdown(this);
  data = this->name.data;
  this->purged = 0;
  if ( Lexer::LoadFile(v3, data, 0) )
  {
    Lexer::ExpectTokenString(v3, "MD5RVersion");
    v10 = Lexer::ParseInt(v3);
    if ( v10 != 1 )
      Lexer::Error(v3, "Invalid version %d.  Should be version %d\n", v10, 1);
    Lexer::ReadToken(v3, &token);
    if ( !idStr::Icmp(token.data, "CommandLine") )
      Lexer::ReadToken(v3, &token);
    if ( !idStr::Icmp(token.data, "Joint") )
    {
      rvRenderModelMD5R::ParseJoints(this, v3);
      Lexer::ReadToken(v3, &token);
    }
    if ( idStr::Icmp(token.data, "VertexBuffer") )
      Lexer::Error(v3, "Expected VertexBuffer keyword");
    rvRenderModelMD5R::ParseVertexBuffers(this, v3);
    Lexer::ReadToken(v3, &token);
    if ( !idStr::Icmp(token.data, "IndexBuffer") )
    {
      rvRenderModelMD5R::ParseIndexBuffers(this, v3);
      Lexer::ReadToken(v3, &token);
    }
    if ( !idStr::Icmp(token.data, "SilhouetteEdge") )
    {
      rvRenderModelMD5R::ParseSilhouetteEdges(this, v3);
      Lexer::ReadToken(v3, &token);
    }
    if ( !idStr::Icmp(token.data, "LevelOfDetail") )
    {
      rvRenderModelMD5R::ParseLevelOfDetail(this, v3);
      Lexer::ReadToken(v3, &token);
    }
    if ( idStr::Icmp(token.data, "Mesh") )
      Lexer::Error(v3, "Expected Mesh keyword");
    rvRenderModelMD5R::ParseMeshes(this, v3);
    rvRenderModelMD5R::BuildLevelsOfDetail(this);
    Lexer::ExpectTokenString(v3, "Bounds");
    min = Lexer::ParseFloat(v3, 0);
    min_4 = Lexer::ParseFloat(v3, 0);
    *(float *)&min_8 = Lexer::ParseFloat(v3, 0);
    max.x = Lexer::ParseFloat(v3, 0);
    max.y = Lexer::ParseFloat(v3, 0);
    *(float *)&v20[4] = Lexer::ParseFloat(v3, 0);
    v19 = min;
    v20[1] = min_8;
    *(float *)v20 = min_4;
    v11 = &v19;
    v20[2] = LODWORD(max.x);
    v20[3] = LODWORD(max.y);
    p_y = &this->bounds.b[0].y;
    v15 = 2;
    do
    {
      *(p_y - 1) = *v11;
      *p_y = *(float *)((char *)p_y + (char *)&v19 - (char *)&this->bounds);
      p_y[1] = *(float *)((char *)p_y + (char *)v20 - (char *)&this->bounds);
      v11 += 3;
      p_y += 3;
      --v15;
    }
    while ( v15 );
    if ( Lexer::ReadToken(v3, &token) && !idStr::Icmp(token.data, "HasSky") )
      this->SetHasSky(this, 1);
    fileSystem->ReadFile(fileSystem, this->name.data, 0, &this->timeStamp);
    if ( !this->m_numJoints )
      rvRenderModelMD5R::GenerateStaticSurfaces(this);
    LOBYTE(v24) = 0;
    v6 = v3 == 0;
  }
  else
  {
    idRenderModelStatic::MakeDefaultModel(this);
    LOBYTE(v24) = 0;
    v6 = v3 == 0;
  }
LABEL_40:
  if ( !v6 )
  {
    Lexer::~Lexer(v3);
    Memory::Free(v3);
  }
  v24 = -1;
  idStr::FreeData(&token);
}
