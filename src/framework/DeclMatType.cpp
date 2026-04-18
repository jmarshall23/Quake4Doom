// DeclMatType.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "../renderer/tr_local.h"

/*
=======================
rvDeclMatType::DefaultDefinition
=======================
*/
const char* rvDeclMatType::DefaultDefinition(void) const {
	return "{ description \"<DEFAULTED>\" rgb 0,0,0 }";
}

/*
=======================
rvDeclMatType::Parse
=======================
*/
bool rvDeclMatType::Parse(const char* text, const int textLength) {
	idLexer src;
	idToken	token, token2;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}
		else if (token == "rgb")
		{
			mTint[0] = src.ParseInt();
			src.ExpectTokenString(",");
			mTint[1] = src.ParseInt();
			src.ExpectTokenString(",");
			mTint[2] = src.ParseInt();
		}
		else if (token == "description")
		{
			src.ReadToken(&token);
			mDescription = token;
			continue;
		}
		else
		{
			src.Error("rvDeclMatType::Parse: Invalid or unexpected token %s\n", token.c_str());
			return false;
		}
	}
	return true;
}

/*
=======================
rvDeclMatType::FreeData
=======================
*/
void rvDeclMatType::FreeData(void) {

}

/*
=======================
rvDeclMatType::Size
=======================
*/
size_t rvDeclMatType::Size(void) const {
	return sizeof(rvDeclMatType);
}

const rvDeclMatType* MT_FindMaterialTypeByTint(unsigned int tint) {
	const int numDecls = declManager->GetNumDecls(DECL_MATERIALTYPE);

	for (int i = 0; i < numDecls; ++i) {
		const rvDeclMatType* materialType = declManager->MaterialTypeByIndex(i, true);
		if (materialType->GetTint() == tint) {
			return materialType;
		}
	}

	return nullptr;
}

const rvDeclMatType* MT_FindMaterialTypeByClosestTint(unsigned int tint) {
	int bestDistance = 0x01000001;
	const rvDeclMatType* bestMaterialType = nullptr;

	const int count = declManager->GetNumDecls(DECL_MATERIALTYPE);
	for (int i = 0; i < count; ++i) {
		const rvDeclMatType* materialType = declManager->MaterialTypeByIndex(i, true);
		if (materialType == nullptr) {
			continue;
		}

		const unsigned int materialTint = materialType->GetTint();

		const int r1 = (tint >> 24) & 0xFF;
		const int g1 = (tint >> 16) & 0xFF;
		const int b1 = (tint >> 8) & 0xFF;

		const int r2 = (materialTint >> 24) & 0xFF;
		const int g2 = (materialTint >> 16) & 0xFF;
		const int b2 = (materialTint >> 8) & 0xFF;

		const int dr = r1 - r2;
		const int dg = g1 - g2;
		const int db = b1 - b2;

		const int distance = dr * dr + dg * dg + db * db;
		if (distance < bestDistance) {
			bestDistance = distance;
			bestMaterialType = materialType;
		}
	}

	return bestMaterialType;
}

unsigned char* MT_GetMaterialTypeArray(idStr image, int& width, int& height) {
#if 0
	if (mat_useHitMaterials.internalVar->integerValue != 0) {
		idStr::SetFileExtension(&image, ".hit");

		idFile* file = fileSystem->OpenFileRead(fileSystem, image.data, true, nullptr);
		if (file != nullptr) {
			file->Read(file, height, sizeof(int));
			*height = LittleLong(*height);

			file->Read(file, width, sizeof(int));
			*width = LittleLong(*width);

			const int pixelCount = (*width) * (*height);
			unsigned char* array = static_cast<unsigned char*>(Mem_ClearedAlloc(pixelCount, TAG_TEMP));

			file->Read(file, array, pixelCount);
			fileSystem->CloseFile(fileSystem, file);

			idStr::FreeData(&image);
			return array;
		}

		idStr::StripFileExtension(&image);
	}
#endif
	unsigned char* pic = nullptr;
	R_LoadImage(image.c_str(), &pic, &width, &height, nullptr, false);

	if (pic == nullptr) {
		common->Printf("Failed to load hit material image %s", image.c_str());
		return nullptr;
	}

	const int pixelCount = (width) * (height);
	unsigned char* array = static_cast<unsigned char*>(Mem_ClearedAlloc(pixelCount));

	const unsigned char* src = pic;
	for (int i = 0; i < pixelCount; ++i, src += 4) {
		const unsigned int tint = *reinterpret_cast<const unsigned int*>(src);

		const rvDeclMatType* materialType = MT_FindMaterialTypeByTint(tint);
		if (materialType == nullptr) {
			materialType = MT_FindMaterialTypeByClosestTint(tint);
		}

		array[i] = materialType->base->Index();
	}

	R_StaticFree(pic);
#if 0
	if (mat_writeHitMaterials.internalVar->integerValue != 0) {
		idStr::SetFileExtension(&image, ".hit");

		idFile* file = fileSystem->OpenFileWrite(fileSystem, image.data, "fs_savepath", 0);
		if (file != nullptr) {
			file->Write(file, height, sizeof(int));
			file->Write(file, width, sizeof(int));
			file->Write(file, array, pixelCount);
			fileSystem->CloseFile(fileSystem, file);
		}
	}

	idStr::FreeData(&image);
#endif
	return array;
}