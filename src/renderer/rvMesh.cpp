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
#pragma hdrstop

#include "tr_local.h"
#include "Model_local.h"
#include "Interaction.h"

static ID_INLINE void ClearBoundsToEmpty(idBounds& b) {
	b[0].x = idMath::INFINITY;
	b[0].y = idMath::INFINITY;
	b[0].z = idMath::INFINITY;
	b[1].x = -idMath::INFINITY;
	b[1].y = -idMath::INFINITY;
	b[1].z = -idMath::INFINITY;
}

static ID_INLINE void AddBoundsPoint(idBounds& b, const idVec3& p) {
	if (p.x < b[0].x) {
		b[0].x = p.x;
	}
	if (p.y < b[0].y) {
		b[0].y = p.y;
	}
	if (p.z < b[0].z) {
		b[0].z = p.z;
	}
	if (p.x > b[1].x) {
		b[1].x = p.x;
	}
	if (p.y > b[1].y) {
		b[1].y = p.y;
	}
	if (p.z > b[1].z) {
		b[1].z = p.z;
	}
}

/*
===============================================================================

	rvMesh batch allocation

===============================================================================
*/

rvPrimBatch* rvMesh::AllocPrimBatches(int count) {
	if (count <= 0) {
		return NULL;
	}

	rvPrimBatch* batches = new rvPrimBatch[count];
	return batches;
}

void rvMesh::FreePrimBatches(rvPrimBatch*& primBatches) {
	if (primBatches == NULL) {
		return;
	}

	delete[] primBatches;
	primBatches = NULL;
}

/*
===============================================================================

	rvMesh core

===============================================================================
*/

rvMesh::rvMesh() {
	ResetValues();
}

rvMesh::~rvMesh() {
	FreePrimBatches(m_primBatches);
	ResetValues();
}

void rvMesh::ResetValues() {
	ClearBoundsToEmpty(m_bounds);

	m_renderModel = NULL;
	m_material = NULL;
	m_nextInLOD = NULL;
	m_primBatches = NULL;
	m_numPrimBatches = 0;
	m_levelOfDetail = -1;
	m_surfaceNum = -1;
	m_meshIdentifier = 0;

	m_silTraceVertexBuffer = -1;
	m_silTraceIndexBuffer = -1;
	m_drawVertexBuffer = -1;
	m_drawIndexBuffer = -1;
	m_shadowVolVertexBuffer = -1;
	m_shadowVolIndexBuffer = -1;

	m_numSilTraceVertices = 0;
	m_numSilTraceIndices = 0;
	m_numSilTracePrimitives = 0;
	m_numSilEdges = 0;
	m_numDrawVertices = 0;
	m_numDrawIndices = 0;
	m_numDrawPrimitives = 0;
	m_numTransforms = 0;

	m_drawSetUp = false;
}

void rvMesh::CalcGeometryProfile() {
	m_numSilTraceVertices = 0;
	m_numSilTraceIndices = 0;
	m_numSilTracePrimitives = 0;
	m_numSilEdges = 0;
	m_numDrawVertices = 0;
	m_numDrawIndices = 0;
	m_numDrawPrimitives = 0;
	m_numTransforms = 0;

	for (int i = 0; i < m_numPrimBatches; ++i) {
		const rvPrimBatch& batch = m_primBatches[i];

		m_numSilTraceVertices += batch.m_silTraceGeoSpec.m_vertexCount;
		m_numSilTraceIndices += batch.m_silTraceGeoSpec.m_primitiveCount * 3;
		m_numSilTracePrimitives += batch.m_silTraceGeoSpec.m_primitiveCount;
		m_numSilEdges += batch.m_silEdgeCount;
		m_numDrawVertices += batch.m_drawGeoSpec.m_vertexCount;
		m_numDrawIndices += batch.m_drawGeoSpec.m_primitiveCount * 3;
		m_numDrawPrimitives += batch.m_drawGeoSpec.m_primitiveCount;
		m_numTransforms += batch.m_numTransforms;
	}
}

/*
===============================================================================

	draw/setup

===============================================================================
*/

const rvVertexBuffer* rvMesh::GetDrawVertexBufferFormat() const {
	if (m_drawVertexBuffer < 0 || m_renderModel == NULL) {
		return NULL;
	}
	return &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
}

const rvVertexBuffer* rvMesh::GetShadowVolVertexBufferFormat() const {
	if (m_shadowVolVertexBuffer < 0 || m_renderModel == NULL) {
		return NULL;
	}
	return &m_renderModel->m_vertexBuffers[m_shadowVolVertexBuffer];
}

void rvMesh::SetupForDrawRender(const rvVertexFormat* vertexComponentsNeeded) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0) {
		return;
	}

	rvVertexBuffer* vertexBuffer = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
	const rvVertexFormat* format = (vertexComponentsNeeded != NULL) ? vertexComponentsNeeded : &vertexBuffer->m_format;

	vertexBuffer->SetupForRender(0, *format);

	m_drawSetUp = true;
}

