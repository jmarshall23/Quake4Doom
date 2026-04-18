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
#include "Model_local.h"

rvRenderModelMD5R* rvRenderModelMD5R::ms_modelList = NULL;

// Default LOD ranges used when the source data does not provide explicit ranges.
static const float MD5R_DefaultLODRanges[] = {
	100.0f,
	250.0f,
	500.0f
};

rvRenderModelMD5R::rvRenderModelMD5R() {
	ResetValues();
	m_source = MD5R_SOURCE_NONE;
	m_next = ms_modelList;
	ms_modelList = this;
}

rvRenderModelMD5R::~rvRenderModelMD5R() {
	RemoveFromList(this);
	Shutdown();
}

void rvRenderModelMD5R::ResetValues() {
	purged = true;

	m_joints = NULL;
	m_skinSpaceToLocalMats = NULL;
	m_defaultPose = NULL;
	m_numJoints = 0;

	m_vertexBuffers = NULL;
	m_allocVertexBuffers = NULL;
	m_numVertexBuffers = 0;

	m_indexBuffers = NULL;
	m_allocIndexBuffers = NULL;
	m_numIndexBuffers = 0;

	m_silEdges = NULL;
	m_allocSilEdges = NULL;
	m_numSilEdges = 0;
	m_numSilEdgesAdded = 0;

	m_meshes = NULL;
	m_numMeshes = 0;

	m_numLODs = 0;
	m_lods = NULL;
	m_allLODMeshes = NULL;
}

srfTriangles_s* rvRenderModelMD5R::GenerateStaticTriSurface(int meshOffset) {
	rvMesh& mesh = m_meshes[meshOffset];

	srfTriangles_s* tri = R_AllocStaticTriSurf();
	tri->numVerts = mesh.m_numDrawVertices;
	tri->numIndexes = mesh.m_numDrawIndices;

	R_AllocStaticTriSurfVerts(tri, tri->numVerts);
	R_AllocStaticTriSurfIndexes(tri, tri->numIndexes);
	mesh.CopyTriangles(tri->verts, tri->indexes, 0);

	return tri;
}

int rvRenderModelMD5R::NumJoints() const {
	return m_numJoints;
}

const idMD5Joint* rvRenderModelMD5R::GetJoints() const {
	return m_joints;
}

const idJointQuat* rvRenderModelMD5R::GetDefaultPose() const {
	return m_defaultPose;
}

void rvRenderModelMD5R::ParseVertexBuffers(Lexer& lex) {
	lex.ExpectTokenString("[");
	m_numVertexBuffers = lex.ParseInt();
	lex.ExpectTokenString("]");
	lex.ExpectTokenString("{");

	if (m_numVertexBuffers <= 0) {
		m_allocVertexBuffers = NULL;
		m_vertexBuffers = NULL;
		lex.ExpectTokenString("}");
		return;
	}

	m_allocVertexBuffers = new rvVertexBuffer[m_numVertexBuffers];
	m_vertexBuffers = m_allocVertexBuffers;

	for (int i = 0; i < m_numVertexBuffers; ++i) {
		lex.ExpectTokenString("VertexBuffer");
		m_vertexBuffers[i].Init(lex);
	}

	lex.ExpectTokenString("}");
}

void rvRenderModelMD5R::ParseIndexBuffers(Lexer& lex) {
	lex.ExpectTokenString("[");
	m_numIndexBuffers = lex.ParseInt();
	lex.ExpectTokenString("]");
	lex.ExpectTokenString("{");

	if (m_numIndexBuffers <= 0) {
		m_allocIndexBuffers = NULL;
		m_indexBuffers = NULL;
		lex.ExpectTokenString("}");
		return;
	}

	m_allocIndexBuffers = new rvIndexBuffer[m_numIndexBuffers];
	m_indexBuffers = m_allocIndexBuffers;

	for (int i = 0; i < m_numIndexBuffers; ++i) {
		lex.ExpectTokenString("IndexBuffer");
		m_indexBuffers[i].Init(lex);
	}

	lex.ExpectTokenString("}");
}

void rvRenderModelMD5R::ParseSilhouetteEdges(Lexer& lex) {
	lex.ExpectTokenString("[");
	m_numSilEdges = lex.ParseInt();
	m_numSilEdgesAdded = m_numSilEdges;
	lex.ExpectTokenString("]");
	lex.ExpectTokenString("{");

	if (m_numSilEdges <= 0) {
		m_allocSilEdges = NULL;
		m_silEdges = NULL;
		lex.ExpectTokenString("}");
		return;
	}

	m_allocSilEdges = new silEdge_t[m_numSilEdges];
	m_silEdges = m_allocSilEdges;

	for (int i = 0; i < m_numSilEdges; ++i) {
		m_silEdges[i].p1 = lex.ParseInt();
		m_silEdges[i].p2 = lex.ParseInt();
		m_silEdges[i].v1 = lex.ParseInt();
		m_silEdges[i].v2 = lex.ParseInt();
	}

	lex.ExpectTokenString("}");
}

