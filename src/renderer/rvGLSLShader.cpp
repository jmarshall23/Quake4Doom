/*
===========================================================================

Quake4Doom 

Copyright (C) 2026 Justin Marshall
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Quake4Doom Project by Justin Marshall
This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Quake4Doom is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Quake4Doom is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake4Doom.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Quake4Doom is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#include "tr_local.h"

#define Q4_SUPPORT_GLSL_PATH 0

// ----------------------------------------------------------------------------
// ErrorWithInfoLog
// ----------------------------------------------------------------------------

void ErrorWithInfoLog(unsigned int obj, const char* name) {
	int infoLogLength = 0;
	int charsWritten = 0;
	int objectSubtype = 0;
#if Q4_SUPPORT_GLSL_PATH
	glGetObjectParameterivARB(obj, 0x8B84, &infoLogLength); // GL_OBJECT_INFO_LOG_LENGTH_ARB

	const char* infoText = "Unknown error";
	char* dynamicLog = nullptr;

	if (infoLogLength > 0) {
		dynamicLog = new char[infoLogLength + 1];
		dynamicLog[0] = '\0';
		glGetInfoLogARB(obj, infoLogLength, &charsWritten, dynamicLog);
		dynamicLog[charsWritten < infoLogLength ? charsWritten : infoLogLength] = '\0';
		infoText = dynamicLog;
	}

	glGetObjectParameterivARB(obj, 0x8B4F, &objectSubtype); // GL_OBJECT_SUBTYPE_ARB

	const char* shaderType = "unknown";
	if (objectSubtype == 35633) {
		shaderType = "vertex";
	}
	else if (objectSubtype == 35632) {
		shaderType = "fragment";
	}

	if (common->IsInitialized()) {
		common->Warning("Failed to compile %s shader %s:\n%s", shaderType, name, infoText);
	}
	else {
		common->Printf("Failed to compile %s shader %s:\n%s", shaderType, name, infoText);
	}

	delete[] dynamicLog;
#endif
}

// ----------------------------------------------------------------------------
// rvGLSLShader
// ----------------------------------------------------------------------------

rvGLSLShader::~rvGLSLShader() {
#if Q4_SUPPORT_GLSL_PATH
	if (program && vertexShader) {
		glDetachObjectARB(program, vertexShader);
	}

	if (program && fragmentShader) {
		glDetachObjectARB(program, fragmentShader);
	}

	if (vertexShader) {
		glDeleteObjectARB(vertexShader);
		vertexShader = 0;
	}

	if (fragmentShader) {
		glDeleteObjectARB(fragmentShader);
		fragmentShader = 0;
	}

	if (program) {
		glDeleteObjectARB(program);
		program = 0;
	}
#endif
}

unsigned int rvGLSLShader::GetVariablePosition(const char* name) {
#if Q4_SUPPORT_GLSL_PATH
	return glGetUniformLocationARB(program, name);
#else
	return 0;
#endif
}

void rvGLSLShader::Bind() {
#if Q4_SUPPORT_GLSL_PATH
	glUseProgramObjectARB(program);
#endif
}

void rvGLSLShader::UnBind() {
#if Q4_SUPPORT_GLSL_PATH
	glUseProgramObjectARB(0);
#endif
}

void rvGLSLShader::SetTexture(int position, int unit, idImage* image) {
	GL_SelectTexture(unit);
	image->Bind();
#if Q4_SUPPORT_GLSL_PATH
	glUniform1iARB(position, unit);
#endif
}

void rvGLSLShader::UnBindTexture(int position, int unit, idImage* image) {
	GL_SelectTexture(unit);
	globalImages->BindNull();
	(void)position;
}

bool rvGLSLShader::LoadProgram() {
#if Q4_SUPPORT_GLSL_PATH
	idStr fullPath("glprogs/");
	fullPath += name;

	idStr fileName;

	char* vertexBuffer = nullptr;
	int vertexLength = 0;

	char* fragmentBuffer = nullptr;
	int fragmentLength = 0;

	fileName = fullPath;
	fileName += "vp";
	fileSystem->ReadFile(fileName.c_str(), reinterpret_cast<void**>(&vertexBuffer), nullptr);

	if (!vertexBuffer) {
		common->Printf(": File not found\n");
		return false;
	}

	fileName = fullPath;
	fileName += "fp";
	fileSystem->ReadFile(fileName.c_str(), reinterpret_cast<void**>(&fragmentBuffer), nullptr);

	if (!fragmentBuffer) {
		fileSystem->FreeFile(vertexBuffer);
		common->Printf(": File not found\n");
		return false;
	}

	int status = 0;

	vertexShader = glCreateShaderObjectARB(0x8B31);   // GL_VERTEX_SHADER_ARB
	fragmentShader = glCreateShaderObjectARB(0x8B30); // GL_FRAGMENT_SHADER_ARB

	glShaderSourceARB(
		vertexShader,
		1,
		const_cast<const char**>(&vertexBuffer),
		(vertexLength != 0) ? &vertexLength : nullptr
	);

	glShaderSourceARB(
		fragmentShader,
		1,
		const_cast<const char**>(&fragmentBuffer),
		(fragmentLength != 0) ? &fragmentLength : nullptr
	);

	fileSystem->FreeFile(vertexBuffer);
	fileSystem->FreeFile(fragmentBuffer);

	glGetError(); // clear any prior error

	glCompileShaderARB(vertexShader);
	unsigned int error = glGetError();
	glGetObjectParameterivARB(vertexShader, 0x8B81, &status); // GL_OBJECT_COMPILE_STATUS_ARB

	if (error || !status) {
		ErrorWithInfoLog(vertexShader, name);
		return false;
	}

	glCompileShaderARB(fragmentShader);
	error = glGetError();
	glGetObjectParameterivARB(fragmentShader, 0x8B81, &status); // GL_OBJECT_COMPILE_STATUS_ARB

	if (error || !status) {
		ErrorWithInfoLog(fragmentShader, name);
		return false;
	}

	program = glCreateProgramObjectARB();
	glAttachObjectARB(program, vertexShader);
	glAttachObjectARB(program, fragmentShader);
	glLinkProgramARB(program);

	error = glGetError();
	glGetObjectParameterivARB(program, 0x8B82, &status); // GL_OBJECT_LINK_STATUS_ARB

	if (error || !status) {
		ErrorWithInfoLog(program, name);
		return false;
	}

	return true;
#else
	return false;
#endif
}

// ----------------------------------------------------------------------------
// rvGLSLShaderStage
// ----------------------------------------------------------------------------

rvGLSLShaderStage::rvGLSLShaderStage() {
	shaderName[0] = '\0';

	memset(shaderParmNames, 0, sizeof(shaderParmNames));
	memset(shaderParmLocations, 0, sizeof(shaderParmLocations));
	memset(shaderParmRegisters, 0xFF, sizeof(shaderParmRegisters));
	memset(shaderParmNumRegisters, 0, sizeof(shaderParmNumRegisters));

	memset(textureParmNames, 0, sizeof(textureParmNames));

	for (int i = 0; i < 8; ++i) {
		textureParmLocations[i] = -1;
		textureParmImages[i] = nullptr;
	}

	shaderProgram = nullptr;
	invalidShader = false;
}

void rvGLSLShaderStage::Resolve() {
	if (!IsSupported()) {
		shaderProgram = nullptr;
		invalidShader = true;
		return;
	}

	shaderProgram = rvNewShaderStage::FindShaderProgram(shaderName);

	if (!shaderProgram) {
		rvGLSLShader* glslShader = new rvGLSLShader();

		if (!glslShader) {
			common->Warning("Invalid GLSL shader '%s'\n", shaderName);
			invalidShader = true;
			return;
		}

		// The original decompile shows the base rvShader constructor being called
		// with shaderName before the vtable is switched to rvGLSLShader.
		// That setup needs to happen in your real class constructor hierarchy.
		shaderProgram = glslShader;
		rvNewShaderStage::AddShaderProgram(shaderProgram);
	}

	if (!shaderProgram) {
		common->Warning("Invalid GLSL shader '%s'\n", shaderName);
		invalidShader = true;
		return;
	}

	invalidShader = false;

	for (int i = 0; i < 96; ++i) {
		if (shaderParmNames[i][0] == '\0') {
			break;
		}

		shaderParmLocations[i] = shaderProgram->GetVariablePosition(shaderParmNames[i]);
	}

	for (int i = 0; i < 8; ++i) {
		if (textureParmNames[i][0] == '\0') {
			break;
		}

		textureParmLocations[i] = shaderProgram->GetVariablePosition(textureParmNames[i]);
	}
}

void rvGLSLShaderStage::BindShaderParameter(
	int slot,
	int numParms,
	const float* floatVector,
	int arraySize
) {
#if Q4_SUPPORT_GLSL_PATH
	switch (numParms) {
	case 1:
		glUniform1fvARB(shaderParmLocations[slot], arraySize, floatVector);
		break;
	case 2:
		glUniform2fvARB(shaderParmLocations[slot], arraySize, floatVector);
		break;
	case 3:
		glUniform3fvARB(shaderParmLocations[slot], arraySize, floatVector);
		break;
	case 4:
		glUniform4fvARB(shaderParmLocations[slot], arraySize, floatVector);
		break;
	default:
		break;
	}
#endif
}