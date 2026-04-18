class idLexer;
class idMaterial;
class idImage;
class rvShader;
struct drawInteraction_t;

class rvShader {
public:
	rvShader() { }
	rvShader(unsigned int initTarget, const char* initName);
	virtual ~rvShader() {}

	virtual bool			LoadProgram() = 0;
	virtual unsigned int	GetVariablePosition(const char* name) = 0;
	virtual void			Bind() = 0;
	virtual void			UnBind() = 0;
	virtual void			SetTexture(int position, int unit, idImage* image) = 0;
	virtual void			UnBindTexture(int position, int unit, idImage* image) = 0;

public:
	unsigned int	target;
	int				ident;
	char			name[256];
};

class rvNewShaderStage {
public:
	rvNewShaderStage();

	virtual void	UnBind();
	virtual void	Shutdown();
	virtual void	Bind(const float* registers, const drawInteraction_t* din);

	virtual bool	ParseProgram(idLexer& src, idMaterial* material);
	virtual bool	ParseShaderParm(idLexer& src, idMaterial* material);
	virtual bool	ParseTextureParm(idLexer& src, idMaterial* material, int trpDefault);

	virtual void	Resolve();
	virtual bool	IsSupported() const = 0;

	virtual void	BindShaderParameter(int slot, int numParms, const float* floatVector, int arraySize) = 0;
	virtual void	UpdateShaderParms(const float* registers, const drawInteraction_t* din);

	void			BindShaderTextureConstant(int slot, int bindingType, const drawInteraction_t* din);
	void			BindShaderParameterConstant(int slot, int bindingType, const drawInteraction_t* din);
	void			SetTextureParm(const char* name, idImage* image);
	int				FindShaderParameter(const char* name);
	void			SetShaderParameter(int index, float* registers, const float* floatVector, int arraySize);

	static bool		AddShaderProgram(rvShader* shaderProg);
	static rvShader* FindShaderProgram(const char* program);
	static void		R_Shaders_Init();
	static void		R_Shaders_Shutdown();

public:
	char		shaderName[256];

	char		shaderParmNames[96][32];
	int			shaderParmLocations[96];
	int			shaderParmRegisters[96][4];
	int			shaderParmNumRegisters[96];

	char		textureParmNames[8][32];
	int			textureParmLocations[8];
	idImage* textureParmImages[8];

	rvShader* shaderProgram;
	bool		invalidShader;

	static rvShader* ShaderList[200];
	static int NumShaders;
};