void rvRenderModelMD5R::ParseLevelOfDetail(Lexer& lex) {
	lex.ExpectTokenString("[");
	m_numLODs = lex.ParseInt();
	lex.ExpectTokenString("]");
	lex.ExpectTokenString("{");

	if (m_numLODs <= 0) {
		m_lods = NULL;
		lex.ExpectTokenString("}");
		return;
	}

	m_lods = new levelOfDetailMD5R_s[m_numLODs];

	for (int i = 0; i < m_numLODs; ++i) {
		m_lods[i].m_rangeEnd = lex.ParseFloat(false);
		m_lods[i].m_rangeEndSquared = m_lods[i].m_rangeEnd * m_lods[i].m_rangeEnd;
		m_lods[i].m_meshList = NULL;
	}

	lex.ExpectTokenString("}");
}

void rvRenderModelMD5R::ParseMeshes(Lexer& lex) {
	lex.ExpectTokenString("[");
	m_numMeshes = lex.ParseInt();
	lex.ExpectTokenString("]");
	lex.ExpectTokenString("{");

	if (m_numMeshes <= 0) {
		m_meshes = NULL;
		lex.ExpectTokenString("}");
		return;
	}

	m_meshes = new rvMesh[m_numMeshes];

	for (int i = 0; i < m_numMeshes; ++i) {
		lex.ExpectTokenString("Mesh");
		m_meshes[i].Init(*this, lex);
		m_meshes[i].m_meshIdentifier = i;
	}

	lex.ExpectTokenString("}");
}

void rvRenderModelMD5R::BuildLevelsOfDetail() {
	int highestLODReferenced = -1;

	for (int i = 0; i < m_numMeshes; ++i) {
		if (m_meshes[i].m_levelOfDetail > highestLODReferenced) {
			highestLODReferenced = m_meshes[i].m_levelOfDetail;
		}
	}

	if (highestLODReferenced >= 0 && m_numLODs == 0) {
		m_numLODs = highestLODReferenced + 1;
		m_lods = new levelOfDetailMD5R_s[m_numLODs];

		for (int i = 0; i < m_numLODs; ++i) {
			if (i < 3) {
				m_lods[i].m_rangeEnd = MD5R_DefaultLODRanges[i];
			}
			else {
				m_lods[i].m_rangeEnd = m_lods[i - 1].m_rangeEnd * 2.0f;
			}
			m_lods[i].m_rangeEndSquared = m_lods[i].m_rangeEnd * m_lods[i].m_rangeEnd;
			m_lods[i].m_meshList = NULL;
		}
	}

	m_allLODMeshes = NULL;

	for (int i = 0; i < m_numMeshes; ++i) {
		rvMesh& mesh = m_meshes[i];

		if (mesh.m_levelOfDetail < 0 || mesh.m_levelOfDetail >= m_numLODs) {
			mesh.m_nextInLOD = m_allLODMeshes;
			m_allLODMeshes = &mesh;
		}
		else {
			mesh.m_nextInLOD = m_lods[mesh.m_levelOfDetail].m_meshList;
			m_lods[mesh.m_levelOfDetail].m_meshList = &mesh;
		}
	}
}

int rvRenderModelMD5R::CalcMaxBonesPerVertex(const jointWeight_t* jointWeights, int numVerts, const srfTriangles_s& tri) {
	int maxBonesPerVertex = 0;
	int weightIndex = 0;

	(void)tri;

	for (int vertNum = 0; vertNum < numVerts; ++vertNum) {
		int numWeightsForVertex = 0;

		do {
			++numWeightsForVertex;
		} while (jointWeights[weightIndex++].nextVertexOffset != 12);

		if (numWeightsForVertex > maxBonesPerVertex) {
			maxBonesPerVertex = numWeightsForVertex;
		}

		if (numWeightsForVertex > 4) {
			idLib::common->Warning(
				"Vertex %d in %s is weighted to %d transforms",
				vertNum,
				Name(),
				numWeightsForVertex);
		}
	}

	return maxBonesPerVertex;
}

void rvRenderModelMD5R::FixBadTangentSpaces(srfTriangles_s& tri) {
	for (int i = 0; i < tri.numVerts; ++i) {
		idDrawVert& v = tri.verts[i];

		const bool badTangent =
			(*(int*)&v.tangents[0].x == -4194304) ||
			(*(int*)&v.tangents[0].y == -4194304) ||
			(*(int*)&v.tangents[0].z == -4194304) ||
			(*(int*)&v.tangents[1].x == -4194304) ||
			(*(int*)&v.tangents[1].y == -4194304) ||
			(*(int*)&v.tangents[1].z == -4194304);

		if (!badTangent) {
			continue;
		}

		if (idMath::Fabs(v.normal.z) <= 0.7f) {
			const float lenSq = v.normal.x * v.normal.x + v.normal.y * v.normal.y;
			const float invLen = idMath::InvSqrt(lenSq);

			v.tangents[0].x = -invLen * v.normal.y;
			v.tangents[0].y = invLen * v.normal.x;
			v.tangents[0].z = 0.0f;

			v.tangents[1].x = -(v.tangents[0].y * v.normal.z);
			v.tangents[1].y = (v.tangents[0].x * v.normal.z);
			v.tangents[1].z = invLen * lenSq;
		}
		else {
			const float lenSq = v.normal.y * v.normal.y + v.normal.z * v.normal.z;
			const float invLen = idMath::InvSqrt(lenSq);

			v.tangents[1].x = 0.0f;
			v.tangents[1].y = invLen * v.normal.z;
			v.tangents[1].z = -invLen * v.normal.y;

			v.tangents[0].x = invLen * lenSq;
			v.tangents[0].y = -(v.tangents[1].z * v.normal.x);
			v.tangents[0].z = (v.tangents[1].y * v.normal.x);
		}
	}
}