void rvMesh::SetupForShadowVolRender(const rvVertexFormat* vertexComponentsNeeded) {
	if (m_renderModel == NULL || m_shadowVolVertexBuffer < 0) {
		return;
	}

	rvVertexBuffer* vertexBuffer = &m_renderModel->m_vertexBuffers[m_shadowVolVertexBuffer];
	const rvVertexFormat* format = (vertexComponentsNeeded != NULL) ? vertexComponentsNeeded : &vertexBuffer->m_format;

	vertexBuffer->SetupForRender(0, *format);
}

/*
===============================================================================

	draw execution

===============================================================================
*/

void rvMesh::Draw(const float* skinToModelTransforms, const rvVertexFormat* vertexComponentsNeeded) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0 || m_drawIndexBuffer < 0) {
		return;
	}

	rvVertexBuffer* vertexBuffer = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
	rvIndexBuffer* indexBuffer = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].LoadMatrixPaletteIntoVPParams(skinToModelTransforms);
		m_primBatches[i].Draw(*vertexBuffer, *indexBuffer, vertexComponentsNeeded);
		skinToModelTransforms += 16 * m_primBatches[i].m_numTransforms;
	}

	m_drawSetUp = false;
}

void rvMesh::Draw(const float* skinToModelTransforms, int* indices, int /*numIndices*/, const rvVertexFormat* vertexComponentsNeeded) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0 || indices == NULL) {
		return;
	}

	int* batchIndexCounts = indices;
	int* batchIndices = indices + m_numPrimBatches;

	rvVertexBuffer* vertexBuffer = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].LoadMatrixPaletteIntoVPParams(skinToModelTransforms);
		m_primBatches[i].Draw(*vertexBuffer, batchIndices, batchIndexCounts[i], vertexComponentsNeeded);

		skinToModelTransforms += 16 * m_primBatches[i].m_numTransforms;
		batchIndices += batchIndexCounts[i];
	}

	m_drawSetUp = false;
}

void rvMesh::DrawShadowVolume(const float* skinToModelTransforms, int* indices, bool drawCaps, const rvVertexFormat* vertexComponentsNeeded) {
	if (m_renderModel == NULL || m_shadowVolVertexBuffer < 0) {
		return;
	}

	rvVertexBuffer* shadowVB = &m_renderModel->m_vertexBuffers[m_shadowVolVertexBuffer];

	if (indices == NULL) {
		if (m_shadowVolIndexBuffer < 0) {
			return;
		}

		rvIndexBuffer* shadowIB = &m_renderModel->m_indexBuffers[m_shadowVolIndexBuffer];

		for (int i = 0; i < m_numPrimBatches; ++i) {
			m_primBatches[i].LoadMatrixPaletteIntoVPParams(skinToModelTransforms);
			m_primBatches[i].DrawShadowVolume(*shadowVB, *shadowIB, drawCaps, vertexComponentsNeeded);
			skinToModelTransforms += 16 * m_primBatches[i].m_numTransforms;
		}

		m_drawSetUp = false;
		return;
	}

	int* batchCounts = indices;
	int* batchIndices = indices + (m_numPrimBatches * 2);

	for (int i = 0; i < m_numPrimBatches; ++i) {
		const int count = batchCounts[i * 2 + (drawCaps ? 1 : 0)];
		if (count > 0) {
			m_primBatches[i].LoadMatrixPaletteIntoVPParams(skinToModelTransforms);
			m_primBatches[i].DrawShadowVolume(*shadowVB, batchIndices, count, vertexComponentsNeeded);
		}

		skinToModelTransforms += 16 * m_primBatches[i].m_numTransforms;
		batchIndices += batchCounts[i * 2 + 1];
	}

	m_drawSetUp = false;
}

/*
===============================================================================

	culling / tracing / triangle derivation

===============================================================================
*/

srfCullInfo_t* rvMesh::FlipOutsideBackFaces(srfCullInfo_t& cullInfo) {
	unsigned char* cullBits = cullInfo.cullBits;
	unsigned char* facing = cullInfo.facing;

	rvIndexBuffer* silTraceIndexBuffer = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];
	srfCullInfo_t* numFlipped = NULL;

	for (int i = 0; i < m_numPrimBatches; ++i) {
		numFlipped = reinterpret_cast<srfCullInfo_t*>(
			reinterpret_cast<char*>(numFlipped) +
			m_primBatches[i].FlipOutsideBackFaces(facing, cullBits, *silTraceIndexBuffer));

		facing += m_primBatches[i].m_silTraceGeoSpec.m_primitiveCount;
		cullBits += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
	}

	return numFlipped;
}

void rvMesh::DeriveTriPlanes(idPlane* planes, const rvSilTraceVertT* silTraceVerts) {
	if (m_renderModel == NULL || m_silTraceIndexBuffer < 0) {
		return;
	}

	rvIndexBuffer* silTraceIB = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].DeriveTriPlanes(planes, silTraceVerts, *silTraceIB);
		silTraceVerts += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
		planes += m_primBatches[i].m_silTraceGeoSpec.m_primitiveCount;
	}
}

