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

rvShader* rvNewShaderStage::ShaderList[200];
int rvNewShaderStage::NumShaders = 0;


// ---------------------------------------------------
// rvShader
// ---------------------------------------------------

rvShader::rvShader(unsigned int initTarget, const char* initName) {
	target = initTarget;
	ident = rvNewShaderStage::NumShaders;
	name[0] = '\0';

	if (initName) {
		strncpy(name, initName, sizeof(name));
		name[sizeof(name) - 1] = '\0';
	}
}

// ---------------------------------------------------
// rvNewShaderStage
// ---------------------------------------------------

rvNewShaderStage::rvNewShaderStage() {
	shaderName[0] = '\0';

	memset(shaderParmNames, 0, sizeof(shaderParmNames));
	memset(shaderParmLocations, 0, sizeof(shaderParmLocations));
	memset(shaderParmRegisters, 0xFF, sizeof(shaderParmRegisters));
	memset(shaderParmNumRegisters, 0, sizeof(shaderParmNumRegisters));

	memset(textureParmNames, 0, sizeof(textureParmNames));

	for (int i = 0; i < 8; ++i) {
		textureParmLocations[i] = -1;
		textureParmImages[i] = 0;
	}

	shaderProgram = 0;
	invalidShader = false;
}

bool rvNewShaderStage::AddShaderProgram(rvShader* shaderProg) {
	int index = NumShaders;
	ShaderList[index] = shaderProg;
	NumShaders = index + 1;
	return shaderProg->LoadProgram();
}

rvShader* rvNewShaderStage::FindShaderProgram(const char* program) {
	if (!program) {
		return 0;
	}

	for (int i = 0; i < NumShaders; ++i) {
		rvShader* shader = ShaderList[i];
		if (!shader) {
			continue;
		}

		char lhs[256];
		char rhs[256];

		strncpy(lhs, program, sizeof(lhs));
		lhs[sizeof(lhs) - 1] = '\0';

		strncpy(rhs, shader->name, sizeof(rhs));
		rhs[sizeof(rhs) - 1] = '\0';

		char* dot = strrchr(lhs, '.');
		if (dot) {
			*dot = '\0';
		}

		dot = strrchr(rhs, '.');
		if (dot) {
			*dot = '\0';
		}

		if (!idStr::Icmp(lhs, rhs)) {
			return shader;
		}
	}

	if (NumShaders == 200) {
		common->Printf("R_FindShaderProgram: MAX_SHADER_PROGS");
	}

	return 0;
}

void rvNewShaderStage::UnBind() {
	if (!shaderProgram) {
		return;
	}

	for (int i = 0; i < 8; ++i) {
		if (!textureParmNames[i][0]) {
			break;
		}

		shaderProgram->UnBindTexture(
			textureParmLocations[i],
			i,
			textureParmImages[i]
		);
	}

	GL_SelectTexture(0);
	shaderProgram->UnBind();
}

void rvNewShaderStage::Shutdown() {
	shaderProgram = 0;
}

void rvNewShaderStage::BindShaderTextureConstant(int slot, int bindingType, const drawInteraction_t* din) {
	// Intentionally left minimal.
	// Wire this to your real drawInteraction_t / engine constants.
	(void)slot;
	(void)bindingType;
	(void)din;
}

void rvNewShaderStage::SetTextureParm(const char* name, idImage* image) {
	for (int i = 0; i < 8; ++i) {
		if (!idStr::Icmp(name, textureParmNames[i])) {
			textureParmImages[i] = image;
			return;
		}
	}
}

int rvNewShaderStage::FindShaderParameter(const char* name) {
	for (int i = 0; i < 96; ++i) {
		if (!idStr::Icmp(name, shaderParmNames[i])) {
			return i;
		}
	}
	return -1;
}

