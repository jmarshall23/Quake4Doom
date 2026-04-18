/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Quake4Doom Project by Justin Marshall

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"

#ifndef MF_NEED_CURRENT_RENDER
#define MF_NEED_CURRENT_RENDER (1 << 8)
#endif
#ifndef MF_NEEDCURRENTRENDER
#define MF_NEEDCURRENTRENDER MF_NEED_CURRENT_RENDER
#endif

#ifndef DECL_MATERIALTYPE
#define DECL_MATERIALTYPE ((declType_t)6)
#endif

#ifndef GLS_SRCBLEND_SRC_COLOR
#define GLS_SRCBLEND_SRC_COLOR 10
#endif
#ifndef GLS_SRCBLEND_ONE_MINUS_SRC_COLOR
#define GLS_SRCBLEND_ONE_MINUS_SRC_COLOR 11
#endif
#ifndef GLS_DSTBLEND_DST_COLOR
#define GLS_DSTBLEND_DST_COLOR 144
#endif
#ifndef GLS_DSTBLEND_ONE_MINUS_DST_COLOR
#define GLS_DSTBLEND_ONE_MINUS_DST_COLOR 160
#endif

#ifdef Q4_PORT_ENABLE_PORTAL_FEATURES
#undef Q4_PORT_ENABLE_PORTAL_FEATURES
#endif
#define Q4_PORT_ENABLE_PORTAL_FEATURES 1

#ifdef Q4_PORT_ENABLE_MATERIAL_TYPES
#undef Q4_PORT_ENABLE_MATERIAL_TYPES
#endif
#define Q4_PORT_ENABLE_MATERIAL_TYPES 1

#ifdef Q4_PORT_ENABLE_FRAGMENT_PARMS
#undef Q4_PORT_ENABLE_FRAGMENT_PARMS
#endif
#define Q4_PORT_ENABLE_FRAGMENT_PARMS 1

#ifdef Q4_PORT_ENABLE_GLSL_STAGES
#undef Q4_PORT_ENABLE_GLSL_STAGES
#endif
#define Q4_PORT_ENABLE_GLSL_STAGES 1

#ifdef Q4_PORT_ENABLE_ALPHA_FUNC
#undef Q4_PORT_ENABLE_ALPHA_FUNC
#endif
#define Q4_PORT_ENABLE_ALPHA_FUNC 1

#ifdef Q4_PORT_ENABLE_VERTEX_RANDOM
#undef Q4_PORT_ENABLE_VERTEX_RANDOM
#endif
#define Q4_PORT_ENABLE_VERTEX_RANDOM 1

#ifdef Q4_PORT_ENABLE_DECAL_MAXANGLE
#undef Q4_PORT_ENABLE_DECAL_MAXANGLE
#endif
#define Q4_PORT_ENABLE_DECAL_MAXANGLE 1

#ifdef Q4_PORT_ENABLE_EXTRA_METHODS
#undef Q4_PORT_ENABLE_EXTRA_METHODS
#endif
#define Q4_PORT_ENABLE_EXTRA_METHODS 1

static int MaterialNextPowerOfTwo(int x) {
	int pot = 1;
	while (pot < x) {
		pot <<= 1;
	}
	return pot;
}


/*

Any errors during parsing just set MF_DEFAULTED and return, rather than throwing
a hard error. This will cause the material to fall back to default material,
but otherwise let things continue.

Each material may have a set of calculations that must be evaluated before
drawing with it.

Every expression that a material uses can be evaluated at one time, which
will allow for perfect common subexpression removal when I get around to
writing it.

Without this, scrolling an entire surface could result in evaluating the
same texture matrix calculations a half dozen times.

  Open question: should I allow arbitrary per-vertex color, texCoord, and vertex
  calculations to be specified in the material code?

  Every stage will definately have a valid image pointer.

  We might want the ability to change the sort value based on conditionals,
  but it could be a hassle to implement,

*/

// keep all of these on the stack, when they are static it makes material parsing non-reentrant
typedef struct mtrParsingData_s {
	bool			registerIsTemporary[MAX_EXPRESSION_REGISTERS];
	float			shaderRegisters[MAX_EXPRESSION_REGISTERS];
	expOp_t			shaderOps[MAX_EXPRESSION_OPS];
	shaderStage_t	parseStages[MAX_SHADER_STAGES];

	bool			registersAreConstant;
	bool			forceOverlays;
} mtrParsingData_t;


/*
=============
idMaterial::CommonInit
=============
*/
void idMaterial::CommonInit() {
	desc = "<none>";
	renderBump = "";
	contentFlags = CONTENTS_SOLID;
	surfaceFlags = SURFTYPE_NONE;
	materialFlags = 0;
	sort = SS_BAD;
	coverage = MC_BAD;
	cullType = CT_FRONT_SIDED;
	deform = DFRM_NONE;
	numOps = 0;
	ops = NULL;
	numRegisters = 0;
	expressionRegisters = NULL;
	constantRegisters = NULL;
	numStages = 0;
	numAmbientStages = 0;
	stages = NULL;
	editorImage = NULL;
	lightFalloffImage = NULL;
	shouldCreateBackSides = false;
	entityGui = 0;
	fogLight = false;
	blendLight = false;
	ambientLight = false;
	noFog = false;
	hasSubview = false;
	allowOverlays = true;
	unsmoothedTangents = false;
	gui = NULL;
	memset(deformRegisters, 0, sizeof(deformRegisters));
	editorAlpha = 1.0f;
	spectrum = 0;
	polygonOffset = 0.0f;
	suppressInSubview = false;
	refCount = 0;
	portalSky = false;
	portalDistanceNear = 262144.0f;
	portalDistanceFar = 262144.0f;
	portalImage = NULL;
	materialType = NULL;
	materialTypeArray = NULL;
	materialTypeArrayName = "";
	MTAWidth = 0;
	MTAHeight = 0;
	useCount = 0;
	globalUseCount = 0;
	decalInfo.stayTime = 10000;
	decalInfo.maxAngle = 0.1f;
}

/*
=============
idMaterial::idMaterial
=============
*/
idMaterial::idMaterial() {
	CommonInit();

	// we put this here instead of in CommonInit, because
	// we don't want it cleared when a material is purged
	surfaceArea = 0;
}

/*
=============
idMaterial::~idMaterial
=============
*/
idMaterial::~idMaterial() {
}

/*
===============
idMaterial::FreeData
===============
*/
void idMaterial::FreeData() {
	int i;

	if (stages) {
		// delete any idCinematic textures
		for (i = 0; i < numStages; i++) {
			if (stages[i].texture.cinematic != NULL) {
				delete stages[i].texture.cinematic;
				stages[i].texture.cinematic = NULL;
			}
			if (stages[i].newStage != NULL) {
				Mem_Free(stages[i].newStage);
				stages[i].newStage = NULL;
			}
#if Q4_PORT_ENABLE_GLSL_STAGES
			if (stages[i].newShaderStage != NULL) {
				delete stages[i].newShaderStage;
				stages[i].newShaderStage = NULL;
			}
#endif
		}
		R_StaticFree(stages);
		stages = NULL;
	}
	if (expressionRegisters != NULL) {
		R_StaticFree(expressionRegisters);
		expressionRegisters = NULL;
	}
	if (constantRegisters != NULL) {
		R_StaticFree(constantRegisters);
		constantRegisters = NULL;
	}
	if (ops != NULL) {
		R_StaticFree(ops);
		ops = NULL;
	}
	if (materialTypeArray != NULL) {
		Mem_Free(materialTypeArray);
		materialTypeArray = NULL;
		materialTypeArrayName = "";
		MTAWidth = 0;
		MTAHeight = 0;
	}
}

/*
==============
idMaterial::GetEditorImage
==============
*/
idImage* idMaterial::GetEditorImage(void) const {
	if (editorImage) {
		return editorImage;
	}

	// if we don't have an editorImageName, use the first stage image
	if (!editorImageName.Length()) {
		// _D3XP :: First check for a diffuse image, then use the first
		if (numStages && stages) {
			int i;
			for (i = 0; i < numStages; i++) {
				if (stages[i].lighting == SL_DIFFUSE) {
					editorImage = stages[i].texture.image;
					break;
				}
			}
			if (!editorImage) {
				editorImage = stages[0].texture.image;
			}
		}
		else {
			editorImage = globalImages->defaultImage;
		}
	}
	else {
		// look for an explicit one
		editorImage = globalImages->ImageFromFile(editorImageName, TF_DEFAULT, true, TR_REPEAT, TD_DEFAULT);
	}

	if (!editorImage) {
		editorImage = globalImages->defaultImage;
	}

	return editorImage;
}


// info parms
typedef struct {
	char* name;
	int		clearSolid, surfaceFlags, contents;
} infoParm_t;

