#pragma once

class rvGLSLShader : public rvShader {
public:
	rvGLSLShader() = default;
	virtual ~rvGLSLShader();

	virtual unsigned int GetVariablePosition(const char* name);
	virtual void Bind();
	virtual void UnBind();
	virtual void SetTexture(int position, int unit, idImage* image);
	virtual void UnBindTexture(int position, int unit, idImage* image);
	virtual bool LoadProgram();

protected:
	unsigned int program = 0;
	unsigned int vertexShader = 0;
	unsigned int fragmentShader = 0;
};

class rvGLSLShaderStage : public rvNewShaderStage {
public:
	rvGLSLShaderStage();
	virtual void Resolve();
	virtual void BindShaderParameter(int slot, int numParms, const float* floatVector, int arraySize);

	virtual bool IsSupported() const { return true;  }

public:
	char     shaderName[256];

	char     shaderParmNames[96][32];
	int      shaderParmLocations[96];
	int      shaderParmRegisters[96];
	int      shaderParmNumRegisters[96];

	char     textureParmNames[8][32];
	int      textureParmLocations[8];
	idImage* textureParmImages[8];

	rvShader* shaderProgram;
	bool      invalidShader;
};

void ErrorWithInfoLog(unsigned int obj, const char* name);