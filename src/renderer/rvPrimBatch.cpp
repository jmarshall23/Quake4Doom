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

#undef min
#undef max

namespace
{
    constexpr int kTubeEdgePairs[6][2] =
    {
        {0, 1},
        {0, 2},
        {0, 3},
        {1, 2},
        {1, 3},
        {2, 3},
    };

    struct Float3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct FixedWindingAccess : public idFixedWinding
    {
        static FixedWindingAccess& Ref(idFixedWinding& winding)
        {
            return reinterpret_cast<FixedWindingAccess&>(winding);
        }

        static const FixedWindingAccess& Ref(const idFixedWinding& winding)
        {
            return reinterpret_cast<const FixedWindingAccess&>(winding);
        }

        static int& NumPoints(idFixedWinding& winding)
        {
            return Ref(winding).numPoints;
        }

        static const int& NumPoints(const idFixedWinding& winding)
        {
            return Ref(winding).numPoints;
        }

        static int& AllocedSize(idFixedWinding& winding)
        {
            return Ref(winding).allocedSize;
        }

        static idVec5* Points(idFixedWinding& winding)
        {
            return Ref(winding).p;
        }

        static const idVec5* Points(const idFixedWinding& winding)
        {
            return Ref(winding).p;
        }

        static void ReAllocate(idFixedWinding& winding, int numPoints, bool keep)
        {
            Ref(winding).ReAllocate(winding, numPoints, keep);
        }
    };

    inline const rvDeclMatType* GetBaseMaterialType(const idMaterial* shader)
    {
        return shader != nullptr ? shader->GetMaterialType() : nullptr;
    }

    inline const rvDeclMatType* GetMappedMaterialType(const idMaterial* shader, idVec2& st)
    {
        return shader != nullptr ? shader->GetMaterialType(st) : nullptr;
    }

    inline bool Is16BitIndexBuffer(const rvIndexBuffer& indexBuffer)
    {
        return (indexBuffer.m_flags & 4) != 0;
    }

    inline unsigned int GetGlIndexType(const rvIndexBuffer& indexBuffer)
    {
        return Is16BitIndexBuffer(indexBuffer) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    }

    inline int GetIndexSizeBytes(const rvIndexBuffer& indexBuffer)
    {
        return Is16BitIndexBuffer(indexBuffer) ? 2 : 4;
    }

    inline int ReadIndex(const void* indexData, bool is16Bit, int index)
    {
        if (is16Bit)
        {
            return static_cast<int>(reinterpret_cast<const unsigned short*>(indexData)[index]);
        }

        return static_cast<int>(reinterpret_cast<const unsigned int*>(indexData)[index]);
    }

    template <typename T>
    inline T* ByteOffset(T* base, int strideBytes, int index = 0)
    {
        return reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(base) + strideBytes * index);
    }

    template <typename T>
    inline const T* ByteOffset(const T* base, int strideBytes, int index = 0)
    {
        return reinterpret_cast<const T*>(reinterpret_cast<const unsigned char*>(base) + strideBytes * index);
    }

    inline Float3 MakeFloat3(const idVec3& v)
    {
        Float3 out;
        out.x = v.x;
        out.y = v.y;
        out.z = v.z;
        return out;
    }

    inline Float3 MakeFloat3(const rvSilTraceVertT& v)
    {
        Float3 out;
        out.x = v.xyzw.x;
        out.y = v.xyzw.y;
        out.z = v.xyzw.z;
        return out;
    }

    inline void StoreFloat3(idVec3& dst, const Float3& src)
    {
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
    }

    inline Float3 Add(const Float3& a, const Float3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    inline Float3 Sub(const Float3& a, const Float3& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    inline Float3 Mul(const Float3& v, float s)
    {
        return { v.x * s, v.y * s, v.z * s };
    }

    inline float Dot(const Float3& a, const Float3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline Float3 Cross(const Float3& a, const Float3& b)
    {
        return
        {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    inline float LengthSquared(const Float3& v)
    {
        return Dot(v, v);
    }

    inline float DistanceSquared(const Float3& a, const Float3& b)
    {
        return LengthSquared(Sub(a, b));
    }

    inline Float3 Normalize(const Float3& v)
    {
        const float lenSqr = LengthSquared(v);
        if (lenSqr <= 0.0f)
        {
            return {};
        }

        const float invLen = 1.0f / sqrt(lenSqr);
        return Mul(v, invLen);
    }

    inline float PlaneDistance(const idPlane& plane, const Float3& point)
    {
        return plane.a * point.x + plane.b * point.y + plane.c * point.z + plane.d;
    }

    inline void ExpandBounds(idBounds& bounds, const Float3& p)
    {
        if (p.x < bounds.b[0].x) bounds.b[0].x = p.x;
        if (p.y < bounds.b[0].y) bounds.b[0].y = p.y;
        if (p.z < bounds.b[0].z) bounds.b[0].z = p.z;

        if (p.x > bounds.b[1].x) bounds.b[1].x = p.x;
        if (p.y > bounds.b[1].y) bounds.b[1].y = p.y;
        if (p.z > bounds.b[1].z) bounds.b[1].z = p.z;
    }

    inline void InitBounds(idBounds& bounds, const Float3& p)
    {
        bounds.b[0].x = bounds.b[1].x = p.x;
        bounds.b[0].y = bounds.b[1].y = p.y;
        bounds.b[0].z = bounds.b[1].z = p.z;
    }

    inline void ComputeBoundsFromIndexedSilVerts(
        idBounds& bounds,
        const rvSilTraceVertT* silTraceVerts,
        const int* indices,
        int numIndices)
    {
        if (numIndices <= 0)
        {
            bounds.b[0].x = bounds.b[0].y = bounds.b[0].z = 0.0f;
            bounds.b[1] = bounds.b[0];
            return;
        }

        InitBounds(bounds, MakeFloat3(silTraceVerts[indices[0]]));

        for (int i = 1; i < numIndices; ++i)
        {
            ExpandBounds(bounds, MakeFloat3(silTraceVerts[indices[i]]));
        }
    }

    inline void FixDegenerateNormal(idPlane& plane)
    {
        if (fabs(plane.a) == 1.0f)
        {
            plane.a = plane.a > 0.0f ? 1.0f : -1.0f;
            plane.b = 0.0f;
            plane.c = 0.0f;
            return;
        }

        if (fabs(plane.b) == 1.0f)
        {
            plane.a = 0.0f;
            plane.b = plane.b > 0.0f ? 1.0f : -1.0f;
            plane.c = 0.0f;
            return;
        }

        if (fabs(plane.c) == 1.0f)
        {
            plane.a = 0.0f;
            plane.b = 0.0f;
            plane.c = plane.c > 0.0f ? 1.0f : -1.0f;
        }
    }

    inline bool BuildPlaneFromTriangle(idPlane& plane, const Float3& a, const Float3& b, const Float3& c)
    {
        const Float3 edge0 = Sub(c, b);
        const Float3 edge1 = Sub(a, b);
        Float3 normal = Cross(edge0, edge1);

        const float normalLenSqr = LengthSquared(normal);
        if (normalLenSqr == 0.0f)
        {
            plane.a = plane.b = plane.c = plane.d = 0.0f;
            return false;
        }

        normal = Normalize(normal);

        plane.a = normal.x;
        plane.b = normal.y;
        plane.c = normal.z;
        FixDegenerateNormal(plane);
        plane.d = -(plane.a * b.x + plane.b * b.y + plane.c * b.z);
        return true;
    }

    inline bool TriangleCullMaskReject(unsigned char mask)
    {
        return (((mask ^ (mask >> 4)) & 3u) == 0u) && (((mask ^ (mask >> 1)) & 4u) == 0u);
    }

    inline bool SegmentPlaneIntersection(
        const idPlane& plane,
        const Float3& start,
        const Float3& end,
        float maxFraction,
        float& fraction,
        Float3& hitPoint)
    {
        const float startDist = PlaneDistance(plane, start);
        const float endDist = PlaneDistance(plane, end);

        if (!(startDist > endDist && startDist >= 0.0f && endDist <= 0.0f))
        {
            return false;
        }

        const float denom = startDist - endDist;
        if (denom == 0.0f)
        {
            return false;
        }

        fraction = startDist / denom;
        if (fraction < 0.0f || fraction >= maxFraction)
        {
            return false;
        }

        hitPoint = Add(start, Mul(Sub(end, start), fraction));
        return true;
    }

    inline bool PointInTriangle(const Float3& p, const Float3& a, const Float3& b, const Float3& c, const idPlane& plane)
    {
        const Float3 n{ plane.a, plane.b, plane.c };

        const Float3 c0 = Cross(Sub(b, a), Sub(p, a));
        if (Dot(c0, n) < 0.0f)
        {
            return false;
        }

        const Float3 c1 = Cross(Sub(c, b), Sub(p, b));
        if (Dot(c1, n) < 0.0f)
        {
            return false;
        }

        const Float3 c2 = Cross(Sub(a, c), Sub(p, c));
        return Dot(c2, n) >= 0.0f;
    }

    inline Float3 ClosestPointOnTriangle(const Float3& p, const Float3& a, const Float3& b, const Float3& c)
    {
        const Float3 ab = Sub(b, a);
        const Float3 ac = Sub(c, a);
        const Float3 ap = Sub(p, a);

        const float d1 = Dot(ab, ap);
        const float d2 = Dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f)
        {
            return a;
        }

        const Float3 bp = Sub(p, b);
        const float d3 = Dot(ab, bp);
        const float d4 = Dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
        {
            return b;
        }

        const float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            const float v = d1 / (d1 - d3);
            return Add(a, Mul(ab, v));
        }

        const Float3 cp = Sub(p, c);
        const float d5 = Dot(ab, cp);
        const float d6 = Dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
        {
            return c;
        }

        const float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            const float w = d2 / (d2 - d6);
            return Add(a, Mul(ac, w));
        }

        const float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            const Float3 bc = Sub(c, b);
            const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return Add(b, Mul(bc, w));
        }

        const float denom = 1.0f / (va + vb + vc);
        const float v = vb * denom;
        const float w = vc * denom;
        return Add(a, Add(Mul(ab, v), Mul(ac, w)));
    }

    inline float DistanceSquaredPointTriangle(const Float3& p, const Float3& a, const Float3& b, const Float3& c)
    {
        return DistanceSquared(p, ClosestPointOnTriangle(p, a, b, c));
    }

    inline Float3 TransformPoint(const idMat4& transform, const Float3& in)
    {
        const float* m = transform.ToFloatPtr();

        const float w =
            in.x * m[12] +
            in.y * m[13] +
            in.z * m[14] +
            m[15];

        if (w == 0.0f)
        {
            return {};
        }

        Float3 out;
        out.x =
            in.x * m[0] +
            in.y * m[1] +
            in.z * m[2] +
            m[3];

        out.y =
            in.x * m[4] +
            in.y * m[5] +
            in.z * m[6] +
            m[7];

        out.z =
            in.x * m[8] +
            in.y * m[9] +
            in.z * m[10] +
            m[11];

        if (w != 1.0f)
        {
            const float invW = 1.0f / w;
            out = Mul(out, invW);
        }

        return out;
    }

    inline void SetDrawVertPosition(idDrawVert& vert, const Float3& pos)
    {
        vert.xyz.x = pos.x;
        vert.xyz.y = pos.y;
        vert.xyz.z = pos.z;
    }

    inline void SetDrawVertTexCoord(idDrawVert& vert, const float* st)
    {
        vert.st.x = st[0];
        vert.st.y = st[1];
    }

    inline void SetDrawVertColor(idDrawVert& vert, const unsigned char* color)
    {
        vert.color[0] = color[0];
        vert.color[1] = color[1];
        vert.color[2] = color[2];
        vert.color[3] = color[3];
    }

    inline void FillTriangleCopyVertex(
        idDrawVert& dst,
        const rvSilTraceVertT& srcPos,
        const float* texCoordBase,
        int texCoordStride,
        const unsigned char* colorBase,
        int colorStride,
        int drawVertexIndex)
    {
        dst.xyz.x = srcPos.xyzw.x;
        dst.xyz.y = srcPos.xyzw.y;
        dst.xyz.z = srcPos.xyzw.z;

        const float* st = ByteOffset(texCoordBase, texCoordStride, drawVertexIndex);
        SetDrawVertTexCoord(dst, st);

        const unsigned char* color = ByteOffset(colorBase, colorStride, drawVertexIndex);
        SetDrawVertColor(dst, color);
    }
} // namespace