bool rvMesh::PreciseCullSurface(idBounds& ndcBounds, const rvSilTraceVertT* silTraceVerts, const idVec3& localView, const float* modelMatrix) {
	if (m_renderModel == NULL || m_silTraceIndexBuffer < 0) {
		return false;
	}

	rvIndexBuffer* silTraceIB = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		if (m_primBatches[i].PreciseCullSurface(ndcBounds, silTraceVerts, localView, modelMatrix, *silTraceIB)) {
			return true;
		}
		silTraceVerts += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
	}

	return false;
}

int rvMesh::CreateSilShadowVolTris(int primBatch, int* shadowIndices, const unsigned char* facing) {
	return m_primBatches[primBatch].CreateSilShadowVolTris(shadowIndices, facing, m_renderModel->m_silEdges);
}

int rvMesh::CreateFrontBackShadowVolTris(int primBatch, int* shadowIndices, const unsigned char* facing) {
	return m_primBatches[primBatch].CreateFrontBackShadowVolTris(
		shadowIndices,
		facing,
		m_renderModel->m_indexBuffers[m_silTraceIndexBuffer]);
}

void rvMesh::LocalTrace(
	localTrace_s& hit,
	int& c_testPlanes,
	int& c_testEdges,
	int& c_intersect,
	const idVec3& start,
	const idVec3& end,
	const unsigned char* cullBits,
	const idPlane* facePlanes,
	const rvSilTraceVertT* silTraceVerts,
	float radius,
	const idMaterial* shader) {

	if (m_renderModel == NULL || m_silTraceIndexBuffer < 0 || m_drawIndexBuffer < 0) {
		return;
	}

	rvIndexBuffer* silTraceIB = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];
	rvIndexBuffer* drawIB = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];

	c_testPlanes = 0;
	c_testEdges = 0;
	c_intersect = 0;

	for (int i = 0; i < m_numPrimBatches; ++i) {
		int batchIntersect = 0;

		m_primBatches[i].LocalTrace(
			(localTrace_t &)hit,
			c_testPlanes,
			c_testEdges,
			batchIntersect,
			start,
			end,
			cullBits,
			facePlanes,
			silTraceVerts,
			radius,
			*silTraceIB,
			*drawIB);

		if (batchIntersect > 0) {
			c_intersect += batchIntersect;
		}

		const int vertexCount = m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
		const int primitiveCount = m_primBatches[i].m_silTraceGeoSpec.m_primitiveCount;

		cullBits += vertexCount;
		silTraceVerts += vertexCount;
		facePlanes += primitiveCount;
	}

	// jmarshall - fix me
	//if (c_intersect > 0 && lastIntersectBatch >= 0) {
	//	hit.materialType = m_primBatches[lastIntersectBatch].GetMaterialType(
	//		const_cast<idMaterial*>(shader),
	//		hit,
	//		&m_renderModel->m_vertexBuffers[m_drawVertexBuffer]);
	//}
	// jmarshall end
}

/*
===============================================================================

	overlay / decal / collision

===============================================================================
*/

void rvMesh::CreateOverlayTriangles(
	overlayVertex_t* overlayVerts,
	int& numVerts,
	int* overlayIndices,
	int& numIndices,
	const idPlane* planes,
	const rvSilTraceVertT* silTraceVerts) {

	if (m_renderModel == NULL || m_silTraceIndexBuffer < 0) {
		return;
	}

	rvIndexBuffer* silTraceIndexBuffer = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];

	int vertexBase = 0;
	for (int i = 0; i < m_numPrimBatches; ++i) {
		const int vertexCount = m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;

		idVec2* texCoords = (idVec2*)_alloca(sizeof(idVec2) * vertexCount);
		byte* cullBits = (byte*)_alloca(vertexCount);

		SIMDProcessor->OverlayPointCull(cullBits, texCoords, planes, (const idDrawVert*)silTraceVerts, vertexCount);

		m_primBatches[i].FindOverlayTriangles(
			overlayVerts,
			numVerts,
			overlayIndices,
			numIndices,
			cullBits,
			texCoords,
			vertexBase,
			*silTraceIndexBuffer);

		vertexBase += vertexCount;
		silTraceVerts += vertexCount;
	}
}

void rvMesh::CreateDecalTriangles(
	idRenderModelDecal& decalModel,
	const decalProjectionInfo_s& localInfo,
	const idPlane* facePlanes,
	const rvSilTraceVertT* silTraceVerts) {

	if (m_renderModel == NULL || m_silTraceIndexBuffer < 0) {
		return;
	}

	rvIndexBuffer* silTraceIndexBuffer = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].FindDecalTriangles(
			decalModel,
			localInfo,
			facePlanes,
			silTraceVerts,
			*silTraceIndexBuffer);

		silTraceVerts += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
		if (facePlanes != NULL) {
			facePlanes += m_primBatches[i].m_silTraceGeoSpec.m_primitiveCount;
		}
	}
}

void rvMesh::GenerateCollisionPolys(idCollisionModelManagerLocal& modelManager, idCollisionModelLocal& collisionModel) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0 || m_drawIndexBuffer < 0) {
		return;
	}

	rvVertexBuffer* drawVertexBuffer = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
	rvIndexBuffer* drawIndexBuffer = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].GenerateCollisionPolys(
			modelManager,
			collisionModel,
			*m_material,
			*drawVertexBuffer,
			*drawIndexBuffer);
	}
}