void rvNewShaderStage::SetShaderParameter(int index, float* registers, const float* floatVector, int arraySize) {
	if (index < 0) {
		return;
	}

	if (arraySize > 4) {
		arraySize = 4;
	}

	for (int i = 0; i < arraySize; ++i) {
		int reg = shaderParmRegisters[index][i];
		if (reg >= 0) {
			registers[reg] = floatVector[i];
		}
	}
}

void rvNewShaderStage::Resolve() {
	if (!IsSupported()) {
		shaderProgram = 0;
		invalidShader = true;
		return;
	}

	shaderProgram = FindShaderProgram(shaderName);
	if (!shaderProgram) {
		invalidShader = true;
		return;
	}

	invalidShader = false;

	for (int i = 0; i < 96; ++i) {
		if (!shaderParmNames[i][0]) {
			break;
		}
		shaderParmLocations[i] = shaderProgram->GetVariablePosition(shaderParmNames[i]);
	}

	for (int i = 0; i < 8; ++i) {
		if (!textureParmNames[i][0]) {
			break;
		}
		textureParmLocations[i] = shaderProgram->GetVariablePosition(textureParmNames[i]);
	}
}

void rvNewShaderStage::Bind(const float* registers, const drawInteraction_t* din) {
	if (!shaderProgram) {
		Resolve();
		if (invalidShader) {
			return;
		}
	}

	shaderProgram->Bind();
	UpdateShaderParms(registers, din);

	for (int i = 0; i < 8; ++i) {
		if (!textureParmNames[i][0]) {
			break;
		}

		int location = textureParmLocations[i];

		if (location >= 0 || textureParmImages[i] != 0) {
			shaderProgram->SetTexture(location, i, textureParmImages[i]);
		}
		else {
			BindShaderTextureConstant(i, -location, din);
		}
	}
}

void rvNewShaderStage::BindShaderParameterConstant(int slot, int bindingType, const drawInteraction_t* din) {
	// Intentionally left minimal.
	// Wire this to your real renderer constants.
	(void)slot;
	(void)bindingType;
	(void)din;
}

void rvNewShaderStage::UpdateShaderParms(const float* registers, const drawInteraction_t* din) {
	float data[4];

	for (int i = 0; i < 96; ++i) {
		if (!shaderParmNames[i][0]) {
			break;
		}

		memset(data, 0, sizeof(data));

		int firstReg = shaderParmRegisters[i][0];
		if (firstReg < 0) {
			BindShaderParameterConstant(i, -1 - firstReg, din);
			continue;
		}

		int numRegs = shaderParmNumRegisters[i];
		if (numRegs > 4) {
			numRegs = 4;
		}

		for (int j = 0; j < numRegs; ++j) {
			int reg = shaderParmRegisters[i][j];
			if (reg >= 0) {
				data[j] = registers[reg];
			}
		}

		BindShaderParameter(i, numRegs, data, 1);
	}
}

bool rvNewShaderStage::ParseProgram(idLexer& src, idMaterial* material) {
	(void)material;

	idToken token;
	if (!src.ReadToken(&token)) {
		return false;
	}

	strncpy(shaderName, token.c_str(), sizeof(shaderName));
	shaderName[sizeof(shaderName) - 1] = '\0';

	return IsSupported();
}

bool rvNewShaderStage::ParseShaderParm(idLexer& src, idMaterial* material) {
	// Intentionally stubbed because this depends on your real ParseExpression path
	// and shader constant/register table.
	(void)src;
	(void)material;
	return true;
}

bool rvNewShaderStage::ParseTextureParm(idLexer& src, idMaterial* material, int trpDefault) {
	// Intentionally stubbed because this depends on your real image parser /
	// image manager API.
	(void)src;
	(void)material;
	(void)trpDefault;
	return true;
}

void rvNewShaderStage::R_Shaders_Init() {
	NumShaders = 0;
	// Leave material iteration to your real material API.
}

void rvNewShaderStage::R_Shaders_Shutdown() {
	for (int i = 0; i < NumShaders; ++i) {
		delete ShaderList[i];
		ShaderList[i] = 0;
	}
	NumShaders = 0;
}