void rvPrimBatch::IndexedTriListSpec::Reset()
{
    m_vertexStart = 0;
    m_vertexCount = 0;
    m_indexStart = -1;
    m_primitiveCount = 0;
}

rvPrimBatch::rvPrimBatch()
{
    m_silTraceGeoSpec.Reset();
    m_drawGeoSpec.Reset();
    m_shadowVolGeoSpec.Reset();
}

rvPrimBatch::~rvPrimBatch()
{
    Shutdown();
}

void rvPrimBatch::LoadMatrixPaletteIntoVPParams(const float* skinToModelTransforms)
{
    if (m_transformPalette == nullptr || skinToModelTransforms == nullptr || m_numTransforms <= 0)
    {
        return;
    }

    const float* matrix = skinToModelTransforms;
    unsigned int programParam = 0;

    for (int i = 0; i < m_numTransforms; ++i, matrix += 16, programParam += 3)
    {
        qglProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, programParam + 0, matrix + 0);
        qglProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, programParam + 1, matrix + 4);
        qglProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, programParam + 2, matrix + 8);
    }
}

int rvPrimBatch::FlipOutsideBackFaces(unsigned char* facing, const unsigned char* cullBits, rvIndexBuffer& indexBuffer)
{
    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    int outsideBackFaceCount = 0;

    void* indexData = nullptr;
    indexBuffer.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &indexData);

    const bool is16Bit = Is16BitIndexBuffer(indexBuffer);
    const int numTris = m_silTraceGeoSpec.m_primitiveCount;

    for (int tri = 0; tri < numTris; ++tri)
    {
        if (facing[tri] != 0)
        {
            continue;
        }

        const int i0 = ReadIndex(indexData, is16Bit, tri * 3 + 0);
        const int i1 = ReadIndex(indexData, is16Bit, tri * 3 + 1);
        const int i2 = ReadIndex(indexData, is16Bit, tri * 3 + 2);

        if ((cullBits[i0] & cullBits[i1] & cullBits[i2]) != 0)
        {
            facing[tri] = 1;
        }
        else
        {
            ++outsideBackFaceCount;
        }
    }

    indexBuffer.Unlock();
    return outsideBackFaceCount;
}

int rvPrimBatch::CreateSilShadowVolTris(int* shadowIndices, const unsigned char* facing, silEdge_t* silEdgeBuffer)
{
    int* out = shadowIndices;
    const int shadowVertexBase = m_shadowVolGeoSpec.m_vertexStart;

    silEdge_t* edge = silEdgeBuffer + m_silEdgeStart;
    silEdge_t* end = edge + m_silEdgeCount;

    for (; edge < end; ++edge)
    {
        const unsigned int face0 = facing[edge->p1];
        const unsigned int face1 = facing[edge->p2];
        if (face0 == face1)
        {
            continue;
        }

        const int v1 = edge->v1 * 2;
        const int v2 = edge->v2 * 2;

        out[0] = shadowVertexBase + v1;
        out[1] = shadowVertexBase + (face0 ^ v2);
        out[2] = shadowVertexBase + (face1 ^ v2);
        out[3] = shadowVertexBase + (face1 ^ v1);
        out[4] = shadowVertexBase + (face0 ^ v1);
        out[5] = shadowVertexBase + (v2 ^ 1);
        out += 6;
    }

    return static_cast<int>(out - shadowIndices);
}

int rvPrimBatch::CreateFrontBackShadowVolTris(int* shadowIndices, const unsigned char* facing, rvIndexBuffer& indexBuffer)
{
    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    const int shadowVertexBase = m_shadowVolGeoSpec.m_vertexStart;

    void* indexData = nullptr;
    indexBuffer.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &indexData);

    const bool is16Bit = Is16BitIndexBuffer(indexBuffer);
    int* out = shadowIndices;

    for (int tri = 0; tri < m_silTraceGeoSpec.m_primitiveCount; ++tri)
    {
        if (facing[tri] != 0)
        {
            continue;
        }

        const int i0 = ReadIndex(indexData, is16Bit, tri * 3 + 0);
        const int i1 = ReadIndex(indexData, is16Bit, tri * 3 + 1);
        const int i2 = ReadIndex(indexData, is16Bit, tri * 3 + 2);

        const int v0 = 2 * i0;
        const int v1 = 2 * i1;
        const int v2 = 2 * i2;

        out[0] = shadowVertexBase + v2;
        out[1] = shadowVertexBase + v1;
        out[2] = shadowVertexBase + v0;
        out[3] = shadowVertexBase + (v0 ^ 1);
        out[4] = shadowVertexBase + (v1 ^ 1);
        out[5] = shadowVertexBase + (v2 ^ 1);
        out += 6;
    }

    indexBuffer.Unlock();
    return static_cast<int>(out - shadowIndices);
}

void rvPrimBatch::FindOverlayTriangles(
    overlayVertex_t* overlayVerts,
    int& numVerts,
    int* overlayIndices,
    int& numIndices,
    const unsigned char* cullBits,
    const idVec2* texCoords,
    int vertexBase,
    rvIndexBuffer& rfSilTraceIB)
{
    std::vector<int> remap(m_silTraceGeoSpec.m_vertexCount, -1);

    const int numSilTraceIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    void* indexData = nullptr;
    rfSilTraceIB.Lock(m_silTraceGeoSpec.m_indexStart, numSilTraceIndices, 1u, &indexData);

    const bool is16Bit = Is16BitIndexBuffer(rfSilTraceIB);

    for (int tri = 0; tri < m_silTraceGeoSpec.m_primitiveCount; ++tri)
    {
        const int i0 = ReadIndex(indexData, is16Bit, tri * 3 + 0);
        const int i1 = ReadIndex(indexData, is16Bit, tri * 3 + 1);
        const int i2 = ReadIndex(indexData, is16Bit, tri * 3 + 2);

        if ((cullBits[i0] & cullBits[i1] & cullBits[i2]) != 0)
        {
            continue;
        }

        const int triIndices[3] = { i0, i1, i2 };
        for (int corner = 0; corner < 3; ++corner)
        {
            const int silVertIndex = triIndices[corner];

            if (remap[silVertIndex] == -1)
            {
                remap[silVertIndex] = numVerts;
                overlayVerts[numVerts].vertexNum = silVertIndex + vertexBase;
                overlayVerts[numVerts].st[0] = texCoords[silVertIndex].x;
                overlayVerts[numVerts].st[1] = texCoords[silVertIndex].y;
                ++numVerts;
            }

            overlayIndices[numIndices++] = remap[silVertIndex];
        }
    }

    rfSilTraceIB.Unlock();
}

void rvPrimBatch::GetTextureBounds(float bounds[2][2], rvVertexBuffer& drawVertexBuffer)
{
    float* texCoordSrc = nullptr;
    unsigned char* texCoordSrcBytes = nullptr;
    int texCoordStride = 0;

    drawVertexBuffer.LockTextureCoordinate(
        0,
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        texCoordSrcBytes,
        texCoordStride);
    texCoordSrc = reinterpret_cast<float*>(texCoordSrcBytes);

    for (int i = 0; i < m_drawGeoSpec.m_vertexCount; ++i)
    {
        const float* st = ByteOffset(texCoordSrc, texCoordStride, i);

        if (st[0] < bounds[0][0]) bounds[0][0] = st[0];
        if (st[1] < bounds[0][1]) bounds[0][1] = st[1];
        if (st[0] > bounds[1][0]) bounds[1][0] = st[0];
        if (st[1] > bounds[1][1]) bounds[1][1] = st[1];
    }

    drawVertexBuffer.Unlock();
}

