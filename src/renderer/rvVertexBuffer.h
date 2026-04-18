#pragma once


class rvVertexBuffer {
public:
    rvVertexBuffer();
    ~rvVertexBuffer();

    void Init(const rvVertexFormat& vertexFormat, int numVertices, std::uint32_t flagMask);
    void Init(Lexer& lex);
    void Shutdown();
    void Resize(int numVertices);

    void SetLoadFormat(const rvVertexFormat& loadFormat);
    void SetupForRender(int vertexStartOffset, const rvVertexFormat& formatNeeded);
    void Unlock();
    void Write(idFile& outFile, const char* prepend);

    void CopyData(
        int destVertexOffset,
        int numVertices,
        const unsigned char* srcPtr,
        int srcStride,
        const rvVertexFormat& srcFormat,
        const unsigned int* copyMapping);

    void CopyData(
        unsigned char* destPtr,
        int destStride,
        const rvVertexFormat& destFormat,
        int srcVertexOffset,
        int numVertices,
        const unsigned int* copyMapping);

    void CopyData(int vertexBufferOffset, int numVertices, const idDrawVert* srcVertData);

    void CopyRemappedData(
        int vertexBufferOffset,
        int numVertices,
        const unsigned int* copyMapping,
        const unsigned int* transformMapOldToNew,
        const rvBlend4DrawVert* srcVertData,
        bool absoluteBlendWeights);

    void CopyRemappedShadowVolData(
        int vertexBufferOffset,
        int numVertices,
        const unsigned int* copyMapping,
        const unsigned int* transformMapOldToNew,
        const rvBlend4DrawVert* srcVertData);

    unsigned int CopySilTraceData(
        unsigned int* indexMapping,
        int vertexBufferOffset,
        int numVertices,
        const int* copyMapping,
        const idDrawVert* srcVertData);

    void CopyRemappedShadowVolData(
        int vertexBufferOffset,
        int numVertices,
        const int* copyMapping,
        const idDrawVert* srcVertData);

    void CopyShadowVolData(int vertexBufferOffset, int numVertices, const shadowCache_s* srcShadowVertData);

    bool LockPosition(
        int vertexBufferOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        unsigned char*& posPtr,
        int& stride);

    bool LockBlendIndex(
        int vertexBufferOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        unsigned char*& blendIndexPtr,
        int& stride);

    bool LockBlendWeight(
        int vertexBufferOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        unsigned char*& blendWeightPtr,
        int& stride);

    bool LockNormal(
        int vertexBufferOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        unsigned char*& normalPtr,
        int& stride);

    bool LockTangent(
        int vertexBufferOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        unsigned char*& tangentPtr,
        int& stride);

    bool LockBinormal(
        int vertexBufferOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        unsigned char*& binormalPtr,
        int& stride);

    bool LockDiffuseColor(
        int vertexBufferOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        unsigned char*& diffuseColorPtr,
        int& stride);

    bool LockTextureCoordinate(
        int texCoordOffset,
        int vertexBufferOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        unsigned char*& texCoordPtr,
        int& stride);

    void ResetValues();
    void CreateVertexStorage();
    bool LockInterleaved(
        int vertexOffset,
        int numVerticesToLock,
        std::uint32_t lockFlags,
        void*& startPtr,
        int& stride);
    void TransferSoAToAoS(unsigned char* vertexDest);
    rvSilTraceVertT* GetSilTraceVertexArray(int vertexOffset);

    // Declared here because the uploaded snippet calls it, but the function body
    // was not included in the source dump and is expected to live elsewhere.
    static void ComponentCopy(
        unsigned char* destPtr,
        int destStride,
        Rv_Vertex_Data_Type_t destDataType,
        int destNumValuesPerComponent,
        const unsigned char* srcPtr,
        int srcStride,
        Rv_Vertex_Data_Type_t srcDataType,
        int srcNumValuesPerComponent,
        int numComponents,
        const unsigned int* copyMapping,
        const float* srcTailPtr,
        bool absFlag);

    rvVertexFormat m_format;
    rvVertexFormat m_loadFormat;

    std::uint32_t m_flags = 0;
    std::uint32_t m_lockStatus = 0;
    int m_numVertices = 0;
    std::uint32_t m_vbID = 0;
    int m_lockVertexOffset = 0;
    int m_lockVertexCount = 0;
    unsigned char* m_lockedBase = nullptr;
    int m_numVerticesWritten = 0;

    unsigned char* m_interleavedStorage = nullptr;
    float* m_positionArray = nullptr;
    float* m_swizzledPositionArray = nullptr;
    std::uint32_t* m_blendIndexArray = nullptr;
    float* m_blendWeightArray = nullptr;
    float* m_normalArray = nullptr;
    float* m_tangentArray = nullptr;
    float* m_binormalArray = nullptr;
    std::uint32_t* m_diffuseColorArray = nullptr;
    std::uint32_t* m_specularColorArray = nullptr;
    float* m_pointSizeArray = nullptr;
    float* m_texCoordArrays[7] = {};
};
