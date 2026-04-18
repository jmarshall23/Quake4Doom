#ifndef __RV_INDEXBUFFER_H__
#define __RV_INDEXBUFFER_H__

class rvIndexBuffer {
public:
	rvIndexBuffer();
	~rvIndexBuffer();

	void				Init(int numIndices, unsigned int flagMask);
	void				Init(Lexer& lex);
	void				Shutdown();
	void				Resize(int numIndices);

	bool				Lock(int indexOffset, int numIndicesToLock, unsigned int lockFlags, void** indexPtr);
	void				Unlock();

	void				CopyData(unsigned int indexBufferOffset, int numIndices, const int* indices, unsigned int indexBase);
	void				CopyRemappedData(unsigned int indexBufferOffset, int numIndices, const unsigned int* indexMapping, const int* indices, unsigned int indexBase);

	void				Write(idFile& outFile, const char* prepend);

	void				CreateIndexStorage();


	enum bufferFlags_t {
		IBF_SYSTEM_MEMORY = 1 << 0,
		IBF_VIDEO_MEMORY = 1 << 1,
		IBF_INDEX16 = 1 << 2,
		IBF_DISCARDABLE = 1 << 3
	};

	enum lockFlags_t {
		IBLOCK_READ = 1 << 0,
		IBLOCK_WRITE = 1 << 1,
		IBLOCK_DISCARD = 1 << 2
	};

	int					m_flags;
	unsigned int		m_lockStatus;
	int					m_numIndices;
	unsigned int		m_ibID;
	int					m_lockIndexOffset;
	int					m_lockIndexCount;
	unsigned char* m_lockedBase;
	unsigned char* m_systemMemStorage;
	int					m_numIndicesWritten;
};

#endif // __RV_INDEXBUFFER_H__