void rvRenderModelMD5R::RemoveFromList(rvRenderModelMD5R* model) {
	rvRenderModelMD5R* prev = NULL;
	rvRenderModelMD5R* cur = ms_modelList;

	while (cur != NULL) {
		if (cur == model) {
			if (prev != NULL) {
				prev->m_next = cur->m_next;
			}
			else {
				ms_modelList = cur->m_next;
			}
			model->m_next = NULL;
			return;
		}
		prev = cur;
		cur = cur->m_next;
	}
}

void rvRenderModelMD5R::CompressVertexFormat(rvVertexFormat& compressedFormat, const rvVertexFormat& format) {
	compressedFormat.Init(format);

	if ((compressedFormat.m_flags & 0x10) != 0) {
		compressedFormat.m_dataTypes[4] = 19;
		compressedFormat.CalcSize();
	}
	if ((compressedFormat.m_flags & 0x20) != 0) {
		compressedFormat.m_dataTypes[5] = 19;
		compressedFormat.CalcSize();
	}
	if ((compressedFormat.m_flags & 0x40) != 0) {
		compressedFormat.m_dataTypes[6] = 19;
		compressedFormat.CalcSize();
	}
}

void rvRenderModelMD5R::WriteSilhouetteEdges(idFile& outFile, const char* prepend, const char* prependPlusTab) {
	if (m_numSilEdgesAdded <= 0) {
		return;
	}

	outFile.WriteFloatString("%sSilhouetteEdge[ %d ]\n", prepend, m_numSilEdgesAdded);
	outFile.WriteFloatString("%s{\n", prepend);

	for (int i = 0; i < m_numSilEdgesAdded; ++i) {
		outFile.WriteFloatString(
			"%s%d %d %d %d\n",
			prependPlusTab,
			m_silEdges[i].p1,
			m_silEdges[i].p2,
			m_silEdges[i].v1,
			m_silEdges[i].v2);
	}

	outFile.WriteFloatString("%s}\n", prepend);
}

void rvRenderModelMD5R::WriteLevelOfDetail(idFile& outFile, const char* prepend, const char* prependPlusTab) {
	if (m_numLODs <= 0) {
		return;
	}

	bool writeLODSection = (m_numLODs > 3);
	if (!writeLODSection) {
		for (int i = 0; i < m_numLODs; ++i) {
			const float expected = (i < 3)
				? MD5R_DefaultLODRanges[i]
				: (m_lods[i - 1].m_rangeEnd * 2.0f);

			if (m_lods[i].m_rangeEnd != expected) {
				writeLODSection = true;
				break;
			}
		}
	}

	if (!writeLODSection) {
		return;
	}

	outFile.WriteFloatString("%sLevelOfDetail[ %d ]\n", prepend, m_numLODs);
	outFile.WriteFloatString("%s{\n", prepend);

	for (int i = 0; i < m_numLODs; ++i) {
		outFile.WriteFloatString("%s%f\n", prependPlusTab, m_lods[i].m_rangeEnd);
	}

	outFile.WriteFloatString("%s}\n", prepend);
}

void rvRenderModelMD5R::WriteMeshes(idFile& outFile, const char* prepend) {
	if (m_numMeshes <= 0) {
		return;
	}

	idStr prependPlusTab(prepend);
	prependPlusTab += "\t";

	outFile.WriteFloatString("%sMesh[ %d ]\n", prepend, m_numMeshes);
	outFile.WriteFloatString("%s{\n", prepend);

	for (int i = 0; i < m_numMeshes; ++i) {
		m_meshes[i].Write(outFile, prependPlusTab.c_str());
	}

	outFile.WriteFloatString("%s}\n", prepend);
}

void rvRenderModelMD5R::GenerateCollisionModel(idCollisionModelManagerLocal& modelManager, idCollisionModelLocal& collisionModel) {
	bool hasCollisionMesh = false;

	for (int i = 0; i < m_numMeshes; ++i) {
		const idMaterial* material = m_meshes[i].m_material;
		if (material != NULL && (material->GetSurfaceFlags() & SURF_COLLISION) != 0) {
			hasCollisionMesh = true;
			break;
		}
	}

	for (int i = 0; i < m_numMeshes; ++i) {
		const idMaterial* material = m_meshes[i].m_material;
		if (material == NULL) {
			continue;
		}

		if ((material->GetContentFlags() & 0xFFCFFFFF) != 0 &&
			(!hasCollisionMesh || (material->GetSurfaceFlags() & SURF_COLLISION) != 0)) {
			m_meshes[i].GenerateCollisionPolys(modelManager, collisionModel);
		}
	}
}