void rvPrimBatch::CopyDrawVertices(idDrawVert* destDrawVerts, rvVertexBuffer& drawVertexBuffer)
{
    float* positionSrc = nullptr;
    unsigned char* positionSrcBytes = nullptr;
    float* normalSrc = nullptr;
    unsigned char* normalSrcBytes = nullptr;
    float* binormalSrc = nullptr;
    unsigned char* binormalSrcBytes = nullptr;
    float* tangentSrc = nullptr;
    unsigned char* tangentSrcBytes = nullptr;
    float* texCoordSrc = nullptr;
    unsigned char* texCoordSrcBytes = nullptr;
    unsigned char* diffuseColorSrc = nullptr;

    int positionStride = 0;
    int normalStride = 0;
    int binormalStride = 0;
    int tangentStride = 0;
    int texCoordStride = 0;
    int diffuseColorStride = 0;

    drawVertexBuffer.LockPosition(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        positionSrcBytes,
        positionStride);
    positionSrc = reinterpret_cast<float*>(positionSrcBytes);

    drawVertexBuffer.LockNormal(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        normalSrcBytes,
        normalStride);
    normalSrc = reinterpret_cast<float*>(normalSrcBytes);

    drawVertexBuffer.LockBinormal(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        binormalSrcBytes,
        binormalStride);
    binormalSrc = reinterpret_cast<float*>(binormalSrcBytes);

    drawVertexBuffer.LockTangent(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        tangentSrcBytes,
        tangentStride);
    tangentSrc = reinterpret_cast<float*>(tangentSrcBytes);

    drawVertexBuffer.LockTextureCoordinate(
        0,
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        texCoordSrcBytes,
        texCoordStride);
    texCoordSrc = reinterpret_cast<float*>(texCoordSrcBytes);

    drawVertexBuffer.LockDiffuseColor(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        diffuseColorSrc,
        diffuseColorStride);

    for (int i = 0; i < m_drawGeoSpec.m_vertexCount; ++i)
    {
        idDrawVert& dst = destDrawVerts[i];

        const float* position = ByteOffset(positionSrc, positionStride, i);
        const float* normal = ByteOffset(normalSrc, normalStride, i);
        const float* tangent = ByteOffset(tangentSrc, tangentStride, i);
        const float* binormal = ByteOffset(binormalSrc, binormalStride, i);
        const float* st = ByteOffset(texCoordSrc, texCoordStride, i);
        const unsigned char* color = ByteOffset(diffuseColorSrc, diffuseColorStride, i);

        dst.xyz.x = position[0];
        dst.xyz.y = position[1];
        dst.xyz.z = position[2];

        dst.normal.x = normal[0];
        dst.normal.y = normal[1];
        dst.normal.z = normal[2];

        dst.tangents[0].x = tangent[0];
        dst.tangents[0].y = tangent[1];
        dst.tangents[0].z = tangent[2];

        dst.tangents[1].x = binormal[0];
        dst.tangents[1].y = binormal[1];
        dst.tangents[1].z = binormal[2];

        dst.st.x = st[0];
        dst.st.y = st[1];

        dst.color[0] = color[0];
        dst.color[1] = color[1];
        dst.color[2] = color[2];
        dst.color[3] = color[3];
    }

    drawVertexBuffer.Unlock();
}

void rvPrimBatch::CopyDrawIndices(int* destIndices, rvIndexBuffer& drawIndexBuffer, unsigned int destBase)
{
    const unsigned int baseDelta = destBase - m_drawGeoSpec.m_vertexStart;
    const int numIndices = 3 * m_drawGeoSpec.m_primitiveCount;

    void* indexData = nullptr;
    drawIndexBuffer.Lock(m_drawGeoSpec.m_indexStart, numIndices, 1u, &indexData);

    const bool is16Bit = Is16BitIndexBuffer(drawIndexBuffer);
    for (int i = 0; i < numIndices; ++i)
    {
        destIndices[i] = baseDelta + ReadIndex(indexData, is16Bit, i);
    }

    drawIndexBuffer.Unlock();
}


void rvPrimBatch::CopySilTraceVertices(
    rvVertexBuffer& silTraceVertexBuffer,
    rvIndexBuffer& silTraceIndexBuffer,
    rvVertexBuffer& drawVertexBuffer,
    rvIndexBuffer& drawIndexBuffer)
{
    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    const int drawVertexBase = m_drawGeoSpec.m_vertexStart;

    std::vector<unsigned int> remap(m_silTraceGeoSpec.m_vertexCount, 0u);

    void* silIndexData = nullptr;
    void* drawIndexData = nullptr;

    silTraceIndexBuffer.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &silIndexData);
    drawIndexBuffer.Lock(m_drawGeoSpec.m_indexStart, numIndices, 1u, &drawIndexData);

    const bool silTraceIs16Bit = Is16BitIndexBuffer(silTraceIndexBuffer);
    const bool drawIs16Bit = Is16BitIndexBuffer(drawIndexBuffer);

    for (int i = 0; i < numIndices; i += 3)
    {
        const int sil0 = ReadIndex(silIndexData, silTraceIs16Bit, i + 0);
        const int sil1 = ReadIndex(silIndexData, silTraceIs16Bit, i + 1);
        const int sil2 = ReadIndex(silIndexData, silTraceIs16Bit, i + 2);

        const int draw0 = ReadIndex(drawIndexData, drawIs16Bit, i + 0) - drawVertexBase;
        const int draw1 = ReadIndex(drawIndexData, drawIs16Bit, i + 1) - drawVertexBase;
        const int draw2 = ReadIndex(drawIndexData, drawIs16Bit, i + 2) - drawVertexBase;

        remap[sil0] = draw0;
        remap[sil1] = draw1;
        remap[sil2] = draw2;
    }

    drawIndexBuffer.Unlock();
    silTraceIndexBuffer.Unlock();

    unsigned char* silTraceVertex = nullptr;
    unsigned char* drawVertex = nullptr;
    int silTraceVertexStride = 0;
    int drawVertexStride = 0;

    float srcTail[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    silTraceVertexBuffer.LockPosition(
        m_silTraceGeoSpec.m_vertexStart,
        m_silTraceGeoSpec.m_vertexCount,
        2u,
        silTraceVertex,
        silTraceVertexStride);

    drawVertexBuffer.LockPosition(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        drawVertex,
        drawVertexStride);

    rvVertexBuffer::ComponentCopy(
        &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[0]],
        silTraceVertexStride,
        static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[0]),
        silTraceVertexBuffer.m_format.m_dimensions & 7,
        &drawVertex[drawVertexBuffer.m_format.m_byteOffset[0]],
        drawVertexStride,
        static_cast<Rv_Vertex_Data_Type_t>(drawVertexBuffer.m_format.m_dataTypes[0]),
        drawVertexBuffer.m_format.m_dimensions & 7,
        m_silTraceGeoSpec.m_vertexCount,
        remap.data(),
        srcTail,
        0);

    if ((drawVertexBuffer.m_format.m_flags & 4) != 0)
    {
        rvVertexBuffer::ComponentCopy(
            &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[2]],
            silTraceVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[2]),
            1,
            &drawVertex[drawVertexBuffer.m_format.m_byteOffset[2]],
            drawVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(drawVertexBuffer.m_format.m_dataTypes[2]),
            1,
            m_silTraceGeoSpec.m_vertexCount,
            remap.data(),
            nullptr,
            0);
    }

    if ((drawVertexBuffer.m_format.m_flags & 8) != 0)
    {
        rvVertexBuffer::ComponentCopy(
            &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[3]],
            silTraceVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[3]),
            (silTraceVertexBuffer.m_format.m_dimensions >> 3) & 7,
            &drawVertex[drawVertexBuffer.m_format.m_byteOffset[3]],
            drawVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(drawVertexBuffer.m_format.m_dataTypes[3]),
            (drawVertexBuffer.m_format.m_dimensions >> 3) & 7,
            m_silTraceGeoSpec.m_vertexCount,
            remap.data(),
            nullptr,
            1);
    }

    drawVertexBuffer.Unlock();
    silTraceVertexBuffer.Unlock();
}

void rvPrimBatch::CopyShadowVertices(rvVertexBuffer& shadowVertexBuffer, rvVertexBuffer& silTraceVertexBuffer)
{
    unsigned char* shadowVertex = nullptr;
    unsigned char* silTraceVertex = nullptr;
    int shadowVertexStride = 0;
    int silTraceVertexStride = 0;

    float srcTail0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float srcTail1[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    shadowVertexBuffer.LockPosition(
        m_shadowVolGeoSpec.m_vertexStart,
        m_shadowVolGeoSpec.m_vertexCount,
        2u,
        shadowVertex,
        shadowVertexStride);

    silTraceVertexBuffer.LockPosition(
        m_silTraceGeoSpec.m_vertexStart,
        m_silTraceGeoSpec.m_vertexCount,
        1u,
        silTraceVertex,
        silTraceVertexStride);

    rvVertexBuffer::ComponentCopy(
        &shadowVertex[shadowVertexBuffer.m_format.m_byteOffset[0]],
        2 * shadowVertexStride,
        static_cast<Rv_Vertex_Data_Type_t>(shadowVertexBuffer.m_format.m_dataTypes[0]),
        shadowVertexBuffer.m_format.m_dimensions & 7,
        &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[0]],
        silTraceVertexStride,
        static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[0]),
        3,
        m_silTraceGeoSpec.m_vertexCount,
        nullptr,
        srcTail1,
        0);

    rvVertexBuffer::ComponentCopy(
        &shadowVertex[shadowVertexBuffer.m_format.m_byteOffset[0] + shadowVertexStride],
        2 * shadowVertexStride,
        static_cast<Rv_Vertex_Data_Type_t>(shadowVertexBuffer.m_format.m_dataTypes[0]),
        shadowVertexBuffer.m_format.m_dimensions & 7,
        &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[0]],
        silTraceVertexStride,
        static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[0]),
        3,
        m_silTraceGeoSpec.m_vertexCount,
        nullptr,
        srcTail0,
        0);

    if ((silTraceVertexBuffer.m_format.m_flags & 4) != 0)
    {
        rvVertexBuffer::ComponentCopy(
            &shadowVertex[shadowVertexBuffer.m_format.m_byteOffset[2]],
            2 * shadowVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(shadowVertexBuffer.m_format.m_dataTypes[2]),
            1,
            &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[2]],
            silTraceVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[2]),
            1,
            m_silTraceGeoSpec.m_vertexCount,
            nullptr,
            nullptr,
            0);

        rvVertexBuffer::ComponentCopy(
            &shadowVertex[shadowVertexBuffer.m_format.m_byteOffset[2] + shadowVertexStride],
            2 * shadowVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(shadowVertexBuffer.m_format.m_dataTypes[2]),
            1,
            &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[2]],
            silTraceVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[2]),
            1,
            m_silTraceGeoSpec.m_vertexCount,
            nullptr,
            nullptr,
            0);
    }

    if ((silTraceVertexBuffer.m_format.m_flags & 8) != 0)
    {
        rvVertexBuffer::ComponentCopy(
            &shadowVertex[shadowVertexBuffer.m_format.m_byteOffset[3]],
            2 * shadowVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(shadowVertexBuffer.m_format.m_dataTypes[3]),
            (shadowVertexBuffer.m_format.m_dimensions >> 3) & 7,
            &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[3]],
            silTraceVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[3]),
            (silTraceVertexBuffer.m_format.m_dimensions >> 3) & 7,
            m_silTraceGeoSpec.m_vertexCount,
            nullptr,
            nullptr,
            1);

        rvVertexBuffer::ComponentCopy(
            &shadowVertex[shadowVertexBuffer.m_format.m_byteOffset[3] + shadowVertexStride],
            2 * shadowVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(shadowVertexBuffer.m_format.m_dataTypes[3]),
            (shadowVertexBuffer.m_format.m_dimensions >> 3) & 7,
            &silTraceVertex[silTraceVertexBuffer.m_format.m_byteOffset[3]],
            silTraceVertexStride,
            static_cast<Rv_Vertex_Data_Type_t>(silTraceVertexBuffer.m_format.m_dataTypes[3]),
            (silTraceVertexBuffer.m_format.m_dimensions >> 3) & 7,
            m_silTraceGeoSpec.m_vertexCount,
            nullptr,
            nullptr,
            1);
    }

    silTraceVertexBuffer.Unlock();
    shadowVertexBuffer.Unlock();
}

