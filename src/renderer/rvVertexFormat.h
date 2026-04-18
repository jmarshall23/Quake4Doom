#pragma once

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------

enum Rv_Vertex_Component_t {
	RV_VERTEX_COMPONENT_POSITION = 0,
	RV_VERTEX_COMPONENT_SWIZZLED_POSITION = 1,
	RV_VERTEX_COMPONENT_BLEND_INDEX = 2,
	RV_VERTEX_COMPONENT_BLEND_WEIGHT = 3,
	RV_VERTEX_COMPONENT_NORMAL = 4,
	RV_VERTEX_COMPONENT_TANGENT = 5,
	RV_VERTEX_COMPONENT_BINORMAL = 6,
	RV_VERTEX_COMPONENT_DIFFUSE_COLOR = 7,
	RV_VERTEX_COMPONENT_SPECULAR_COLOR = 8,
	RV_VERTEX_COMPONENT_POINT_SIZE = 9,
	RV_VERTEX_COMPONENT_TEXCOORD0 = 10,
	RV_VERTEX_COMPONENT_TEXCOORD1 = 11,
	RV_VERTEX_COMPONENT_TEXCOORD2 = 12,
	RV_VERTEX_COMPONENT_TEXCOORD3 = 13,
	RV_VERTEX_COMPONENT_TEXCOORD4 = 14,
	RV_VERTEX_COMPONENT_TEXCOORD5 = 15,
	RV_VERTEX_COMPONENT_TEXCOORD6 = 16,
	RV_VERTEX_COMPONENT_COUNT = 17
};

enum Rv_Vertex_Data_Type_t {
	RV_VERTEX_DATA_TYPE_INVALID = 0,
	RV_VERTEX_DATA_TYPE_FLOAT = 1,
	RV_VERTEX_DATA_TYPE_FLOAT16 = 2,
	RV_VERTEX_DATA_TYPE_INT = 3,
	RV_VERTEX_DATA_TYPE_UINT = 4,
	RV_VERTEX_DATA_TYPE_INTN = 5,
	RV_VERTEX_DATA_TYPE_UINTN = 6,
	RV_VERTEX_DATA_TYPE_COLOR = 7,
	RV_VERTEX_DATA_TYPE_UBYTE = 8,
	RV_VERTEX_DATA_TYPE_BYTE = 9,
	RV_VERTEX_DATA_TYPE_UBYTEN = 10,
	RV_VERTEX_DATA_TYPE_BYTEN = 11,
	RV_VERTEX_DATA_TYPE_SHORT = 12,
	RV_VERTEX_DATA_TYPE_USHORT = 13,
	RV_VERTEX_DATA_TYPE_SHORTN = 14,
	RV_VERTEX_DATA_TYPE_USHORTN = 15,
	RV_VERTEX_DATA_TYPE_UDEC_10_10_10 = 16,
	RV_VERTEX_DATA_TYPE_DEC_10_10_10 = 17,
	RV_VERTEX_DATA_TYPE_UDEC_10_10_10N = 18,
	RV_VERTEX_DATA_TYPE_DEC_10_10_10N = 19,
	RV_VERTEX_DATA_TYPE_UDEC_10_11_11 = 20,
	RV_VERTEX_DATA_TYPE_DEC_10_11_11 = 21,
	RV_VERTEX_DATA_TYPE_UDEC_10_11_11N = 22,
	RV_VERTEX_DATA_TYPE_DEC_10_11_11N = 23,
	RV_VERTEX_DATA_TYPE_UDEC_11_11_10 = 24,
	RV_VERTEX_DATA_TYPE_DEC_11_11_10 = 25,
	RV_VERTEX_DATA_TYPE_UDEC_11_11_10N = 26,
	RV_VERTEX_DATA_TYPE_DEC_11_11_10N = 27
};

// -----------------------------------------------------------------------------
// External descriptor types
// -----------------------------------------------------------------------------

struct vertexStorageDesc_t {
	int m_tokenSubType;
	int m_numComponents;
};

struct vertexFormatDesc_t {
	unsigned int	m_countMask;
	unsigned int	m_countAdd;
	unsigned int	m_compressedCount;
	unsigned int	m_glStorage;
	bool			m_normalized;
};

// -----------------------------------------------------------------------------
// External globals / API hooks
// -----------------------------------------------------------------------------

extern const vertexStorageDesc_t vertexStorageDescArray[];
extern const vertexFormatDesc_t formatDescs[];
extern const char* outputDataTypeStrings[];

// -----------------------------------------------------------------------------
// rvVertexFormat
// -----------------------------------------------------------------------------

class rvVertexFormat {
public:
	rvVertexFormat();
	rvVertexFormat(
		unsigned int vtxFmtFlags,
		int posDim,
		int numWeights,
		int numTransforms,
		const int* texDimArray);

	void Init(
		unsigned int vtxFmtFlags,
		int posDim,
		int numWeights,
		int numTransforms,
		const int* texDimArray,
		Rv_Vertex_Data_Type_t* dataTypes = nullptr);

	void Init(Lexer& lex);
	void Init(const rvVertexFormat& vf);

	void Shutdown();

	void Write(idFile& outFile, const char* name);

	void GetTokenSubTypes(
		Rv_Vertex_Component_t vertexComponent,
		int* tokenSubTypes) const;

	void SetVertexDeclaration(int vertexStartOffset) const;
	void EnableVertexDeclaration() const;

	bool HasSameComponents(const rvVertexFormat& vf) const;
	bool HasSameDataTypes(const rvVertexFormat& vf) const;

	void CalcSize();
	void BuildDataTypes(Rv_Vertex_Data_Type_t* dataTypes);
	void ParseComponentDataType(
		Rv_Vertex_Component_t vertexComponent,
		Rv_Vertex_Data_Type_t defaultDataType,
		Lexer& lex);

public:
	unsigned int				m_flags;
	unsigned int				m_dimensions;
	int							m_size;
	int							m_numValues;
	int							m_byteOffset[18];
	unsigned char				m_dataTypes[18];
	unsigned char				m_tokenSubTypes[72];
	unsigned int				m_glVASMask;
};