void rvRenderModelMD5R::WriteSansBuffers(idFile& outFile, const char* prepend) {
	if (m_numMeshes <= 0) {
		return;
	}

	WriteMeshes(outFile, prepend);
	outFile.WriteFloatString(
		"%sBounds %f %f %f  %f %f %f\n",
		prepend,
		bounds.b[0].x, bounds.b[0].y, bounds.b[0].z,
		bounds.b[1].x, bounds.b[1].y, bounds.b[1].z);

	bool hasSky = false;
	for (int i = 0; i < m_numMeshes; ++i) {
		if (m_meshes[i].m_material != NULL && m_meshes[i].m_material->TestMaterialFlag(MF_SKY)) {
			hasSky = true;
			break;
		}
	}

	if (hasSky) {
		outFile.WriteFloatString("%sHasSky\n", prepend);
	}
}

void rvRenderModelMD5R::Shutdown() {
	idRenderModelStatic::PurgeModel();

	if (m_joints != NULL) {
		for (int i = 0; i < m_numJoints; ++i) {
			m_joints[i].~idMD5Joint();
		}
		Mem_Free(m_joints);
		m_joints = NULL;
	}

	if (m_skinSpaceToLocalMats != NULL) {
		Mem_Free16(m_skinSpaceToLocalMats);
		m_skinSpaceToLocalMats = NULL;
	}

	if (m_defaultPose != NULL) {
		Mem_Free16(m_defaultPose);
		m_defaultPose = NULL;
	}

	if (m_allocVertexBuffers != NULL) {
		delete[] m_allocVertexBuffers;
		m_allocVertexBuffers = NULL;
		m_vertexBuffers = NULL;
	}

	if (m_allocIndexBuffers != NULL) {
		delete[] m_allocIndexBuffers;
		m_allocIndexBuffers = NULL;
		m_indexBuffers = NULL;
	}

	if (m_allocSilEdges != NULL) {
		delete[] m_allocSilEdges;
		m_allocSilEdges = NULL;
		m_silEdges = NULL;
	}

	if (m_meshes != NULL) {
		delete[] m_meshes;
		m_meshes = NULL;
	}

	if (m_lods != NULL) {
		delete[] m_lods;
		m_lods = NULL;
	}

	ResetValues();
}

void rvRenderModelMD5R::PurgeModel() {
	Shutdown();
}

void rvRenderModelMD5R::Write(idFile& outFile, const char* prepend, bool compressed) {
	(void)compressed;

	if (IsDefaultModel()) {
		return;
	}

	idStr prependPlusTab(prepend);
	prependPlusTab += "\t";

	WriteJoints(outFile, prepend, prependPlusTab.c_str());

	if (m_numVertexBuffers > 0) {
		outFile.WriteFloatString("%sVertexBuffer[ %d ]\n", prepend, m_numVertexBuffers);
		outFile.WriteFloatString("%s{\n", prepend);

		for (int i = 0; i < m_numVertexBuffers; ++i) {
			m_vertexBuffers[i].Write(outFile, prependPlusTab.c_str());
		}

		outFile.WriteFloatString("%s}\n", prepend);
	}

	if (m_numIndexBuffers > 0) {
		outFile.WriteFloatString("%sIndexBuffer[ %d ]\n", prepend, m_numIndexBuffers);
		outFile.WriteFloatString("%s{\n", prepend);

		for (int i = 0; i < m_numIndexBuffers; ++i) {
			m_indexBuffers[i].Write(outFile, prependPlusTab.c_str());
		}

		outFile.WriteFloatString("%s}\n", prepend);
	}

	WriteSilhouetteEdges(outFile, prepend, prependPlusTab.c_str());
	WriteLevelOfDetail(outFile, prepend, prependPlusTab.c_str());
	WriteSansBuffers(outFile, prepend);
}

void rvRenderModelMD5R::InitFromFile(const char* fileName) {
	if (m_meshes != NULL) {
		Shutdown();
	}

	name = fileName;
	m_source = MD5R_SOURCE_FILE;
	LoadModel();
	reloadable = true;
}

void rvRenderModelMD5R::GenerateSurface(rvMesh& mesh) {
	modelSurface_t surf;

	memset(&surf, 0, sizeof(surf));
	surf.id = mesh.m_meshIdentifier;
	surf.shader = mesh.m_material;
	surf.geometry = GenerateStaticTriSurface(mesh.m_meshIdentifier);

	AddSurface(surf);
	mesh.m_surfaceNum = NumSurfaces() - 1;
}

void rvRenderModelMD5R::GenerateStaticSurfaces() {
	if (purged) {
		common->Warning("model %s instantiated while purged", Name());
		LoadModel();
	}

	for (int i = 0; i < m_numMeshes; ++i) {
		GenerateSurface(m_meshes[i]);
	}
}