static infoParm_t	infoParms[] = {
	// game relevant attributes
	{"solid",		0,	0,	CONTENTS_SOLID },		// may need to override a clearSolid
	{"water",		1,	0,	CONTENTS_WATER },		// used for water
	{"playerclip",	0,	0,	CONTENTS_PLAYERCLIP },	// solid to players
	{"monsterclip",	0,	0,	CONTENTS_MONSTERCLIP },	// solid to monsters
	{"moveableclip",0,	0,	CONTENTS_MOVEABLECLIP },// solid to moveable entities
	{"ikclip",		0,	0,	CONTENTS_IKCLIP },		// solid to IK
	{"blood",		0,	0,	CONTENTS_BLOOD },		// used to detect blood decals
	{"trigger",		0,	0,	CONTENTS_TRIGGER },		// used for triggers
	{"aassolid",	0,	0,	CONTENTS_AAS_SOLID },	// solid for AAS
	{"aasobstacle",	0,	0,	CONTENTS_AAS_OBSTACLE },// used to compile an obstacle into AAS that can be enabled/disabled
	{"flashlight_trigger",	0,	0,	CONTENTS_FLASHLIGHT_TRIGGER }, // used for triggers that are activated by the flashlight
	{"sightclip",	0,	0,	CONTENTS_SIGHTCLIP },
	{"largeshotclip",	0,	0,	CONTENTS_LARGESHOTCLIP },
	{"notacticalfeatures",0,	0,	CONTENTS_NOTACTICALFEATURES },
	{"vehicleclip",	0,	0,	CONTENTS_VEHICLECLIP },
	{"flyclip",	0,	0,	CONTENTS_FLYCLIP },
	{"itemclip",	0,	0,	CONTENTS_ITEMCLIP },
	{"projectileclip",0,	0,	CONTENTS_PROJECTILECLIP },
	{"nonsolid",	1,	0,	0 },					// clears the solid flag
	{"nullNormal",	0,	SURF_NULLNORMAL,0 },		// renderbump will draw as 0x80 0x80 0x80

	// utility relevant attributes
	{"areaportal",	1,	0,	CONTENTS_AREAPORTAL },	// divides areas
	{"qer_nocarve",	1,	0,	CONTENTS_NOCSG},		// don't cut brushes in editor

	{"discrete",	1,	SURF_DISCRETE,	0 },		// surfaces should not be automatically merged together or
	// clipped to the world,
	// because they represent discrete objects like gui shaders
	// mirrors, or autosprites
{"noFragment",	0,	SURF_NOFRAGMENT,	0 },
{"bounce",		0,	SURF_BOUNCE,		0 },
{"noTangentFix",0,	SURF_NO_T_FIX,	0 },
{"no_t_fix",	0,	SURF_NO_T_FIX,	0 },

{"slick",		0,	SURF_SLICK,		0 },
{"collision",	0,	SURF_COLLISION,	0 },
{"noimpact",	0,	SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
{"nodamage",	0,	SURF_NODAMAGE,	0 },		// no falling damage when hitting
{"ladder",		0,	SURF_LADDER,	0 },		// climbable
{"nosteps",		0,	SURF_NOSTEPS,	0 },		// no footsteps

// material types for particle, sound, footstep feedback
{"metal",		0,  SURFTYPE_METAL,		0 },	// metal
{"stone",		0,  SURFTYPE_STONE,		0 },	// stone
{"flesh",		0,  SURFTYPE_FLESH,		0 },	// flesh
{"wood",		0,  SURFTYPE_WOOD,		0 },	// wood
{"cardboard",	0,	SURFTYPE_CARDBOARD,	0 },	// cardboard
{"liquid",		0,	SURFTYPE_LIQUID,	0 },	// liquid
{"glass",		0,	SURFTYPE_GLASS,		0 },	// glass
{"plastic",		0,	SURFTYPE_PLASTIC,	0 },	// plastic
{"ricochet",	0,	SURFTYPE_RICOCHET,	0 },	// behaves like metal but causes a ricochet sound

// unassigned surface types
{"surftype10",	0,	SURFTYPE_10,	0 },
{"surftype11",	0,	SURFTYPE_11,	0 },
{"surftype12",	0,	SURFTYPE_12,	0 },
{"surftype13",	0,	SURFTYPE_13,	0 },
{"surftype14",	0,	SURFTYPE_14,	0 },
{"surftype15",	0,	SURFTYPE_15,	0 },
};

static const int numInfoParms = sizeof(infoParms) / sizeof(infoParms[0]);


/*
===============
idMaterial::CheckSurfaceParm

See if the current token matches one of the surface parm bit flags
===============
*/
bool idMaterial::CheckSurfaceParm(idToken* token) {

	for (int i = 0; i < numInfoParms; i++) {
		if (!token->Icmp(infoParms[i].name)) {
			if (infoParms[i].surfaceFlags & SURF_TYPE_MASK) {
				// ensure we only have one surface type set
				surfaceFlags &= ~SURF_TYPE_MASK;
			}
			surfaceFlags |= infoParms[i].surfaceFlags;
			contentFlags |= infoParms[i].contents;
			if (infoParms[i].clearSolid) {
				contentFlags &= ~CONTENTS_SOLID;
			}
			return true;
		}
	}
	return false;
}

/*
===============
idMaterial::MatchToken

Sets defaultShader and returns false if the next token doesn't match
===============
*/
bool idMaterial::MatchToken(idLexer& src, const char* match) {
	if (!src.ExpectTokenString(match)) {
		SetMaterialFlag(MF_DEFAULTED);
		return false;
	}
	return true;
}

/*
=================
idMaterial::ParseSort
=================
*/
void idMaterial::ParseSort(idLexer& src) {
	idToken token;

	if (!src.ReadTokenOnLine(&token)) {
		src.Warning("missing sort parameter");
		SetMaterialFlag(MF_DEFAULTED);
		return;
	}

	if (!token.Icmp("subview")) {
		sort = SS_SUBVIEW;
	}
	else if (!token.Icmp("pregui")) {
		sort = SS_PREGUI;
	}
	else if (!token.Icmp("gui")) {
		sort = SS_GUI;
	}
	else if (!token.Icmp("opaque")) {
		sort = SS_OPAQUE;
	}
	else if (!token.Icmp("decal")) {
		sort = SS_DECAL;
	}
	else if (!token.Icmp("far")) {
		sort = SS_FAR;
	}
	else if (!token.Icmp("medium")) {
		sort = SS_MEDIUM;
	}
	else if (!token.Icmp("close")) {
		sort = SS_CLOSE;
	}
	else if (!token.Icmp("almostNearest")) {
		sort = SS_ALMOST_NEAREST;
	}
	else if (!token.Icmp("nearest")) {
		sort = SS_NEAREST;
	}
	else if (!token.Icmp("postProcess")) {
		sort = SS_POST_PROCESS;
	}
	else if (!token.Icmp("portalSky")) {
		sort = SS_PORTAL_SKY;
	}
	else {
		sort = atof(token);
	}
}

/*
=================
idMaterial::ParseDecalInfo
=================
*/
void idMaterial::ParseDecalInfo(idLexer& src) {
	idToken token;

	decalInfo.stayTime = src.ParseFloat() * 1000.0f;
	decalInfo.maxAngle = 0.1f;

	if (src.ReadToken(&token)) {
		if (token == ",") {
			decalInfo.maxAngle = src.ParseFloat();
		}
		else {
			src.UnreadToken(&token);
		}
	}
}

/*
=============
idMaterial::GetExpressionConstant
=============
*/
int idMaterial::GetExpressionConstant(float f) {
	int		i;

	for (i = EXP_REG_NUM_PREDEFINED; i < numRegisters; i++) {
		if (!pd->registerIsTemporary[i] && pd->shaderRegisters[i] == f) {
			return i;
		}
	}
	if (numRegisters == MAX_EXPRESSION_REGISTERS) {
		common->Warning("GetExpressionConstant: material '%s' hit MAX_EXPRESSION_REGISTERS", GetName());
		SetMaterialFlag(MF_DEFAULTED);
		return 0;
	}
	pd->registerIsTemporary[i] = false;
	pd->shaderRegisters[i] = f;
	numRegisters++;

	return i;
}

/*
=============
idMaterial::GetExpressionTemporary
=============
*/
int idMaterial::GetExpressionTemporary(void) {
	if (numRegisters == MAX_EXPRESSION_REGISTERS) {
		common->Warning("GetExpressionTemporary: material '%s' hit MAX_EXPRESSION_REGISTERS", GetName());
		SetMaterialFlag(MF_DEFAULTED);
		return 0;
	}
	pd->registerIsTemporary[numRegisters] = true;
	numRegisters++;
	return numRegisters - 1;
}

/*
=============
idMaterial::GetExpressionOp
=============
*/
expOp_t* idMaterial::GetExpressionOp(void) {
	if (numOps == MAX_EXPRESSION_OPS) {
		common->Warning("GetExpressionOp: material '%s' hit MAX_EXPRESSION_OPS", GetName());
		SetMaterialFlag(MF_DEFAULTED);
		return &pd->shaderOps[0];
	}

	return &pd->shaderOps[numOps++];
}

/*
=================
idMaterial::EmitOp
=================
*/
int idMaterial::EmitOp(int a, int b, expOpType_t opType) {
	expOp_t* op;

	// optimize away identity operations
	if (opType == OP_TYPE_ADD) {
		if (!pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 0) {
			return b;
		}
		if (!pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 0) {
			return a;
		}
		if (!pd->registerIsTemporary[a] && !pd->registerIsTemporary[b]) {
			return GetExpressionConstant(pd->shaderRegisters[a] + pd->shaderRegisters[b]);
		}
	}
	if (opType == OP_TYPE_MULTIPLY) {
		if (!pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 1) {
			return b;
		}
		if (!pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 0) {
			return a;
		}
		if (!pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 1) {
			return a;
		}
		if (!pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 0) {
			return b;
		}
		if (!pd->registerIsTemporary[a] && !pd->registerIsTemporary[b]) {
			return GetExpressionConstant(pd->shaderRegisters[a] * pd->shaderRegisters[b]);
		}
	}

	op = GetExpressionOp();
	op->opType = opType;
	op->a = a;
	op->b = b;
	op->c = GetExpressionTemporary();

	return op->c;
}

/*
=================
idMaterial::ParseEmitOp
=================
*/
int idMaterial::ParseEmitOp(idLexer& src, int a, expOpType_t opType, int priority) {
	int		b;

	b = ParseExpressionPriority(src, priority);
	return EmitOp(a, b, opType);
}

/*
=================
idMaterial::ParseTerm

Returns a register index
=================
*/
int idMaterial::ParseTerm(idLexer& src) {
	idToken token;
	int		a, b;

	src.ReadToken(&token);

	if (token == "(") {
		a = ParseExpression(src);
		MatchToken(src, ")");
		return a;
	}

	if (!token.Icmp("time")) {
		pd->registersAreConstant = false;
		return EXP_REG_TIME;
	}
	if (!token.Icmp("parm0")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM0;
	}
	if (!token.Icmp("parm1")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM1;
	}
	if (!token.Icmp("parm2")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM2;
	}
	if (!token.Icmp("parm3")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM3;
	}
	if (!token.Icmp("parm4") || !token.Icmp("DecalLife")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM4;
	}
	if (!token.Icmp("parm5") || !token.Icmp("DecalSpawn")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM5;
	}
	if (!token.Icmp("parm6")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM6;
	}
	if (!token.Icmp("parm7")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM7;
	}
	if (!token.Icmp("parm8")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM8;
	}
	if (!token.Icmp("parm9")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM9;
	}
	if (!token.Icmp("parm10")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM10;
	}
	if (!token.Icmp("parm11")) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM11;
	}
	if (!token.Icmp("global0")) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL0;
	}
	if (!token.Icmp("global1")) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL1;
	}
	if (!token.Icmp("global2")) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL2;
	}
	if (!token.Icmp("global3")) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL3;
	}
	if (!token.Icmp("global4")) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL4;
	}
	if (!token.Icmp("global5")) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL5;
	}
	if (!token.Icmp("global6")) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL6;
	}
	if (!token.Icmp("global7")) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL7;
	}
	if (!token.Icmp("IsMultiplayer")) {
		return GetExpressionConstant(session->IsMultiplayer() ? 1.0f : 0.0f);
	}
	if (!token.Icmp("fragmentPrograms")) {
		return GetExpressionConstant((float)glConfig.ARBFragmentProgramAvailable);
	}
	if (!token.Icmp("glslPrograms")) {
		pd->registersAreConstant = false;
		return EmitOp(0, 0, OP_TYPE_GLSL_ENABLED);
	}
	if (!token.Icmp("POTCorrectionX")) {
		const int pot = MaterialNextPowerOfTwo(glConfig.vidWidth > 0 ? glConfig.vidWidth : 1);
		return GetExpressionConstant((float)glConfig.vidWidth / (float)pot);
	}
	if (!token.Icmp("POTCorrectionY")) {
		const int pot = MaterialNextPowerOfTwo(glConfig.vidHeight > 0 ? glConfig.vidHeight : 1);
		return GetExpressionConstant((float)glConfig.vidHeight / (float)pot);
	}
	if (!token.Icmp("VideoWidth")) {
		return GetExpressionConstant((float)glConfig.vidWidth);
	}
	if (!token.Icmp("VideoHeight")) {
		return GetExpressionConstant((float)glConfig.vidHeight);
	}
	if (!token.Icmp("VertexRandomizer")) {
		pd->registersAreConstant = false;
		return EXP_REG_VERTEX_RANDOMIZER;
	}

	if (!token.Icmp("sound")) {
		pd->registersAreConstant = false;
		return EmitOp(0, 0, OP_TYPE_SOUND);
	}

	// parse negative numbers
	if (token == "-") {
		src.ReadToken(&token);
		if (token.type == TT_NUMBER || token == ".") {
			return GetExpressionConstant(-(float)token.GetFloatValue());
		}
		src.Warning("Bad negative number '%s'", token.c_str());
		SetMaterialFlag(MF_DEFAULTED);
		return 0;
	}

	if (token.type == TT_NUMBER || token == "." || token == "-") {
		return GetExpressionConstant((float)token.GetFloatValue());
	}

	// see if it is a table name
	const idDeclTable* table = static_cast<const idDeclTable*>(declManager->FindType(DECL_TABLE, token.c_str(), false));
	if (!table) {
		src.Warning("Bad term '%s'", token.c_str());
		SetMaterialFlag(MF_DEFAULTED);
		return 0;
	}

	// parse a table expression
	MatchToken(src, "[");

	b = ParseExpression(src);

	MatchToken(src, "]");

	return EmitOp(table->Index(), b, OP_TYPE_TABLE);
}

/*
=================
idMaterial::ParseExpressionPriority

Returns a register index
=================
*/
#define	TOP_PRIORITY 4
int idMaterial::ParseExpressionPriority(idLexer& src, int priority) {
	idToken token;
	int		a;

	if (priority == 0) {
		return ParseTerm(src);
	}

	a = ParseExpressionPriority(src, priority - 1);

	if (TestMaterialFlag(MF_DEFAULTED)) {	// we have a parse error
		return 0;
	}

	if (!src.ReadToken(&token)) {
		// we won't get EOF in a real file, but we can
		// when parsing from generated strings
		return a;
	}

	if (priority == 1 && token == "*") {
		return ParseEmitOp(src, a, OP_TYPE_MULTIPLY, priority);
	}
	if (priority == 1 && token == "/") {
		return ParseEmitOp(src, a, OP_TYPE_DIVIDE, priority);
	}
	if (priority == 1 && token == "%") {	// implied truncate both to integer
		return ParseEmitOp(src, a, OP_TYPE_MOD, priority);
	}
	if (priority == 2 && token == "+") {
		return ParseEmitOp(src, a, OP_TYPE_ADD, priority);
	}
	if (priority == 2 && token == "-") {
		return ParseEmitOp(src, a, OP_TYPE_SUBTRACT, priority);
	}
	if (priority == 3 && token == ">") {
		return ParseEmitOp(src, a, OP_TYPE_GT, priority);
	}
	if (priority == 3 && token == ">=") {
		return ParseEmitOp(src, a, OP_TYPE_GE, priority);
	}
	if (priority == 3 && token == "<") {
		return ParseEmitOp(src, a, OP_TYPE_LT, priority);
	}
	if (priority == 3 && token == "<=") {
		return ParseEmitOp(src, a, OP_TYPE_LE, priority);
	}
	if (priority == 3 && token == "==") {
		return ParseEmitOp(src, a, OP_TYPE_EQ, priority);
	}
	if (priority == 3 && token == "!=") {
		return ParseEmitOp(src, a, OP_TYPE_NE, priority);
	}
	if (priority == 4 && token == "&&") {
		return ParseEmitOp(src, a, OP_TYPE_AND, priority);
	}
	if (priority == 4 && token == "||") {
		return ParseEmitOp(src, a, OP_TYPE_OR, priority);
	}

	// assume that anything else terminates the expression
	// not too robust error checking...

	src.UnreadToken(&token);

	return a;
}

/*
=================
idMaterial::ParseExpression

Returns a register index
=================
*/
int idMaterial::ParseExpression(idLexer& src) {
	return ParseExpressionPriority(src, TOP_PRIORITY);
}


/*
===============
idMaterial::ClearStage
===============
*/
void idMaterial::ClearStage(shaderStage_t* ss) {
	ss->drawStateBits = 0;
	ss->mStageRegisterStart = numRegisters;
	ss->mNumStageRegisters = 0;
	ss->mStageOpsStart = numOps;
	ss->mNumStageOps = 0;
	ss->conditionRegister = GetExpressionConstant(1);
	ss->color.registers[0] =
		ss->color.registers[1] =
		ss->color.registers[2] =
		ss->color.registers[3] = GetExpressionConstant(1);
}

