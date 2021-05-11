// DeclPlayerModel.cpp
//

#include "precompiled.h"

/*
===================
rvDeclPlayerModel::Size
===================
*/
size_t rvDeclPlayerModel::Size(void) const {
	return sizeof(rvDeclPlayerModel);
}

/*
===================
rvDeclPlayerModel::DefaultDefinition
===================
*/
const char* rvDeclPlayerModel::DefaultDefinition() const {
	return "{\n\t\"model\"\t\"model_player_marine\"\n\t\"def_head\"\t\"char_marinehead_kane2_client\"\n}";
}

/*
===================
rvDeclPlayerModel::FreeData
===================
*/
bool rvDeclPlayerModel::Parse(const char* text, const int textLength) {
	// jmarshall: can't see were this is used. 
	return true;
}

/*
===================
rvDeclPlayerModel::FreeData
===================
*/
void rvDeclPlayerModel::FreeData(void) {

}

/*
===================
rvDeclPlayerModel::DefaultDefinition
===================
*/
void rvDeclPlayerModel::Print(void) {

}