idRenderModel* rvRenderModelMD5R::InstantiateDynamicModel(const renderEntity_s* ent, const viewDef_s* view, idRenderModel* cachedModel, unsigned int surfMask) {
	(void)view;

	if (m_numJoints == 0) {
		return NULL;
	}

	idRenderModelStatic* staticModel = static_cast<idRenderModelStatic*>(cachedModel);

	if (staticModel != NULL && !r_useCachedDynamicModels.GetBool()) {
		delete staticModel;
		staticModel = NULL;
	}

	if (purged) {
		common->Warning("model %s instantiated while purged", Name());
		LoadModel();
	}

	if (ent == NULL || ent->joints == NULL) {
		common->Printf("rvRenderModelMD5R::InstantiateDynamicModel: NULL joints on renderEntity for '%s'\n", Name());
		delete staticModel;
		return NULL;
	}

	if (ent->numJoints != m_numJoints) {
		common->Printf("rvRenderModelMD5R::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n", Name());
		delete staticModel;
		return NULL;
	}

	if (staticModel == NULL) {
		staticModel = new idRenderModelStatic;
	}

	staticModel->InitEmpty("_MD5R_Snapshot_");
	staticModel->bounds.Clear();

	for (int i = 0; i < m_numMeshes; ++i) {
		rvMesh& mesh = m_meshes[i];

		if (surfMask != 0 && (surfMask & (1u << mesh.m_meshIdentifier)) == 0) {
			continue;
		}

		modelSurface_t surf;
		memset(&surf, 0, sizeof(surf));

		surf.id = mesh.m_meshIdentifier;
		surf.shader = R_RemapShaderBySkin(
			mesh.m_material,
			(idDeclSkin*)ent->customSkin,
			ent->customShader);
		surf.geometry = GenerateStaticTriSurface(i);

		staticModel->AddSurface(surf);
		staticModel->bounds.AddBounds(surf.geometry->bounds);
	}

	return staticModel;
}

void rvRenderModelMD5R::LoadModel(void) {
	idToken			token;
	idVec3			mins;
	idVec3			maxs;
	Lexer* lexer;
	idRenderModel* tempModel;

	lexer = LexerFactory::MakeLexer(144);

	if (m_source == MD5R_SOURCE_MD5) {
		idRenderModelMD5* md5Model = NewRenderModel<idRenderModelMD5>(RV_HEAP_ID_LEVEL);
		if (md5Model == NULL) {
			common->Error("rvRenderModelMD5R: load failed, out of memory");
			if (lexer != NULL) {
				delete lexer;
			}
			return;
		}

		md5Model->InitFromFile(name.c_str());

		if (!md5Model->IsDefaultModel()) {
			Init(*md5Model, MD5R_SOURCE_MD5);
		}
		else {
			MakeDefaultModel();
		}

		md5Model->~idRenderModelMD5();
		if (lexer != NULL) {
			delete lexer;
		}
		return;
	}

	if (m_source == MD5R_SOURCE_LWO_ASE_FLT) {
		idRenderModelStatic* staticModel = NewRenderModel<idRenderModelStatic>(RV_HEAP_ID_LEVEL);
		if (staticModel == NULL) {
			common->Error("rvRenderModelMD5R: load failed, out of memory");
			if (lexer != NULL) {
				delete lexer;
			}
			return;
		}

		staticModel->InitFromFile(name.c_str());

		if (!staticModel->IsDefaultModel()) {
			Init(*staticModel, MD5R_SOURCE_LWO_ASE_FLT);
		}
		else {
			MakeDefaultModel();
		}

		staticModel->~idRenderModelStatic();
		if (lexer != NULL) {
			delete lexer;
		}
		return;
	}

	if (m_meshes) {
		Shutdown();
	}

	purged = false;

	if (!lexer->LoadFile(name.c_str(), 0)) {
		MakeDefaultModel();

		if (lexer != NULL) {
			delete lexer;
		}
		return;
	}

	lexer->ExpectTokenString("MD5RVersion");
	int version = lexer->ParseInt();
	if (version != 1) {
		lexer->Error("Invalid version %d.  Should be version %d\n", version, 1);
	}

	lexer->ReadToken(&token);

	if (!idStr::Icmp(token.c_str(), "CommandLine")) {
		lexer->ReadToken(&token);
	}

	if (!idStr::Icmp(token.c_str(), "Joint")) {
		ParseJoints(*lexer);
		lexer->ReadToken(&token);
	}

	if (idStr::Icmp(token.c_str(), "VertexBuffer")) {
		lexer->Error("Expected VertexBuffer keyword");
	}
	ParseVertexBuffers(*lexer);

	lexer->ReadToken(&token);

	if (!idStr::Icmp(token.c_str(), "IndexBuffer")) {
		ParseIndexBuffers(*lexer);
		lexer->ReadToken(&token);
	}

	if (!idStr::Icmp(token.c_str(), "SilhouetteEdge")) {
		ParseSilhouetteEdges(*lexer);
		lexer->ReadToken(&token);
	}

	if (!idStr::Icmp(token.c_str(), "LevelOfDetail")) {
		ParseLevelOfDetail(*lexer);
		lexer->ReadToken(&token);
	}

	if (idStr::Icmp(token.c_str(), "Mesh")) {
		lexer->Error("Expected Mesh keyword");
	}

	ParseMeshes(*lexer);
	BuildLevelsOfDetail();

	lexer->ExpectTokenString("Bounds");

	mins.x = lexer->ParseFloat(false);
	mins.y = lexer->ParseFloat(false);
	mins.z = lexer->ParseFloat(false);

	maxs.x = lexer->ParseFloat(false);
	maxs.y = lexer->ParseFloat(false);
	maxs.z = lexer->ParseFloat(false);

	bounds[0] = mins;
	bounds[1] = maxs;

	//if (lexer->ReadToken(&token) && !idStr::Icmp(token.c_str(), "HasSky")) {
	//	SetHasSky(true);
	//}

	fileSystem->ReadFile(name.c_str(), NULL, &timeStamp);

	if (!m_numJoints) {
		GenerateStaticSurfaces();
	}

	if (lexer != NULL) {
		delete lexer;
	}
}

