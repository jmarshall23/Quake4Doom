#pragma once

#include "../cm/CollisionModel_local.h"

struct decalProjectionInfo_s;
struct localTrace_s;
struct modelSurface_s;
struct overlayVertex_t;
struct renderEntity_s;

struct srfCullInfo_t;
struct srfTriangles_s;

class rvBlend4DrawVert;
class rvIndexBuffer;
class rvPrimBatch;
class rvRenderModelMD5R;
class rvSilTraceVertT;
class rvVertexBuffer;
class rvVertexFormat;

class rvMesh {
public:
    rvMesh();
    ~rvMesh();

    srfCullInfo_t* FlipOutsideBackFaces(srfCullInfo_t& cullInfo);
    void                        CalcGeometryProfile();

    void                        Draw(const float* skinToModelTransforms, const rvVertexFormat* vertexComponentsNeeded);
    void                        Draw(const float* skinToModelTransforms, int* indices, int numIndices, const rvVertexFormat* vertexComponentsNeeded);
    void                        DrawShadowVolume(const float* skinToModelTransforms, int* indices, bool drawCaps, const rvVertexFormat* vertexComponentsNeeded);

    void                        DeriveTriPlanes(idPlane* planes, const rvSilTraceVertT* silTraceVerts);
    bool                        PreciseCullSurface(idBounds& ndcBounds, const rvSilTraceVertT* silTraceVerts, const idVec3& localView, const float* modelMatrix);
    int                         CreateSilShadowVolTris(int primBatch, int* shadowIndices, const unsigned char* facing);
    int                         CreateFrontBackShadowVolTris(int primBatch, int* shadowIndices, const unsigned char* facing);

    const rvVertexBuffer* GetDrawVertexBufferFormat() const;
    const rvVertexBuffer* GetShadowVolVertexBufferFormat() const;

    void                        LocalTrace(
        localTrace_s& hit,
        int& c_testPlanes,
        int& c_testEdges,
        int& c_intersect,
        const idVec3& start,
        const idVec3& end,
        const unsigned char* cullBits,
        const idPlane* facePlanes,
        const rvSilTraceVertT* silTraceVerts,
        float radius,
        const idMaterial* shader);

    void                        CreateOverlayTriangles(
        overlayVertex_t* overlayVerts,
        int& numVerts,
        int* overlayIndices,
        int& numIndices,
        const idPlane* planes,
        const rvSilTraceVertT* silTraceVerts);

    void                        CreateDecalTriangles(
        class idRenderModelDecal& decalModel,
        const decalProjectionInfo_s& localInfo,
        const idPlane* facePlanes,
        const rvSilTraceVertT* silTraceVerts);

    void                        GenerateCollisionPolys(
        idCollisionModelManagerLocal& modelManager,
        idCollisionModelLocal& collisionModel);

    void                        CopyTriangles(idDrawVert* destDrawVerts, int* destIndices, const rvSilTraceVertT* silTraceVerts);
    void                        CopyTriangles(idDrawVert* destDrawVerts, int* destIndices, int destBase);
    void                        TransformTriangles(idDrawVert* destDrawVerts, int* destIndices, const idMat4& transform, int colorShift, unsigned char* colorAdd, int destBase);
    void                        TubeDeform(idDrawVert* destDrawVerts, int* destIndices, const idVec3& localView, const rvSilTraceVertT* silTraceVerts);
    void                        PlaneForSurface(idPlane& destPlane, const rvSilTraceVertT* silTraceVerts);
    void                        SetupForDrawRender(const rvVertexFormat* vertexComponentsNeeded);
    void                        SetupForShadowVolRender(const rvVertexFormat* vertexComponentsNeeded);

    void                        Init(
        rvRenderModelMD5R& renderModel,
        modelSurface_s& srcSurface,
        int* silRemap,
        short silTraceVB,
        short drawVB,
        short shadowVB,
        short silTraceIB,
        short drawIB,
        short shadowIB);

    void                        Init(rvRenderModelMD5R& renderModel, Lexer& lex);

    void                        Init(
        rvRenderModelMD5R& renderModel,
        const idMaterial* material,
        int numTransforms,
        rvBlend4DrawVert* drawVertArray,
        int numDrawVerts,
        int* drawIndices,
        int numIndices,
        silEdge_t* silEdges,
        int numSilEdges,
        int* silRemap,
        short silTraceVB,
        short drawVB,
        short shadowVB,
        short silTraceIB,
        short drawIB);

    void                        UpdateSurface(modelSurface_s& surface, const renderEntity_s& entity, idJointMat* skinSpaceToLocalMats);
    void                        UpdateSurface(modelSurface_s& surface);

    void                        CreateLightTris(
        srfTriangles_s& newTri,
        int& c_backfaced,
        int& c_distance,
        srfCullInfo_t& cullInfo,
        const rvSilTraceVertT* silTraceVerts,
        bool includeBackFaces);

    void                        CreateFrontFaceTris(
        srfTriangles_s& newTri,
        int& c_backfaced,
        srfCullInfo_t& cullInfo,
        const rvSilTraceVertT* silTraceVerts);

    void                        Write(idFile& outFile, const char* prepend);
    void                        SurfaceToTextureAxis(idVec3& origin, idVec3* axis, const rvSilTraceVertT* silTraceVerts);

    void                        ResetValues();

    static rvPrimBatch* AllocPrimBatches(int count);
    static void                 FreePrimBatches(rvPrimBatch*& primBatches);


    idBounds                    m_bounds;
    rvRenderModelMD5R* m_renderModel;
    const idMaterial* m_material;
    rvMesh* m_nextInLOD;
    rvPrimBatch* m_primBatches;
    int                         m_numPrimBatches;
    short                       m_levelOfDetail;
    short                       m_surfaceNum;
    int                         m_meshIdentifier;
    short                       m_silTraceVertexBuffer;
    short                       m_silTraceIndexBuffer;
    short                       m_drawVertexBuffer;
    short                       m_drawIndexBuffer;
    short                       m_shadowVolVertexBuffer;
    short                       m_shadowVolIndexBuffer;
    int                         m_numSilTraceVertices;
    int                         m_numSilTraceIndices;
    int                         m_numSilTracePrimitives;
    int                         m_numSilEdges;
    int                         m_numDrawVertices;
    int                         m_numDrawIndices;
    int                         m_numDrawPrimitives;
    int                         m_numTransforms;
    bool                        m_drawSetUp;
};