/*
===============================================================================

	triangle extraction / transforms

===============================================================================
*/

void rvMesh::CopyTriangles(idDrawVert* destDrawVerts, int* destIndices, const rvSilTraceVertT* silTraceVerts) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0 || m_drawIndexBuffer < 0 || m_silTraceIndexBuffer < 0) {
		return;
	}

	rvVertexBuffer* drawVertexBuffer = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
	rvIndexBuffer* drawIndexBuffer = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];
	rvIndexBuffer* silTraceIndexBuffer = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];

	int baseVertex = 0;

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].CopyTriangles(
			destDrawVerts,
			destIndices,
			*drawVertexBuffer,
			*drawIndexBuffer,
			silTraceVerts,
			*silTraceIndexBuffer,
			baseVertex);

		baseVertex += m_primBatches[i].m_drawGeoSpec.m_vertexCount;
		destDrawVerts += m_primBatches[i].m_drawGeoSpec.m_vertexCount;
		destIndices += m_primBatches[i].m_drawGeoSpec.m_primitiveCount * 3;
		silTraceVerts += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
	}
}

void rvMesh::CopyTriangles(idDrawVert* destDrawVerts, int* destIndices, int destBase) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0 || m_drawIndexBuffer < 0) {
		return;
	}

	rvVertexBuffer* drawVertexBuffer = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
	rvIndexBuffer* drawIndexBuffer = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].CopyDrawVertices(destDrawVerts, *drawVertexBuffer);
		m_primBatches[i].CopyDrawIndices(destIndices, *drawIndexBuffer, destBase);

		destBase += m_primBatches[i].m_drawGeoSpec.m_vertexCount;
		destDrawVerts += m_primBatches[i].m_drawGeoSpec.m_vertexCount;
		destIndices += m_primBatches[i].m_drawGeoSpec.m_primitiveCount * 3;
	}
}

void rvMesh::TransformTriangles(idDrawVert* destDrawVerts, int* destIndices, const idMat4& transform, int colorShift, byte* colorAdd, int destBase) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0 || m_drawIndexBuffer < 0) {
		return;
	}

	rvVertexBuffer* drawVertexBuffer = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
	rvIndexBuffer* drawIndexBuffer = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].TransformDrawVertices(destDrawVerts, *drawVertexBuffer, transform, colorShift, colorAdd);
		m_primBatches[i].CopyDrawIndices(destIndices, *drawIndexBuffer, destBase);

		destBase += m_primBatches[i].m_drawGeoSpec.m_vertexCount;
		destDrawVerts += m_primBatches[i].m_drawGeoSpec.m_vertexCount;
		destIndices += m_primBatches[i].m_drawGeoSpec.m_primitiveCount * 3;
	}
}

void rvMesh::TubeDeform(idDrawVert* destDrawVerts, int* destIndices, const idVec3& localView, const rvSilTraceVertT* silTraceVerts) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0 || m_drawIndexBuffer < 0 || m_silTraceIndexBuffer < 0) {
		return;
	}

	rvVertexBuffer* drawVertexBuffer = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
	rvIndexBuffer* drawIndexBuffer = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];
	rvIndexBuffer* silTraceIndexBuffer = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].TubeDeform(
			destDrawVerts,
			destIndices,
			localView,
			*drawVertexBuffer,
			*drawIndexBuffer,
			silTraceVerts,
			*silTraceIndexBuffer);

		destDrawVerts += m_primBatches[i].m_drawGeoSpec.m_vertexCount;
		destIndices += m_primBatches[i].m_drawGeoSpec.m_primitiveCount * 3;
		silTraceVerts += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
	}
}

void rvMesh::PlaneForSurface(idPlane& destPlane, const rvSilTraceVertT* silTraceVerts) {
	if (m_renderModel == NULL || m_silTraceIndexBuffer < 0 || m_numPrimBatches <= 0) {
		destPlane.Zero();
		return;
	}

	m_primBatches[0].PlaneForSurface(
		destPlane,
		silTraceVerts,
		m_renderModel->m_indexBuffers[m_silTraceIndexBuffer]);
}

/*
===============================================================================

	init from model surface

===============================================================================
*/