void rvPrimBatch::Write(idFile& outFile, const char* prepend)
{
    const std::string indent = prepend != nullptr ? prepend : "";
    const std::string innerIndent = indent + "\t";
    const std::string transformIndent = innerIndent + "\t";

    outFile.WriteFloatString("%sPrimBatch\n", indent.c_str());
    outFile.WriteFloatString("%s{\n", indent.c_str());

    if (m_transformPalette != nullptr)
    {
        outFile.WriteFloatString("%sTransform[ %d ]\n", innerIndent.c_str(), m_numTransforms);
        outFile.WriteFloatString("%s{\n", innerIndent.c_str());

        for (int i = 0; i < m_numTransforms; ++i)
        {
            outFile.WriteFloatString("%s%d\n", transformIndent.c_str(), m_transformPalette[i]);
        }

        outFile.WriteFloatString("%s}\n", innerIndent.c_str());
    }

    if (m_silTraceGeoSpec.m_primitiveCount > 0)
    {
        outFile.WriteFloatString(
            "%sSilTraceIndexedTriList %d %d %d %d\n",
            innerIndent.c_str(),
            m_silTraceGeoSpec.m_vertexStart,
            m_silTraceGeoSpec.m_vertexCount,
            m_silTraceGeoSpec.m_indexStart,
            m_silTraceGeoSpec.m_primitiveCount);
    }

    if (m_drawGeoSpec.m_primitiveCount > 0)
    {
        outFile.WriteFloatString(
            "%sDrawIndexedTriList %d %d %d %d\n",
            innerIndent.c_str(),
            m_drawGeoSpec.m_vertexStart,
            m_drawGeoSpec.m_vertexCount,
            m_drawGeoSpec.m_indexStart,
            m_drawGeoSpec.m_primitiveCount);
    }

    if (m_shadowVolGeoSpec.m_primitiveCount > 0)
    {
        outFile.WriteFloatString(
            "%sShadowIndexedTriList %d %d %d %d %d %d\n",
            innerIndent.c_str(),
            m_shadowVolGeoSpec.m_vertexStart,
            m_shadowVolGeoSpec.m_vertexCount,
            m_shadowVolGeoSpec.m_indexStart,
            m_shadowVolGeoSpec.m_primitiveCount,
            m_numShadowPrimitivesNoCaps,
            m_shadowCapPlaneBits);
    }
    else if (m_shadowVolGeoSpec.m_vertexCount > 0)
    {
        outFile.WriteFloatString("%sShadowVerts %d\n", innerIndent.c_str(), m_shadowVolGeoSpec.m_vertexStart);
    }

    if (m_silEdgeCount > 0)
    {
        outFile.WriteFloatString(
            "%sSilhouetteEdge %d %d\n",
            innerIndent.c_str(),
            m_silEdgeStart,
            m_silEdgeCount);
    }

    outFile.WriteFloatString("%s}\n", indent.c_str());
}

void rvPrimBatch::Shutdown()
{
    if (m_transformPalette != nullptr)
    {
        Mem_Free(m_transformPalette);
        m_transformPalette = nullptr;
    }

    m_numTransforms = 0;
    m_silEdgeStart = 0;
    m_silEdgeCount = 0;
    m_numShadowPrimitivesNoCaps = 0;
    m_shadowCapPlaneBits = 0;

    m_silTraceGeoSpec.Reset();
    m_drawGeoSpec.Reset();
    m_shadowVolGeoSpec.Reset();
}


void rvPrimBatch::Draw(rvVertexBuffer& vertexBuffer, rvIndexBuffer& indexBuffer, const rvVertexFormat* vertexComponentsNeeded)
{
    (void)vertexBuffer;
    (void)vertexComponentsNeeded;

    const int numPrimitives = m_drawGeoSpec.m_primitiveCount;
    const int numIndices = 3 * numPrimitives;

    ++backEnd.pc.c_drawElements;
    backEnd.pc.c_drawIndexes += numIndices;
    backEnd.pc.c_drawVertexes += m_drawGeoSpec.m_vertexCount;

    intptr_t indexOffsetBytes = static_cast<intptr_t>(m_drawGeoSpec.m_indexStart) * GetIndexSizeBytes(indexBuffer);
    int drawCount = r_singleTriangle.GetInteger() ? 3 : numIndices;

    qglDrawElements(
        GL_TRIANGLES,
        drawCount,
        GetGlIndexType(indexBuffer),
        reinterpret_cast<const void*>(indexOffsetBytes));

    backEnd.pc.c_vboIndexes += numIndices;
}

void rvPrimBatch::Draw(rvVertexBuffer& vertexBuffer, int* indices, int numIndices, const rvVertexFormat* vertexComponentsNeeded)
{
    (void)vertexBuffer;
    (void)vertexComponentsNeeded;

    if (numIndices <= 0)
    {
        return;
    }

    ++backEnd.pc.c_drawElements;
    backEnd.pc.c_drawIndexes += numIndices;
    backEnd.pc.c_drawVertexes += m_drawGeoSpec.m_vertexCount;

    qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    const int drawCount = r_singleTriangle.GetInteger() ? 3 : numIndices;
    qglDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, indices);

    backEnd.pc.c_vboIndexes += numIndices;
}

void rvPrimBatch::DrawShadowVolume(
    rvVertexBuffer& vertexBuffer,
    int* indices,
    int numIndices,
    const rvVertexFormat* vertexComponentsNeeded)
{
    (void)vertexBuffer;
    (void)vertexComponentsNeeded;

    if (numIndices <= 0)
    {
        return;
    }

    ++backEnd.pc.c_shadowElements;
    backEnd.pc.c_shadowIndexes += numIndices;
    backEnd.pc.c_shadowVertexes += m_shadowVolGeoSpec.m_vertexCount;

    qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    const int drawCount = r_singleTriangle.GetInteger() ? 3 : numIndices;
    qglDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, indices);
}

void rvPrimBatch::DrawShadowVolume(
    rvVertexBuffer& vertexBuffer,
    rvIndexBuffer& indexBuffer,
    bool drawCaps,
    const rvVertexFormat* vertexComponentsNeeded)
{
    (void)vertexBuffer;
    (void)vertexComponentsNeeded;

    const int numPrimitives = drawCaps ? m_shadowVolGeoSpec.m_primitiveCount : m_numShadowPrimitivesNoCaps;
    const int numIndices = 3 * numPrimitives;
    if (numIndices <= 0)
    {
        return;
    }

    ++backEnd.pc.c_shadowElements;
    backEnd.pc.c_shadowIndexes += numIndices;
    backEnd.pc.c_shadowVertexes += m_shadowVolGeoSpec.m_vertexCount;

    intptr_t indexOffsetBytes = static_cast<intptr_t>(m_shadowVolGeoSpec.m_indexStart) * GetIndexSizeBytes(indexBuffer);
    int drawCount = r_singleTriangle.GetInteger() ? 3 : numIndices;

    qglDrawElements(
        GL_TRIANGLES,
        drawCount,
        GetGlIndexType(indexBuffer),
        reinterpret_cast<const void*>(indexOffsetBytes));
}

void rvPrimBatch::TransformVertsMinMax(
    rvSilTraceVertT* destSilTraceVerts,
    idVec3& bndMin,
    idVec3& bndMax,
    rvVertexBuffer& silTraceVB,
    idJointMat* skinSpaceToLocalMats,
    idJointMat* localToModelMats,
    float* skinToModelTransforms)
{
    (void)skinSpaceToLocalMats;
    (void)localToModelMats;
    (void)skinToModelTransforms;

    unsigned char* positionData = nullptr;
    int positionStride = 0;

    silTraceVB.LockPosition(
        m_silTraceGeoSpec.m_vertexStart,
        m_silTraceGeoSpec.m_vertexCount,
        1u,
        positionData,
        positionStride);

    for (int i = 0; i < m_silTraceGeoSpec.m_vertexCount; ++i)
    {
        const float* position = ByteOffset(reinterpret_cast<const float*>(positionData), positionStride, i);
        rvSilTraceVertT& dst = destSilTraceVerts[i];
        dst.xyzw.x = position[0];
        dst.xyzw.y = position[1];
        dst.xyzw.z = position[2];
        dst.xyzw.w = 1.0f;

        if (i == 0)
        {
            bndMin.x = bndMax.x = position[0];
            bndMin.y = bndMax.y = position[1];
            bndMin.z = bndMax.z = position[2];
        }
        else
        {
            if (position[0] < bndMin.x) bndMin.x = position[0];
            if (position[1] < bndMin.y) bndMin.y = position[1];
            if (position[2] < bndMin.z) bndMin.z = position[2];
            if (position[0] > bndMax.x) bndMax.x = position[0];
            if (position[1] > bndMax.y) bndMax.y = position[1];
            if (position[2] > bndMax.z) bndMax.z = position[2];
        }
    }

    silTraceVB.Unlock();
}