void rvRenderModelMD5R::ParseJoints(Lexer& lex) {
	idToken token;

	lex.ExpectTokenString("[");
	m_numJoints = lex.ParseInt();
	lex.ExpectTokenString("]");
	lex.ExpectTokenString("{");

	// Original layout was: [count][idMD5Joint...]
	{
		void* mem = malloc(sizeof(int) + sizeof(idMD5Joint) * m_numJoints);
		if (mem == NULL) {
			lex.Error("Out of memory");
		}

		int* countPtr = reinterpret_cast<int*>(mem);
		*countPtr = m_numJoints;

		m_joints = reinterpret_cast<idMD5Joint*>(countPtr + 1);
		for (int i = 0; i < m_numJoints; ++i) {
			new (&m_joints[i]) idMD5Joint();
		}
	}

	m_skinSpaceToLocalMats = reinterpret_cast<idJointMat*>(Mem_Alloc16(sizeof(idJointMat) * m_numJoints, 0xE));
	m_defaultPose = reinterpret_cast<idJointQuat*>(Mem_Alloc16(sizeof(idJointQuat) * m_numJoints, 0xE));

	if (m_joints == NULL || m_skinSpaceToLocalMats == NULL || m_defaultPose == NULL) {
		lex.Error("Out of memory");
	}

	for (int curJoint = 0; curJoint < m_numJoints; ++curJoint) {
		lex.ReadToken(&token);

		m_joints[curJoint].name = token.c_str();

		const int parentIndex = lex.ParseInt();

		idVec3 t;
		t.x = lex.ParseFloat(0);
		t.y = lex.ParseFloat(0);
		t.z = lex.ParseFloat(0);

		idQuat q;
		q.x = lex.ParseFloat(0);
		q.y = lex.ParseFloat(0);
		q.z = lex.ParseFloat(0);
		q.w = idMath::Sqrt(idMath::Fabs(1.0f - (q.x * q.x + q.y * q.y + q.z * q.z)));

		idMat3 localMat = q.ToMat3();

		idJointMat& jointMat = m_skinSpaceToLocalMats[curJoint];

		jointMat.mat[0] = localMat[0][0];
		jointMat.mat[1] = localMat[1][0];
		jointMat.mat[2] = localMat[2][0];
		jointMat.mat[4] = localMat[0][1];
		jointMat.mat[5] = localMat[1][1];
		jointMat.mat[6] = localMat[2][1];
		jointMat.mat[8] = localMat[0][2];
		jointMat.mat[9] = localMat[1][2];
		jointMat.mat[10] = localMat[2][2];

		jointMat.mat[3] = t.x;
		jointMat.mat[7] = t.y;
		jointMat.mat[11] = t.z;

		if (parentIndex >= 0) {
			if (parentIndex >= (m_numJoints - 1)) {
				lex.Error("Invalid parent for joint '%s'", m_joints[curJoint].name.c_str());
			}

			m_joints[curJoint].parent = &m_joints[parentIndex];

			const idJointMat& parentMat = m_skinSpaceToLocalMats[parentIndex];

			idMat3 parentRot;
			parentRot[0][0] = parentMat.mat[0];
			parentRot[1][0] = parentMat.mat[1];
			parentRot[2][0] = parentMat.mat[2];
			parentRot[0][1] = parentMat.mat[4];
			parentRot[1][1] = parentMat.mat[5];
			parentRot[2][1] = parentMat.mat[6];
			parentRot[0][2] = parentMat.mat[8];
			parentRot[1][2] = parentMat.mat[9];
			parentRot[2][2] = parentMat.mat[10];

			idMat3 worldMat = parentRot * localMat;
			m_defaultPose[curJoint].q = worldMat.ToQuat();

			const float deltaX = jointMat.mat[3] - parentMat.mat[3];
			const float deltaY = jointMat.mat[7] - parentMat.mat[7];
			const float deltaZ = jointMat.mat[11] - parentMat.mat[11];

			m_defaultPose[curJoint].t.x =
				deltaX * parentMat.mat[0] +
				deltaY * parentMat.mat[4] +
				deltaZ * parentMat.mat[8];

			m_defaultPose[curJoint].t.y =
				deltaX * parentMat.mat[1] +
				deltaY * parentMat.mat[5] +
				deltaZ * parentMat.mat[9];

			m_defaultPose[curJoint].t.z =
				deltaX * parentMat.mat[2] +
				deltaY * parentMat.mat[6] +
				deltaZ * parentMat.mat[10];
		}
		else {
			m_joints[curJoint].parent = NULL;
			m_defaultPose[curJoint].q = q;
			m_defaultPose[curJoint].t = t;
		}
	}

	for (int i = 0; i < m_numJoints; ++i) {
		idJointMat& m = m_skinSpaceToLocalMats[i];

		const float tx = m.mat[3];
		const float ty = m.mat[7];
		const float tz = m.mat[11];

		m.mat[3] = -(ty * m.mat[4] + tx * m.mat[0] + tz * m.mat[8]);
		m.mat[7] = -(ty * m.mat[5] + tx * m.mat[1] + tz * m.mat[9]);
		m.mat[11] = -(tz * m.mat[10] + tx * m.mat[2] + ty * m.mat[6]);

		const float old01 = m.mat[1];
		const float old02 = m.mat[2];
		const float old06 = m.mat[6];

		m.mat[1] = m.mat[4];
		m.mat[2] = m.mat[8];
		m.mat[4] = old01;
		m.mat[6] = m.mat[9];
		m.mat[8] = old02;
		m.mat[9] = old06;
	}

	lex.ExpectTokenString("}");
}