void rvMesh::Init(
	rvRenderModelMD5R& renderModel,
	modelSurface_s& srcSurface,
	int* silRemap,
	short silTraceVB,
	short drawVB,
	short shadowVB,
	short silTraceIB,
	short drawIB,
	short shadowIB) {

	ResetValues();

	m_renderModel = &renderModel;
	m_material = srcSurface.shader;
	m_numPrimBatches = 1;
	m_primBatches = AllocPrimBatches(1);
	m_silTraceVertexBuffer = silTraceVB;
	m_silTraceIndexBuffer = silTraceIB;
	m_drawVertexBuffer = drawVB;
	m_drawIndexBuffer = drawIB;
	m_shadowVolVertexBuffer = shadowVB;
	m_shadowVolIndexBuffer = (srcSurface.geometry != NULL && srcSurface.geometry->shadowVertexes != NULL) ? shadowIB : -1;

	if (m_primBatches == NULL) {
		common->Error("rvMesh::Init: out of memory");
		return;
	}

	m_primBatches[0].Init(0, 1);

	srfTriangles_s* geometry = srcSurface.geometry;
	if (geometry == NULL) {
		return;
	}

	m_primBatches[0].m_silTraceGeoSpec.m_vertexStart = 0;
	m_primBatches[0].m_silTraceGeoSpec.m_indexStart = 0;
	m_primBatches[0].m_silTraceGeoSpec.m_vertexCount = geometry->numVerts;
	m_primBatches[0].m_silTraceGeoSpec.m_primitiveCount = geometry->numIndexes / 3;

	m_primBatches[0].m_drawGeoSpec.m_vertexStart = 0;
	m_primBatches[0].m_drawGeoSpec.m_indexStart = 0;
	m_primBatches[0].m_drawGeoSpec.m_vertexCount = geometry->numVerts;
	m_primBatches[0].m_drawGeoSpec.m_primitiveCount = geometry->numIndexes / 3;

	m_primBatches[0].m_shadowVolGeoSpec.m_vertexStart = 0;
	m_primBatches[0].m_shadowVolGeoSpec.m_indexStart = 0;
	m_primBatches[0].m_shadowVolGeoSpec.m_vertexCount = geometry->numVerts;
	m_primBatches[0].m_shadowVolGeoSpec.m_primitiveCount = geometry->numIndexes / 3;

	m_primBatches[0].m_numShadowPrimitivesNoCaps = geometry->numShadowIndexesNoCaps / 3;
	m_primBatches[0].m_shadowCapPlaneBits = geometry->shadowCapPlaneBits;
	m_primBatches[0].m_silEdgeStart = renderModel.m_numSilEdgesAdded;
	m_primBatches[0].m_silEdgeCount = geometry->numSilEdges;

	if (silRemap != NULL && geometry->silEdges != NULL && geometry->numSilEdges > 0) {
		for (int i = 0; i < geometry->numSilEdges; ++i) {
			silEdge_t newEdge;
			newEdge.p1 = geometry->silEdges[i].p1;
			newEdge.p2 = geometry->silEdges[i].p2;
			newEdge.v1 = silRemap[geometry->silEdges[i].v1];
			newEdge.v2 = silRemap[geometry->silEdges[i].v2];
			renderModel.m_silEdges[renderModel.m_numSilEdgesAdded++] = newEdge;
		}
	}

	CalcGeometryProfile();
	m_bounds = geometry->bounds;
}

/*
===============================================================================

	init from lexer

===============================================================================
*/

void rvMesh::Init(rvRenderModelMD5R& renderModel, Lexer& lex) {
	idToken token;
	idStr materialName;

	if (m_renderModel != NULL) {
		FreePrimBatches(m_primBatches);
		ResetValues();
	}

	lex.ExpectTokenString("{");
	if (!lex.ReadToken(&token)) {
		lex.Error("Expected keyword.");
		return;
	}

	if (!idStr::Icmp(token.c_str(), "LevelOfDetail")) {
		m_levelOfDetail = (short)lex.ParseInt();
		lex.ReadToken(&token);
	}

	if (idStr::Icmp(token.c_str(), "Material")) {
		lex.Error("Expected Material keyword.");
		return;
	}

	lex.ReadToken(&token);
	materialName = token.c_str();
	m_material = declManager->FindMaterial(materialName.c_str());

	if (!lex.ReadToken(&token)) {
		lex.Error("Expected mesh buffer keyword.");
		return;
	}

	if (!idStr::Icmp(token.c_str(), "SilTraceBuffers")) {
		m_silTraceVertexBuffer = (short)lex.ParseInt();
		m_silTraceIndexBuffer = (short)lex.ParseInt();
		lex.ReadToken(&token);
	}

	if (!idStr::Icmp(token.c_str(), "DrawBuffers")) {
		m_drawVertexBuffer = (short)lex.ParseInt();
		m_drawIndexBuffer = (short)lex.ParseInt();
		lex.ReadToken(&token);
	}

	if (!idStr::Icmp(token.c_str(), "ShadowVolumeBuffers")) {
		m_shadowVolVertexBuffer = (short)lex.ParseInt();
		m_shadowVolIndexBuffer = (short)lex.ParseInt();
		lex.ReadToken(&token);
	}

	if (idStr::Icmp(token.c_str(), "PrimBatch")) {
		lex.Error("Expected PrimBatch keyword.");
		return;
	}

	lex.ExpectTokenString("[");
	m_numPrimBatches = lex.ParseInt();
	lex.ExpectTokenString("]");
	lex.ExpectTokenString("{");

	m_primBatches = AllocPrimBatches(m_numPrimBatches);
	if (m_primBatches == NULL) {
		lex.Error("Out of memory");
		return;
	}

	for (int i = 0; i < m_numPrimBatches; ++i) {
		lex.ExpectTokenString("PrimBatch");
		m_primBatches[i].Init(lex);
	}

	CalcGeometryProfile();

	lex.ExpectTokenString("}");
	lex.ReadToken(&token);

	if (!idStr::Icmp(token.c_str(), "Bounds")) {
		m_bounds[0].x = lex.ParseFloat();
		m_bounds[0].y = lex.ParseFloat();
		m_bounds[0].z = lex.ParseFloat();
		m_bounds[1].x = lex.ParseFloat();
		m_bounds[1].y = lex.ParseFloat();
		m_bounds[1].z = lex.ParseFloat();
		lex.ReadToken(&token);
	}

	if (idStr::Icmp(token.c_str(), "}")) {
		lex.Error("couldn't find expected '}'");
		return;
	}

	m_renderModel = &renderModel;
}