void rvPrimBatch::DeriveTriPlanes(idPlane* planes, const rvSilTraceVertT* silTraceVerts, rvIndexBuffer& silTraceIB)
{
    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;

    void* indexData = nullptr;
    silTraceIB.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &indexData);

    const bool is16Bit = Is16BitIndexBuffer(silTraceIB);
    for (int tri = 0; tri < m_silTraceGeoSpec.m_primitiveCount; ++tri)
    {
        const int i0 = ReadIndex(indexData, is16Bit, tri * 3 + 0);
        const int i1 = ReadIndex(indexData, is16Bit, tri * 3 + 1);
        const int i2 = ReadIndex(indexData, is16Bit, tri * 3 + 2);

        BuildPlaneFromTriangle(
            planes[tri],
            MakeFloat3(silTraceVerts[i0]),
            MakeFloat3(silTraceVerts[i1]),
            MakeFloat3(silTraceVerts[i2]));
    }

    silTraceIB.Unlock();
}

void rvPrimBatch::LocalTrace(
    localTrace_t& hit,
    int& c_testPlanes,
    int& c_testEdges,
    int& c_intersect,
    const idVec3& start,
    const idVec3& end,
    const unsigned char* cullBits,
    const idPlane* facePlanes,
    const rvSilTraceVertT* silTraceVerts,
    float radius,
    rvIndexBuffer& silTraceIB,
    rvIndexBuffer& drawIB)
{
    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    const int drawVertexBase = m_drawGeoSpec.m_vertexStart;
    const float radiusSqr = radius * radius;

    void* silIndexData = nullptr;
    void* drawIndexData = nullptr;

    silTraceIB.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &silIndexData);
    drawIB.Lock(m_drawGeoSpec.m_indexStart, numIndices, 1u, &drawIndexData);

    const bool silIs16Bit = Is16BitIndexBuffer(silTraceIB);
    const bool drawIs16Bit = Is16BitIndexBuffer(drawIB);

    const Float3 startPt = MakeFloat3(start);
    const Float3 endPt = MakeFloat3(end);

    for (int tri = 0; tri < m_silTraceGeoSpec.m_primitiveCount; ++tri)
    {
        const int i0 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 0);
        const int i1 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 1);
        const int i2 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 2);

        const unsigned char mask = cullBits[i0] | cullBits[i1] | cullBits[i2];
        if (TriangleCullMaskReject(mask))
        {
            continue;
        }

        ++c_testPlanes;

        const idPlane& plane = facePlanes[tri];
        float fraction = hit.fraction;
        Float3 hitPoint;
        if (!SegmentPlaneIntersection(plane, startPt, endPt, hit.fraction, fraction, hitPoint))
        {
            continue;
        }

        ++c_testEdges;

        const Float3 a = MakeFloat3(silTraceVerts[i0]);
        const Float3 b = MakeFloat3(silTraceVerts[i1]);
        const Float3 c = MakeFloat3(silTraceVerts[i2]);

        bool intersects = PointInTriangle(hitPoint, a, b, c, plane);
        if (!intersects && radiusSqr > 0.0f)
        {
            intersects = DistanceSquaredPointTriangle(hitPoint, a, b, c) <= radiusSqr;
        }

        if (!intersects)
        {
            continue;
        }

        ++c_intersect;

        hit.fraction = fraction;
        hit.normal.x = plane.a;
        hit.normal.y = plane.b;
        hit.normal.z = plane.c;
        StoreFloat3(hit.point, hitPoint);

        hit.indexes[0] = m_silTraceGeoSpec.m_indexStart + tri * 3 + 0;
        hit.indexes[1] = m_silTraceGeoSpec.m_indexStart + tri * 3 + 1;
        hit.indexes[2] = m_silTraceGeoSpec.m_indexStart + tri * 3 + 2;

        StoreFloat3(hit.vertices[0], a);
        StoreFloat3(hit.vertices[1], b);
        StoreFloat3(hit.vertices[2], c);

        const int d0 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 0);
        const int d1 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 1);
        const int d2 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 2);

        hit.drawIndices[0] = d0 - drawVertexBase;
        hit.drawIndices[1] = d1 - drawVertexBase;
        hit.drawIndices[2] = d2 - drawVertexBase;
    }

    drawIB.Unlock();
    silTraceIB.Unlock();
}

const rvDeclMatType* rvPrimBatch::GetMaterialType(const idMaterial* shader, const localTrace_t& hit, rvVertexBuffer& drawVB)
{
    if (shader == nullptr)
    {
        return nullptr;
    }

    if (GetBaseMaterialType(shader) == nullptr)
    {
        return GetBaseMaterialType(shader);
    }

    if ((drawVB.m_flags & 1) == 0)
    {
        return GetBaseMaterialType(shader);
    }

    if ((drawVB.m_format.m_flags & 0x400) == 0)
    {
        return GetBaseMaterialType(shader);
    }

    float* texCoordBase = nullptr;
    unsigned char* texCoordBaseBytes = nullptr;
    int texCoordStride = 0;

    drawVB.LockTextureCoordinate(
        0,
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        texCoordBaseBytes,
        texCoordStride);
    texCoordBase = reinterpret_cast<float*>(texCoordBaseBytes);

    idVec3 triangle[3];
    triangle[0] = hit.vertices[0];
    triangle[1] = hit.vertices[1];
    triangle[2] = hit.vertices[2];

    idVec2 tc[3];
    for (int i = 0; i < 3; ++i)
    {
        const float* st = ByteOffset(texCoordBase, texCoordStride, hit.drawIndices[i]);
        tc[i].x = st[0];
        tc[i].y = st[1];
    }

    idVec2 result;
    const float area = idMath::BarycentricTriangleArea(hit.point, triangle[0], triangle[1], triangle[2]);
    if (area == 0.0f)
    {
        drawVB.Unlock();
        return GetBaseMaterialType(shader);
    }

    idMath::BarycentricEvaluate(result, hit.point, hit.normal, area, triangle, tc);
    drawVB.Unlock();

    return GetMappedMaterialType(shader, result);
}


void rvPrimBatch::CreateLightTris(
    int* destDrawIndices,
    int& destIndexCount,
    idBounds& bounds,
    int& c_backfaced,
    int& c_distance,
    const unsigned char* facing,
    const unsigned char* cullBits,
    const idPlane* localClipPlanes,
    const rvSilTraceVertT* silTraceVerts,
    bool includeBackFaces,
    rvIndexBuffer& silTraceIB,
    rvIndexBuffer& drawIB)
{
    (void)localClipPlanes;

    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    std::vector<int> destSilTraceIndices;
    destSilTraceIndices.reserve(numIndices);

    void* silIndexData = nullptr;
    void* drawIndexData = nullptr;

    silTraceIB.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &silIndexData);
    drawIB.Lock(m_drawGeoSpec.m_indexStart, numIndices, 1u, &drawIndexData);

    const bool silIs16Bit = Is16BitIndexBuffer(silTraceIB);
    const bool drawIs16Bit = Is16BitIndexBuffer(drawIB);

    destIndexCount = 0;

    for (int tri = 0; tri < m_silTraceGeoSpec.m_primitiveCount; ++tri)
    {
        if (!includeBackFaces && facing[tri] == 0)
        {
            ++c_backfaced;
            continue;
        }

        const int s0 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 0);
        const int s1 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 1);
        const int s2 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 2);

        if ((cullBits[s0] & cullBits[s1] & cullBits[s2]) != 0)
        {
            ++c_distance;
            continue;
        }

        const int d0 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 0);
        const int d1 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 1);
        const int d2 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 2);

        destDrawIndices[destIndexCount + 0] = d0;
        destDrawIndices[destIndexCount + 1] = d1;
        destDrawIndices[destIndexCount + 2] = d2;
        destIndexCount += 3;

        destSilTraceIndices.push_back(s0);
        destSilTraceIndices.push_back(s1);
        destSilTraceIndices.push_back(s2);
    }

    drawIB.Unlock();
    silTraceIB.Unlock();

    ComputeBoundsFromIndexedSilVerts(bounds, silTraceVerts, destSilTraceIndices.data(), destIndexCount);
}

void rvPrimBatch::CreateFrontFaceTris(
    int* destDrawIndices,
    int& destIndexCount,
    idBounds& bounds,
    int& c_backfaced,
    const unsigned char* facing,
    const rvSilTraceVertT* silTraceVerts,
    rvIndexBuffer& silTraceIB,
    rvIndexBuffer& drawIB)
{
    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    std::vector<int> destSilTraceIndices;
    destSilTraceIndices.reserve(numIndices);

    void* silIndexData = nullptr;
    void* drawIndexData = nullptr;

    silTraceIB.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &silIndexData);
    drawIB.Lock(m_drawGeoSpec.m_indexStart, numIndices, 1u, &drawIndexData);

    const bool silIs16Bit = Is16BitIndexBuffer(silTraceIB);
    const bool drawIs16Bit = Is16BitIndexBuffer(drawIB);

    destIndexCount = 0;

    for (int tri = 0; tri < m_silTraceGeoSpec.m_primitiveCount; ++tri)
    {
        if (facing[tri] == 0)
        {
            ++c_backfaced;
            continue;
        }

        const int s0 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 0);
        const int s1 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 1);
        const int s2 = ReadIndex(silIndexData, silIs16Bit, tri * 3 + 2);

        const int d0 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 0);
        const int d1 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 1);
        const int d2 = ReadIndex(drawIndexData, drawIs16Bit, tri * 3 + 2);

        destDrawIndices[destIndexCount + 0] = d0;
        destDrawIndices[destIndexCount + 1] = d1;
        destDrawIndices[destIndexCount + 2] = d2;
        destIndexCount += 3;

        destSilTraceIndices.push_back(s0);
        destSilTraceIndices.push_back(s1);
        destSilTraceIndices.push_back(s2);
    }

    drawIB.Unlock();
    silTraceIB.Unlock();

    ComputeBoundsFromIndexedSilVerts(bounds, silTraceVerts, destSilTraceIndices.data(), destIndexCount);
}