void rvRenderModelMD5R::WriteJoints(idFile& outFile, const char* prepend, const char* prependPlusTab) {
	if (m_numJoints <= 0) {
		return;
	}

	outFile.WriteFloatString("%sJoint[ %d ]\n", prepend, m_numJoints);
	outFile.WriteFloatString("%s{\n", prepend);

	for (int curJoint = 0; curJoint < m_numJoints; ++curJoint) {
		const int parentIndex = (m_joints[curJoint].parent != NULL) ? static_cast<int>(m_joints[curJoint].parent - m_joints) : -1;

		idJointMat jointMat = m_skinSpaceToLocalMats[curJoint];

		// Convert the stored matrix back into the writable joint-space form.
		const float tx = jointMat.mat[3];
		const float ty = jointMat.mat[7];
		const float old06 = jointMat.mat[6];

		jointMat.mat[3] = -(jointMat.mat[8] * jointMat.mat[11] + jointMat.mat[4] * jointMat.mat[7] + jointMat.mat[3] * jointMat.mat[0]);
		jointMat.mat[7] = -(jointMat.mat[9] * jointMat.mat[11] + jointMat.mat[5] * jointMat.mat[7] + jointMat.mat[1] * tx);
		jointMat.mat[11] = -(jointMat.mat[10] * jointMat.mat[11] + jointMat.mat[6] * ty + jointMat.mat[2] * tx);

		const float old01 = jointMat.mat[1];
		const float old02 = jointMat.mat[2];

		jointMat.mat[6] = jointMat.mat[9];
		jointMat.mat[1] = jointMat.mat[4];
		jointMat.mat[2] = jointMat.mat[8];
		jointMat.mat[4] = old01;
		jointMat.mat[8] = old02;
		jointMat.mat[9] = old06;

		idJointQuat jointQuat;
		idCQuat cquat;

		jointQuat = jointMat.ToJointQuat();
		cquat = jointQuat.q.ToCQuat();

		outFile.WriteFloatString(
			"%s\"%s\" %d  %f %f %f  %f %f %f\n",
			prependPlusTab,
			m_joints[curJoint].name.c_str(),
			parentIndex,
			jointMat.mat[3],
			jointMat.mat[7],
			jointMat.mat[11],
			cquat.x,
			cquat.y,
			cquat.z
		);
	}

	outFile.WriteFloatString("%s}\n", prepend);
}