/*
===============================================================================

	init skinned mesh

===============================================================================
*/

void rvMesh::Init(
	rvRenderModelMD5R& renderModel,
	const idMaterial* material,
	int numTransforms,
	rvBlend4DrawVert* drawVertArray,
	int numDrawVerts,
	int* drawIndices,
	int numIndices,
	silEdge_t* silEdges,
	int numSilEdges,
	int* silRemap,
	short silTraceVB,
	short drawVB,
	short shadowVB,
	short silTraceIB,
	short drawIB) {

	if (m_renderModel != NULL) {
		FreePrimBatches(m_primBatches);
		ResetValues();
	}

	m_renderModel = &renderModel;
	m_material = material;
	m_silTraceVertexBuffer = silTraceVB;
	m_silTraceIndexBuffer = silTraceIB;
	m_drawVertexBuffer = drawVB;
	m_drawIndexBuffer = drawIB;
	m_shadowVolVertexBuffer = shadowVB;
	m_shadowVolIndexBuffer = -1;

	m_numPrimBatches = 1;
	m_primBatches = AllocPrimBatches(1);
	if (m_primBatches == NULL) {
		common->Error("rvMesh::Init: out of memory");
		return;
	}

	m_primBatches[0].Init(0, 1);

	m_primBatches[0].m_numTransforms = numTransforms;
	m_primBatches[0].m_silTraceGeoSpec.m_vertexCount = numDrawVerts;
	m_primBatches[0].m_silTraceGeoSpec.m_primitiveCount = numIndices / 3;
	m_primBatches[0].m_drawGeoSpec.m_vertexCount = numDrawVerts;
	m_primBatches[0].m_drawGeoSpec.m_primitiveCount = numIndices / 3;
	m_primBatches[0].m_shadowVolGeoSpec.m_vertexCount = numDrawVerts * 2;
	m_primBatches[0].m_silEdgeStart = renderModel.m_numSilEdgesAdded;
	m_primBatches[0].m_silEdgeCount = numSilEdges;

	if (silEdges != NULL && silRemap != NULL && numSilEdges > 0) {
		for (int i = 0; i < numSilEdges; ++i) {
			silEdge_t newEdge;
			newEdge.p1 = silEdges[i].p1;
			newEdge.p2 = silEdges[i].p2;
			newEdge.v1 = silRemap[silEdges[i].v1];
			newEdge.v2 = silRemap[silEdges[i].v2];
			renderModel.m_silEdges[renderModel.m_numSilEdgesAdded++] = newEdge;
		}
	}

	ClearBoundsToEmpty(m_bounds);
	if (drawVertArray != NULL && numDrawVerts > 0) {
		for (int i = 0; i < numDrawVerts; ++i) {
			AddBoundsPoint(m_bounds, drawVertArray[i].xyz);
		}
	}

	CalcGeometryProfile();
}

/*
===============================================================================

	surface updates

===============================================================================
*/

void rvMesh::UpdateSurface(modelSurface_s& surface, const renderEntity_s& entity, idJointMat* skinSpaceToLocalMats) {
	surface.shader = m_material;

	++tr.pc.c_deformedSurfaces;
	tr.pc.c_deformedVerts += m_numSilTraceVertices;
	tr.pc.c_deformedIndexes += m_numSilTraceIndices;

	if (surface.geometry == NULL) {
		surface.geometry = R_AllocStaticTriSurf();
	}
	else {
		R_FreeStaticTriSurfVertexCaches(surface.geometry);
	}

	srfTriangles_s* tri = surface.geometry;
	tri->deformedSurface = true;
	tri->tangentsCalculated = true;
	tri->facePlanesCalculated = false;
	tri->numVerts = m_numSilTraceVertices;
	tri->numIndexes = m_numSilTraceIndices;
	tri->numSilEdges = m_numSilEdges;

	if (m_numSilEdges > 0 && m_numPrimBatches > 0) {
		tri->silEdges = &m_renderModel->m_silEdges[m_primBatches[0].m_silEdgeStart];
	}
	else {
		tri->silEdges = NULL;
	}

	tri->bounds = m_bounds;

	m_drawSetUp = false;
}