void rvPrimBatch::GetTriangle(
    idDrawVert& a,
    idDrawVert& b,
    idDrawVert& c,
    int triOffset,
    rvVertexBuffer& drawVertexBuffer,
    rvIndexBuffer& drawIndexBuffer,
    const rvSilTraceVertT* silTraceVerts,
    rvIndexBuffer& silTraceIndexBuffer)
{
    void* silIndexData = nullptr;
    void* drawIndexData = nullptr;
    float* texCoordSrc = nullptr;
    unsigned char* texCoordSrcBytes = nullptr;
    int texCoordStride = 0;

    const int firstIndex = triOffset * 3;

    silTraceIndexBuffer.Lock(m_silTraceGeoSpec.m_indexStart + firstIndex, 3, 1u, &silIndexData);
    drawIndexBuffer.Lock(m_drawGeoSpec.m_indexStart + firstIndex, 3, 1u, &drawIndexData);

    drawVertexBuffer.LockTextureCoordinate(
        0,
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        texCoordSrcBytes,
        texCoordStride);
    texCoordSrc = reinterpret_cast<float*>(texCoordSrcBytes);

    const bool silIs16Bit = Is16BitIndexBuffer(silTraceIndexBuffer);
    const bool drawIs16Bit = Is16BitIndexBuffer(drawIndexBuffer);

    const int s0 = ReadIndex(silIndexData, silIs16Bit, 0);
    const int s1 = ReadIndex(silIndexData, silIs16Bit, 1);
    const int s2 = ReadIndex(silIndexData, silIs16Bit, 2);

    const int d0 = ReadIndex(drawIndexData, drawIs16Bit, 0) - m_drawGeoSpec.m_vertexStart;
    const int d1 = ReadIndex(drawIndexData, drawIs16Bit, 1) - m_drawGeoSpec.m_vertexStart;
    const int d2 = ReadIndex(drawIndexData, drawIs16Bit, 2) - m_drawGeoSpec.m_vertexStart;

    a.xyz.x = silTraceVerts[s0].xyzw.x;
    a.xyz.y = silTraceVerts[s0].xyzw.y;
    a.xyz.z = silTraceVerts[s0].xyzw.z;
    SetDrawVertTexCoord(a, ByteOffset(texCoordSrc, texCoordStride, d0));

    b.xyz.x = silTraceVerts[s1].xyzw.x;
    b.xyz.y = silTraceVerts[s1].xyzw.y;
    b.xyz.z = silTraceVerts[s1].xyzw.z;
    SetDrawVertTexCoord(b, ByteOffset(texCoordSrc, texCoordStride, d1));

    c.xyz.x = silTraceVerts[s2].xyzw.x;
    c.xyz.y = silTraceVerts[s2].xyzw.y;
    c.xyz.z = silTraceVerts[s2].xyzw.z;
    SetDrawVertTexCoord(c, ByteOffset(texCoordSrc, texCoordStride, d2));

    drawVertexBuffer.Unlock();
    drawIndexBuffer.Unlock();
    silTraceIndexBuffer.Unlock();
}

void rvPrimBatch::CopyTriangles(
    idDrawVert* destDrawVerts,
    int* destIndices,
    rvVertexBuffer& drawVertexBuffer,
    rvIndexBuffer& drawIndexBuffer,
    const rvSilTraceVertT* silTraceVerts,
    rvIndexBuffer& silTraceIndexBuffer,
    int destBase)
{
    const int numIndices = 3 * m_drawGeoSpec.m_primitiveCount;
    const int drawVertexBase = m_drawGeoSpec.m_vertexStart;

    void* silIndexData = nullptr;
    void* drawIndexData = nullptr;

    silTraceIndexBuffer.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &silIndexData);
    drawIndexBuffer.Lock(m_drawGeoSpec.m_indexStart, numIndices, 1u, &drawIndexData);

    float* texCoordSrc = nullptr;
    unsigned char* texCoordSrcBytes = nullptr;
    unsigned char* diffuseColorSrc = nullptr;
    int texCoordStride = 0;
    int diffuseColorStride = 0;

    drawVertexBuffer.LockTextureCoordinate(
        0,
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        texCoordSrcBytes,
        texCoordStride);
    texCoordSrc = reinterpret_cast<float*>(texCoordSrcBytes);

    drawVertexBuffer.LockDiffuseColor(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        diffuseColorSrc,
        diffuseColorStride);

    const bool silIs16Bit = Is16BitIndexBuffer(silTraceIndexBuffer);
    const bool drawIs16Bit = Is16BitIndexBuffer(drawIndexBuffer);

    for (int i = 0; i < numIndices; i += 3)
    {
        const int s0 = ReadIndex(silIndexData, silIs16Bit, i + 0);
        const int s1 = ReadIndex(silIndexData, silIs16Bit, i + 1);
        const int s2 = ReadIndex(silIndexData, silIs16Bit, i + 2);

        const int d0 = ReadIndex(drawIndexData, drawIs16Bit, i + 0) - drawVertexBase;
        const int d1 = ReadIndex(drawIndexData, drawIs16Bit, i + 1) - drawVertexBase;
        const int d2 = ReadIndex(drawIndexData, drawIs16Bit, i + 2) - drawVertexBase;

        destIndices[i + 0] = destBase + d0;
        destIndices[i + 1] = destBase + d1;
        destIndices[i + 2] = destBase + d2;

        FillTriangleCopyVertex(destDrawVerts[d0], silTraceVerts[s0], texCoordSrc, texCoordStride, diffuseColorSrc, diffuseColorStride, d0);
        FillTriangleCopyVertex(destDrawVerts[d1], silTraceVerts[s1], texCoordSrc, texCoordStride, diffuseColorSrc, diffuseColorStride, d1);
        FillTriangleCopyVertex(destDrawVerts[d2], silTraceVerts[s2], texCoordSrc, texCoordStride, diffuseColorSrc, diffuseColorStride, d2);
    }

    drawVertexBuffer.Unlock();
    drawIndexBuffer.Unlock();
    silTraceIndexBuffer.Unlock();
}


void rvPrimBatch::TransformDrawVertices(
    idDrawVert* destDrawVerts,
    rvVertexBuffer& drawVertexBuffer,
    const idMat4& transform,
    int colorShift,
    unsigned char* colorAdd)
{
    float* positionSrc = nullptr;
    unsigned char* positionSrcBytes = nullptr;
    float* normalSrc = nullptr;
    unsigned char* normalSrcBytes = nullptr;
    float* binormalSrc = nullptr;
    unsigned char* binormalSrcBytes = nullptr;
    float* tangentSrc = nullptr;
    unsigned char* tangentSrcBytes = nullptr;
    float* texCoordSrc = nullptr;
    unsigned char* texCoordSrcBytes = nullptr;
    unsigned char* diffuseColorSrc = nullptr;

    int positionStride = 0;
    int normalStride = 0;
    int binormalStride = 0;
    int tangentStride = 0;
    int texCoordStride = 0;
    int diffuseColorStride = 0;

    drawVertexBuffer.LockPosition(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        positionSrcBytes,
        positionStride);
    positionSrc = reinterpret_cast<float*>(positionSrcBytes);

    drawVertexBuffer.LockNormal(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        normalSrcBytes,
        normalStride);
    normalSrc = reinterpret_cast<float*>(normalSrcBytes);

    drawVertexBuffer.LockBinormal(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        binormalSrcBytes,
        binormalStride);
    binormalSrc = reinterpret_cast<float*>(binormalSrcBytes);

    drawVertexBuffer.LockTangent(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        tangentSrcBytes,
        tangentStride);
    tangentSrc = reinterpret_cast<float*>(tangentSrcBytes);

    drawVertexBuffer.LockTextureCoordinate(
        0,
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        texCoordSrcBytes,
        texCoordStride);
    texCoordSrc = reinterpret_cast<float*>(texCoordSrcBytes);

    drawVertexBuffer.LockDiffuseColor(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        diffuseColorSrc,
        diffuseColorStride);

    for (int i = 0; i < m_drawGeoSpec.m_vertexCount; ++i)
    {
        idDrawVert& dst = destDrawVerts[i];

        const float* position = ByteOffset(positionSrc, positionStride, i);
        const float* normal = ByteOffset(normalSrc, normalStride, i);
        const float* tangent = ByteOffset(tangentSrc, tangentStride, i);
        const float* binormal = ByteOffset(binormalSrc, binormalStride, i);
        const float* st = ByteOffset(texCoordSrc, texCoordStride, i);
        const unsigned char* color = ByteOffset(diffuseColorSrc, diffuseColorStride, i);

        const Float3 transformed = TransformPoint(transform, { position[0], position[1], position[2] });
        SetDrawVertPosition(dst, transformed);

        dst.normal.x = normal[0];
        dst.normal.y = normal[1];
        dst.normal.z = normal[2];

        dst.tangents[0].x = tangent[0];
        dst.tangents[0].y = tangent[1];
        dst.tangents[0].z = tangent[2];

        dst.tangents[1].x = binormal[0];
        dst.tangents[1].y = binormal[1];
        dst.tangents[1].z = binormal[2];

        dst.st.x = st[0];
        dst.st.y = st[1];

        dst.color[0] = colorAdd[0] + (color[0] >> colorShift);
        dst.color[1] = colorAdd[1] + (color[1] >> colorShift);
        dst.color[2] = colorAdd[2] + (color[2] >> colorShift);
        dst.color[3] = colorAdd[3] + (color[3] >> colorShift);
    }

    drawVertexBuffer.Unlock();
}

