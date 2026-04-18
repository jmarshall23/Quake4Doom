class rvTexRenderTarget {
public:
	rvTexRenderTarget();
	~rvTexRenderTarget();

	void ResetValues();
	bool Init(
		int resWidth,
		int resHeight,
		int numColorBits,
		int numRedBits,
		int numGreenBits,
		int numBlueBits,
		int numAlphaBits,
		int numDepthBits,
		int numStencilBits,
		int flags
	);

	void Release();
	bool Restore();

	void BeginRender(int cubeFace);
	void EndRender(bool restorePreviousDC);

	void BeginTexture(
		unsigned int textureObjName,
		int minFilter,
		int magFilter,
		int wrap
	);
	void EndTexture();

	void DefaultD3GL();

private:
	HPBUFFERARB m_hPBuffer;
	HDC         m_hDC;
	HGLRC       m_hGLRC;
	unsigned int m_textureObjName;

	int m_resWidth;
	int m_resHeight;
	int m_flags;

	int m_numColorBits;
	int m_numRedBits;
	int m_numGreenBits;
	int m_numBlueBits;
	int m_numAlphaBits;
	int m_numDepthBits;
	int m_numStencilBits;

	int m_target;

	int m_resWidthSave;
	int m_resHeightSave;
	int m_flagsSave;

	int m_numColorBitsSave;
	int m_numRedBitsSave;
	int m_numGreenBitsSave;
	int m_numBlueBitsSave;
	int m_numAlphaBitsSave;
	int m_numDepthBitsSave;
	int m_numStencilBitsSave;

private:
	static HDC   m_hPrevDC;
	static HGLRC m_hPrevGLRC;
	static int   m_prevDrawBuffer;
	static int   m_prevReadBuffer;
	static int   m_prevViewport[4];
};