void rvMesh::UpdateSurface(modelSurface_s& surface) {
	surface.shader = m_material;

	if (surface.geometry == NULL) {
		surface.geometry = R_AllocStaticTriSurf();
	}
	else {
		R_FreeStaticTriSurfVertexCaches(surface.geometry);
	}

	srfTriangles_s* tri = surface.geometry;
	tri->deformedSurface = false;
	tri->tangentsCalculated = true;
	tri->facePlanesCalculated = false;
	tri->numVerts = m_numSilTraceVertices;
	tri->numIndexes = m_numSilTraceIndices;
	tri->numSilEdges = m_numSilEdges;
	tri->bounds = m_bounds;

	if (m_numSilEdges > 0 && m_numPrimBatches > 0) {
		tri->silEdges = &m_renderModel->m_silEdges[m_primBatches[0].m_silEdgeStart];
	}
	else {
		tri->silEdges = NULL;
	}
}

/*
===============================================================================

	light tris / front-face tris

===============================================================================
*/

void rvMesh::CreateLightTris(
	srfTriangles_s& newTri,
	int& c_backfaced,
	int& c_distance,
	srfCullInfo_t& cullInfo,
	const rvSilTraceVertT* silTraceVerts,
	bool includeBackFaces) {

	unsigned char* cullBits = cullInfo.cullBits;
	unsigned char* facing = cullInfo.facing;
	idPlane* localClipPlanes = cullInfo.localClipPlanes;

	rvIndexBuffer* silTraceIB = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];
	rvIndexBuffer* drawIB = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];

	const int maxIndexCount = m_numSilTraceIndices + m_numPrimBatches;
	R_AllocStaticTriSurfIndexes(&newTri, maxIndexCount);

	ClearBoundsToEmpty(newTri.bounds);

	int numIndices = 0;
	int* counts = newTri.indexes;
	int* batchIndexDest = counts + m_numPrimBatches;

	for (int i = 0; i < m_numPrimBatches; ++i) {
		int batchNumIndices = 0;
		int batchBackFaced = 0;
		int batchDistance = 0;
		idBounds batchBounds;

		m_primBatches[i].CreateLightTris(
			batchIndexDest,
			batchNumIndices,
			batchBounds,
			batchBackFaced,
			batchDistance,
			facing,
			cullBits,
			localClipPlanes,
			silTraceVerts,
			includeBackFaces,
			*silTraceIB,
			*drawIB);

		counts[i] = batchNumIndices;
		numIndices += batchNumIndices;
		c_backfaced += batchBackFaced;
		c_distance += batchDistance;

		if (batchNumIndices > 0) {
			AddBoundsPoint(newTri.bounds, batchBounds[0]);
			AddBoundsPoint(newTri.bounds, batchBounds[1]);
		}

		batchIndexDest += batchNumIndices;
		facing += m_primBatches[i].m_silTraceGeoSpec.m_primitiveCount;
		cullBits += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
		silTraceVerts += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
	}

	R_ResizeStaticTriSurfIndexes(&newTri, (numIndices > 0) ? (numIndices + m_numPrimBatches) : 0);
	newTri.numIndexes = numIndices;
}

void rvMesh::CreateFrontFaceTris(
	srfTriangles_s& newTri,
	int& c_backfaced,
	srfCullInfo_t& cullInfo,
	const rvSilTraceVertT* silTraceVerts) {

	unsigned char* facing = cullInfo.facing;

	rvIndexBuffer* silTraceIB = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];
	rvIndexBuffer* drawIB = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];

	const int maxIndexCount = m_numSilTraceIndices + m_numPrimBatches;
	R_AllocStaticTriSurfIndexes(&newTri, maxIndexCount);

	ClearBoundsToEmpty(newTri.bounds);

	int numIndices = 0;
	int* counts = newTri.indexes;
	int* batchIndexDest = counts + m_numPrimBatches;

	for (int i = 0; i < m_numPrimBatches; ++i) {
		int batchNumIndices = 0;
		int batchBackFaced = 0;
		idBounds batchBounds;

		m_primBatches[i].CreateFrontFaceTris(
			batchIndexDest,
			batchNumIndices,
			batchBounds,
			batchBackFaced,
			facing,
			silTraceVerts,
			*silTraceIB,
			*drawIB);

		counts[i] = batchNumIndices;
		numIndices += batchNumIndices;
		c_backfaced += batchBackFaced;

		if (batchNumIndices > 0) {
			AddBoundsPoint(newTri.bounds, batchBounds[0]);
			AddBoundsPoint(newTri.bounds, batchBounds[1]);
		}

		batchIndexDest += batchNumIndices;
		facing += m_primBatches[i].m_silTraceGeoSpec.m_primitiveCount;
		silTraceVerts += m_primBatches[i].m_silTraceGeoSpec.m_vertexCount;
	}

	R_ResizeStaticTriSurfIndexes(&newTri, (numIndices > 0) ? (numIndices + m_numPrimBatches) : 0);
	newTri.numIndexes = numIndices;
}

/*
===============================================================================

	serialization / texture axis

===============================================================================
*/