void rvPrimBatch::TubeDeform(
    idDrawVert* destDrawVerts,
    int* destIndices,
    const idVec3& localView,
    rvVertexBuffer& drawVertexBuffer,
    rvIndexBuffer& drawIndexBuffer,
    const rvSilTraceVertT* silTraceVerts,
    rvIndexBuffer& silTraceIndexBuffer)
{
    const int drawNumIndices = 3 * m_drawGeoSpec.m_primitiveCount;
    const int silNumIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;

    void* silIndexData = nullptr;
    void* drawIndexData = nullptr;
    silTraceIndexBuffer.Lock(m_silTraceGeoSpec.m_indexStart, silNumIndices, 1u, &silIndexData);
    drawIndexBuffer.Lock(m_drawGeoSpec.m_indexStart, drawNumIndices, 1u, &drawIndexData);

    const bool silIs16Bit = Is16BitIndexBuffer(silTraceIndexBuffer);
    const bool drawIs16Bit = Is16BitIndexBuffer(drawIndexBuffer);

    for (int i = 0; i < drawNumIndices; ++i)
    {
        destIndices[i] = ReadIndex(drawIndexData, drawIs16Bit, i) - m_drawGeoSpec.m_vertexStart;
    }

    float* texCoordSrc = nullptr;
    unsigned char* texCoordSrcBytes = nullptr;
    unsigned char* diffuseColorSrc = nullptr;
    int texCoordStride = 0;
    int diffuseColorStride = 0;

    drawVertexBuffer.LockTextureCoordinate(
        0,
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        texCoordSrcBytes,
        texCoordStride);
    texCoordSrc = reinterpret_cast<float*>(texCoordSrcBytes);

    drawVertexBuffer.LockDiffuseColor(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        diffuseColorSrc,
        diffuseColorStride);

    const Float3 view = MakeFloat3(localView);

    const int usableIndexCount = std::min(silNumIndices, m_drawGeoSpec.m_vertexCount);
    for (int baseVert = 0; baseVert + 3 < usableIndexCount; baseVert += 4)
    {
        int shortestPair = 0;
        int secondShortestPair = 1;
        float shortestLength = std::numeric_limits<float>::max();
        float secondShortestLength = std::numeric_limits<float>::max();

        for (int pairIndex = 0; pairIndex < 6; ++pairIndex)
        {
            const int i0 = ReadIndex(silIndexData, silIs16Bit, baseVert + kTubeEdgePairs[pairIndex][0]);
            const int i1 = ReadIndex(silIndexData, silIs16Bit, baseVert + kTubeEdgePairs[pairIndex][1]);
            const float length = sqrt(DistanceSquared(MakeFloat3(silTraceVerts[i0]), MakeFloat3(silTraceVerts[i1])));

            if (length < shortestLength)
            {
                secondShortestLength = shortestLength;
                secondShortestPair = shortestPair;
                shortestLength = length;
                shortestPair = pairIndex;
            }
            else if (length < secondShortestLength)
            {
                secondShortestLength = length;
                secondShortestPair = pairIndex;
            }
        }

        const int pairIndices[2] = { shortestPair, secondShortestPair };
        const float pairLengths[2] = { shortestLength, secondShortestLength };

        Float3 midpoints[2];
        for (int j = 0; j < 2; ++j)
        {
            const int pairIndex = pairIndices[j];
            const int i0 = ReadIndex(silIndexData, silIs16Bit, baseVert + kTubeEdgePairs[pairIndex][0]);
            const int i1 = ReadIndex(silIndexData, silIs16Bit, baseVert + kTubeEdgePairs[pairIndex][1]);

            const Float3 p0 = MakeFloat3(silTraceVerts[i0]);
            const Float3 p1 = MakeFloat3(silTraceVerts[i1]);
            midpoints[j] = Mul(Add(p0, p1), 0.5f);
        }

        const Float3 axis = Sub(midpoints[1], midpoints[0]);

        for (int j = 0; j < 2; ++j)
        {
            const int pairIndex = pairIndices[j];
            const int dst0 = destIndices[baseVert + kTubeEdgePairs[pairIndex][0]];
            const int dst1 = destIndices[baseVert + kTubeEdgePairs[pairIndex][1]];

            idDrawVert& v0 = destDrawVerts[dst0];
            idDrawVert& v1 = destDrawVerts[dst1];

            SetDrawVertTexCoord(v0, ByteOffset(texCoordSrc, texCoordStride, dst0));
            SetDrawVertTexCoord(v1, ByteOffset(texCoordSrc, texCoordStride, dst1));

            SetDrawVertColor(v0, ByteOffset(diffuseColorSrc, diffuseColorStride, dst0));
            SetDrawVertColor(v1, ByteOffset(diffuseColorSrc, diffuseColorStride, dst1));

            const float halfLength = pairLengths[j] * 0.5f;
            const Float3 viewToMid = Sub(midpoints[j], view);
            const Float3 normal = Normalize(Cross(viewToMid, axis));

            if (j == 0)
            {
                SetDrawVertPosition(v0, Add(midpoints[j], Mul(normal, halfLength)));
                SetDrawVertPosition(v1, Sub(midpoints[j], Mul(normal, halfLength)));
            }
            else
            {
                SetDrawVertPosition(v0, Sub(midpoints[j], Mul(normal, halfLength)));
                SetDrawVertPosition(v1, Add(midpoints[j], Mul(normal, halfLength)));
            }
        }
    }

    drawVertexBuffer.Unlock();
    drawIndexBuffer.Unlock();
    silTraceIndexBuffer.Unlock();
}

void rvPrimBatch::Init(int* transformPalette, int numTransforms)
{
    if (m_numTransforms != 0)
    {
        Shutdown();
    }

    if (numTransforms > MAX_TRANSFORMS_PER_BATCH)
    {
        idLib::common->FatalError("Primitive batch initialization failed - too many transforms per batch");
        return;
    }

    if (transformPalette != nullptr && numTransforms > 0)
    {
        m_transformPalette = static_cast<int*>(Mem_Alloc(sizeof(int) * numTransforms));
        if (m_transformPalette == nullptr)
        {
            idLib::common->FatalError("Out of memory");
            return;
        }

        std::memcpy(m_transformPalette, transformPalette, sizeof(int) * numTransforms);
    }

    m_numTransforms = numTransforms;
}

void rvPrimBatch::Init(Lexer& lex)
{
    idToken token;

    if (m_numTransforms != 0)
    {
        Shutdown();
    }

    lex.ExpectTokenString("{");
    lex.ReadToken(&token);

    if (idStr::Icmp(token.c_str(), "Transform") != 0)
    {
        m_numTransforms = 1;
    }
    else
    {
        lex.ExpectTokenString("[");
        m_numTransforms = lex.ParseInt();
        lex.ExpectTokenString("]");
        lex.ExpectTokenString("{");

        if (m_numTransforms > MAX_TRANSFORMS_PER_BATCH)
        {
            lex.Error("Primitive batch initialization failed - too many transforms per batch");
            return;
        }

        if (m_numTransforms == 1)
        {
            const int transformIndex = lex.ParseInt();
            if (transformIndex != 0)
            {
                m_transformPalette = static_cast<int*>(Mem_Alloc(sizeof(int)));
                if (m_transformPalette == nullptr)
                {
                    lex.Error("Out of memory");
                    return;
                }

                m_transformPalette[0] = transformIndex;
            }
        }
        else if (m_numTransforms > 0)
        {
            m_transformPalette = static_cast<int*>(Mem_Alloc(sizeof(int) * m_numTransforms));
            if (m_transformPalette == nullptr)
            {
                lex.Error("Out of memory");
                return;
            }

            for (int i = 0; i < m_numTransforms; ++i)
            {
                m_transformPalette[i] = lex.ParseInt();
            }
        }

        lex.ExpectTokenString("}");
        lex.ReadToken(&token);
    }

    if (idStr::Icmp(token.c_str(), "SilTraceIndexedTriList") == 0)
    {
        m_silTraceGeoSpec.m_vertexStart = lex.ParseInt();
        m_silTraceGeoSpec.m_vertexCount = lex.ParseInt();
        m_silTraceGeoSpec.m_indexStart = lex.ParseInt();
        m_silTraceGeoSpec.m_primitiveCount = lex.ParseInt();
        lex.ReadToken(&token);
    }

    if (idStr::Icmp(token.c_str(), "DrawIndexedTriList") == 0)
    {
        m_drawGeoSpec.m_vertexStart = lex.ParseInt();
        m_drawGeoSpec.m_vertexCount = lex.ParseInt();
        m_drawGeoSpec.m_indexStart = lex.ParseInt();
        m_drawGeoSpec.m_primitiveCount = lex.ParseInt();
        lex.ReadToken(&token);
    }

    if (idStr::Icmp(token.c_str(), "ShadowVerts") == 0)
    {
        m_shadowVolGeoSpec.m_vertexStart = lex.ParseInt();
        m_shadowVolGeoSpec.m_vertexCount = 2 * m_silTraceGeoSpec.m_vertexCount;

        if (m_silTraceGeoSpec.m_vertexCount == 0)
        {
            lex.Error("Primitive batch initialization failed - expected SilTraceIndexedTriList statement");
            return;
        }

        lex.ReadToken(&token);
    }
    else if (idStr::Icmp(token.c_str(), "ShadowIndexedTriList") == 0)
    {
        m_shadowVolGeoSpec.m_vertexStart = lex.ParseInt();
        m_shadowVolGeoSpec.m_vertexCount = lex.ParseInt();
        m_shadowVolGeoSpec.m_indexStart = lex.ParseInt();
        m_shadowVolGeoSpec.m_primitiveCount = lex.ParseInt();
        m_numShadowPrimitivesNoCaps = lex.ParseInt();
        m_shadowCapPlaneBits = lex.ParseInt();
        lex.ReadToken(&token);
    }

    if (idStr::Icmp(token.c_str(), "SilhouetteEdge") == 0)
    {
        m_silEdgeStart = lex.ParseInt();
        m_silEdgeCount = lex.ParseInt();
        lex.ReadToken(&token);
    }

    if (idStr::Icmp(token.c_str(), "}") != 0)
    {
        lex.Error("Expected }.");
    }
}


bool rvPrimBatch::PreciseCullSurface(
    idBounds& ndcBounds,
    const rvSilTraceVertT* silTraceVerts,
    const idVec3& localView,
    const float* modelMatrix,
    rvIndexBuffer& silTraceIB)
{
    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    void* indexData = nullptr;

    silTraceIB.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &indexData);

    const bool is16Bit = Is16BitIndexBuffer(silTraceIB);
    const Float3 view = MakeFloat3(localView);

    for (int tri = 0; tri < m_silTraceGeoSpec.m_primitiveCount; ++tri)
    {
        const int i0 = ReadIndex(indexData, is16Bit, tri * 3 + 0);
        const int i1 = ReadIndex(indexData, is16Bit, tri * 3 + 1);
        const int i2 = ReadIndex(indexData, is16Bit, tri * 3 + 2);

        const Float3 a = MakeFloat3(silTraceVerts[i0]);
        const Float3 b = MakeFloat3(silTraceVerts[i1]);
        const Float3 c = MakeFloat3(silTraceVerts[i2]);

        if (!tr.guiRecursionLevel)
        {
            const Float3 normal = Cross(Sub(b, a), Sub(c, a));
            if (Dot(normal, Sub(a, view)) >= 0.0f)
            {
                silTraceIB.Unlock();
                return true;
            }
        }

        idVec3 localPoint;
        idVec3 globalPoint;
        idVec3 screenPoint;

        localPoint.x = a.x;
        localPoint.y = a.y;
        localPoint.z = a.z;
        R_LocalPointToGlobal(modelMatrix, localPoint, globalPoint);
        R_GlobalToNormalizedDeviceCoordinates(globalPoint, screenPoint);
        ExpandBounds(ndcBounds, MakeFloat3(screenPoint));

        localPoint.x = b.x;
        localPoint.y = b.y;
        localPoint.z = b.z;
        R_LocalPointToGlobal(modelMatrix, localPoint, globalPoint);
        R_GlobalToNormalizedDeviceCoordinates(globalPoint, screenPoint);
        ExpandBounds(ndcBounds, MakeFloat3(screenPoint));

        localPoint.x = c.x;
        localPoint.y = c.y;
        localPoint.z = c.z;
        R_LocalPointToGlobal(modelMatrix, localPoint, globalPoint);
        R_GlobalToNormalizedDeviceCoordinates(globalPoint, screenPoint);
        ExpandBounds(ndcBounds, MakeFloat3(screenPoint));
    }

    silTraceIB.Unlock();
    return false;
}

