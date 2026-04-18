#pragma once

struct localTrace_t;

class rvPrimBatch
{
public:
    static constexpr int MAX_TRANSFORMS_PER_BATCH = 25;

    struct IndexedTriListSpec
    {
        int m_vertexStart = 0;
        int m_vertexCount = 0;
        int m_indexStart = -1;
        int m_primitiveCount = 0;

        void Reset();
    };

    rvPrimBatch();
    ~rvPrimBatch();

    void LoadMatrixPaletteIntoVPParams(const float* skinToModelTransforms);

    int FlipOutsideBackFaces(unsigned char* facing, const unsigned char* cullBits, rvIndexBuffer& indexBuffer);
    int CreateSilShadowVolTris(int* shadowIndices, const unsigned char* facing, silEdge_t* silEdgeBuffer);
    int CreateFrontBackShadowVolTris(int* shadowIndices, const unsigned char* facing, rvIndexBuffer& indexBuffer);

    void FindOverlayTriangles(
        overlayVertex_t* overlayVerts,
        int& numVerts,
        int* overlayIndices,
        int& numIndices,
        const unsigned char* cullBits,
        const idVec2* texCoords,
        int vertexBase,
        rvIndexBuffer& rfSilTraceIB);

    void GetTextureBounds(float bounds[2][2], rvVertexBuffer& drawVertexBuffer);
    void CopyDrawVertices(idDrawVert* destDrawVerts, rvVertexBuffer& drawVertexBuffer);
    void CopyDrawIndices(int* destIndices, rvIndexBuffer& drawIndexBuffer, unsigned int destBase);

    void CopySilTraceVertices(
        rvVertexBuffer& silTraceVertexBuffer,
        rvIndexBuffer& silTraceIndexBuffer,
        rvVertexBuffer& drawVertexBuffer,
        rvIndexBuffer& drawIndexBuffer);

    void CopyShadowVertices(rvVertexBuffer& shadowVertexBuffer, rvVertexBuffer& silTraceVertexBuffer);

    void Write(idFile& outFile, const char* prepend);
    void Shutdown();

    void Draw(rvVertexBuffer& vertexBuffer, rvIndexBuffer& indexBuffer, const rvVertexFormat* vertexComponentsNeeded);
    void Draw(rvVertexBuffer& vertexBuffer, int* indices, int numIndices, const rvVertexFormat* vertexComponentsNeeded);

    void DrawShadowVolume(
        rvVertexBuffer& vertexBuffer,
        int* indices,
        int numIndices,
        const rvVertexFormat* vertexComponentsNeeded);

    void DrawShadowVolume(
        rvVertexBuffer& vertexBuffer,
        rvIndexBuffer& indexBuffer,
        bool drawCaps,
        const rvVertexFormat* vertexComponentsNeeded);

    void TransformVertsMinMax(
        rvSilTraceVertT* destSilTraceVerts,
        idVec3& bndMin,
        idVec3& bndMax,
        rvVertexBuffer& silTraceVB,
        idJointMat* skinSpaceToLocalMats,
        idJointMat* localToModelMats,
        float* skinToModelTransforms);

    void DeriveTriPlanes(idPlane* planes, const rvSilTraceVertT* silTraceVerts, rvIndexBuffer& silTraceIB);

    void LocalTrace(
        localTrace_t& hit,
        int& c_testPlanes,
        int& c_testEdges,
        int& c_intersect,
        const idVec3& start,
        const idVec3& end,
        const unsigned char* cullBits,
        const idPlane* facePlanes,
        const rvSilTraceVertT* silTraceVerts,
        float radius,
        rvIndexBuffer& silTraceIB,
        rvIndexBuffer& drawIB);

    const rvDeclMatType* GetMaterialType(const idMaterial* shader, const localTrace_t& hit, rvVertexBuffer& drawVB);

    void CreateLightTris(
        int* destDrawIndices,
        int& destIndexCount,
        idBounds& bounds,
        int& c_backfaced,
        int& c_distance,
        const unsigned char* facing,
        const unsigned char* cullBits,
        const idPlane* localClipPlanes,
        const rvSilTraceVertT* silTraceVerts,
        bool includeBackFaces,
        rvIndexBuffer& silTraceIB,
        rvIndexBuffer& drawIB);

    void CreateFrontFaceTris(
        int* destDrawIndices,
        int& destIndexCount,
        idBounds& bounds,
        int& c_backfaced,
        const unsigned char* facing,
        const rvSilTraceVertT* silTraceVerts,
        rvIndexBuffer& silTraceIB,
        rvIndexBuffer& drawIB);

    void GetTriangle(
        idDrawVert& a,
        idDrawVert& b,
        idDrawVert& c,
        int triOffset,
        rvVertexBuffer& drawVertexBuffer,
        rvIndexBuffer& drawIndexBuffer,
        const rvSilTraceVertT* silTraceVerts,
        rvIndexBuffer& silTraceIndexBuffer);

    void CopyTriangles(
        idDrawVert* destDrawVerts,
        int* destIndices,
        rvVertexBuffer& drawVertexBuffer,
        rvIndexBuffer& drawIndexBuffer,
        const rvSilTraceVertT* silTraceVerts,
        rvIndexBuffer& silTraceIndexBuffer,
        int destBase);

    void TransformDrawVertices(
        idDrawVert* destDrawVerts,
        rvVertexBuffer& drawVertexBuffer,
        const idMat4& transform,
        int colorShift,
        unsigned char* colorAdd);

    void TubeDeform(
        idDrawVert* destDrawVerts,
        int* destIndices,
        const idVec3& localView,
        rvVertexBuffer& drawVertexBuffer,
        rvIndexBuffer& drawIndexBuffer,
        const rvSilTraceVertT* silTraceVerts,
        rvIndexBuffer& silTraceIndexBuffer);

    void Init(int* transformPalette, int numTransforms);
    void Init(Lexer& lex);

    bool PreciseCullSurface(
        idBounds& ndcBounds,
        const rvSilTraceVertT* silTraceVerts,
        const idVec3& localView,
        const float* modelMatrix,
        rvIndexBuffer& silTraceIB);

    void FindDecalTriangles(
        idRenderModelDecal& decalModel,
        const decalProjectionInfo_s& localInfo,
        const idPlane* facePlanes,
        const rvSilTraceVertT* silTraceVerts,
        rvIndexBuffer& rfSilTraceIB);

    void PlaneForSurface(idPlane& destPlane, const rvSilTraceVertT* silTraceVerts, rvIndexBuffer& silTraceIndexBuffer);

    void GenerateCollisionPolys(
        idCollisionModelManagerLocal& modelManager,
        idCollisionModelLocal& collisionModel,
        const idMaterial& material,
        rvVertexBuffer& drawVertexBuffer,
        rvIndexBuffer& drawIndexBuffer);

public:
    int* m_transformPalette = nullptr;
    int m_numTransforms = 0;
    int m_silEdgeStart = 0;
    int m_silEdgeCount = 0;

    IndexedTriListSpec m_silTraceGeoSpec;
    IndexedTriListSpec m_drawGeoSpec;
    IndexedTriListSpec m_shadowVolGeoSpec;

    int m_numShadowPrimitivesNoCaps = 0;
    int m_shadowCapPlaneBits = 0;
};