void rvMesh::Write(idFile& outFile, const char* prepend) {
	idStr indent = prepend;
	idStr childIndent = indent;
	childIndent += "\t";

	outFile.WriteFloatString("%sMesh\n", prepend);
	outFile.WriteFloatString("%s{\n", prepend);

	if (m_levelOfDetail != -1) {
		outFile.WriteFloatString("%sLevelOfDetail %d\n", childIndent.c_str(), m_levelOfDetail);
	}

	if (m_material != NULL) {
		outFile.WriteFloatString("%sMaterial \"%s\"\n", childIndent.c_str(), m_material->GetName());
	}

	if (m_silTraceVertexBuffer >= 0 || m_silTraceIndexBuffer >= 0) {
		outFile.WriteFloatString(
			"%sSilTraceBuffers %d %d\n",
			childIndent.c_str(),
			m_silTraceVertexBuffer,
			m_silTraceIndexBuffer);
	}

	if (m_drawVertexBuffer >= 0 || m_drawIndexBuffer >= 0) {
		outFile.WriteFloatString(
			"%sDrawBuffers %d %d\n",
			childIndent.c_str(),
			m_drawVertexBuffer,
			m_drawIndexBuffer);
	}

	if (m_shadowVolVertexBuffer >= 0 || m_shadowVolIndexBuffer >= 0) {
		outFile.WriteFloatString(
			"%sShadowVolumeBuffers %d %d\n",
			childIndent.c_str(),
			m_shadowVolVertexBuffer,
			m_shadowVolIndexBuffer);
	}

	outFile.WriteFloatString("%sPrimBatch[ %d ]\n", childIndent.c_str(), m_numPrimBatches);
	outFile.WriteFloatString("%s{\n", childIndent.c_str());

	idStr batchIndent = childIndent;
	batchIndent += "\t";

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].Write(outFile, batchIndent.c_str());
	}

	outFile.WriteFloatString("%s}\n", childIndent.c_str());

	if (m_bounds[0].x <= m_bounds[1].x) {
		outFile.WriteFloatString(
			"%sBounds %f %f %f  %f %f %f\n",
			childIndent.c_str(),
			m_bounds[0].x, m_bounds[0].y, m_bounds[0].z,
			m_bounds[1].x, m_bounds[1].y, m_bounds[1].z);
	}

	outFile.WriteFloatString("%s}\n", prepend);
}

void rvMesh::SurfaceToTextureAxis(idVec3& origin, idVec3* axis, const rvSilTraceVertT* silTraceVerts) {
	if (m_renderModel == NULL || m_drawVertexBuffer < 0 || m_drawIndexBuffer < 0 || m_silTraceIndexBuffer < 0 || m_numPrimBatches <= 0) {
		origin.Zero();
		axis[0].Zero();
		axis[1].Zero();
		axis[2].Zero();
		return;
	}

	rvVertexBuffer* drawVB = &m_renderModel->m_vertexBuffers[m_drawVertexBuffer];
	rvIndexBuffer* drawIB = &m_renderModel->m_indexBuffers[m_drawIndexBuffer];
	rvIndexBuffer* silTraceIB = &m_renderModel->m_indexBuffers[m_silTraceIndexBuffer];

	float bounds[2][2];
	bounds[0][0] = bounds[0][1] = 1.0e30f;
	bounds[1][0] = bounds[1][1] = -1.0e30f;

	for (int i = 0; i < m_numPrimBatches; ++i) {
		m_primBatches[i].GetTextureBounds(bounds, *drawVB);
	}

	const float boundsOrgS = idMath::Floor((bounds[1][0] + bounds[0][0]) * 0.5f);
	const float boundsOrgT = idMath::Floor((bounds[1][1] + bounds[0][1]) * 0.5f);

	idDrawVert a, b, c;
	m_primBatches[0].GetTriangle(a, b, c, 0, *drawVB, *drawIB, silTraceVerts, *silTraceIB);

	const idVec3 d0 = b.xyz - a.xyz;
	const idVec3 d1 = c.xyz - a.xyz;
	const float ds0 = b.st.x - a.st.x;
	const float dt0 = b.st.y - a.st.y;
	const float ds1 = c.st.x - a.st.x;
	const float dt1 = c.st.y - a.st.y;

	const float det = dt1 * ds0 - ds1 * dt0;
	if (det == 0.0f) {
		axis[0].Zero();
		axis[1].Zero();
		axis[2].Zero();
		origin.Zero();
		return;
	}

	const float invDet = 1.0f / det;

	axis[0].x = (dt1 * d0.x - d1.x * dt0) * invDet;
	axis[0].y = (dt1 * d0.y - d1.y * dt0) * invDet;
	axis[0].z = (dt1 * d0.z - d1.z * dt0) * invDet;

	axis[1].x = (d1.x * ds0 - ds1 * d0.x) * invDet;
	axis[1].y = (d1.y * ds0 - ds1 * d0.y) * invDet;
	axis[1].z = (d1.z * ds0 - ds1 * d0.z) * invDet;

	idVec3 normal = (c.xyz - b.xyz).Cross(a.xyz - b.xyz);
	normal.Normalize();
	axis[2] = normal;

	origin = a.xyz + axis[0] * (boundsOrgS - a.st.x) + axis[1] * (boundsOrgT - a.st.y);
}