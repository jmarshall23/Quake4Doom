#pragma once

#include "../cm/CollisionModel_local.h"

class rvMesh;

enum rvMD5RSource_t {
    MD5R_SOURCE_NONE = 0,
    MD5R_SOURCE_MD5,
    MD5R_SOURCE_FILE,
    MD5R_SOURCE_LWO_ASE_FLT,
    MD5R_SOURCE_PROC,
    MD5R_SOURCE_UNKNOWN
};

struct levelOfDetailMD5R_s {
    float   m_rangeEnd;
    float   m_rangeEndSquared;
    rvMesh* m_meshList;
};

class rvRenderModelMD5R : public idRenderModelStatic {
public:
    rvRenderModelMD5R();
    ~rvRenderModelMD5R();

    virtual void                PurgeModel();
    virtual void                InitFromFile(const char* fileName);
    virtual idRenderModel* InstantiateDynamicModel(const renderEntity_s* ent, const viewDef_s* view, idRenderModel* cachedModel, unsigned int surfMask);
    virtual void                LoadModel();

    srfTriangles_s* GenerateStaticTriSurface(int meshOffset);
    int                         NumJoints() const;
    const idMD5Joint* GetJoints() const;
    const idJointQuat* GetDefaultPose() const;

    bool                        Init(idRenderModelMD5& renderModelMD5);
    bool                        Init(idRenderModelStatic& renderModelStatic, rvMD5RSource_t source);
    bool                        Init(idRenderModelStatic& renderModelStatic,
        rvVertexBuffer* vertexBuffers,
        int vertexBufferStart,
        int numVertexBuffers,
        rvIndexBuffer* indexBuffers,
        int indexBufferStart,
        int numIndexBuffers,
        silEdge_t* silEdges,
        int silEdgeStart,
        int maxNumSilEdges,
        rvMD5RSource_t source);
    bool                        Init(Lexer& lex,
        rvVertexBuffer* vertexBuffers,
        int numVertexBuffers,
        rvIndexBuffer* indexBuffers,
        int numIndexBuffers,
        silEdge_t* silEdges,
        int maxNumSilEdges,
        rvMD5RSource_t source);

    void                        Shutdown();
    void                        GenerateCollisionModel(idCollisionModelManagerLocal& modelManager, idCollisionModelLocal& collisionModel);
    void                        WriteSansBuffers(idFile& outFile, const char* prepend);
    void                        Write(idFile& outFile, const char* prepend, bool compressed);
    void                        GenerateStaticSurfaces();

    static void                 WriteAll(bool compressed);
    static void                 RemoveFromList(rvRenderModelMD5R* model);
    static void                 CompressVertexFormat(rvVertexFormat& compressedFormat, const rvVertexFormat& format);

    void                        ResetValues();

    void                        ParseVertexBuffers(Lexer& lex);
    void                        ParseIndexBuffers(Lexer& lex);
    void                        ParseSilhouetteEdges(Lexer& lex);
    void                        ParseLevelOfDetail(Lexer& lex);
    void                        ParseMeshes(Lexer& lex);
    void                        ParseJoints(Lexer& lex);

    void                        BuildLevelsOfDetail();
    int                         CalcMaxBonesPerVertex(const jointWeight_t* jointWeights, int numVerts, const srfTriangles_s& tri);
    void                        FixBadTangentSpaces(srfTriangles_s& tri);
    int                         BuildBlend4VertArray(class idMD5Mesh& md5Mesh, modelSurface_s& surface, rvBlend4DrawVert* drawVertArray);

    void                        WriteSilhouetteEdges(idFile& outFile, const char* prepend, const char* prependPlusTab);
    void                        WriteLevelOfDetail(idFile& outFile, const char* prepend, const char* prependPlusTab);
    void                        WriteMeshes(idFile& outFile, const char* prepend);
    void                        WriteJoints(idFile& outFile, const char* prepend, const char* prependPlusTab);

    modelSurface_s* GenerateSurface(idRenderModelStatic& staticModel, rvMesh& mesh, const renderEntity_s& ent, unsigned int surfMask);
    void                        GenerateSurface(rvMesh& mesh);
    rvMD5RSource_t              m_source;

    idMD5Joint* m_joints;
    idJointMat* m_skinSpaceToLocalMats;
    idJointQuat* m_defaultPose;
    int                         m_numJoints;

    rvVertexBuffer* m_vertexBuffers;
    rvVertexBuffer* m_allocVertexBuffers;
    int                         m_numVertexBuffers;

    rvIndexBuffer* m_indexBuffers;
    rvIndexBuffer* m_allocIndexBuffers;
    int                         m_numIndexBuffers;

    silEdge_t* m_silEdges;
    silEdge_t* m_allocSilEdges;
    int                         m_numSilEdges;
    int                         m_numSilEdgesAdded;

    rvMesh* m_meshes;
    int                         m_numMeshes;

    int                         m_numLODs;
    levelOfDetailMD5R_s* m_lods;
    rvMesh* m_allLODMeshes;

    rvRenderModelMD5R* m_next;

    static rvRenderModelMD5R* ms_modelList;
};