void rvPrimBatch::FindDecalTriangles(
    idRenderModelDecal& decalModel,
    const decalProjectionInfo_s& localInfo,
    const idPlane* facePlanes,
    const rvSilTraceVertT* silTraceVerts,
    rvIndexBuffer& rfSilTraceIB)
{
    std::vector<unsigned char> cullBits(m_silTraceGeoSpec.m_vertexCount, 0);

    for (int i = 0; i < m_silTraceGeoSpec.m_vertexCount; ++i)
    {
        const Float3 p = MakeFloat3(silTraceVerts[i]);
        unsigned char bits = 0;

        for (int planeIndex = 0; planeIndex < 6; ++planeIndex)
        {
            if (PlaneDistance(localInfo.boundingPlanes[planeIndex], p) < 0.0f)
            {
                bits |= static_cast<unsigned char>(1u << planeIndex);
            }
        }

        cullBits[i] = bits;
    }

    const int numIndices = 3 * m_silTraceGeoSpec.m_primitiveCount;
    void* indexData = nullptr;
    rfSilTraceIB.Lock(m_silTraceGeoSpec.m_indexStart, numIndices, 1u, &indexData);

    const bool is16Bit = Is16BitIndexBuffer(rfSilTraceIB);

    for (int tri = 0; tri < m_silTraceGeoSpec.m_primitiveCount; ++tri)
    {
        const int i0 = ReadIndex(indexData, is16Bit, tri * 3 + 0);
        const int i1 = ReadIndex(indexData, is16Bit, tri * 3 + 1);
        const int i2 = ReadIndex(indexData, is16Bit, tri * 3 + 2);

        if ((cullBits[i0] & cullBits[i1] & cullBits[i2]) != 0)
        {
            continue;
        }

        if (facePlanes != nullptr)
        {
            const idPlane& facePlane = facePlanes[tri];
            const idPlane& projectionPlane = localInfo.boundingPlanes[4];
            const float facing =
                facePlane.a * projectionPlane.a +
                facePlane.b * projectionPlane.b +
                facePlane.c * projectionPlane.c;

            if (facing < -0.1f)
            {
                continue;
            }
        }

        idFixedWinding winding;
        FixedWindingAccess::NumPoints(winding) = 3;

        const int triIndices[3] = { i0, i1, i2 };
        for (int corner = 0; corner < 3; ++corner)
        {
            const Float3 p = MakeFloat3(silTraceVerts[triIndices[corner]]);
            FixedWindingAccess::Points(winding)[corner].x = p.x;
            FixedWindingAccess::Points(winding)[corner].y = p.y;
            FixedWindingAccess::Points(winding)[corner].z = p.z;

            if (!localInfo.parallel)
            {
                const Float3 dir = Sub(p, MakeFloat3(localInfo.projectionOrigin));
                const float denom =
                    dir.x * localInfo.boundingPlanes[5].a +
                    dir.y * localInfo.boundingPlanes[5].b +
                    dir.z * localInfo.boundingPlanes[5].c;

                float scale = 0.0f;
                if (denom != 0.0f)
                {
                    scale = -PlaneDistance(localInfo.boundingPlanes[5], p) / denom;
                }

                const Float3 projected = Add(p, Mul(dir, scale));
                FixedWindingAccess::Points(winding)[corner].s =
                    projected.x * localInfo.textureAxis[0].a +
                    projected.y * localInfo.textureAxis[0].b +
                    projected.z * localInfo.textureAxis[0].c +
                    localInfo.textureAxis[0].d;

                FixedWindingAccess::Points(winding)[corner].t =
                    projected.x * localInfo.textureAxis[1].a +
                    projected.y * localInfo.textureAxis[1].b +
                    projected.z * localInfo.textureAxis[1].c +
                    localInfo.textureAxis[1].d;
            }
            else
            {
                FixedWindingAccess::Points(winding)[corner].s =
                    p.x * localInfo.textureAxis[0].a +
                    p.y * localInfo.textureAxis[0].b +
                    p.z * localInfo.textureAxis[0].c +
                    localInfo.textureAxis[0].d;

                FixedWindingAccess::Points(winding)[corner].t =
                    p.x * localInfo.textureAxis[1].a +
                    p.y * localInfo.textureAxis[1].b +
                    p.z * localInfo.textureAxis[1].c +
                    localInfo.textureAxis[1].d;
            }
        }

        const unsigned char combinedCullBits = cullBits[i0] | cullBits[i1] | cullBits[i2];
        for (int planeIndex = 0; planeIndex < 6; ++planeIndex)
        {
            if ((combinedCullBits & (1u << planeIndex)) == 0)
            {
                continue;
            }

            idPlane plane;
            plane.a = -localInfo.boundingPlanes[planeIndex].a;
            plane.b = -localInfo.boundingPlanes[planeIndex].b;
            plane.c = -localInfo.boundingPlanes[planeIndex].c;
            plane.d = -localInfo.boundingPlanes[planeIndex].d;

            if (!winding.ClipInPlace(plane, 0.1f, false))
            {
                break;
            }
        }

        if (FixedWindingAccess::NumPoints(winding) > 0)
        {
            decalModel.AddDepthFadedWinding(
                winding,
                localInfo.material,
                localInfo.fadePlanes,
                localInfo.fadeDepth,
                localInfo.startTime);
        }
    }

    rfSilTraceIB.Unlock();
}

void rvPrimBatch::PlaneForSurface(idPlane& destPlane, const rvSilTraceVertT* silTraceVerts, rvIndexBuffer& silTraceIndexBuffer)
{
    void* indexData = nullptr;
    silTraceIndexBuffer.Lock(m_silTraceGeoSpec.m_indexStart, 3, 1u, &indexData);

    const bool is16Bit = Is16BitIndexBuffer(silTraceIndexBuffer);
    const int i0 = ReadIndex(indexData, is16Bit, 0);
    const int i1 = ReadIndex(indexData, is16Bit, 1);
    const int i2 = ReadIndex(indexData, is16Bit, 2);

    BuildPlaneFromTriangle(
        destPlane,
        MakeFloat3(silTraceVerts[i0]),
        MakeFloat3(silTraceVerts[i1]),
        MakeFloat3(silTraceVerts[i2]));

    silTraceIndexBuffer.Unlock();
}

void rvPrimBatch::GenerateCollisionPolys(
    idCollisionModelManagerLocal& modelManager,
    idCollisionModelLocal& collisionModel,
    const idMaterial& material,
    rvVertexBuffer& drawVertexBuffer,
    rvIndexBuffer& drawIndexBuffer)
{
    const int numIndices = 3 * m_drawGeoSpec.m_primitiveCount;
    void* indexData = nullptr;

    drawIndexBuffer.Lock(m_drawGeoSpec.m_indexStart, numIndices, 1u, &indexData);

    float* texCoordSrc = nullptr;
    unsigned char* texCoordSrcBytes = nullptr;
    float* positionSrc = nullptr;
    unsigned char* positionSrcBytes = nullptr;
    int texCoordStride = 0;
    int positionStride = 0;

    drawVertexBuffer.LockTextureCoordinate(
        0,
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        texCoordSrcBytes,
        texCoordStride);
    texCoordSrc = reinterpret_cast<float*>(texCoordSrcBytes);

    drawVertexBuffer.LockPosition(
        m_drawGeoSpec.m_vertexStart,
        m_drawGeoSpec.m_vertexCount,
        1u,
        positionSrcBytes,
        positionStride);
    positionSrc = reinterpret_cast<float*>(positionSrcBytes);

    const bool is16Bit = Is16BitIndexBuffer(drawIndexBuffer);
    const int drawVertexBase = m_drawGeoSpec.m_vertexStart;

    for (int tri = 0; tri < m_drawGeoSpec.m_primitiveCount; ++tri)
    {
        const int i0 = ReadIndex(indexData, is16Bit, tri * 3 + 0) - drawVertexBase;
        const int i1 = ReadIndex(indexData, is16Bit, tri * 3 + 1) - drawVertexBase;
        const int i2 = ReadIndex(indexData, is16Bit, tri * 3 + 2) - drawVertexBase;

        idFixedWinding winding;
        FixedWindingAccess::NumPoints(winding) = 0;

        const int indices[3] = { i2, i1, i0 };
        for (int corner = 0; corner < 3; ++corner)
        {
            const float* pos = ByteOffset(positionSrc, positionStride, indices[corner]);
            const float* st = ByteOffset(texCoordSrc, texCoordStride, indices[corner]);

            if (FixedWindingAccess::NumPoints(winding) + 1 > FixedWindingAccess::AllocedSize(winding))
            {
                FixedWindingAccess::ReAllocate(winding, FixedWindingAccess::NumPoints(winding) + 1, true);
            }

            idVec5& point = FixedWindingAccess::Points(winding)[FixedWindingAccess::NumPoints(winding)++];
            point.x = pos[0];
            point.y = pos[1];
            point.z = pos[2];
            point.s = st[0];
            point.t = st[1];
        }

        idPlane plane;
        winding.GetPlane(plane);

        plane.a = -plane.a;
        plane.b = -plane.b;
        plane.c = -plane.c;
        plane.d = -plane.d;

        modelManager.PolygonFromWinding(&collisionModel, &winding, plane, &material, 1);
    }

    drawVertexBuffer.Unlock();
    drawIndexBuffer.Unlock();
}