/*
===============
idMaterial::NameToSrcBlendMode
===============
*/
int idMaterial::NameToSrcBlendMode(const idStr& name) {
	if (!name.Icmp("GL_ONE")) {
		return GLS_SRCBLEND_ONE;
	}
	else if (!name.Icmp("GL_ZERO")) {
		return GLS_SRCBLEND_ZERO;
	}
	else if (!name.Icmp("GL_DST_COLOR")) {
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if (!name.Icmp("GL_ONE_MINUS_DST_COLOR")) {
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if (!name.Icmp("GL_SRC_ALPHA")) {
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if (!name.Icmp("GL_ONE_MINUS_SRC_ALPHA")) {
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!name.Icmp("GL_DST_ALPHA")) {
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if (!name.Icmp("GL_ONE_MINUS_DST_ALPHA")) {
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!name.Icmp("GL_SRC_ALPHA_SATURATE")) {
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}
	else if (!name.Icmp("GL_SRC_COLOR")) {
		return GLS_SRCBLEND_SRC_COLOR;
	}
	else if (!name.Icmp("GL_ONE_MINUS_SRC_COLOR")) {
		return GLS_SRCBLEND_ONE_MINUS_SRC_COLOR;
	}

	common->Warning("unknown blend mode '%s' in material '%s'", name.c_str(), GetName());
	SetMaterialFlag(MF_DEFAULTED);

	return GLS_SRCBLEND_ONE;
}

/*
===============
idMaterial::NameToDstBlendMode
===============
*/
int idMaterial::NameToDstBlendMode(const idStr& name) {
	if (!name.Icmp("GL_ONE")) {
		return GLS_DSTBLEND_ONE;
	}
	else if (!name.Icmp("GL_ZERO")) {
		return GLS_DSTBLEND_ZERO;
	}
	else if (!name.Icmp("GL_SRC_ALPHA")) {
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if (!name.Icmp("GL_ONE_MINUS_SRC_ALPHA")) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!name.Icmp("GL_DST_ALPHA")) {
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if (!name.Icmp("GL_ONE_MINUS_DST_ALPHA")) {
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!name.Icmp("GL_SRC_COLOR")) {
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if (!name.Icmp("GL_ONE_MINUS_SRC_COLOR")) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}
	else if (!name.Icmp("GL_DST_COLOR")) {
		return GLS_DSTBLEND_DST_COLOR;
	}
	else if (!name.Icmp("GL_ONE_MINUS_DST_COLOR")) {
		return GLS_DSTBLEND_ONE_MINUS_DST_COLOR;
	}

	common->Warning("unknown blend mode '%s' in material '%s'", name.c_str(), GetName());
	SetMaterialFlag(MF_DEFAULTED);

	return GLS_DSTBLEND_ONE;
}

/*
================
idMaterial::ParseBlend
================
*/
void idMaterial::ParseBlend(idLexer& src, shaderStage_t* stage) {
	idToken token;
	int		srcBlend, dstBlend;

	if (!src.ReadToken(&token)) {
		return;
	}

	// blending combinations
	if (!token.Icmp("blend")) {
		stage->drawStateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		return;
	}
	if (!token.Icmp("add")) {
		stage->drawStateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
		return;
	}
	if (!token.Icmp("filter") || !token.Icmp("modulate")) {
		stage->drawStateBits = GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		return;
	}
	if (!token.Icmp("none")) {
		// none is used when defining an alpha mask that doesn't draw
		stage->drawStateBits = GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE;
		return;
	}
	if (!token.Icmp("bumpmap")) {
		stage->lighting = SL_BUMP;
		return;
	}
	if (!token.Icmp("diffusemap")) {
		stage->lighting = SL_DIFFUSE;
		return;
	}
	if (!token.Icmp("specularmap")) {
		stage->lighting = SL_SPECULAR;
		return;
	}

	srcBlend = NameToSrcBlendMode(token);

	MatchToken(src, ",");
	if (!src.ReadToken(&token)) {
		return;
	}
	dstBlend = NameToDstBlendMode(token);

	stage->drawStateBits = srcBlend | dstBlend;
}

/*
================
idMaterial::ParseVertexParm

If there is a single value, it will be repeated across all elements
If there are two values, 3 = 0.0, 4 = 1.0
if there are three values, 4 = 1.0
================
*/
void idMaterial::ParseVertexParm(idLexer& src, newShaderStage_t* newStage) {
	idToken				token;

	src.ReadTokenOnLine(&token);
	int	parm = token.GetIntValue();
	if (!token.IsNumeric() || parm < 0 || parm >= MAX_VERTEX_PARMS) {
		common->Warning("bad vertexParm number\n");
		SetMaterialFlag(MF_DEFAULTED);
		return;
	}
	if (parm >= newStage->numVertexParms) {
		newStage->numVertexParms = parm + 1;
	}

	newStage->vertexParms[parm][0] = ParseExpression(src);

	src.ReadTokenOnLine(&token);
	if (!token[0] || token.Icmp(",")) {
		newStage->vertexParms[parm][1] =
			newStage->vertexParms[parm][2] =
			newStage->vertexParms[parm][3] = newStage->vertexParms[parm][0];
		return;
	}

	newStage->vertexParms[parm][1] = ParseExpression(src);

	src.ReadTokenOnLine(&token);
	if (!token[0] || token.Icmp(",")) {
		newStage->vertexParms[parm][2] = GetExpressionConstant(0);
		newStage->vertexParms[parm][3] = GetExpressionConstant(1);
		return;
	}

	newStage->vertexParms[parm][2] = ParseExpression(src);

	src.ReadTokenOnLine(&token);
	if (!token[0] || token.Icmp(",")) {
		newStage->vertexParms[parm][3] = GetExpressionConstant(1);
		return;
	}

	newStage->vertexParms[parm][3] = ParseExpression(src);
}


#if Q4_PORT_ENABLE_FRAGMENT_PARMS
/*
================
idMaterial::ParseFragmentParm
================
*/
void idMaterial::ParseFragmentParm(idLexer& src, newShaderStage_t* newStage) {
	idToken token;

	src.ReadTokenOnLine(&token);
	int parm = token.GetIntValue();
	if (!token.IsNumeric() || parm < 0 || parm >= MAX_FRAGMENT_PARMS) {
		common->Warning("bad fragmentParm number\n");
		SetMaterialFlag(MF_DEFAULTED);
		return;
	}
	if (parm >= newStage->numFragmentParms) {
		newStage->numFragmentParms = parm + 1;
	}

	newStage->fragmentParms[parm][0] = ParseExpression(src);

	src.ReadTokenOnLine(&token);
	if (!token[0] || token.Icmp(",")) {
		newStage->fragmentParms[parm][1] =
			newStage->fragmentParms[parm][2] =
			newStage->fragmentParms[parm][3] = newStage->fragmentParms[parm][0];
		return;
	}

	newStage->fragmentParms[parm][1] = ParseExpression(src);

	src.ReadTokenOnLine(&token);
	if (!token[0] || token.Icmp(",")) {
		newStage->fragmentParms[parm][2] = GetExpressionConstant(0);
		newStage->fragmentParms[parm][3] = GetExpressionConstant(1);
		return;
	}

	newStage->fragmentParms[parm][2] = ParseExpression(src);

	src.ReadTokenOnLine(&token);
	if (!token[0] || token.Icmp(",")) {
		newStage->fragmentParms[parm][3] = GetExpressionConstant(1);
		return;
	}

	newStage->fragmentParms[parm][3] = ParseExpression(src);
}
#endif

/*
================
idMaterial::ParseFragmentMap
================
*/
void idMaterial::ParseFragmentMap(idLexer& src, newShaderStage_t* newStage) {
	const char* str = NULL;
	textureFilter_t		tf;
	textureRepeat_t		trp;
	textureDepth_t		td;
	cubeFiles_t			cubeMap;
	bool				allowPicmip;
	idToken				token;

	tf = TF_DEFAULT;
	trp = TR_REPEAT;
	td = TD_DEFAULT;
	allowPicmip = true;
	cubeMap = CF_2D;

	src.ReadTokenOnLine(&token);
	int	unit = token.GetIntValue();
	if (!token.IsNumeric() || unit < 0 || unit >= MAX_FRAGMENT_IMAGES) {
		common->Warning("bad fragmentMap number\n");
		SetMaterialFlag(MF_DEFAULTED);
		return;
	}

	// unit 1 is the normal map.. make sure it gets flagged as the proper depth
	if (unit == 1) {
		td = TD_BUMP;
	}

	if (unit >= newStage->numFragmentProgramImages) {
		newStage->numFragmentProgramImages = unit + 1;
	}

	while (1) {
		src.ReadTokenOnLine(&token);

		if (!token.Icmp("cubeMap")) {
			cubeMap = CF_NATIVE;
			continue;
		}
		if (!token.Icmp("cameraCubeMap")) {
			cubeMap = CF_CAMERA;
			continue;
		}
		if (!token.Icmp("nearest")) {
			tf = TF_NEAREST;
			continue;
		}
		if (!token.Icmp("linear")) {
			tf = TF_LINEAR;
			continue;
		}
		if (!token.Icmp("clamp")) {
			trp = TR_CLAMP;
			continue;
		}
		if (!token.Icmp("noclamp")) {
			trp = TR_REPEAT;
			continue;
		}
		if (!token.Icmp("zeroclamp")) {
			trp = TR_CLAMP_TO_ZERO;
			continue;
		}
		if (!token.Icmp("alphazeroclamp")) {
			trp = TR_CLAMP_TO_ZERO_ALPHA;
			continue;
		}
#ifdef TR_MIRRORED_REPEAT
		if (!token.Icmp("mirroredrepeat")) {
			trp = TR_MIRRORED_REPEAT;
			continue;
		}
#endif
		if (!token.Icmp("nomips")) {
			// Doom 3's ImageFromFile() path here does not expose Quake 4's imageFlags.
			// Accept the token for script compatibility.
			continue;
		}
		if (!token.Icmp("forceHighQuality")) {
			td = TD_HIGH_QUALITY;
			continue;
		}

		if (!token.Icmp("uncompressed") || !token.Icmp("highquality")) {
			if (!globalImages->image_ignoreHighQuality.GetInteger()) {
				td = TD_HIGH_QUALITY;
			}
			continue;
		}
		if (!token.Icmp("nopicmip")) {
			allowPicmip = false;
			continue;
		}

		// Quake 4 built-in fragment bindings. Doom 3 does not have the runtime
		// binding table, but we can still resolve the ones that map directly.
		if (!token.Icmp("lightfalloffImage")) {
			newStage->fragmentProgramImages[unit] = lightFalloffImage ? lightFalloffImage : globalImages->defaultImage;
			return;
		}
		if (!token.Icmp("lightImage")) {
			// Placeholder until a light-image runtime binding is wired up.
			newStage->fragmentProgramImages[unit] = globalImages->defaultImage;
			return;
		}
		if (!token.Icmp("ambientNormalMap") ||
			!token.Icmp("normalCubeMap") ||
			!token.Icmp("specularTableImage")) {
			newStage->fragmentProgramImages[unit] = globalImages->defaultImage;
			return;
		}

		// assume anything else is the image name
		src.UnreadToken(&token);
		break;
	}

	str = R_ParsePastImageProgram(src);

	newStage->fragmentProgramImages[unit] =
		globalImages->ImageFromFile(str, tf, allowPicmip, trp, td, cubeMap);
	if (!newStage->fragmentProgramImages[unit]) {
		newStage->fragmentProgramImages[unit] = globalImages->defaultImage;
	}
}

/*
===============
idMaterial::MultiplyTextureMatrix
===============
*/
void idMaterial::MultiplyTextureMatrix(textureStage_t* ts, int registers[2][3]) {
	int		old[2][3];

	if (!ts->hasMatrix) {
		ts->hasMatrix = true;
		memcpy(ts->matrix, registers, sizeof(ts->matrix));
		return;
	}

	memcpy(old, ts->matrix, sizeof(old));

	// multiply the two maticies
	ts->matrix[0][0] = EmitOp(
		EmitOp(old[0][0], registers[0][0], OP_TYPE_MULTIPLY),
		EmitOp(old[0][1], registers[1][0], OP_TYPE_MULTIPLY), OP_TYPE_ADD);
	ts->matrix[0][1] = EmitOp(
		EmitOp(old[0][0], registers[0][1], OP_TYPE_MULTIPLY),
		EmitOp(old[0][1], registers[1][1], OP_TYPE_MULTIPLY), OP_TYPE_ADD);
	ts->matrix[0][2] = EmitOp(
		EmitOp(
			EmitOp(old[0][0], registers[0][2], OP_TYPE_MULTIPLY),
			EmitOp(old[0][1], registers[1][2], OP_TYPE_MULTIPLY), OP_TYPE_ADD),
		old[0][2], OP_TYPE_ADD);

	ts->matrix[1][0] = EmitOp(
		EmitOp(old[1][0], registers[0][0], OP_TYPE_MULTIPLY),
		EmitOp(old[1][1], registers[1][0], OP_TYPE_MULTIPLY), OP_TYPE_ADD);
	ts->matrix[1][1] = EmitOp(
		EmitOp(old[1][0], registers[0][1], OP_TYPE_MULTIPLY),
		EmitOp(old[1][1], registers[1][1], OP_TYPE_MULTIPLY), OP_TYPE_ADD);
	ts->matrix[1][2] = EmitOp(
		EmitOp(
			EmitOp(old[1][0], registers[0][2], OP_TYPE_MULTIPLY),
			EmitOp(old[1][1], registers[1][2], OP_TYPE_MULTIPLY), OP_TYPE_ADD),
		old[1][2], OP_TYPE_ADD);

}

/*
=================
idMaterial::ParseStage

An open brace has been parsed


{
	if <expression>
	map <imageprogram>
	"nearest" "linear" "clamp" "zeroclamp" "uncompressed" "highquality" "nopicmip"
	scroll, scale, rotate
}

=================
*/
bool idMaterial::ParseStage(idLexer& src, const textureRepeat_t trpDefault) {
	idToken				token;
	const char* str;
	shaderStage_t* ss;
	textureStage_t* ts;
	textureFilter_t		tf;
	textureRepeat_t		trp;
	textureDepth_t		td;
	cubeFiles_t			cubeMap;
	bool				allowPicmip;
	char				imageName[MAX_IMAGE_NAME];
	int					a, b;
	int					matrix[2][3];
	newShaderStage_t	newStage;

#if Q4_PORT_ENABLE_GLSL_STAGES
	rvNewShaderStage* q4Stage = NULL;
#endif

	if (numStages >= MAX_SHADER_STAGES) {
		SetMaterialFlag(MF_DEFAULTED);
		common->Warning("material '%s' exceeded %i stages", GetName(), MAX_SHADER_STAGES);
	}

	tf = TF_DEFAULT;
	trp = trpDefault;
	td = TD_DEFAULT;
	allowPicmip = true;
	cubeMap = CF_2D;

	imageName[0] = 0;

	memset(&newStage, 0, sizeof(newStage));

	ss = &pd->parseStages[numStages];
	ts = &ss->texture;

	ClearStage(ss);

	while (1) {
		if (TestMaterialFlag(MF_DEFAULTED)) {	// we have a parse error
			return false;
		}
		if (!src.ExpectAnyToken(&token)) {
			SetMaterialFlag(MF_DEFAULTED);
			return false;
		}

		// the close brace for the entire material ends the draw block
		if (token == "}") {
			break;
		}

		// Added for stage naming in editors
		if (!token.Icmp("name")) {
			src.SkipRestOfLine();
			continue;
		}

		// image options
		if (!token.Icmp("blend")) {
			ParseBlend(src, ss);
			continue;
		}

		if (!token.Icmp("map")) {
			str = R_ParsePastImageProgram(src);
			idStr::Copynz(imageName, str, sizeof(imageName));
			continue;
		}

		if (!token.Icmp("remoteRenderMap")) {
			ts->dynamic = DI_REMOTE_RENDER;
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			continue;
		}

		if (!token.Icmp("mirrorRenderMap")) {
			ts->dynamic = DI_MIRROR_RENDER;
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			ts->texgen = TG_SCREEN;
			continue;
		}

		if (!token.Icmp("reflectionRenderMap")) {
#ifdef DI_REFLECTION_RENDER
			ts->dynamic = DI_REFLECTION_RENDER;
#else
			ts->dynamic = DI_REMOTE_RENDER;
#endif
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			continue;
		}

		if (!token.Icmp("refractionRenderMap")) {
#ifdef DI_REFRACTION_RENDER
			ts->dynamic = DI_REFRACTION_RENDER;
#else
			ts->dynamic = DI_REMOTE_RENDER;
#endif
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			ts->texgen = TG_SCREEN;
			continue;
		}

		if (!token.Icmp("cubeRenderMap")) {
			int cubeSize = src.ParseInt();
#ifdef DI_CUBE_RENDER
			ts->dynamic = DI_CUBE_RENDER;
#else
			ts->dynamic = DI_REMOTE_RENDER;
#endif
			ts->width = cubeSize;
			ts->height = cubeSize;
			ts->texgen = TG_REFLECT_CUBE;
			continue;
		}

		if (!token.Icmp("xrayRenderMap")) {
#ifdef DI_XRAY_RENDER
			ts->dynamic = DI_XRAY_RENDER;
#else
			ts->dynamic = DI_MIRROR_RENDER;
#endif
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			ts->texgen = TG_SCREEN;
			continue;
		}
		if (!token.Icmp("screen")) {
			ts->texgen = TG_SCREEN;
			continue;
		}
		if (!token.Icmp("screen2")) {
#ifdef TG_SCREEN2
			ts->texgen = TG_SCREEN2;
#else
			ts->texgen = TG_SCREEN;
#endif
			continue;
		}
		if (!token.Icmp("glassWarp")) {
#ifdef TG_GLASSWARP
			ts->texgen = TG_GLASSWARP;
#else
			ts->texgen = TG_SCREEN;
#endif
			continue;
		}

		if (!token.Icmp("videomap")) {
			// note that videomaps will always be in clamp mode, so texture
			// coordinates had better be in the 0 to 1 range
			if (!src.ReadToken(&token)) {
				common->Warning("missing parameter for 'videoMap' keyword in material '%s'", GetName());
				continue;
			}
			bool loop = false;
			if (!token.Icmp("loop")) {
				loop = true;
				if (!src.ReadToken(&token)) {
					common->Warning("missing parameter for 'videoMap' keyword in material '%s'", GetName());
					continue;
				}
			}
			ts->cinematic = idCinematic::Alloc();
			ts->cinematic->InitFromFile(token.c_str(), loop);
			continue;
		}

		if (!token.Icmp("soundmap")) {
			if (!src.ReadToken(&token)) {
				common->Warning("missing parameter for 'soundmap' keyword in material '%s'", GetName());
				continue;
			}
			ts->cinematic = new idSndWindow();
			ts->cinematic->InitFromFile(token.c_str(), true);
			continue;
		}

		if (!token.Icmp("cubeMap")) {
			str = R_ParsePastImageProgram(src);
			idStr::Copynz(imageName, str, sizeof(imageName));
			cubeMap = CF_NATIVE;
			continue;
		}

		if (!token.Icmp("cameraCubeMap")) {
			str = R_ParsePastImageProgram(src);
			idStr::Copynz(imageName, str, sizeof(imageName));
			cubeMap = CF_CAMERA;
			continue;
		}

		if (!token.Icmp("ignoreAlphaTest")) {
			ss->ignoreAlphaTest = true;
			continue;
		}
		if (!token.Icmp("nearest")) {
			tf = TF_NEAREST;
			continue;
		}
		if (!token.Icmp("linear")) {
			tf = TF_LINEAR;
			continue;
		}
		if (!token.Icmp("clamp")) {
			trp = TR_CLAMP;
			continue;
		}
		if (!token.Icmp("noclamp")) {
			trp = TR_REPEAT;
			continue;
		}
		if (!token.Icmp("zeroclamp")) {
			trp = TR_CLAMP_TO_ZERO;
			continue;
		}
		if (!token.Icmp("alphazeroclamp")) {
			trp = TR_CLAMP_TO_ZERO_ALPHA;
			continue;
		}
#ifdef TR_MIRRORED_REPEAT
		if (!token.Icmp("mirroredrepeat")) {
			trp = TR_MIRRORED_REPEAT;
			continue;
		}
#endif
		if (!token.Icmp("nomips")) {
			// Doom 3 does not expose the Quake 4 nomips image flag here.
			continue;
		}
		if (!token.Icmp("uncompressed") || !token.Icmp("highquality")) {
			if (!globalImages->image_ignoreHighQuality.GetInteger()) {
				td = TD_HIGH_QUALITY;
			}
			continue;
		}
		if (!token.Icmp("forceHighQuality")) {
			td = TD_HIGH_QUALITY;
			continue;
		}
		if (!token.Icmp("nopicmip")) {
			allowPicmip = false;
			continue;
		}
		if (!token.Icmp("vertexColor")) {
			ss->vertexColor = SVC_MODULATE;
			continue;
		}
		if (!token.Icmp("inverseVertexColor")) {
			ss->vertexColor = SVC_INVERSE_MODULATE;
			continue;
		}

		// privatePolygonOffset
		else if (!token.Icmp("privatePolygonOffset")) {
			if (!src.ReadTokenOnLine(&token)) {
				ss->privatePolygonOffset = 1;
				continue;
			}
			// explict larger (or negative) offset
			src.UnreadToken(&token);
			ss->privatePolygonOffset = src.ParseFloat();
			continue;
		}

		// texture coordinate generation
		if (!token.Icmp("texGen")) {
			src.ExpectAnyToken(&token);
			if (!token.Icmp("normal")) {
				ts->texgen = TG_DIFFUSE_CUBE;
			}
			else if (!token.Icmp("reflect")) {
				ts->texgen = TG_REFLECT_CUBE;
			}
			else if (!token.Icmp("skybox")) {
				ts->texgen = TG_SKYBOX_CUBE;
			}
			else if (!token.Icmp("wobbleSky")) {
				ts->texgen = TG_WOBBLESKY_CUBE;
				texGenRegisters[0] = ParseExpression(src);
				texGenRegisters[1] = ParseExpression(src);
				texGenRegisters[2] = ParseExpression(src);
#ifdef TG_POT_CORRECTION
			}
			else if (!token.Icmp("potCorrection")) {
				ts->texgen = TG_POT_CORRECTION;
#endif
			}
			else {
				common->Warning("bad texGen '%s' in material %s", token.c_str(), GetName());
				SetMaterialFlag(MF_DEFAULTED);
			}
			continue;
		}
		if (!token.Icmp("scroll") || !token.Icmp("translate")) {
			a = ParseExpression(src);
			MatchToken(src, ",");
			b = ParseExpression(src);
			matrix[0][0] = GetExpressionConstant(1);
			matrix[0][1] = GetExpressionConstant(0);
			matrix[0][2] = a;
			matrix[1][0] = GetExpressionConstant(0);
			matrix[1][1] = GetExpressionConstant(1);
			matrix[1][2] = b;

			MultiplyTextureMatrix(ts, matrix);
			continue;
		}
		if (!token.Icmp("scale")) {
			a = ParseExpression(src);
			MatchToken(src, ",");
			b = ParseExpression(src);
			// this just scales without a centering
			matrix[0][0] = a;
			matrix[0][1] = GetExpressionConstant(0);
			matrix[0][2] = GetExpressionConstant(0);
			matrix[1][0] = GetExpressionConstant(0);
			matrix[1][1] = b;
			matrix[1][2] = GetExpressionConstant(0);

			MultiplyTextureMatrix(ts, matrix);
			continue;
		}
		if (!token.Icmp("centerScale")) {
			a = ParseExpression(src);
			MatchToken(src, ",");
			b = ParseExpression(src);
			// this subtracts 0.5, then scales, then adds 0.5
			matrix[0][0] = a;
			matrix[0][1] = GetExpressionConstant(0);
			matrix[0][2] = EmitOp(GetExpressionConstant(0.5f), EmitOp(GetExpressionConstant(0.5f), a, OP_TYPE_MULTIPLY), OP_TYPE_SUBTRACT);
			matrix[1][0] = GetExpressionConstant(0);
			matrix[1][1] = b;
			matrix[1][2] = EmitOp(GetExpressionConstant(0.5f), EmitOp(GetExpressionConstant(0.5f), b, OP_TYPE_MULTIPLY), OP_TYPE_SUBTRACT);

			MultiplyTextureMatrix(ts, matrix);
			continue;
		}
		if (!token.Icmp("shear")) {
			a = ParseExpression(src);
			MatchToken(src, ",");
			b = ParseExpression(src);
			// this subtracts 0.5, then shears, then adds 0.5
			matrix[0][0] = GetExpressionConstant(1);
			matrix[0][1] = a;
			matrix[0][2] = EmitOp(GetExpressionConstant(-0.5f), a, OP_TYPE_MULTIPLY);
			matrix[1][0] = b;
			matrix[1][1] = GetExpressionConstant(1);
			matrix[1][2] = EmitOp(GetExpressionConstant(-0.5f), b, OP_TYPE_MULTIPLY);

			MultiplyTextureMatrix(ts, matrix);
			continue;
		}
		if (!token.Icmp("rotate")) {
			const idDeclTable* table;
			int		sinReg, cosReg;

			// in cycles
			a = ParseExpression(src);

			table = static_cast<const idDeclTable*>(declManager->FindType(DECL_TABLE, "sinTable", false));
			if (!table) {
				common->Warning("no sinTable for rotate defined");
				SetMaterialFlag(MF_DEFAULTED);
				return false;
			}
			sinReg = EmitOp(table->Index(), a, OP_TYPE_TABLE);

			table = static_cast<const idDeclTable*>(declManager->FindType(DECL_TABLE, "cosTable", false));
			if (!table) {
				common->Warning("no cosTable for rotate defined");
				SetMaterialFlag(MF_DEFAULTED);
				return false;
			}
			cosReg = EmitOp(table->Index(), a, OP_TYPE_TABLE);

			// this subtracts 0.5, then rotates, then adds 0.5
			matrix[0][0] = cosReg;
			matrix[0][1] = EmitOp(GetExpressionConstant(0), sinReg, OP_TYPE_SUBTRACT);
			matrix[0][2] = EmitOp(EmitOp(EmitOp(GetExpressionConstant(-0.5f), cosReg, OP_TYPE_MULTIPLY),
				EmitOp(GetExpressionConstant(0.5f), sinReg, OP_TYPE_MULTIPLY), OP_TYPE_ADD),
				GetExpressionConstant(0.5f), OP_TYPE_ADD);

			matrix[1][0] = sinReg;
			matrix[1][1] = cosReg;
			matrix[1][2] = EmitOp(EmitOp(EmitOp(GetExpressionConstant(-0.5f), sinReg, OP_TYPE_MULTIPLY),
				EmitOp(GetExpressionConstant(-0.5f), cosReg, OP_TYPE_MULTIPLY), OP_TYPE_ADD),
				GetExpressionConstant(0.5f), OP_TYPE_ADD);

			MultiplyTextureMatrix(ts, matrix);
			continue;
		}

		// color mask options
		if (!token.Icmp("maskRed")) {
			ss->drawStateBits |= GLS_REDMASK;
			continue;
		}
		if (!token.Icmp("maskGreen")) {
			ss->drawStateBits |= GLS_GREENMASK;
			continue;
		}
		if (!token.Icmp("maskBlue")) {
			ss->drawStateBits |= GLS_BLUEMASK;
			continue;
		}
		if (!token.Icmp("maskAlpha")) {
			ss->drawStateBits |= GLS_ALPHAMASK;
			continue;
		}
		if (!token.Icmp("maskColor")) {
			ss->drawStateBits |= GLS_COLORMASK;
			continue;
		}
		if (!token.Icmp("maskDepth")) {
			ss->drawStateBits |= GLS_DEPTHMASK;
			continue;
		}
		if (!token.Icmp("alphaTest")) {
			ss->hasAlphaTest = true;
#if Q4_PORT_ENABLE_ALPHA_FUNC
			if (!ss->hasAlphaFunc) {
				ss->alphaTestMode = GL_GREATER;
			}
#endif
			ss->alphaTestRegister = ParseExpression(src);
			coverage = MC_PERFORATED;
			continue;
		}
		if (!token.Icmp("alphaFunc")) {
			// Quake 4 adds alpha comparison mode selection. Doom 3 only exposes the
			// alpha test threshold register in stock shaderStage_t, so accept the
			// token for script compatibility and keep the default comparison unless
			// the extra fields are wired up.
			idToken alphaToken;
			if (src.ReadToken(&alphaToken)) {
#if Q4_PORT_ENABLE_ALPHA_FUNC
				ss->hasAlphaFunc = true;
				if (!alphaToken.Icmp("less")) {
					ss->alphaTestMode = GL_LESS;
				}
				else if (!alphaToken.Icmp("equal")) {
					ss->alphaTestMode = GL_EQUAL;
				}
				else {
					ss->alphaTestMode = GL_GREATER;
				}
#endif
			}
			continue;
		}

		// shorthand for 2D modulated
		if (!token.Icmp("colored")) {
			ss->color.registers[0] = EXP_REG_PARM0;
			ss->color.registers[1] = EXP_REG_PARM1;
			ss->color.registers[2] = EXP_REG_PARM2;
			ss->color.registers[3] = EXP_REG_PARM3;
			pd->registersAreConstant = false;
			continue;
		}

		if (!token.Icmp("color")) {
			ss->color.registers[0] = ParseExpression(src);
			MatchToken(src, ",");
			ss->color.registers[1] = ParseExpression(src);
			MatchToken(src, ",");
			ss->color.registers[2] = ParseExpression(src);
			MatchToken(src, ",");
			ss->color.registers[3] = ParseExpression(src);
			continue;
		}
		if (!token.Icmp("red")) {
			ss->color.registers[0] = ParseExpression(src);
			continue;
		}
		if (!token.Icmp("green")) {
			ss->color.registers[1] = ParseExpression(src);
			continue;
		}
		if (!token.Icmp("blue")) {
			ss->color.registers[2] = ParseExpression(src);
			continue;
		}
		if (!token.Icmp("alpha")) {
			ss->color.registers[3] = ParseExpression(src);
			continue;
		}
		if (!token.Icmp("rgb")) {
			ss->color.registers[0] = ss->color.registers[1] =
				ss->color.registers[2] = ParseExpression(src);
			continue;
		}
		if (!token.Icmp("rgba")) {
			ss->color.registers[0] = ss->color.registers[1] =
				ss->color.registers[2] = ss->color.registers[3] = ParseExpression(src);
			continue;
		}

		if (!token.Icmp("if")) {
			ss->conditionRegister = ParseExpression(src);
			continue;
		}
		if (!token.Icmp("program")) {
			if (src.ReadTokenOnLine(&token)) {
				newStage.vertexProgram = R_FindARBProgram(GL_VERTEX_PROGRAM_ARB, token.c_str());
				newStage.fragmentProgram = R_FindARBProgram(GL_FRAGMENT_PROGRAM_ARB, token.c_str());
			}
			continue;
		}
#if Q4_PORT_ENABLE_GLSL_STAGES
		if (!token.Icmp("glslProgram")) {
			if (q4Stage == NULL) {
				q4Stage = new rvGLSLShaderStage();
			}
			if (!q4Stage->ParseProgram(src, this)) {
				SetMaterialFlag(MF_DEFAULTED);
			}
			continue;
		}
		if (!token.Icmp("shaderParm")) {
			if (q4Stage == NULL) {
				common->Warning("ParseStage: shaderParm set before shader type declared");
				SetMaterialFlag(MF_DEFAULTED);
				return false;
			}
			q4Stage->ParseShaderParm(src, this);
			continue;
		}
		if (!token.Icmp("shaderTexture")) {
			if (q4Stage == NULL) {
				common->Warning("ParseStage: shaderTexture set before shader type declared");
				SetMaterialFlag(MF_DEFAULTED);
				return false;
			}
			q4Stage->ParseTextureParm(src, this, trpDefault);
			continue;
		}
#else
		if (!token.Icmp("glslProgram") || !token.Icmp("shaderParm") || !token.Icmp("shaderTexture")) {
			// Keep the token stream in sync even without the Quake 4 GLSL stage backend.
			src.SkipRestOfLine();
			continue;
		}
#endif
		if (!token.Icmp("fragmentProgram")) {
			if (src.ReadTokenOnLine(&token)) {
				newStage.fragmentProgram = R_FindARBProgram(GL_FRAGMENT_PROGRAM_ARB, token.c_str());
			}
			continue;
		}
		if (!token.Icmp("vertexProgram")) {
			if (src.ReadTokenOnLine(&token)) {
				newStage.vertexProgram = R_FindARBProgram(GL_VERTEX_PROGRAM_ARB, token.c_str());
			}
			continue;
		}
		if (!token.Icmp("megaTexture")) {
			if (src.ReadTokenOnLine(&token)) {
				newStage.megaTexture = new idMegaTexture;
				if (!newStage.megaTexture->InitFromMegaFile(token.c_str())) {
					delete newStage.megaTexture;
					SetMaterialFlag(MF_DEFAULTED);
					continue;
				}
				newStage.vertexProgram = R_FindARBProgram(GL_VERTEX_PROGRAM_ARB, "megaTexture.vfp");
				newStage.fragmentProgram = R_FindARBProgram(GL_FRAGMENT_PROGRAM_ARB, "megaTexture.vfp");
				continue;
			}
		}


		if (!token.Icmp("vertexParm")) {
			ParseVertexParm(src, &newStage);
			continue;
		}

#if Q4_PORT_ENABLE_FRAGMENT_PARMS
		if (!token.Icmp("fragmentParm")) {
			ParseFragmentParm(src, &newStage);
			continue;
		}
#else
		if (!token.Icmp("fragmentParm")) {
			src.SkipRestOfLine();
			continue;
		}
#endif

		if (!token.Icmp("fragmentMap")) {
			ParseFragmentMap(src, &newStage);
			continue;
		}

		common->Warning("unknown token '%s' in material '%s'", token.c_str(), GetName());
		SetMaterialFlag(MF_DEFAULTED);
		return false;
	}


	// if we are using newStage, allocate a copy of it
	if (newStage.fragmentProgram || newStage.vertexProgram) {
		ss->newStage = (newShaderStage_t*)Mem_Alloc(sizeof(newStage));
		*(ss->newStage) = newStage;
	}

#if Q4_PORT_ENABLE_GLSL_STAGES
	if (q4Stage != NULL) {
		ss->newShaderStage = q4Stage;
		q4Stage->Resolve();
	}
#endif

	// successfully parsed a stage
	numStages++;
	ss->mNumStageRegisters = numRegisters - ss->mStageRegisterStart;
	ss->mNumStageOps = numOps - ss->mStageOpsStart;

	// select a compressed depth based on what the stage is
	if (td == TD_DEFAULT) {
		switch (ss->lighting) {
		case SL_BUMP:
			td = TD_BUMP;
			break;
		case SL_DIFFUSE:
			td = TD_DIFFUSE;
			break;
		case SL_SPECULAR:
			td = TD_SPECULAR;
			break;
		default:
			break;
		}
	}

	// now load the image with all the parms we parsed
	if (imageName[0]) {
		ts->image = globalImages->ImageFromFile(imageName, tf, allowPicmip, trp, td, cubeMap);
		if (!ts->image) {
			ts->image = globalImages->defaultImage;
		}
#if Q4_PORT_ENABLE_GLSL_STAGES
	}
	else if (!ts->cinematic && !ts->dynamic && !ss->newStage && !ss->newShaderStage) {
#else
	}
	else if (!ts->cinematic && !ts->dynamic && !ss->newStage) {
#endif
		common->Warning("material '%s' had stage with no image", GetName());
		ts->image = globalImages->defaultImage;
	}

	return !TestMaterialFlag(MF_DEFAULTED);
}

/*
===============
idMaterial::ParseDeform
===============
*/
void idMaterial::ParseDeform(idLexer& src) {
	idToken token;

	if (!src.ExpectAnyToken(&token)) {
		return;
	}

	if (!token.Icmp("sprite")) {
		deform = DFRM_SPRITE;
		cullType = CT_TWO_SIDED;
		SetMaterialFlag(MF_NOSHADOWS);
		return;
	}
	if (!token.Icmp("rectsprite")) {
#ifdef DFRM_RECTSPRITE
		deform = DFRM_RECTSPRITE;
#else
		deform = DFRM_SPRITE;
#endif
		cullType = CT_TWO_SIDED;
		SetMaterialFlag(MF_NOSHADOWS);
		return;
	}
	if (!token.Icmp("tube")) {
		deform = DFRM_TUBE;
		cullType = CT_TWO_SIDED;
		SetMaterialFlag(MF_NOSHADOWS);
		return;
	}
	if (!token.Icmp("flare")) {
		deform = DFRM_FLARE;
		cullType = CT_TWO_SIDED;
		deformRegisters[0] = ParseExpression(src);
		SetMaterialFlag(MF_NOSHADOWS);
		return;
	}
	if (!token.Icmp("expand")) {
		deform = DFRM_EXPAND;
		deformRegisters[0] = ParseExpression(src);
		return;
	}
	if (!token.Icmp("move")) {
		deform = DFRM_MOVE;
		deformRegisters[0] = ParseExpression(src);
		return;
	}
	if (!token.Icmp("turbulent")) {
		deform = DFRM_TURB;

		if (!src.ExpectAnyToken(&token)) {
			src.Warning("deform particle missing particle name");
			SetMaterialFlag(MF_DEFAULTED);
			return;
		}
		deformDecl = declManager->FindType(DECL_TABLE, token.c_str(), true);

		deformRegisters[0] = ParseExpression(src);
		deformRegisters[1] = ParseExpression(src);
		deformRegisters[2] = ParseExpression(src);
		return;
	}
	if (!token.Icmp("eyeBall")) {
		deform = DFRM_EYEBALL;
		return;
	}
#if defined(DFRM_PARTICLE) && defined(DECL_PARTICLE)
	if (!token.Icmp("particle")) {
		deform = DFRM_PARTICLE;
		if (!src.ExpectAnyToken(&token)) {
			src.Warning("deform particle missing particle name");
			SetMaterialFlag(MF_DEFAULTED);
			return;
		}
		deformDecl = declManager->FindType(DECL_PARTICLE, token.c_str(), true);
		return;
	}
#endif
#if defined(DFRM_PARTICLE2) && defined(DECL_PARTICLE)
	if (!token.Icmp("particle2")) {
		deform = DFRM_PARTICLE2;
		if (!src.ExpectAnyToken(&token)) {
			src.Warning("deform particle missing particle name");
			SetMaterialFlag(MF_DEFAULTED);
			return;
		}
		deformDecl = declManager->FindType(DECL_PARTICLE, token.c_str(), true);
		return;
	}
#endif
	src.Warning("Bad deform type '%s'", token.c_str());
	SetMaterialFlag(MF_DEFAULTED);
}


/*
==============
idMaterial::AddImplicitStages

If a material has diffuse or specular stages without any
bump stage, add an implicit _flat bumpmap stage.

If a material has a bump stage but no diffuse or specular
stage, add a _white diffuse stage.

It is valid to have either a diffuse or specular without the other.

It is valid to have a reflection map and a bump map for bumpy reflection
==============
*/
bool idMaterial::AddImplicitStages(const textureRepeat_t trpDefault /* = TR_REPEAT  */) {
	char	buffer[1024];
	idLexer		newSrc;
	bool hasDiffuse = false;
	bool hasSpecular = false;
	bool hasBump = false;
	bool hasReflection = false;

	for (int i = 0; i < numStages; i++) {
		if (pd->parseStages[i].lighting == SL_BUMP) {
			hasBump = true;
		}
		if (pd->parseStages[i].lighting == SL_DIFFUSE) {
			hasDiffuse = true;
		}
		if (pd->parseStages[i].lighting == SL_SPECULAR) {
			hasSpecular = true;
		}
		if (pd->parseStages[i].texture.texgen == TG_REFLECT_CUBE) {
			hasReflection = true;
		}
	}

	// if it doesn't have an interaction at all, don't add anything
	if (!hasBump && !hasDiffuse && !hasSpecular) {
		return true;
	}

	if (numStages == MAX_SHADER_STAGES) {
		return true;
	}

	if (!hasBump) {
		idStr::snPrintf(buffer, sizeof(buffer), "blend bumpmap\nmap _flat\n}\n");
		newSrc.LoadMemory(buffer, strlen(buffer), "bumpmap", 1);
		newSrc.SetFlags(LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES);
		if (!ParseStage(newSrc, trpDefault)) {
			newSrc.FreeSource();
			return false;
		}
		newSrc.FreeSource();
	}

	if (!hasDiffuse && !hasSpecular && !hasReflection) {
		idStr::snPrintf(buffer, sizeof(buffer), "blend diffusemap\nmap _white\n}\n");
		newSrc.LoadMemory(buffer, strlen(buffer), "diffusemap", 1);
		newSrc.SetFlags(LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES);
		if (!ParseStage(newSrc, trpDefault)) {
			newSrc.FreeSource();
			return false;
		}
		newSrc.FreeSource();
	}

	return true;
}

/*
===============
idMaterial::SortInteractionStages

The renderer expects bump, then diffuse, then specular
There can be multiple bump maps, followed by additional
diffuse and specular stages, which allows cross-faded bump mapping.

Ambient stages can be interspersed anywhere, but they are
ignored during interactions, and all the interaction
stages are ignored during ambient drawing.
===============
*/
void idMaterial::SortInteractionStages() {
	int		j;

	for (int i = 0; i < numStages; i = j) {
		// find the next bump map
		for (j = i + 1; j < numStages; j++) {
			if (pd->parseStages[j].lighting == SL_BUMP) {
				// if the very first stage wasn't a bumpmap,
				// this bumpmap is part of the first group
				if (pd->parseStages[i].lighting != SL_BUMP) {
					continue;
				}
				break;
			}
		}

		// bubble sort everything bump / diffuse / specular
		for (int l = 1; l < j - i; l++) {
			for (int k = i; k < j - l; k++) {
				if (pd->parseStages[k].lighting > pd->parseStages[k + 1].lighting) {
					shaderStage_t	temp;

					temp = pd->parseStages[k];
					pd->parseStages[k] = pd->parseStages[k + 1];
					pd->parseStages[k + 1] = temp;
				}
			}
		}
	}

}

/*
=================
idMaterial::ParseMaterial

The current text pointer is at the explicit text definition of the
Parse it into the global material variable. Later functions will optimize it.

If there is any error during parsing, defaultShader will be set.
=================
*/
bool idMaterial::ParseMaterial(idLexer& src) {
	idToken		token;
	int			s;
	char		buffer[1024];
	const char* str;
	idLexer		newSrc;
	int			i;

	s = 0;

	numOps = 0;
	numRegisters = EXP_REG_NUM_PREDEFINED;	// leave space for the parms to be copied in
	for (i = 0; i < numRegisters; i++) {
		pd->registerIsTemporary[i] = true;		// they aren't constants that can be folded
	}

	numStages = 0;

	textureRepeat_t	trpDefault = TR_REPEAT;		// allow a global setting for repeat

	while (1) {
		if (TestMaterialFlag(MF_DEFAULTED)) {	// we have a parse error
			return false;
		}
		if (!src.ExpectAnyToken(&token)) {
			SetMaterialFlag(MF_DEFAULTED);
			return false;
		}

		// end of material definition
		if (token == "}") {
			break;
		}
		else if (!token.Icmp("qer_editorimage")) {
			src.ReadTokenOnLine(&token);
			editorImageName = token.c_str();
			src.SkipRestOfLine();
			continue;
		}
		// description
		else if (!token.Icmp("description")) {
			src.ReadTokenOnLine(&token);
			desc = token.c_str();
			continue;
		}
		// check for the surface / content bit flags
		else if (CheckSurfaceParm(&token)) {
			continue;
		}

		// polygonOffset
		else if (!token.Icmp("polygonOffset")) {
			SetMaterialFlag(MF_POLYGONOFFSET);
			if (!src.ReadTokenOnLine(&token)) {
				polygonOffset = 1;
				continue;
			}
			// explict larger (or negative) offset
			polygonOffset = token.GetFloatValue();
			continue;
		}
		// noshadow
		else if (!token.Icmp("noShadows")) {
			SetMaterialFlag(MF_NOSHADOWS);
			continue;
		}
		else if (!token.Icmp("needCurrentRender")) {
			SetMaterialFlag(MF_NEED_CURRENT_RENDER);
			continue;
		}
		else if (!token.Icmp("suppressInSubview")) {
			suppressInSubview = true;
			continue;
		}
		else if (!token.Icmp("portalSky")) {
			portalSky = true;
			continue;
		}
		// noSelfShadow
		else if (!token.Icmp("noSelfShadow")) {
			SetMaterialFlag(MF_NOSELFSHADOW);
			continue;
		}
		// noPortalFog
		else if (!token.Icmp("noPortalFog")) {
			SetMaterialFlag(MF_NOPORTALFOG);
			continue;
		}
		// forceShadows allows nodraw surfaces to cast shadows
		else if (!token.Icmp("forceShadows")) {
			SetMaterialFlag(MF_FORCESHADOWS);
			continue;
		}
#if defined(MF_SKY)
		else if (!token.Icmp("sky")) {
			SetMaterialFlag(MF_SKY);
			continue;
		}
#else
		else if (!token.Icmp("sky")) {
			continue;
		}
#endif
#if Q4_PORT_ENABLE_MATERIAL_TYPES
		else if (!token.Icmp("materialType")) {
			src.ReadTokenOnLine(&token);
			materialType = reinterpret_cast<const rvDeclMatType*>(declManager->FindType(DECL_MATERIALTYPE, token.c_str(), true));
			continue;
		}
		else if (!token.Icmp("materialImage")) {
			src.ReadTokenOnLine(&token);
			materialTypeArrayName = token.c_str();
			materialTypeArray = MT_GetMaterialTypeArray(materialTypeArrayName, MTAWidth, MTAHeight);
			continue;
		}
#else
		else if (!token.Icmp("materialType") || !token.Icmp("materialImage")) {
			src.SkipRestOfLine();
			continue;
		}
#endif
#if Q4_PORT_ENABLE_PORTAL_FEATURES
		else if (!token.Icmp("portalDistanceNear")) {
			portalDistanceNear = src.ParseFloat();
			continue;
		}
		else if (!token.Icmp("portalDistanceFar")) {
			portalDistanceFar = src.ParseFloat();
			continue;
		}
		else if (!token.Icmp("portalImage")) {
			src.ReadTokenOnLine(&token);
			portalImage = globalImages->ImageFromFile(token.c_str(), TF_DEFAULT, true, TR_CLAMP, TD_DEFAULT);
			continue;
		}
#else
		else if (!token.Icmp("portalDistanceNear") || !token.Icmp("portalDistanceFar") || !token.Icmp("portalImage")) {
			src.SkipRestOfLine();
			continue;
		}
#endif
		// overlay / decal suppression
		else if (!token.Icmp("noOverlays")) {
			allowOverlays = false;
			continue;
		}
		// moster blood overlay forcing for alpha tested or translucent surfaces
		else if (!token.Icmp("forceOverlays")) {
			pd->forceOverlays = true;
			continue;
		}
		// translucent
		else if (!token.Icmp("translucent")) {
			coverage = MC_TRANSLUCENT;
			continue;
		}
		// global zero clamp
		else if (!token.Icmp("zeroclamp")) {
			trpDefault = TR_CLAMP_TO_ZERO;
			continue;
		}
		// global clamp
		else if (!token.Icmp("clamp")) {
			trpDefault = TR_CLAMP;
			continue;
		}
		// global alpha-zero clamp
		else if (!token.Icmp("alphazeroclamp")) {
			trpDefault = TR_CLAMP_TO_ZERO_ALPHA;
			continue;
		}
		// forceOpaque is used for skies-behind-windows
		else if (!token.Icmp("forceOpaque")) {
			coverage = MC_OPAQUE;
			continue;
		}
		// twoSided
		else if (!token.Icmp("twoSided")) {
			cullType = CT_TWO_SIDED;
			// twoSided implies no-shadows, because the shadow
			// volume would be coplanar with the surface, giving depth fighting
			SetMaterialFlag(MF_NOSHADOWS);
		}
		// backSided
		else if (!token.Icmp("backSided")) {
			cullType = CT_BACK_SIDED;
			// the shadow code doesn't handle this, so just disable shadows.
			SetMaterialFlag(MF_NOSHADOWS);
		}
		// foglight
		else if (!token.Icmp("fogLight")) {
			fogLight = true;
			continue;
		}
		// blendlight
		else if (!token.Icmp("blendLight")) {
			blendLight = true;
			continue;
		}
		// ambientLight
		else if (!token.Icmp("ambientLight")) {
			ambientLight = true;
			continue;
		}
		// mirror
		else if (!token.Icmp("mirror")) {
			sort = SS_SUBVIEW;
			coverage = MC_OPAQUE;
			continue;
		}
		// noFog
		else if (!token.Icmp("noFog")) {
			noFog = true;
			continue;
		}
		// unsmoothedTangents
		else if (!token.Icmp("unsmoothedTangents")) {
			unsmoothedTangents = true;
			continue;
		}
		// lightFalloffImage <imageprogram>
		else if (!token.Icmp("lightFalloffImage")) {
			str = R_ParsePastImageProgram(src);
			idStr	copy;

			copy = str;	// so other things don't step on it
			lightFalloffImage = globalImages->ImageFromFile(copy, TF_DEFAULT, false, TR_CLAMP, TD_DEFAULT);
			continue;
		}
		// guisurf <guifile> | guisurf entity
		else if (!token.Icmp("guisurf")) {
			src.ReadTokenOnLine(&token);
			if (!token.Icmp("entity")) {
				entityGui = 1;
			}
			else if (!token.Icmp("entity2")) {
				entityGui = 2;
			}
			else if (!token.Icmp("entity3")) {
				entityGui = 3;
			}
			else {
				gui = uiManager->FindGui(token.c_str(), true);
			}
			continue;
		}
		// sort
		else if (!token.Icmp("sort")) {
			ParseSort(src);
			continue;
		}
		// spectrum <integer>
		else if (!token.Icmp("spectrum")) {
			src.ReadTokenOnLine(&token);
			spectrum = atoi(token.c_str());
			continue;
		}
		// deform < sprite | tube | flare >
		else if (!token.Icmp("deform")) {
			ParseDeform(src);
			continue;
		}
		// decalInfo
		else if (!token.Icmp("decalInfo")) {
			ParseDecalInfo(src);
			continue;
		}
		// renderbump <args...>
		else if (!token.Icmp("renderbump")) {
			src.ParseRestOfLine(renderBump);
			continue;
		}
		// diffusemap for stage shortcut
		else if (!token.Icmp("diffusemap")) {
			str = R_ParsePastImageProgram(src);
			idStr::snPrintf(buffer, sizeof(buffer), "blend diffusemap\nmap %s\n}\n", str);
			newSrc.LoadMemory(buffer, strlen(buffer), "diffusemap", 1);
			newSrc.SetFlags(LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES);
			if (!ParseStage(newSrc, trpDefault)) {
				newSrc.FreeSource();
				return false;
			}
			newSrc.FreeSource();
			continue;
		}
		// specularmap for stage shortcut
		else if (!token.Icmp("specularmap")) {
			str = R_ParsePastImageProgram(src);
			idStr::snPrintf(buffer, sizeof(buffer), "blend specularmap\nmap %s\n}\n", str);
			newSrc.LoadMemory(buffer, strlen(buffer), "specularmap", 1);
			newSrc.SetFlags(LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES);
			if (!ParseStage(newSrc, trpDefault)) {
				newSrc.FreeSource();
				return false;
			}
			newSrc.FreeSource();
			continue;
		}
		// normalmap for stage shortcut
		else if (!token.Icmp("bumpmap")) {
			str = R_ParsePastImageProgram(src);
			idStr::snPrintf(buffer, sizeof(buffer), "blend bumpmap\nmap %s\n}\n", str);
			newSrc.LoadMemory(buffer, strlen(buffer), "bumpmap", 1);
			newSrc.SetFlags(LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES);
			if (!ParseStage(newSrc, trpDefault)) {
				newSrc.FreeSource();
				return false;
			}
			newSrc.FreeSource();
			continue;
		}
		// DECAL_MACRO for backwards compatibility with the preprocessor macros
		else if (!token.Icmp("DECAL_MACRO")) {
			// polygonOffset
			SetMaterialFlag(MF_POLYGONOFFSET);
			polygonOffset = 1;

			// discrete
			surfaceFlags |= SURF_DISCRETE;
			contentFlags &= ~CONTENTS_SOLID;

			// sort decal
			sort = SS_DECAL;

			// noShadows
			SetMaterialFlag(MF_NOSHADOWS);
			continue;
		}
		else if (token == "{") {
			// create the new stage
			if (!ParseStage(src, trpDefault)) {
				return false;
			}
			continue;
		}
		else {
			common->Warning("unknown general material parameter '%s' in '%s'", token.c_str(), GetName());
			SetMaterialFlag(MF_DEFAULTED);
			return false;
		}
	}

	// add _flat or _white stages if needed
	if (!AddImplicitStages()) {
		return false;
	}

	// order the diffuse / bump / specular stages properly
	SortInteractionStages();

	// if we need to do anything with normals (lighting or environment mapping)
	// and two sided lighting was asked for, flag shouldCreateBackSides()
	if (cullType == CT_TWO_SIDED) {
		for (i = 0; i < numStages; i++) {
			if (pd->parseStages[i].lighting != SL_AMBIENT || pd->parseStages[i].texture.texgen != TG_EXPLICIT) {
				if (cullType == CT_TWO_SIDED) {
					cullType = CT_FRONT_SIDED;
					shouldCreateBackSides = true;
				}
				break;
			}
		}
	}

	// currently a surface can only have one unique texgen for all the stages on old hardware
	texgen_t firstGen = TG_EXPLICIT;
	for (i = 0; i < numStages; i++) {
		if (pd->parseStages[i].texture.texgen != TG_EXPLICIT) {
			if (firstGen == TG_EXPLICIT) {
				firstGen = pd->parseStages[i].texture.texgen;
			}
			else if (firstGen != pd->parseStages[i].texture.texgen) {
				common->Warning("material '%s' has multiple stages with a texgen", GetName());
				return false;
			}
		}
	}

	return !TestMaterialFlag(MF_DEFAULTED);
}

/*
=========================
idMaterial::SetGui
=========================
*/
void idMaterial::SetGui(const char* _gui) const {
	gui = uiManager->FindGui(_gui, true, false, true);
}

/*
=========================
idMaterial::Parse

Parses the current material definition and finds all necessary images.
=========================
*/
bool idMaterial::Parse(const char* text, const int textLength) {
	idLexer	src;
	idToken	token;
	mtrParsingData_t parsingData;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	// reset to the unparsed state
	CommonInit();

	memset(&parsingData, 0, sizeof(parsingData));

	pd = &parsingData;	// this is only valid during parse

	// parse it
	if (!ParseMaterial(src)) {
		pd = NULL;
		MakeDefault();
		return false;
	}

	// if we are doing an fs_copyfiles, also reference the editorImage
	if (cvarSystem->GetCVarInteger("fs_copyFiles")) {
		GetEditorImage();
	}

	//
	// count non-lit stages
	numAmbientStages = 0;
	int i;
	for (i = 0; i < numStages; i++) {
		if (pd->parseStages[i].lighting == SL_AMBIENT) {
			numAmbientStages++;
		}
	}

	// see if there is a subview stage
	if (sort == SS_SUBVIEW) {
		hasSubview = true;
	}
	else {
		hasSubview = false;
		for (i = 0; i < numStages; i++) {
			if (pd->parseStages[i].texture.dynamic) {
				hasSubview = true;
			}
		}
	}

	// automatically determine coverage if not explicitly set
	if (coverage == MC_BAD) {
		// automatically set MC_TRANSLUCENT if we don't have any interaction stages and
		// the first stage is blended and not an alpha test mask or a subview
		if (!numStages) {
			// non-visible
			coverage = MC_TRANSLUCENT;
		}
		else if (numStages != numAmbientStages) {
			// we have an interaction draw
			coverage = MC_OPAQUE;
		}
		else if (
			(pd->parseStages[0].drawStateBits & GLS_DSTBLEND_BITS) != GLS_DSTBLEND_ZERO ||
			(pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR ||
			(pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR ||
			(pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_ALPHA ||
			(pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
			) {
			// blended with the destination
			coverage = MC_TRANSLUCENT;
		}
		else {
			coverage = MC_OPAQUE;
		}
	}

	// translucent automatically implies noshadows
	if (coverage == MC_TRANSLUCENT) {
		SetMaterialFlag(MF_NOSHADOWS);
	}
	else {
		// mark the contents as opaque
		contentFlags |= CONTENTS_OPAQUE;
	}

	// if we are translucent, draw with an alpha in the editor
	if (coverage == MC_TRANSLUCENT) {
		editorAlpha = 0.5f;
	}
	else {
		editorAlpha = 1.0f;
	}

	// the sorts can make reasonable defaults
	if (sort == SS_BAD) {
		if (TestMaterialFlag(MF_POLYGONOFFSET)) {
			sort = SS_DECAL;
		}
		else if (coverage == MC_TRANSLUCENT) {
			sort = SS_MEDIUM;
		}
		else {
			sort = SS_OPAQUE;
		}
	}

	// anything that references _currentRender will automatically get sort = SS_POST_PROCESS
	// and coverage = MC_TRANSLUCENT, unless the material explicitly requested
	// current-render usage handling.
	if (!TestMaterialFlag(MF_NEED_CURRENT_RENDER)) {
		for (i = 0; i < numStages; i++) {
			shaderStage_t* pStage = &pd->parseStages[i];
			if (pStage->texture.image == globalImages->currentRenderImage) {
				if (sort != SS_PORTAL_SKY) {
					sort = SS_POST_PROCESS;
					coverage = MC_TRANSLUCENT;
				}
				break;
			}
			if (pStage->newStage) {
				for (int j = 0; j < pStage->newStage->numFragmentProgramImages; j++) {
					if (pStage->newStage->fragmentProgramImages[j] == globalImages->currentRenderImage) {
						if (sort != SS_PORTAL_SKY) {
							sort = SS_POST_PROCESS;
							coverage = MC_TRANSLUCENT;
						}
						i = numStages;
						break;
					}
				}
			}
		}
	}

	// set the drawStateBits depth flags
	for (i = 0; i < numStages; i++) {
		shaderStage_t* pStage = &pd->parseStages[i];
		if (sort == SS_POST_PROCESS) {
			// post-process effects fill the depth buffer as they draw, so only the
			// topmost post-process effect is rendered
			pStage->drawStateBits |= GLS_DEPTHFUNC_LESS;
		}
		else if (coverage == MC_TRANSLUCENT || pStage->ignoreAlphaTest) {
			// translucent surfaces can extend past the exactly marked depth buffer
			pStage->drawStateBits |= GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK;
		}
		else {
			// opaque and perforated surfaces must exactly match the depth buffer,
			// which gets alpha test correct
			pStage->drawStateBits |= GLS_DEPTHFUNC_EQUAL | GLS_DEPTHMASK;
		}
	}

	// determine if this surface will accept overlays / decals
	if (pd->forceOverlays) {
		// explicitly flagged in material definition
		allowOverlays = true;
	}
	else {
		if (!IsDrawn()) {
			allowOverlays = false;
		}
		if (Coverage() != MC_OPAQUE) {
			allowOverlays = false;
		}
		if (GetSurfaceFlags() & SURF_NOIMPACT) {
			allowOverlays = false;
		}
	}

#if Q4_PORT_ENABLE_PORTAL_FEATURES
	if ((portalDistanceNear < 262144.0f || portalDistanceFar < 262144.0f) && portalImage == NULL) {
		portalImage = globalImages->blackImage;
	}
#endif

	if (numStages) {
		stages = (shaderStage_t*)R_StaticAlloc(numStages * sizeof(stages[0]));
		memcpy(stages, pd->parseStages, numStages * sizeof(stages[0]));
	}

	if (numOps) {
		ops = (expOp_t*)R_StaticAlloc(numOps * sizeof(ops[0]));
		memcpy(ops, pd->shaderOps, numOps * sizeof(ops[0]));
	}

	if (numRegisters) {
		expressionRegisters = (float*)R_StaticAlloc(numRegisters * sizeof(expressionRegisters[0]));
		memcpy(expressionRegisters, pd->shaderRegisters, numRegisters * sizeof(expressionRegisters[0]));
	}

	// see if the registers are completely constant, and don't need to be evaluated
	CheckForConstantRegisters();

	pd = NULL;	// the pointer will be invalid after exiting this function

	// finish things up
	if (TestMaterialFlag(MF_DEFAULTED)) {
		MakeDefault();
		return false;
	}
	return true;
}

/*
===================
idMaterial::Print
===================
*/
char* opNames[] = {
	"OP_TYPE_ADD",
	"OP_TYPE_SUBTRACT",
	"OP_TYPE_MULTIPLY",
	"OP_TYPE_DIVIDE",
	"OP_TYPE_MOD",
	"OP_TYPE_TABLE",
	"OP_TYPE_GT",
	"OP_TYPE_GE",
	"OP_TYPE_LT",
	"OP_TYPE_LE",
	"OP_TYPE_EQ",
	"OP_TYPE_NE",
	"OP_TYPE_AND",
	"OP_TYPE_OR",
	"OP_TYPE_SOUND",
	"OP_TYPE_GLSL_ENABLED",
	"OP_TYPE_POT_X",
	"OP_TYPE_POT_Y"
};

void idMaterial::Print() const {
	int			i;

	for (i = EXP_REG_NUM_PREDEFINED; i < GetNumRegisters(); i++) {
		common->Printf("register %i: %f\n", i, expressionRegisters[i]);
	}
	common->Printf("\n");
	for (i = 0; i < numOps; i++) {
		const expOp_t* op = &ops[i];
		if (op->opType == OP_TYPE_TABLE) {
			common->Printf("%i = %s[ %i ]\n", op->c, declManager->DeclByIndex(DECL_TABLE, op->a)->GetName(), op->b);
		}
		else {
			common->Printf("%i = %i %s %i\n", op->c, op->a, opNames[op->opType], op->b);
		}
	}
}

/*
===============
idMaterial::AddReference
===============
*/
void idMaterial::AddReference() {
	refCount++;

	for (int i = 0; i < numStages; i++) {
		shaderStage_t* s = &stages[i];

		if (s->texture.image) {
			s->texture.image->AddReference();
		}
		if (s->newStage) {
			for (int j = 0; j < s->newStage->numFragmentProgramImages; j++) {
				if (s->newStage->fragmentProgramImages[j]) {
					s->newStage->fragmentProgramImages[j]->AddReference();
				}
			}
		}
	}
}



#if Q4_PORT_ENABLE_EXTRA_METHODS
/*
=============
idMaterial::GetTexelCount
=============
*/
int idMaterial::GetTexelCount() const {
	int count = 0;

	for (int i = 0; i < numStages; i++) {
		if (stages[i].texture.image) {
			count += stages[i].texture.image->uploadWidth * stages[i].texture.image->uploadHeight;
		}
	}
	return count;
}

/*
=============
idMaterial::ResolveUse
=============
*/
void idMaterial::ResolveUse() {
	for (int i = 0; i < numStages; i++) {
		if (stages[i].texture.image) {
			stages[i].texture.image->useCount += useCount;
		}
		if (stages[i].newStage) {
			for (int j = 0; j < stages[i].newStage->numFragmentProgramImages; j++) {
				if (stages[i].newStage->fragmentProgramImages[j]) {
					stages[i].newStage->fragmentProgramImages[j]->useCount += useCount;
				}
			}
		}
	}
	if (lightFalloffImage) {
		lightFalloffImage->useCount += useCount;
	}
#if Q4_PORT_ENABLE_PORTAL_FEATURES
	if (portalImage) {
		portalImage->useCount += useCount;
	}
#endif
}

/*
=============
idMaterial::HasDefaultedImage
=============
*/
bool idMaterial::HasDefaultedImage() const {
	for (int i = 0; i < numStages; i++) {
		if (stages[i].texture.image && stages[i].texture.image->defaulted) {
			return true;
		}
	}
	return false;
}

/*
===================
idMaterial::GetDiffuseImage
===================
*/
idImage* idMaterial::GetDiffuseImage(void) const {
	for (int i = 0; i < numStages; i++) {
		if (stages[i].lighting == SL_DIFFUSE && stages[i].texture.image) {
			return stages[i].texture.image;
		}
	}
	return GetEditorImage();
}
#endif

/*
===================
idMaterial::GetMaterialType
===================
*/
const rvDeclMatType* idMaterial::GetMaterialType(idVec2& tc) const {
	if (materialTypeArray == NULL || MTAWidth <= 0 || MTAHeight <= 0) {
		return materialType;
	}

	tc.x = fmod(tc.x, 1.0f);
	tc.y = fmod(tc.y, 1.0f);
	if (tc.x < 0.0f) {
		tc.x += 1.0f;
	}
	if (tc.y < 0.0f) {
		tc.y += 1.0f;
	}

	int x = (int)(tc.x * MTAWidth);
	int y = (int)(tc.y * MTAHeight);
	if (x < 0) {
		x = 0;
	}
	else if (x >= MTAWidth) {
		x = MTAWidth - 1;
	}
	if (y < 0) {
		y = 0;
	}
	else if (y >= MTAHeight) {
		y = MTAHeight - 1;
	}

	const int index = materialTypeArray[y * MTAWidth + x];
	if (index <= 0) {
		return materialType;
	}

	const idDecl* decl = declManager->DeclByIndex(DECL_MATERIALTYPE, index);
	return decl != NULL ? reinterpret_cast<const rvDeclMatType*>(decl) : materialType;
}

/*
===================
idMaterial::Validate
===================
*/
bool idMaterial::Validate(const char* psText, int iTextLength, idStr& strReportTo) const {
	(void)psText;
	(void)iTextLength;
	strReportTo = "";
	return true;
}

/*
===================
idMaterial::EvaluateStageRegisters
===================
*/
void idMaterial::EvaluateStageRegisters(int stageIndex, float* registers, const float shaderParms[MAX_ENTITY_SHADER_PARMS], float floatTime) const {
	if (stageIndex < 0 || stageIndex >= numStages || stages == NULL || registers == NULL) {
		return;
	}

	for (int i = 0; i < numRegisters; i++) {
		registers[i] = expressionRegisters ? expressionRegisters[i] : 0.0f;
	}

	registers[EXP_REG_TIME] = floatTime;
	registers[EXP_REG_PARM0] = shaderParms[0];
	registers[EXP_REG_PARM1] = shaderParms[1];
	registers[EXP_REG_PARM2] = shaderParms[2];
	registers[EXP_REG_PARM3] = shaderParms[3];
	registers[EXP_REG_PARM4] = shaderParms[4];
	registers[EXP_REG_PARM5] = shaderParms[5];
	registers[EXP_REG_PARM6] = shaderParms[6];
	registers[EXP_REG_PARM7] = shaderParms[7];
	registers[EXP_REG_PARM8] = shaderParms[8];
	registers[EXP_REG_PARM9] = shaderParms[9];
	registers[EXP_REG_PARM10] = shaderParms[10];
	registers[EXP_REG_PARM11] = shaderParms[11];
	registers[EXP_REG_GLOBAL0] = 0.0f;
	registers[EXP_REG_GLOBAL1] = 0.0f;
	registers[EXP_REG_GLOBAL2] = 0.0f;
	registers[EXP_REG_GLOBAL3] = 0.0f;
	registers[EXP_REG_GLOBAL4] = 0.0f;
	registers[EXP_REG_GLOBAL5] = 0.0f;
	registers[EXP_REG_GLOBAL6] = 0.0f;
	registers[EXP_REG_GLOBAL7] = 0.0f;
	registers[EXP_REG_VERTEX_RANDOMIZER] = 0.0f;

	const shaderStage_t& stage = stages[stageIndex];
	const int opStart = stage.mStageOpsStart;
	const int opEnd = stage.mStageOpsStart + stage.mNumStageOps;
	for (int i = opStart; i < opEnd; i++) {
		const expOp_t& op = ops[i];
		switch (op.opType) {
		case OP_TYPE_ADD:
			registers[op.c] = registers[op.a] + registers[op.b];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op.c] = registers[op.a] - registers[op.b];
			break;
		case OP_TYPE_MULTIPLY:
			registers[op.c] = registers[op.a] * registers[op.b];
			break;
		case OP_TYPE_DIVIDE:
			registers[op.c] = registers[op.a] / registers[op.b];
			break;
		case OP_TYPE_MOD:
		{
			int mod = (int)registers[op.b];
			mod = mod != 0 ? mod : 1;
			registers[op.c] = (int)registers[op.a] % mod;
		}
		break;
		case OP_TYPE_TABLE:
		{
			const idDeclTable* table = static_cast<const idDeclTable*>(declManager->DeclByIndex(DECL_TABLE, op.a));
			registers[op.c] = table->TableLookup(registers[op.b]);
		}
		break;
		case OP_TYPE_SOUND:
			registers[op.c] = 0.0f;
			break;
		case OP_TYPE_GLSL_ENABLED:
			registers[op.c] = 0.0f;
			break;
		case OP_TYPE_POT_X:
			registers[op.c] = glConfig.vidWidth > 0 ? (float)glConfig.vidWidth / (float)MaterialNextPowerOfTwo(glConfig.vidWidth) : 1.0f;
			break;
		case OP_TYPE_POT_Y:
			registers[op.c] = glConfig.vidHeight > 0 ? (float)glConfig.vidHeight / (float)MaterialNextPowerOfTwo(glConfig.vidHeight) : 1.0f;
			break;
		case OP_TYPE_GT:
			registers[op.c] = registers[op.a] > registers[op.b];
			break;
		case OP_TYPE_GE:
			registers[op.c] = registers[op.a] >= registers[op.b];
			break;
		case OP_TYPE_LT:
			registers[op.c] = registers[op.a] < registers[op.b];
			break;
		case OP_TYPE_LE:
			registers[op.c] = registers[op.a] <= registers[op.b];
			break;
		case OP_TYPE_EQ:
			registers[op.c] = registers[op.a] == registers[op.b];
			break;
		case OP_TYPE_NE:
			registers[op.c] = registers[op.a] != registers[op.b];
			break;
		case OP_TYPE_AND:
			registers[op.c] = registers[op.a] && registers[op.b];
			break;
		case OP_TYPE_OR:
			registers[op.c] = registers[op.a] || registers[op.b];
			break;
		}
	}
}

/*
===============
idMaterial::EvaluateRegisters

Parameters are taken from the localSpace and the renderView,
then all expressions are evaluated, leaving the material registers
set to their apropriate values.
===============
*/
void idMaterial::EvaluateRegisters(float* registers, const float shaderParms[MAX_ENTITY_SHADER_PARMS],
	const viewDef_s* view, int soundEmitter, idVec3* randomizer) const {
	int		i, b;
	expOp_t* op;
	(void)soundEmitter;

	// copy the material constants
	for (i = EXP_REG_NUM_PREDEFINED; i < numRegisters; i++) {
		registers[i] = expressionRegisters[i];
	}

	// copy the local and global parameters
	registers[EXP_REG_TIME] = view ? view->floatTime : 0.0f;
	registers[EXP_REG_PARM0] = shaderParms[0];
	registers[EXP_REG_PARM1] = shaderParms[1];
	registers[EXP_REG_PARM2] = shaderParms[2];
	registers[EXP_REG_PARM3] = shaderParms[3];
	registers[EXP_REG_PARM4] = shaderParms[4];
	registers[EXP_REG_PARM5] = shaderParms[5];
	registers[EXP_REG_PARM6] = shaderParms[6];
	registers[EXP_REG_PARM7] = shaderParms[7];
	registers[EXP_REG_PARM8] = shaderParms[8];
	registers[EXP_REG_PARM9] = shaderParms[9];
	registers[EXP_REG_PARM10] = shaderParms[10];
	registers[EXP_REG_PARM11] = shaderParms[11];
	registers[EXP_REG_GLOBAL0] = view ? view->renderView.shaderParms[0] : 0.0f;
	registers[EXP_REG_GLOBAL1] = view ? view->renderView.shaderParms[1] : 0.0f;
	registers[EXP_REG_GLOBAL2] = view ? view->renderView.shaderParms[2] : 0.0f;
	registers[EXP_REG_GLOBAL3] = view ? view->renderView.shaderParms[3] : 0.0f;
	registers[EXP_REG_GLOBAL4] = view ? view->renderView.shaderParms[4] : 0.0f;
	registers[EXP_REG_GLOBAL5] = view ? view->renderView.shaderParms[5] : 0.0f;
	registers[EXP_REG_GLOBAL6] = view ? view->renderView.shaderParms[6] : 0.0f;
	registers[EXP_REG_GLOBAL7] = view ? view->renderView.shaderParms[7] : 0.0f;
	registers[EXP_REG_VERTEX_RANDOMIZER] = (randomizer != NULL) ? randomizer->x : 0.0f;

	op = ops;
	for (i = 0; i < numOps; i++, op++) {
		switch (op->opType) {
		case OP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case OP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case OP_TYPE_DIVIDE:
			registers[op->c] = registers[op->a] / registers[op->b];
			break;
		case OP_TYPE_MOD:
			b = (int)registers[op->b];
			b = b != 0 ? b : 1;
			registers[op->c] = (int)registers[op->a] % b;
			break;
		case OP_TYPE_TABLE:
		{
			const idDeclTable* table = static_cast<const idDeclTable*>(declManager->DeclByIndex(DECL_TABLE, op->a));
			registers[op->c] = table->TableLookup(registers[op->b]);
		}
		break;
		case OP_TYPE_SOUND:
			registers[op->c] = 0.0f;
			break;
		case OP_TYPE_GLSL_ENABLED:
			registers[op->c] = 0.0f;
			break;
		case OP_TYPE_POT_X:
			registers[op->c] = glConfig.vidWidth > 0 ? (float)glConfig.vidWidth / (float)MaterialNextPowerOfTwo(glConfig.vidWidth) : 1.0f;
			break;
		case OP_TYPE_POT_Y:
			registers[op->c] = glConfig.vidHeight > 0 ? (float)glConfig.vidHeight / (float)MaterialNextPowerOfTwo(glConfig.vidHeight) : 1.0f;
			break;
		case OP_TYPE_GT:
			registers[op->c] = registers[op->a] > registers[op->b];
			break;
		case OP_TYPE_GE:
			registers[op->c] = registers[op->a] >= registers[op->b];
			break;
		case OP_TYPE_LT:
			registers[op->c] = registers[op->a] < registers[op->b];
			break;
		case OP_TYPE_LE:
			registers[op->c] = registers[op->a] <= registers[op->b];
			break;
		case OP_TYPE_EQ:
			registers[op->c] = registers[op->a] == registers[op->b];
			break;
		case OP_TYPE_NE:
			registers[op->c] = registers[op->a] != registers[op->b];
			break;
		case OP_TYPE_AND:
			registers[op->c] = registers[op->a] && registers[op->b];
			break;
		case OP_TYPE_OR:
			registers[op->c] = registers[op->a] || registers[op->b];
			break;
		default:
			common->FatalError("R_EvaluateExpression: bad opcode");
		}
	}
}

/*
=============
idMaterial::Texgen
=============
*/
texgen_t idMaterial::Texgen() const {
	if (stages) {
		for (int i = 0; i < numStages; i++) {
			if (stages[i].texture.texgen != TG_EXPLICIT) {
				return stages[i].texture.texgen;
			}
		}
	}

	return TG_EXPLICIT;
}

/*
=============
idMaterial::GetImageWidth
=============
*/
int idMaterial::GetImageWidth(void) const {
	assert(GetStage(0) && GetStage(0)->texture.image);
	return GetStage(0)->texture.image->uploadWidth;
}

/*
=============
idMaterial::GetImageHeight
=============
*/
int idMaterial::GetImageHeight(void) const {
	assert(GetStage(0) && GetStage(0)->texture.image);
	return GetStage(0)->texture.image->uploadHeight;
}

/*
=============
idMaterial::CinematicLength
=============
*/
int	idMaterial::CinematicLength() const {
	if (!stages || !stages[0].texture.cinematic) {
		return 0;
	}
	return stages[0].texture.cinematic->AnimationLength();
}

/*
=============
idMaterial::UpdateCinematic
=============
*/
void idMaterial::UpdateCinematic(int time) const {
	if (!stages || !stages[0].texture.cinematic || !backEnd.viewDef) {
		return;
	}
	stages[0].texture.cinematic->ImageForTime(tr.primaryRenderView.time);
}

/*
=============
idMaterial::CloseCinematic
=============
*/
void idMaterial::CloseCinematic(void) const {
	for (int i = 0; i < numStages; i++) {
		if (stages[i].texture.cinematic) {
			stages[i].texture.cinematic->Close();
			delete stages[i].texture.cinematic;
			stages[i].texture.cinematic = NULL;
		}
	}
}

/*
=============
idMaterial::ResetCinematicTime
=============
*/
void idMaterial::ResetCinematicTime(int time) const {
	for (int i = 0; i < numStages; i++) {
		if (stages[i].texture.cinematic) {
			stages[i].texture.cinematic->ResetTime(time);
		}
	}
}

/*
=============
idMaterial::ConstantRegisters
=============
*/
const float* idMaterial::ConstantRegisters() const {
	if (!r_useConstantMaterials.GetBool()) {
		return NULL;
	}
	return constantRegisters;
}

/*
==================
idMaterial::CheckForConstantRegisters

As of 5/2/03, about half of the unique materials loaded on typical
maps are constant, but 2/3 of the surface references are.
This is probably an optimization of dubious value.
==================
*/
static int	c_constant, c_variable;
void idMaterial::CheckForConstantRegisters() {
	if (!pd->registersAreConstant) {
		return;
	}

	// evaluate the registers once, and save them 
	constantRegisters = (float*)R_ClearedStaticAlloc(GetNumRegisters() * sizeof(float));

	float shaderParms[MAX_ENTITY_SHADER_PARMS];
	memset(shaderParms, 0, sizeof(shaderParms));
	viewDef_s	viewDef;
	memset(&viewDef, 0, sizeof(viewDef));

	EvaluateRegisters(constantRegisters, shaderParms, &viewDef, 0);
}

/*
===================
idMaterial::ImageName
===================
*/
const char* idMaterial::ImageName(void) const {
	if (numStages == 0) {
		return "_scratch";
	}
	idImage* image = stages[0].texture.image;
	if (image) {
		return image->imgName;
	}
	return "_scratch";
}

/*
===================
idMaterial::SetImageClassifications

Just for image resource tracking.
===================
*/
void idMaterial::SetImageClassifications(int tag) const {
	for (int i = 0; i < numStages; i++) {
		idImage* image = stages[i].texture.image;
		if (image) {
			image->SetClassification(tag);
		}
	}
}

/*
=================
idMaterial::Size
=================
*/
size_t idMaterial::Size(void) const {
	return sizeof(idMaterial);
}

/*
===================
idMaterial::SetDefaultText
===================
*/
bool idMaterial::SetDefaultText(void) {
	// if there exists an image with the same name
	if (1) { //fileSystem->ReadFile( GetName(), NULL ) != -1 ) {
		char generated[2048];
		idStr::snPrintf(generated, sizeof(generated),
			"material %s // IMPLICITLY GENERATED\n"
			"{\n"
			"{\n"
			"blend blend\n"
			"colored\n"
			"map \"%s\"\n"
			"clamp\n"
			"}\n"
			"}\n", GetName(), GetName());
		SetText(generated);
		return true;
	}
	else {
		return false;
	}
}

/*
===================
idMaterial::DefaultDefinition
===================
*/
const char* idMaterial::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"blend\tblend\n"
		"\t\t"		"map\t\t_default\n"
		"\t"	"}\n"
		"}";
}


/*
===================
idMaterial::GetBumpStage
===================
*/
const shaderStage_t* idMaterial::GetBumpStage(void) const {
	for (int i = 0; i < numStages; i++) {
		if (stages[i].lighting == SL_BUMP) {
			return &stages[i];
		}
	}
	return NULL;
}

/*
===================
idMaterial::ReloadImages
===================
*/
void idMaterial::ReloadImages(bool force) const
{
	for (int i = 0; i < numStages; i++) {
		if (stages[i].newStage) {
			for (int j = 0; j < stages[i].newStage->numFragmentProgramImages; j++) {
				if (stages[i].newStage->fragmentProgramImages[j]) {
					stages[i].newStage->fragmentProgramImages[j]->Reload(false, force);
				}
			}
		}
		else if (stages[i].texture.image) {
			stages[i].texture.image->Reload(false, force);
		}
	}
}