bool rvRenderModelMD5R::Init(idRenderModelStatic& renderModelStatic, rvMD5RSource_t source) {
	rvVertexFormat	vertexFormat;
	int				texDimArray[7];
	int				numDrawSurfs = 0;
	int				totalVerts = 0;
	int				totalIndices = 0;
	int				totalSilEdges = 0;
	int				numMeshes = 0;

	texDimArray[0] = 2;
	memset(&texDimArray[1], 0, sizeof(texDimArray) - sizeof(texDimArray[0]));

	if (m_meshes != NULL) {
		Shutdown();
	}

	InitEmpty(renderModelStatic.Name());

	m_source = source;
	purged = false;
	reloadable = true;

	for (int i = 0; i < renderModelStatic.surfaces.Num(); ++i) {
		const modelSurface_t& surf = renderModelStatic.surfaces[i];
		if (surf.geometry != NULL) {
			if ((surf.shader->GetSurfaceFlags() & 0x40) == 0) {
				++numDrawSurfs;
			}
		}
	}

	for (int i = 0; i < renderModelStatic.surfaces.Num(); ++i) {
		const modelSurface_t& surf = renderModelStatic.surfaces[i];
		const srfTriangles_t* tri = surf.geometry;

		if (tri == NULL) {
			continue;
		}

		if ((surf.shader->GetSurfaceFlags() & 0x40) != 0 && numDrawSurfs > 0) {
			continue;
		}

		totalVerts += tri->numVerts;
		totalIndices += tri->numIndexes;
		totalSilEdges += tri->numSilEdges;
		++numMeshes;
	}

	if (numMeshes == 0) {
		if (source != MD5R_SOURCE_PROC) {
			MakeDefaultModel();
		}
		vertexFormat.Shutdown();
		return false;
	}

	m_numVertexBuffers = 3;
	{
		int* mem = (int *)malloc(sizeof(int) + sizeof(rvVertexBuffer) * m_numVertexBuffers);
		if (mem == NULL) {
			idLib::common->FatalError("Out of memory");
		}

		*mem = m_numVertexBuffers;
		m_allocVertexBuffers = reinterpret_cast<rvVertexBuffer*>(mem + 1);
		m_vertexBuffers = m_allocVertexBuffers;

		for (int i = 0; i < m_numVertexBuffers; ++i) {
			new (&m_vertexBuffers[i]) rvVertexBuffer();
		}
	}

	vertexFormat.Init(1u, 4, 0, 1, 0, 0);
	m_vertexBuffers[0].Init(vertexFormat, totalVerts, 0x41u);

	vertexFormat.Init(0x4F1u, 4, 0, 1, texDimArray, 0);
	m_vertexBuffers[1].Init(vertexFormat, totalVerts, 0x83u);

	vertexFormat.Init(1u, 4, 0, 1, 0, 0);
	m_vertexBuffers[2].Init(vertexFormat, totalVerts * 2, 0x102u);

	m_numIndexBuffers = 2;
	{
		int* mem = (int*)malloc(sizeof(int) + sizeof(rvIndexBuffer) * m_numIndexBuffers);
		if (mem == NULL) {
			idLib::common->FatalError("Out of memory");
		}

		*mem = m_numIndexBuffers;
		m_allocIndexBuffers = reinterpret_cast<rvIndexBuffer*>(mem + 1);
		m_indexBuffers = m_allocIndexBuffers;

		for (int i = 0; i < m_numIndexBuffers; ++i) {
			new (&m_indexBuffers[i]) rvIndexBuffer();
		}
	}

	m_indexBuffers[0].Init(totalIndices, 1u);
	m_indexBuffers[1].Init(totalIndices, 3u);

	m_numSilEdges = totalSilEdges;
	m_numSilEdgesAdded = 0;
	m_allocSilEdges = new silEdge_t[m_numSilEdges];
	m_silEdges = m_allocSilEdges;
	if (m_silEdges == NULL) {
		idLib::common->FatalError("Out of memory");
	}

	m_numMeshes = numMeshes;
	{
		int* mem = (int*)malloc(sizeof(int) + sizeof(rvMesh) * m_numMeshes);
		if (mem == NULL) {
			idLib::common->FatalError("Out of memory");
		}

		*mem = m_numMeshes;
		m_meshes = reinterpret_cast<rvMesh*>(mem + 1);

		for (int i = 0; i < m_numMeshes; ++i) {
			new (&m_meshes[i]) rvMesh();
		}
	}

	int meshIndex = 0;
	int meshIdentifier = 0;

	for (int i = 0; i < renderModelStatic.surfaces.Num(); ++i) {
		modelSurface_t& surf = renderModelStatic.surfaces[i];
		srfTriangles_t* tri = surf.geometry;

		if (tri == NULL) {
			continue;
		}

		if ((surf.shader->GetSurfaceFlags() & 0x40) != 0 && numDrawSurfs > 0) {
			continue;
		}

		FixBadTangentSpaces(*tri);

		int* silRemap = R_CreateSilRemap(tri);

		m_meshes[meshIndex].Init(
			*this,
			surf,
			silRemap,
			0,
			1,
			2,
			0,
			1,
			-1
		);

		m_meshes[meshIndex].m_meshIdentifier = meshIdentifier;

		R_StaticFree(silRemap);

		++meshIndex;
		++meshIdentifier;
	}

	BuildLevelsOfDetail();

	bounds = renderModelStatic.bounds;

	m_vertexBuffers[0].Resize(m_vertexBuffers[0].m_numVerticesWritten);
	m_vertexBuffers[1].Resize(m_vertexBuffers[1].m_numVerticesWritten);
	m_vertexBuffers[2].Resize(m_vertexBuffers[2].m_numVerticesWritten);

	m_indexBuffers[0].Resize(m_indexBuffers[0].m_numIndicesWritten);
	m_indexBuffers[1].Resize(m_indexBuffers[1].m_numIndicesWritten);

	GenerateStaticSurfaces();

//	if (source == MD5R_SOURCE_PROC) {
//		SetHasSky(renderModelStatic.GetHasSky());
//	}
//	else {
//		SetHasSky(false);
//	}

	vertexFormat.Shutdown();
	return true;
}