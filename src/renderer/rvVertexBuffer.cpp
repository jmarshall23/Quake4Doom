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

namespace {

    constexpr unsigned int kMemTagRenderer = 0x11u;

    constexpr std::uint32_t kBufferFlagSystemMemory = 1u << 0;
    constexpr std::uint32_t kBufferFlagVideoMemory = 1u << 1;
    constexpr std::uint32_t kBufferFlagSoA = 1u << 2;
    constexpr std::uint32_t kBufferFlagDynamic = 1u << 3;   // best-effort name
    constexpr std::uint32_t kBufferFlagNoData = 1u << 4;   // serialized without payload
    constexpr std::uint32_t kBufferFlagUseLoadFmt = 1u << 5;   // best-effort name

    constexpr std::uint32_t kLockRead = 1u << 0;
    constexpr std::uint32_t kLockWrite = 1u << 1;
    constexpr std::uint32_t kLockDiscard = 1u << 2;

    constexpr int kComponentPosition = 0;
    constexpr int kComponentSwizzledPosition = 1;
    constexpr int kComponentBlendIndex = 2;
    constexpr int kComponentBlendWeight = 3;
    constexpr int kComponentNormal = 4;
    constexpr int kComponentTangent = 5;
    constexpr int kComponentBinormal = 6;
    constexpr int kComponentDiffuseColor = 7;
    constexpr int kComponentSpecularColor = 8;
    constexpr int kComponentPointSize = 9;
    constexpr int kComponentTexCoord0 = 10;

    template <typename T>
    T* ByteOffset(void* base, int byteOffset) {
        return reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(base) + byteOffset);
    }

    template <typename T>
    const T* ByteOffset(const void* base, int byteOffset) {
        return reinterpret_cast<const T*>(reinterpret_cast<const unsigned char*>(base) + byteOffset);
    }

    inline int AlignUp4(int value) {
        return (value + 3) & ~3;
    }

    inline int MinInt(int a, int b) {
        return (a < b) ? a : b;
    }

    inline int GetPositionDimension(const rvVertexFormat& format) {
        return format.m_dimensions & 0x7;
    }

    inline int GetBlendWeightDimension(const rvVertexFormat& format) {
        return (format.m_dimensions >> 3) & 0x7;
    }

    inline int GetTexCoordDimension(const rvVertexFormat& format, int unit) {
        return (format.m_dimensions >> (9 + unit * 3)) & 0x7;
    }

    inline Rv_Vertex_Component_t ToComponent(int value) {
        return (Rv_Vertex_Component_t)value;
    }

    inline bool HasFlag(unsigned int flags, unsigned int mask) {
        return (flags & mask) != 0;
    }

    template <typename T>
    T* AllocElements(size_t count) {
        return static_cast<T*>(Mem_Alloc16(sizeof(T) * count, kMemTagRenderer));
    }

    template <typename T>
    bool ReallocateBuffer(T*& array, size_t newCount, size_t copyCount) {
        T* newArray = AllocElements<T>(newCount);
        if (newArray == NULL) {
            return false;
        }

        if (array != NULL) {
            if (copyCount > 0) {
                memcpy(newArray, array, sizeof(T) * copyCount);
            }
            Mem_Free16(array);
        }

        array = newArray;
        return true;
    }

    template <typename T>
    void CopyPackedAttributeToAoS(unsigned char* destBase, int destStride, const T* src, int vertexCount) {
        int i;
        for (i = 0; i < vertexCount; ++i) {
            *reinterpret_cast<T*>(destBase) = src[i];
            destBase += destStride;
        }
    }

    void CopyFloatAttributeToAoS(
        unsigned char* destBase,
        int destStride,
        const float* src,
        int vertexCount,
        int componentCount) {
        int i;

        if (componentCount <= 0 || src == NULL) {
            return;
        }

        for (i = 0; i < vertexCount; ++i) {
            memcpy(destBase, src, sizeof(float) * componentCount);
            src += componentCount;
            destBase += destStride;
        }
    }

    void CopySwizzledPositionToAoS(
        unsigned char* destBase,
        int destStride,
        const float* swizzledSource,
        int vertexCount) {
        if (swizzledSource == NULL) {
            return;
        }

        const int fullBlocks = vertexCount / 4;
        int block;
        for (block = 0; block < fullBlocks; ++block) {
            const float* src = swizzledSource + block * 12;
            int lane;
            for (lane = 0; lane < 4; ++lane) {
                float* dst = reinterpret_cast<float*>(destBase + (block * 4 + lane) * destStride);
                dst[0] = src[lane + 0];
                dst[1] = src[lane + 4];
                dst[2] = src[lane + 8];
            }
        }

        const int remainder = vertexCount & 3;
        if (remainder == 0) {
            return;
        }

        const float* src = swizzledSource + fullBlocks * 12;
        int lane;
        for (lane = 0; lane < remainder; ++lane) {
            float* dst = reinterpret_cast<float*>(destBase + (fullBlocks * 4 + lane) * destStride);
            dst[0] = src[lane + 0];
            dst[1] = src[lane + 4];
            dst[2] = src[lane + 8];
        }
    }

    void CopyBlendWeights(
        float* dst,
        int maxWeights,
        const rvBlend4DrawVert& src,
        bool absoluteValues) {
        int i;
        for (i = 0; i < maxWeights; ++i) {
            dst[i] = absoluteValues ? (float)fabs(src.blendWeight[i]) : src.blendWeight[i];
        }
    }

    bool FormatLayoutDiffers(const rvVertexFormat& lhs, const rvVertexFormat& rhs) {
        return lhs.m_flags != rhs.m_flags || lhs.m_dimensions != rhs.m_dimensions;
    }

    // idFile in the target tree appears not to expose WriteNumericStructArray().
    // This best-effort local writer keeps rvVertexBuffer::Write() buildable.
    void WriteNumericStructArray(
        idFile& outFile,
        int numStructElements,
        const int* tokenSubTypes,
        int count,
        const unsigned char* data,
        const char* prepend) {
        (void)tokenSubTypes;

        const char* indent = prepend ? prepend : "";
        int i;
        int j;

        if (data == NULL || numStructElements <= 0 || count <= 0) {
            return;
        }

        for (i = 0; i < count; ++i) {
            outFile.WriteFloatString("%s", indent);
            for (j = 0; j < numStructElements; ++j) {
                if (j != 0) {
                    outFile.WriteFloatString(" ");
                }
                outFile.WriteFloatString("%g", ByteOffset<const float>(data, (i * numStructElements + j) * (int)sizeof(float))[0]);
            }
            outFile.WriteFloatString("\n");
        }
    }

} // namespace

rvVertexBuffer::rvVertexBuffer() {
    ResetValues();
}

rvVertexBuffer::~rvVertexBuffer() {
    Shutdown();
}

void rvVertexBuffer::ResetValues() {
    m_flags = 0;
    m_lockStatus = 0;
    m_numVertices = 0;
    m_vbID = 0;
    m_lockVertexOffset = 0;
    m_lockVertexCount = 0;
    m_lockedBase = NULL;
    m_numVerticesWritten = 0;

    m_interleavedStorage = NULL;
    m_positionArray = NULL;
    m_swizzledPositionArray = NULL;
    m_blendIndexArray = NULL;
    m_blendWeightArray = NULL;
    m_normalArray = NULL;
    m_tangentArray = NULL;
    m_binormalArray = NULL;
    m_diffuseColorArray = NULL;
    m_specularColorArray = NULL;
    m_pointSizeArray = NULL;

    for (int i = 0; i < 7; ++i) {
        m_texCoordArrays[i] = NULL;
    }
}

void rvVertexBuffer::CreateVertexStorage() {
    bool outOfMemory = false;

    if (HasFlag(m_flags, kBufferFlagSystemMemory)) {
        if (HasFlag(m_flags, kBufferFlagSoA)) {
            if (HasFlag(m_format.m_flags, 0x0001u)) {
                m_positionArray = AllocElements<float>(static_cast<size_t>(m_numVertices) * GetPositionDimension(m_format));
                outOfMemory |= (m_positionArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0002u)) {
                m_swizzledPositionArray = AllocElements<float>(static_cast<size_t>(AlignUp4(m_numVertices)) * 3);
                outOfMemory |= (m_swizzledPositionArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0004u)) {
                m_blendIndexArray = AllocElements<unsigned int>(m_numVertices);
                outOfMemory |= (m_blendIndexArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0008u)) {
                m_blendWeightArray = AllocElements<float>(static_cast<size_t>(m_numVertices) * GetBlendWeightDimension(m_format));
                outOfMemory |= (m_blendWeightArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0010u)) {
                m_normalArray = AllocElements<float>(static_cast<size_t>(m_numVertices) * 3);
                outOfMemory |= (m_normalArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0020u)) {
                m_tangentArray = AllocElements<float>(static_cast<size_t>(m_numVertices) * 3);
                outOfMemory |= (m_tangentArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0040u)) {
                m_binormalArray = AllocElements<float>(static_cast<size_t>(m_numVertices) * 3);
                outOfMemory |= (m_binormalArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0080u)) {
                m_diffuseColorArray = AllocElements<unsigned int>(m_numVertices);
                outOfMemory |= (m_diffuseColorArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0100u)) {
                m_specularColorArray = AllocElements<unsigned int>(m_numVertices);
                outOfMemory |= (m_specularColorArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0200u)) {
                m_pointSizeArray = AllocElements<float>(m_numVertices);
                outOfMemory |= (m_pointSizeArray == NULL);
            }

            if (HasFlag(m_format.m_flags, 0x0400u)) {
                for (int unit = 0; unit < 7; ++unit) {
                    const int dim = GetTexCoordDimension(m_format, unit);
                    if (dim == 0) {
                        continue;
                    }

                    m_texCoordArrays[unit] = AllocElements<float>(static_cast<size_t>(m_numVertices) * dim);
                    outOfMemory |= (m_texCoordArrays[unit] == NULL);
                }
            }
        }
        else {
            m_interleavedStorage = AllocElements<unsigned char>(static_cast<size_t>(m_numVertices) * m_format.m_size);
            outOfMemory = (m_interleavedStorage == NULL);
        }
    }

    if (outOfMemory) {
        idLib::common->Error("Ran out of memory trying to allocate system memory vertex storage");
        return;
    }

    if (!HasFlag(m_flags, kBufferFlagVideoMemory)) {
        return;
    }

    glGetError();
    glGenBuffersARB(1, &m_vbID);
    if (m_vbID == 0) {
        idLib::common->Error("rvVertexBuffer: Unable to generate a buffer id");
        return;
    }

    if (HasFlag(m_flags, kBufferFlagDynamic)) {
        return;
    }

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbID);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_format.m_size * m_numVertices, NULL, GL_STATIC_DRAW_ARB);

    const unsigned int glError = glGetError();
    if (glError != 0) {
        idLib::common->Error("Unable to allocate vertex storage - %u", glError);
    }
    else {
        m_format.SetVertexDeclaration(0);
    }

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

bool rvVertexBuffer::LockInterleaved(
    int vertexOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    void*& startPtr,
    int& stride) {
    m_lockVertexOffset = vertexOffset;
    m_lockVertexCount = numVerticesToLock;

    if (HasFlag(m_flags, kBufferFlagSystemMemory)) {
        m_lockedBase = m_interleavedStorage;
    }
    else if (m_lockStatus == 0) {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbID);

        unsigned int effectiveLockFlags = lockFlags;
        unsigned int glAccess = GL_READ_ONLY_ARB;

        if (HasFlag(lockFlags, kLockRead)) {
            glAccess = HasFlag(lockFlags, kLockWrite) ? GL_READ_WRITE_ARB : GL_READ_ONLY_ARB;
        }
        else if (HasFlag(lockFlags, kLockWrite)) {
            glAccess = GL_WRITE_ONLY_ARB;

            if (HasFlag(m_flags, kBufferFlagDynamic)) {
                effectiveLockFlags |= kLockDiscard;
            }

            if (HasFlag(effectiveLockFlags, kLockDiscard)) {
                if (m_lockVertexCount == 0) {
                    m_lockVertexCount = m_numVertices;
                }

                const int sizeBytes = m_lockVertexCount * m_format.m_size;
                m_lockVertexOffset = 0;
                glBufferDataARB(
                    GL_ARRAY_BUFFER_ARB,
                    sizeBytes,
                    NULL,
                    HasFlag(m_flags, kBufferFlagDynamic) ? GL_STREAM_DRAW_ARB : GL_STATIC_DRAW_ARB);
            }
        }

        m_lockedBase = static_cast<unsigned char*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB, glAccess));
        if (m_lockedBase == NULL) {
            return false;
        }

        m_lockStatus = effectiveLockFlags;
    }

    m_lockStatus = lockFlags;
    startPtr = m_lockedBase + m_lockVertexOffset * m_format.m_size;
    stride = m_format.m_size;
    return true;
}

void rvVertexBuffer::TransferSoAToAoS(unsigned char* vertexDest) {
    if (HasFlag(m_format.m_flags, 0x0002u)) {
        CopySwizzledPositionToAoS(vertexDest, m_format.m_size, m_swizzledPositionArray, m_numVertices);
    }
    else if (HasFlag(m_format.m_flags, 0x0001u)) {
        CopyFloatAttributeToAoS(vertexDest, m_format.m_size, m_positionArray, m_numVertices, GetPositionDimension(m_format));
    }

    if (HasFlag(m_format.m_flags, 0x0004u)) {
        CopyPackedAttributeToAoS(
            vertexDest + m_format.m_byteOffset[2],
            m_format.m_size,
            m_blendIndexArray,
            m_numVertices);
    }

    if (HasFlag(m_format.m_flags, 0x0008u)) {
        CopyFloatAttributeToAoS(
            vertexDest + m_format.m_byteOffset[3],
            m_format.m_size,
            m_blendWeightArray,
            m_numVertices,
            GetBlendWeightDimension(m_format));
    }

    if (HasFlag(m_format.m_flags, 0x0010u)) {
        CopyFloatAttributeToAoS(vertexDest + m_format.m_byteOffset[4], m_format.m_size, m_normalArray, m_numVertices, 3);
    }

    if (HasFlag(m_format.m_flags, 0x0020u)) {
        CopyFloatAttributeToAoS(vertexDest + m_format.m_byteOffset[5], m_format.m_size, m_tangentArray, m_numVertices, 3);
    }

    if (HasFlag(m_format.m_flags, 0x0040u)) {
        CopyFloatAttributeToAoS(vertexDest + m_format.m_byteOffset[6], m_format.m_size, m_binormalArray, m_numVertices, 3);
    }

    if (HasFlag(m_format.m_flags, 0x0080u)) {
        CopyPackedAttributeToAoS(
            vertexDest + m_format.m_byteOffset[7],
            m_format.m_size,
            m_diffuseColorArray,
            m_numVertices);
    }

    if (HasFlag(m_format.m_flags, 0x0100u)) {
        CopyPackedAttributeToAoS(
            vertexDest + m_format.m_byteOffset[8],
            m_format.m_size,
            m_specularColorArray,
            m_numVertices);
    }

    if (HasFlag(m_format.m_flags, 0x0200u)) {
        CopyFloatAttributeToAoS(vertexDest + m_format.m_byteOffset[9], m_format.m_size, m_pointSizeArray, m_numVertices, 1);
    }

    if (HasFlag(m_format.m_flags, 0x0400u)) {
        for (int unit = 0; unit < 7; ++unit) {
            const int dim = GetTexCoordDimension(m_format, unit);
            if (dim == 0 || m_texCoordArrays[unit] == NULL) {
                continue;
            }

            CopyFloatAttributeToAoS(
                vertexDest + m_format.m_byteOffset[kComponentTexCoord0 + unit],
                m_format.m_size,
                m_texCoordArrays[unit],
                m_numVertices,
                dim);
        }
    }
}

void rvVertexBuffer::SetupForRender(int vertexStartOffset, const rvVertexFormat& formatNeeded) {
    if (!HasFlag(m_flags, kBufferFlagVideoMemory)) {
        return;
    }

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbID);
    m_format.SetVertexDeclaration(vertexStartOffset);
    formatNeeded.EnableVertexDeclaration();
}

rvSilTraceVertT* rvVertexBuffer::GetSilTraceVertexArray(int vertexOffset) {
    return reinterpret_cast<rvSilTraceVertT*>(m_interleavedStorage + 16 * vertexOffset);
}

void rvVertexBuffer::SetLoadFormat(const rvVertexFormat& loadFormat) {
    m_loadFormat.Init(loadFormat);
}

void rvVertexBuffer::Unlock() {
    if (HasFlag(m_lockStatus, kLockWrite)) {
        if (HasFlag(m_lockStatus, kLockDiscard)) {
            m_numVerticesWritten = m_lockVertexCount;
        }
        else {
            m_numVerticesWritten += m_lockVertexCount;
        }
    }

    if (HasFlag(m_flags, kBufferFlagSystemMemory)) {
        if (HasFlag(m_lockStatus, kLockWrite) && HasFlag(m_flags, kBufferFlagVideoMemory)) {
            const int offsetBytes = m_format.m_size * m_lockVertexOffset;
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbID);

            if (HasFlag(m_flags, kBufferFlagSoA)) {
                glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_format.m_size * m_numVertices, NULL, GL_STATIC_DRAW_ARB);
                unsigned char* mapped = static_cast<unsigned char*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB));
                if (mapped != NULL) {
                    TransferSoAToAoS(mapped);
                    glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
                }
            }
            else if (m_interleavedStorage != NULL) {
                glBufferSubDataARB(
                    GL_ARRAY_BUFFER_ARB,
                    offsetBytes,
                    m_format.m_size * m_lockVertexCount,
                    m_interleavedStorage + offsetBytes);
            }

            glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }
    }
    else if (HasFlag(m_flags, kBufferFlagVideoMemory)) {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbID);
        glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    m_lockStatus = 0;
    m_lockVertexOffset = 0;
    m_lockVertexCount = 0;
    m_lockedBase = NULL;
}

void rvVertexBuffer::Shutdown() {
    if (m_lockStatus != 0) {
        Unlock();
    }

    if (m_interleavedStorage != NULL) {
        Mem_Free16(m_interleavedStorage);
    }
    if (m_positionArray != NULL) {
        Mem_Free16(m_positionArray);
    }
    if (m_swizzledPositionArray != NULL) {
        Mem_Free16(m_swizzledPositionArray);
    }
    if (m_blendIndexArray != NULL) {
        Mem_Free16(m_blendIndexArray);
    }
    if (m_blendWeightArray != NULL) {
        Mem_Free16(m_blendWeightArray);
    }
    if (m_normalArray != NULL) {
        Mem_Free16(m_normalArray);
    }
    if (m_tangentArray != NULL) {
        Mem_Free16(m_tangentArray);
    }
    if (m_binormalArray != NULL) {
        Mem_Free16(m_binormalArray);
    }
    if (m_diffuseColorArray != NULL) {
        Mem_Free16(m_diffuseColorArray);
    }
    if (m_specularColorArray != NULL) {
        Mem_Free16(m_specularColorArray);
    }
    if (m_pointSizeArray != NULL) {
        Mem_Free16(m_pointSizeArray);
    }

    for (int i = 0; i < 7; ++i) {
        if (m_texCoordArrays[i] != NULL) {
            Mem_Free16(m_texCoordArrays[i]);
        }
    }

    if (m_vbID != 0) {
        glDeleteBuffersARB(1, &m_vbID);
    }

    m_format.Shutdown();
    m_loadFormat.Shutdown();
    ResetValues();
}

void rvVertexBuffer::Resize(int numVertices) {
    if (numVertices == m_numVertices || !HasFlag(m_flags, kBufferFlagSystemMemory)) {
        return;
    }

    const int oldVertexCount = m_numVertices;
    const int copyVertexCount = MinInt(oldVertexCount, numVertices);

    bool failed = false;

    if (HasFlag(m_flags, kBufferFlagSoA)) {
        if (HasFlag(m_format.m_flags, 0x0001u)) {
            failed |= !ReallocateBuffer(
                m_positionArray,
                static_cast<size_t>(numVertices) * GetPositionDimension(m_format),
                static_cast<size_t>(copyVertexCount) * GetPositionDimension(m_format));
        }

        if (HasFlag(m_format.m_flags, 0x0002u)) {
            failed |= !ReallocateBuffer(
                m_swizzledPositionArray,
                static_cast<size_t>(AlignUp4(numVertices)) * 3,
                static_cast<size_t>(AlignUp4(copyVertexCount)) * 3);
        }

        if (HasFlag(m_format.m_flags, 0x0004u)) {
            failed |= !ReallocateBuffer(
                m_blendIndexArray,
                static_cast<size_t>(numVertices),
                static_cast<size_t>(copyVertexCount));
        }

        if (HasFlag(m_format.m_flags, 0x0008u)) {
            failed |= !ReallocateBuffer(
                m_blendWeightArray,
                static_cast<size_t>(numVertices) * GetBlendWeightDimension(m_format),
                static_cast<size_t>(copyVertexCount) * GetBlendWeightDimension(m_format));
        }

        if (HasFlag(m_format.m_flags, 0x0010u)) {
            failed |= !ReallocateBuffer(m_normalArray, static_cast<size_t>(numVertices) * 3, static_cast<size_t>(copyVertexCount) * 3);
        }

        if (HasFlag(m_format.m_flags, 0x0020u)) {
            failed |= !ReallocateBuffer(m_tangentArray, static_cast<size_t>(numVertices) * 3, static_cast<size_t>(copyVertexCount) * 3);
        }

        if (HasFlag(m_format.m_flags, 0x0040u)) {
            failed |= !ReallocateBuffer(m_binormalArray, static_cast<size_t>(numVertices) * 3, static_cast<size_t>(copyVertexCount) * 3);
        }

        if (HasFlag(m_format.m_flags, 0x0080u)) {
            failed |= !ReallocateBuffer(
                m_diffuseColorArray,
                static_cast<size_t>(numVertices),
                static_cast<size_t>(copyVertexCount));
        }

        if (HasFlag(m_format.m_flags, 0x0100u)) {
            failed |= !ReallocateBuffer(
                m_specularColorArray,
                static_cast<size_t>(numVertices),
                static_cast<size_t>(copyVertexCount));
        }

        if (HasFlag(m_format.m_flags, 0x0200u)) {
            failed |= !ReallocateBuffer(m_pointSizeArray, static_cast<size_t>(numVertices), static_cast<size_t>(copyVertexCount));
        }

        if (HasFlag(m_format.m_flags, 0x0400u)) {
            for (int unit = 0; unit < 7; ++unit) {
                const int dim = GetTexCoordDimension(m_format, unit);
                if (dim == 0) {
                    continue;
                }

                failed |= !ReallocateBuffer(
                    m_texCoordArrays[unit],
                    static_cast<size_t>(numVertices) * dim,
                    static_cast<size_t>(copyVertexCount) * dim);
            }
        }

        if (failed) {
            idLib::common->Error("Ran out of memory trying to allocate system memory vertex storage");
            Shutdown();
            return;
        }

        m_numVertices = numVertices;

        if (HasFlag(m_flags, kBufferFlagVideoMemory)) {
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbID);
            glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_numVertices * m_format.m_size, NULL, GL_STATIC_DRAW_ARB);

            unsigned char* mapped = static_cast<unsigned char*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB));
            if (mapped != NULL) {
                TransferSoAToAoS(mapped);
                glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
            }

            glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }

        return;
    }

    if (!ReallocateBuffer(
        m_interleavedStorage,
        static_cast<size_t>(numVertices) * m_format.m_size,
        static_cast<size_t>(copyVertexCount) * m_format.m_size)) {
        idLib::common->Error("Ran out of memory trying to allocate system memory vertex storage");
        Shutdown();
        return;
    }

    m_numVertices = numVertices;

    if (HasFlag(m_flags, kBufferFlagVideoMemory)) {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbID);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_numVertices * m_format.m_size, m_interleavedStorage, GL_STATIC_DRAW_ARB);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}

bool rvVertexBuffer::LockPosition(
    int vertexBufferOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    unsigned char*& posPtr,
    int& stride) {
    m_lockVertexOffset = vertexBufferOffset;
    m_lockVertexCount = numVerticesToLock;

    if (!HasFlag(m_flags, kBufferFlagSoA)) {
        if (!LockInterleaved(vertexBufferOffset, numVerticesToLock, lockFlags, reinterpret_cast<void*&>(posPtr), stride)) {
            return false;
        }
        posPtr += m_format.m_byteOffset[0];
        return true;
    }

    const int dimension = GetPositionDimension(m_format);
    m_lockStatus = lockFlags;
    posPtr = reinterpret_cast<unsigned char*>(m_positionArray + vertexBufferOffset * dimension);
    stride = sizeof(float) * dimension;
    return true;
}

bool rvVertexBuffer::LockBlendIndex(
    int vertexBufferOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    unsigned char*& blendIndexPtr,
    int& stride) {
    m_lockVertexOffset = vertexBufferOffset;
    m_lockVertexCount = numVerticesToLock;

    if (!HasFlag(m_flags, kBufferFlagSoA)) {
        if (!LockInterleaved(vertexBufferOffset, numVerticesToLock, lockFlags, reinterpret_cast<void*&>(blendIndexPtr), stride)) {
            return false;
        }
        blendIndexPtr += m_format.m_byteOffset[2];
        return true;
    }

    m_lockStatus = lockFlags;
    blendIndexPtr = reinterpret_cast<unsigned char*>(m_blendIndexArray + vertexBufferOffset);
    stride = sizeof(unsigned int);
    return true;
}

bool rvVertexBuffer::LockBlendWeight(
    int vertexBufferOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    unsigned char*& blendWeightPtr,
    int& stride) {
    m_lockVertexOffset = vertexBufferOffset;
    m_lockVertexCount = numVerticesToLock;

    if (!HasFlag(m_flags, kBufferFlagSoA)) {
        if (!LockInterleaved(vertexBufferOffset, numVerticesToLock, lockFlags, reinterpret_cast<void*&>(blendWeightPtr), stride)) {
            return false;
        }
        blendWeightPtr += m_format.m_byteOffset[3];
        return true;
    }

    const int dimension = GetBlendWeightDimension(m_format);
    m_lockStatus = lockFlags;
    blendWeightPtr = reinterpret_cast<unsigned char*>(m_blendWeightArray + vertexBufferOffset * dimension);
    stride = sizeof(float) * dimension;
    return true;
}

bool rvVertexBuffer::LockNormal(
    int vertexBufferOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    unsigned char*& normalPtr,
    int& stride) {
    m_lockVertexOffset = vertexBufferOffset;
    m_lockVertexCount = numVerticesToLock;

    if (!HasFlag(m_flags, kBufferFlagSoA)) {
        if (!LockInterleaved(vertexBufferOffset, numVerticesToLock, lockFlags, reinterpret_cast<void*&>(normalPtr), stride)) {
            return false;
        }
        normalPtr += m_format.m_byteOffset[4];
        return true;
    }

    m_lockStatus = lockFlags;
    normalPtr = reinterpret_cast<unsigned char*>(m_normalArray + vertexBufferOffset * 3);
    stride = sizeof(float) * 3;
    return true;
}

bool rvVertexBuffer::LockTangent(
    int vertexBufferOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    unsigned char*& tangentPtr,
    int& stride) {
    m_lockVertexOffset = vertexBufferOffset;
    m_lockVertexCount = numVerticesToLock;

    if (!HasFlag(m_flags, kBufferFlagSoA)) {
        if (!LockInterleaved(vertexBufferOffset, numVerticesToLock, lockFlags, reinterpret_cast<void*&>(tangentPtr), stride)) {
            return false;
        }
        tangentPtr += m_format.m_byteOffset[5];
        return true;
    }

    m_lockStatus = lockFlags;
    tangentPtr = reinterpret_cast<unsigned char*>(m_tangentArray + vertexBufferOffset * 3);
    stride = sizeof(float) * 3;
    return true;
}

bool rvVertexBuffer::LockBinormal(
    int vertexBufferOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    unsigned char*& binormalPtr,
    int& stride) {
    m_lockVertexOffset = vertexBufferOffset;
    m_lockVertexCount = numVerticesToLock;

    if (!HasFlag(m_flags, kBufferFlagSoA)) {
        if (!LockInterleaved(vertexBufferOffset, numVerticesToLock, lockFlags, reinterpret_cast<void*&>(binormalPtr), stride)) {
            return false;
        }
        binormalPtr += m_format.m_byteOffset[6];
        return true;
    }

    m_lockStatus = lockFlags;
    binormalPtr = reinterpret_cast<unsigned char*>(m_binormalArray + vertexBufferOffset * 3);
    stride = sizeof(float) * 3;
    return true;
}

bool rvVertexBuffer::LockDiffuseColor(
    int vertexBufferOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    unsigned char*& diffuseColorPtr,
    int& stride) {
    m_lockVertexOffset = vertexBufferOffset;
    m_lockVertexCount = numVerticesToLock;

    if (!HasFlag(m_flags, kBufferFlagSoA)) {
        if (!LockInterleaved(vertexBufferOffset, numVerticesToLock, lockFlags, reinterpret_cast<void*&>(diffuseColorPtr), stride)) {
            return false;
        }
        diffuseColorPtr += m_format.m_byteOffset[7];
        return true;
    }

    m_lockStatus = lockFlags;
    diffuseColorPtr = reinterpret_cast<unsigned char*>(m_diffuseColorArray + vertexBufferOffset);
    stride = sizeof(unsigned int);
    return true;
}

bool rvVertexBuffer::LockTextureCoordinate(
    int texCoordOffset,
    int vertexBufferOffset,
    int numVerticesToLock,
    unsigned int lockFlags,
    unsigned char*& texCoordPtr,
    int& stride) {
    m_lockVertexOffset = vertexBufferOffset;
    m_lockVertexCount = numVerticesToLock;

    if (!HasFlag(m_flags, kBufferFlagSoA)) {
        if (!LockInterleaved(vertexBufferOffset, numVerticesToLock, lockFlags, reinterpret_cast<void*&>(texCoordPtr), stride)) {
            return false;
        }
        texCoordPtr += m_format.m_byteOffset[kComponentTexCoord0 + texCoordOffset];
        return true;
    }

    const int dimension = GetTexCoordDimension(m_format, texCoordOffset);
    m_lockStatus = lockFlags;
    texCoordPtr = reinterpret_cast<unsigned char*>(m_texCoordArrays[texCoordOffset] + vertexBufferOffset * dimension);
    stride = sizeof(float) * dimension;
    return true;
}

void rvVertexBuffer::CopyData(
    int destVertexOffset,
    int numVertices,
    const unsigned char* srcPtr,
    int srcStride,
    const rvVertexFormat& srcFormat,
    const unsigned int* copyMapping) {
    unsigned char* destPtr = NULL;
    int destStride = 0;

    if (!LockInterleaved(destVertexOffset, numVertices, kLockWrite, reinterpret_cast<void*&>(destPtr), destStride)) {
        idLib::common->Error("Vertex buffer cannot be mapped for access");
        return;
    }

    if (HasFlag(m_format.m_flags, 0x0001u) && HasFlag(srcFormat.m_flags, 0x0001u)) {
        ComponentCopy(
            destPtr + m_format.m_byteOffset[0],
            destStride,
            (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[0],
            GetPositionDimension(m_format),
            srcPtr + srcFormat.m_byteOffset[0],
            srcStride,
            (Rv_Vertex_Data_Type_t)srcFormat.m_dataTypes[0],
            GetPositionDimension(srcFormat),
            numVertices,
            copyMapping,
            0,
            0);
    }

    if (HasFlag(m_format.m_flags, 0x0004u) && HasFlag(srcFormat.m_flags, 0x0004u)) {
        ComponentCopy(destPtr + m_format.m_byteOffset[2], destStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[2], 1,
            srcPtr + srcFormat.m_byteOffset[2], srcStride, (Rv_Vertex_Data_Type_t) srcFormat.m_dataTypes[2], 1,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0008u) && HasFlag(srcFormat.m_flags, 0x0008u)) {
        ComponentCopy(destPtr + m_format.m_byteOffset[3], destStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[3], GetBlendWeightDimension(m_format),
            srcPtr + srcFormat.m_byteOffset[3], srcStride, (Rv_Vertex_Data_Type_t)srcFormat.m_dataTypes[3], GetBlendWeightDimension(srcFormat),
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0010u) && HasFlag(srcFormat.m_flags, 0x0010u)) {
        ComponentCopy(destPtr + m_format.m_byteOffset[4], destStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[4], 3,
            srcPtr + srcFormat.m_byteOffset[4], srcStride, (Rv_Vertex_Data_Type_t)srcFormat.m_dataTypes[4], 3,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0020u) && HasFlag(srcFormat.m_flags, 0x0020u)) {
        ComponentCopy(destPtr + m_format.m_byteOffset[5], destStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[5], 3,
            srcPtr + srcFormat.m_byteOffset[5], srcStride, (Rv_Vertex_Data_Type_t)srcFormat.m_dataTypes[5], 3,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0040u) && HasFlag(srcFormat.m_flags, 0x0040u)) {
        ComponentCopy(destPtr + m_format.m_byteOffset[6], destStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[6], 3,
            srcPtr + srcFormat.m_byteOffset[6], srcStride, (Rv_Vertex_Data_Type_t)srcFormat.m_dataTypes[6], 3,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0080u) && HasFlag(srcFormat.m_flags, 0x0080u)) {
        ComponentCopy(destPtr + m_format.m_byteOffset[7], destStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[7], 1,
            srcPtr + srcFormat.m_byteOffset[7], srcStride, (Rv_Vertex_Data_Type_t)srcFormat.m_dataTypes[7], 1,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0100u) && HasFlag(srcFormat.m_flags, 0x0100u)) {
        ComponentCopy(destPtr + m_format.m_byteOffset[8], destStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[8], 1,
            srcPtr + srcFormat.m_byteOffset[8], srcStride, (Rv_Vertex_Data_Type_t)srcFormat.m_dataTypes[8], 1,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0200u) && HasFlag(srcFormat.m_flags, 0x0200u)) {
        ComponentCopy(destPtr + m_format.m_byteOffset[9], destStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[9], 1,
            srcPtr + srcFormat.m_byteOffset[9], srcStride, (Rv_Vertex_Data_Type_t)srcFormat.m_dataTypes[9], 1,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0400u) && HasFlag(srcFormat.m_flags, 0x0400u)) {
        int unit;
        for (unit = 0; unit < 7; ++unit) {
            const int destDimension = GetTexCoordDimension(m_format, unit);
            const int srcDimension = GetTexCoordDimension(srcFormat, unit);
            if (destDimension == 0 || srcDimension == 0) {
                continue;
            }

            ComponentCopy(
                destPtr + m_format.m_byteOffset[kComponentTexCoord0 + unit],
                destStride,
                (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[kComponentTexCoord0 + unit],
                destDimension,
                srcPtr + srcFormat.m_byteOffset[kComponentTexCoord0 + unit],
                srcStride,
               (Rv_Vertex_Data_Type_t) srcFormat.m_dataTypes[kComponentTexCoord0 + unit],
                srcDimension,
                numVertices,
                copyMapping,
                0,
                0);
        }
    }

    Unlock();
}


void rvVertexBuffer::CopyData(
    unsigned char* destPtr,
    int destStride,
    const rvVertexFormat& destFormat,
    int srcVertexOffset,
    int numVertices,
    const unsigned int* copyMapping) {
    const unsigned char* srcPtr = NULL;
    int srcStride = 0;
    void* rawSrcPtr = NULL;

    if (!LockInterleaved(srcVertexOffset, numVertices, kLockRead, rawSrcPtr, srcStride)) {
        idLib::common->Error("Vertex buffer cannot be mapped for access");
        return;
    }

    srcPtr = static_cast<const unsigned char*>(rawSrcPtr);

    if (HasFlag(m_format.m_flags, 0x0001u) && HasFlag(destFormat.m_flags, 0x0001u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[0], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[0], GetPositionDimension(destFormat),
            srcPtr + m_format.m_byteOffset[0], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[0], GetPositionDimension(m_format),
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0004u) && HasFlag(destFormat.m_flags, 0x0004u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[2], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[2], 1,
            srcPtr + m_format.m_byteOffset[2], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[2], 1,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0008u) && HasFlag(destFormat.m_flags, 0x0008u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[3], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[3], GetBlendWeightDimension(destFormat),
            srcPtr + m_format.m_byteOffset[3], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[3], GetBlendWeightDimension(m_format),
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0010u) && HasFlag(destFormat.m_flags, 0x0010u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[4], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[4], 3,
            srcPtr + m_format.m_byteOffset[4], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[4], 3,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0020u) && HasFlag(destFormat.m_flags, 0x0020u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[5], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[5], 3,
            srcPtr + m_format.m_byteOffset[5], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[5], 3,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0040u) && HasFlag(destFormat.m_flags, 0x0040u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[6], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[6], 3,
            srcPtr + m_format.m_byteOffset[6], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[6], 3,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0080u) && HasFlag(destFormat.m_flags, 0x0080u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[7], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[7], 1,
            srcPtr + m_format.m_byteOffset[7], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[7], 1,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0100u) && HasFlag(destFormat.m_flags, 0x0100u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[8], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[8], 1,
            srcPtr + m_format.m_byteOffset[8], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[8], 1,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0200u) && HasFlag(destFormat.m_flags, 0x0200u)) {
        ComponentCopy(destPtr + destFormat.m_byteOffset[9], destStride, (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[9], 1,
            srcPtr + m_format.m_byteOffset[9], srcStride, (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[9], 1,
            numVertices, copyMapping, 0, 0);
    }

    if (HasFlag(m_format.m_flags, 0x0400u) && HasFlag(destFormat.m_flags, 0x0400u)) {
        int unit;
        for (unit = 0; unit < 7; ++unit) {
            const int destDimension = GetTexCoordDimension(destFormat, unit);
            const int srcDimension = GetTexCoordDimension(m_format, unit);
            if (destDimension == 0 || srcDimension == 0) {
                continue;
            }

            ComponentCopy(
                destPtr + destFormat.m_byteOffset[kComponentTexCoord0 + unit],
                destStride,
                (Rv_Vertex_Data_Type_t)destFormat.m_dataTypes[kComponentTexCoord0 + unit],
                destDimension,
                srcPtr + m_format.m_byteOffset[kComponentTexCoord0 + unit],
                srcStride,
                (Rv_Vertex_Data_Type_t)m_format.m_dataTypes[kComponentTexCoord0 + unit],
                srcDimension,
                numVertices,
                copyMapping,
                0,
                0);
        }
    }

    Unlock();
}


void rvVertexBuffer::CopyRemappedData(
    int vertexBufferOffset,
    int numVertices,
    const unsigned int* copyMapping,
    const unsigned int* transformMapOldToNew,
    const rvBlend4DrawVert* srcVertData,
    bool absoluteBlendWeights) {
    if (vertexBufferOffset + numVertices > m_numVertices) {
        idLib::common->Error("rvVertexBuffer: attempt to copy vertex data out-of-bounds");
        return;
    }

    if (HasFlag(m_format.m_flags, 0x0001u) && GetPositionDimension(m_format) >= 3) {
        unsigned char* posPtr = NULL;
        int stride = 0;
        LockPosition(vertexBufferOffset, numVertices, kLockWrite, posPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            float* dst = ByteOffset<float>(posPtr, i * stride);
            dst[0] = src.xyz.x;
            dst[1] = src.xyz.y;
            dst[2] = src.xyz.z;
        }
    }

    if (HasFlag(m_format.m_flags, 0x0004u)) {
        unsigned char* blendIndexPtr = NULL;
        int stride = 0;
        LockBlendIndex(vertexBufferOffset, numVertices, kLockWrite, blendIndexPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            unsigned char* dst = blendIndexPtr + i * stride;
            dst[0] = static_cast<unsigned char>(transformMapOldToNew[src.blendIndex[0]]);
            dst[1] = static_cast<unsigned char>(transformMapOldToNew[src.blendIndex[1]]);
            dst[2] = static_cast<unsigned char>(transformMapOldToNew[src.blendIndex[2]]);
            dst[3] = static_cast<unsigned char>(transformMapOldToNew[src.blendIndex[3]]);
        }
    }

    if (HasFlag(m_format.m_flags, 0x0008u)) {
        unsigned char* blendWeightPtr = NULL;
        int stride = 0;
        const int weightCount = GetBlendWeightDimension(m_format);

        LockBlendWeight(vertexBufferOffset, numVertices, kLockWrite, blendWeightPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            float* dst = ByteOffset<float>(blendWeightPtr, i * stride);
            CopyBlendWeights(dst, weightCount, src, absoluteBlendWeights);
        }
    }

    if (HasFlag(m_format.m_flags, 0x0010u)) {
        unsigned char* normalPtr = NULL;
        int stride = 0;
        LockNormal(vertexBufferOffset, numVertices, kLockWrite, normalPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            float* dst = ByteOffset<float>(normalPtr, i * stride);
            dst[0] = src.normal.x;
            dst[1] = src.normal.y;
            dst[2] = src.normal.z;
        }
    }

    if (HasFlag(m_format.m_flags, 0x0020u)) {
        unsigned char* tangentPtr = NULL;
        int stride = 0;
        LockTangent(vertexBufferOffset, numVertices, kLockWrite, tangentPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            float* dst = ByteOffset<float>(tangentPtr, i * stride);
            dst[0] = src.tangent.x;
            dst[1] = src.tangent.y;
            dst[2] = src.tangent.z;
        }
    }

    if (HasFlag(m_format.m_flags, 0x0040u)) {
        unsigned char* binormalPtr = NULL;
        int stride = 0;
        LockBinormal(vertexBufferOffset, numVertices, kLockWrite, binormalPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            float* dst = ByteOffset<float>(binormalPtr, i * stride);
            dst[0] = src.binormal.x;
            dst[1] = src.binormal.y;
            dst[2] = src.binormal.z;
        }
    }

    if (HasFlag(m_format.m_flags, 0x0080u)) {
        unsigned char* colorPtr = NULL;
        int stride = 0;
        LockDiffuseColor(vertexBufferOffset, numVertices, kLockWrite, colorPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            *ByteOffset<unsigned int>(colorPtr, i * stride) = *reinterpret_cast<const unsigned int*>(src.color);
        }
    }

    if (HasFlag(m_format.m_flags, 0x0400u) && GetTexCoordDimension(m_format, 0) >= 2) {
        unsigned char* texCoordPtr = NULL;
        int stride = 0;
        LockTextureCoordinate(0, vertexBufferOffset, numVertices, kLockWrite, texCoordPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            float* dst = ByteOffset<float>(texCoordPtr, i * stride);
            dst[0] = src.st.x;
            dst[1] = src.st.y;
        }
    }

    if (m_lockStatus != 0) {
        Unlock();
    }
}

void rvVertexBuffer::CopyRemappedShadowVolData(
    int vertexBufferOffset,
    int numVertices,
    const unsigned int* copyMapping,
    const unsigned int* transformMapOldToNew,
    const rvBlend4DrawVert* srcVertData) {
    if (vertexBufferOffset + 2 * numVertices > m_numVertices) {
        idLib::common->Error("rvVertexBuffer: attempt to copy shadow volume vertex data out-of-bounds");
        return;
    }

    const int positionDimension = GetPositionDimension(m_format);

    if (HasFlag(m_format.m_flags, 0x0001u)) {
        unsigned char* posPtr = NULL;
        int stride = 0;
        LockPosition(vertexBufferOffset, numVertices * 2, kLockWrite, posPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];

            float* dst0 = ByteOffset<float>(posPtr, (i * 2 + 0) * stride);
            float* dst1 = ByteOffset<float>(posPtr, (i * 2 + 1) * stride);

            dst0[0] = src.xyz.x;
            dst0[1] = src.xyz.y;
            dst0[2] = src.xyz.z;

            dst1[0] = src.xyz.x;
            dst1[1] = src.xyz.y;
            dst1[2] = src.xyz.z;

            if (positionDimension >= 4) {
                dst0[3] = 1.0f;
                dst1[3] = 0.0f;
            }
        }
    }

    if (HasFlag(m_format.m_flags, 0x0004u)) {
        unsigned char* blendIndexPtr = NULL;
        int stride = 0;
        LockBlendIndex(vertexBufferOffset, numVertices * 2, kLockWrite, blendIndexPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            unsigned char* dst0 = blendIndexPtr + (i * 2 + 0) * stride;
            unsigned char* dst1 = blendIndexPtr + (i * 2 + 1) * stride;

            dst0[0] = static_cast<unsigned char>(transformMapOldToNew[src.blendIndex[0]]);
            dst0[1] = static_cast<unsigned char>(transformMapOldToNew[src.blendIndex[1]]);
            dst0[2] = static_cast<unsigned char>(transformMapOldToNew[src.blendIndex[2]]);

            dst1[0] = dst0[0];
            dst1[1] = dst0[1];
            dst1[2] = dst0[2];

            if (positionDimension == 3) {
                dst0[3] = 0xFFu;
                dst1[3] = 0u;
            }
            else {
                dst0[3] = static_cast<unsigned char>(transformMapOldToNew[src.blendIndex[3]]);
                dst1[3] = dst0[3];
            }
        }
    }

    if (HasFlag(m_format.m_flags, 0x0008u)) {
        unsigned char* blendWeightPtr = NULL;
        int stride = 0;
        const int weightCount = GetBlendWeightDimension(m_format);

        LockBlendWeight(vertexBufferOffset, numVertices * 2, kLockWrite, blendWeightPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const rvBlend4DrawVert& src = srcVertData[copyMapping[i]];
            float* dst0 = ByteOffset<float>(blendWeightPtr, (i * 2 + 0) * stride);
            float* dst1 = ByteOffset<float>(blendWeightPtr, (i * 2 + 1) * stride);

            CopyBlendWeights(dst0, weightCount, src, true);
            CopyBlendWeights(dst1, weightCount, src, true);
        }
    }

    if (m_lockStatus != 0) {
        Unlock();
    }
}

unsigned int rvVertexBuffer::CopySilTraceData(
    unsigned int* indexMapping,
    int vertexBufferOffset,
    int numVertices,
    const int* copyMapping,
    const idDrawVert* srcVertData) {
    if (vertexBufferOffset + numVertices > m_numVertices) {
        idLib::common->Error("rvVertexBuffer: attempt to copy vertex data out-of-bounds");
        return 0;
    }

    const int startingVertexCount = m_numVerticesWritten;
    unsigned int writtenVertices = 0;

    if (HasFlag(m_format.m_flags, 0x0001u) && GetPositionDimension(m_format) >= 3) {
        unsigned char* posPtr = NULL;
        int stride = 0;
        const int positionDimension = GetPositionDimension(m_format);

        LockPosition(vertexBufferOffset, numVertices, kLockWrite, posPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            if (copyMapping[i] != i) {
                continue;
            }

            const idDrawVert& src = srcVertData[i];
            float* dst = ByteOffset<float>(posPtr, static_cast<int>(writtenVertices) * stride);
            dst[0] = src.xyz.x;
            dst[1] = src.xyz.y;
            dst[2] = src.xyz.z;

            if (positionDimension >= 4) {
                dst[3] = 1.0f;
            }

            indexMapping[i] = writtenVertices++;
        }

        for (int i = 0; i < numVertices; ++i) {
            if (copyMapping[i] != i) {
                indexMapping[i] = indexMapping[copyMapping[i]];
            }
        }
    }

    if (m_lockStatus != 0) {
        Unlock();
    }

    m_numVerticesWritten = startingVertexCount + static_cast<int>(writtenVertices);
    return writtenVertices;
}

void rvVertexBuffer::CopyData(int vertexBufferOffset, int numVertices, const idDrawVert* srcVertData) {
    if (vertexBufferOffset + numVertices > m_numVertices) {
        idLib::common->Error("rvVertexBuffer: attempt to copy vertex data out-of-bounds");
        return;
    }

    const int positionDimension = GetPositionDimension(m_format);

    if (HasFlag(m_format.m_flags, 0x0001u) && positionDimension >= 3) {
        unsigned char* posPtr = NULL;
        int stride = 0;
        LockPosition(vertexBufferOffset, numVertices, kLockWrite, posPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const idDrawVert& src = srcVertData[i];
            float* dst = ByteOffset<float>(posPtr, i * stride);
            dst[0] = src.xyz.x;
            dst[1] = src.xyz.y;
            dst[2] = src.xyz.z;
            if (positionDimension >= 4) {
                dst[3] = 1.0f;
            }
        }
    }

    if (HasFlag(m_format.m_flags, 0x0010u)) {
        unsigned char* normalPtr = NULL;
        int stride = 0;
        LockNormal(vertexBufferOffset, numVertices, kLockWrite, normalPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const idDrawVert& src = srcVertData[i];
            float* dst = ByteOffset<float>(normalPtr, i * stride);
            dst[0] = src.normal.x;
            dst[1] = src.normal.y;
            dst[2] = src.normal.z;
        }
    }

    if (HasFlag(m_format.m_flags, 0x0020u)) {
        unsigned char* tangentPtr = NULL;
        int stride = 0;
        LockTangent(vertexBufferOffset, numVertices, kLockWrite, tangentPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const idDrawVert& src = srcVertData[i];
            float* dst = ByteOffset<float>(tangentPtr, i * stride);
            dst[0] = src.tangents[0].x;
            dst[1] = src.tangents[0].y;
            dst[2] = src.tangents[0].z;
        }
    }

    if (HasFlag(m_format.m_flags, 0x0040u)) {
        unsigned char* binormalPtr = NULL;
        int stride = 0;
        LockBinormal(vertexBufferOffset, numVertices, kLockWrite, binormalPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            const idDrawVert& src = srcVertData[i];
            float* dst = ByteOffset<float>(binormalPtr, i * stride);
            dst[0] = src.tangents[1].x;
            dst[1] = src.tangents[1].y;
            dst[2] = src.tangents[1].z;
        }
    }

    if (HasFlag(m_format.m_flags, 0x0080u)) {
        unsigned char* colorPtr = NULL;
        int stride = 0;
        LockDiffuseColor(vertexBufferOffset, numVertices, kLockWrite, colorPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            *ByteOffset<unsigned int>(colorPtr, i * stride) = *reinterpret_cast<const unsigned int*>(srcVertData[i].color);
        }
    }

    if (HasFlag(m_format.m_flags, 0x0400u) && GetTexCoordDimension(m_format, 0) >= 2) {
        unsigned char* texCoordPtr = NULL;
        int stride = 0;
        LockTextureCoordinate(0, vertexBufferOffset, numVertices, kLockWrite, texCoordPtr, stride);

        for (int i = 0; i < numVertices; ++i) {
            float* dst = ByteOffset<float>(texCoordPtr, i * stride);
            dst[0] = srcVertData[i].st.x;
            dst[1] = srcVertData[i].st.y;
        }
    }

    if (m_lockStatus != 0) {
        Unlock();
    }
}

void rvVertexBuffer::CopyRemappedShadowVolData(
    int vertexBufferOffset,
    int numVertices,
    const int* copyMapping,
    const idDrawVert* srcVertData) {
    if (vertexBufferOffset + 2 * numVertices > m_numVertices) {
        idLib::common->Error("rvVertexBuffer: attempt to copy shadow volume vertex data out-of-bounds");
        return;
    }

    const int startingVertexCount = m_numVerticesWritten;
    int currentWritten = startingVertexCount;
    const int positionDimension = GetPositionDimension(m_format);

    if (HasFlag(m_format.m_flags, 0x0001u)) {
        unsigned char* posPtr = NULL;
        int stride = 0;
        LockPosition(vertexBufferOffset, numVertices * 2, kLockWrite, posPtr, stride);

        int dstVertex = 0;
        for (int i = 0; i < numVertices; ++i) {
            if (copyMapping[i] != i) {
                continue;
            }

            const idDrawVert& src = srcVertData[i];
            float* dst0 = ByteOffset<float>(posPtr, (dstVertex + 0) * stride);
            float* dst1 = ByteOffset<float>(posPtr, (dstVertex + 1) * stride);

            dst0[0] = src.xyz.x;
            dst0[1] = src.xyz.y;
            dst0[2] = src.xyz.z;

            dst1[0] = src.xyz.x;
            dst1[1] = src.xyz.y;
            dst1[2] = src.xyz.z;

            if (positionDimension >= 4) {
                dst0[3] = 1.0f;
                dst1[3] = 0.0f;
            }

            dstVertex += 2;
            currentWritten += 2;
        }
    }

    if (m_lockStatus != 0) {
        Unlock();
    }

    m_numVerticesWritten = currentWritten;
}

void rvVertexBuffer::CopyShadowVolData(
    int vertexBufferOffset,
    int numVertices,
    const shadowCache_s* srcShadowVertData) {
    if (vertexBufferOffset + numVertices > m_numVertices) {
        idLib::common->Error("rvVertexBuffer: attempt to copy shadow volume vertex data out-of-bounds");
        return;
    }

    const int positionDimension = GetPositionDimension(m_format);
    if (!HasFlag(m_format.m_flags, 0x0001u)) {
        return;
    }

    unsigned char* posPtr = NULL;
    int stride = 0;
    LockPosition(vertexBufferOffset, numVertices, kLockWrite, posPtr, stride);

    for (int i = 0; i < numVertices; ++i) {
        const shadowCache_s& src = srcShadowVertData[i];
        float* dst = ByteOffset<float>(posPtr, i * stride);

        dst[0] = src.xyz.x;
        dst[1] = src.xyz.y;
        dst[2] = src.xyz.z;

        if (positionDimension >= 4) {
            dst[3] = src.xyz.w;
        }
    }

    if (m_lockStatus != 0) {
        Unlock();
    }
}

void rvVertexBuffer::Init(const rvVertexFormat& vertexFormat, int numVertices, unsigned int flagMask) {
    if (m_format.m_flags != 0) {
        Shutdown();
    }

    m_format.Init(vertexFormat);
    m_loadFormat.Init(vertexFormat);

    m_numVertices = numVertices;
    m_flags = flagMask;
    CreateVertexStorage();
}


void rvVertexBuffer::Write(idFile& outFile, const char* prepend) {
    const char* baseIndent = (prepend != NULL) ? prepend : "";
    char indent[1024];
    char dataIndent[1024];
    int i;

    strcpy(indent, baseIndent);
    strcat(indent, "\t");
    strcpy(dataIndent, indent);
    strcat(dataIndent, "\t");

    outFile.WriteFloatString("%sVertexBuffer\n", baseIndent);
    outFile.WriteFloatString("%s{\n", baseIndent);

    rvVertexFormat* primaryFormat =
        (!HasFlag(m_flags, kBufferFlagUseLoadFmt) || HasFlag(m_flags, kBufferFlagSoA)) ? &m_format : &m_loadFormat;

    outFile.WriteFloatString("%sVertexFormat ", indent);
    primaryFormat->Write(outFile, indent);

    if (!HasFlag(m_flags, kBufferFlagSoA) && FormatLayoutDiffers(*primaryFormat, m_loadFormat)) {
        outFile.WriteFloatString("%sLoadVertexFormat ", indent);
        m_loadFormat.Write(outFile, indent);
    }

    if (HasFlag(m_flags, kBufferFlagSystemMemory)) {
        outFile.WriteFloatString("%sSystemMemory\n", indent);
    }
    if (HasFlag(m_flags, kBufferFlagVideoMemory)) {
        outFile.WriteFloatString("%sVideoMemory\n", indent);
    }
    if (HasFlag(m_flags, kBufferFlagSoA)) {
        outFile.WriteFloatString("%sSoA\n", indent);
    }

    outFile.WriteFloatString("%sVertex[ %d ]\n", indent, m_numVerticesWritten);
    outFile.WriteFloatString("%s{\n", indent);

    if (!HasFlag(m_flags, kBufferFlagNoData)) {
        int tokenSubTypes[72];
        memset(tokenSubTypes, 0, sizeof(tokenSubTypes));

        if (HasFlag(m_flags, kBufferFlagSoA)) {
            if (HasFlag(m_format.m_flags, 0x0002u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentSwizzledPosition), tokenSubTypes);
                WriteNumericStructArray(outFile, 3, tokenSubTypes, AlignUp4(m_numVerticesWritten),
                    reinterpret_cast<const unsigned char*>(m_swizzledPositionArray), dataIndent);
            }
            else if (HasFlag(m_format.m_flags, 0x0001u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentPosition), tokenSubTypes);
                WriteNumericStructArray(outFile, GetPositionDimension(m_format), tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_positionArray), dataIndent);
            }

            if (HasFlag(m_format.m_flags, 0x0004u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentBlendIndex), tokenSubTypes);
                WriteNumericStructArray(outFile, 1, tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_blendIndexArray), dataIndent);
            }
            if (HasFlag(m_format.m_flags, 0x0008u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentBlendWeight), tokenSubTypes);
                WriteNumericStructArray(outFile, GetBlendWeightDimension(m_format), tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_blendWeightArray), dataIndent);
            }
            if (HasFlag(m_format.m_flags, 0x0010u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentNormal), tokenSubTypes);
                WriteNumericStructArray(outFile, 3, tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_normalArray), dataIndent);
            }
            if (HasFlag(m_format.m_flags, 0x0020u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentTangent), tokenSubTypes);
                WriteNumericStructArray(outFile, 3, tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_tangentArray), dataIndent);
            }
            if (HasFlag(m_format.m_flags, 0x0040u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentBinormal), tokenSubTypes);
                WriteNumericStructArray(outFile, 3, tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_binormalArray), dataIndent);
            }
            if (HasFlag(m_format.m_flags, 0x0080u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentDiffuseColor), tokenSubTypes);
                WriteNumericStructArray(outFile, 1, tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_diffuseColorArray), dataIndent);
            }
            if (HasFlag(m_format.m_flags, 0x0100u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentSpecularColor), tokenSubTypes);
                WriteNumericStructArray(outFile, 1, tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_specularColorArray), dataIndent);
            }
            if (HasFlag(m_format.m_flags, 0x0200u)) {
                m_format.GetTokenSubTypes(ToComponent(kComponentPointSize), tokenSubTypes);
                WriteNumericStructArray(outFile, 1, tokenSubTypes, m_numVerticesWritten,
                    reinterpret_cast<const unsigned char*>(m_pointSizeArray), dataIndent);
            }
            if (HasFlag(m_format.m_flags, 0x0400u)) {
                for (i = 0; i < 7; ++i) {
                    const int dimension = GetTexCoordDimension(m_format, i);
                    if (dimension == 0) {
                        continue;
                    }
                    m_format.GetTokenSubTypes(ToComponent(kComponentTexCoord0 + i), tokenSubTypes);
                    WriteNumericStructArray(outFile, dimension, tokenSubTypes, m_numVerticesWritten,
                        reinterpret_cast<const unsigned char*>(m_texCoordArrays[i]), dataIndent);
                }
            }
        }
        else {
            const unsigned char* vertexData = NULL;
            unsigned char* temporaryVertexData = NULL;

            if (HasFlag(m_flags, kBufferFlagSystemMemory)) {
                vertexData = m_interleavedStorage;
            }
            else {
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbID);
                vertexData = static_cast<const unsigned char*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB));
                if (vertexData == NULL) {
                    idLib::common->Error("Vertex buffer cannot be mapped for access");
                    return;
                }
            }

            if (!m_loadFormat.HasSameDataTypes(m_format)) {
                temporaryVertexData = AllocElements<unsigned char>((size_t)m_loadFormat.m_size * (size_t)m_numVerticesWritten);
                if (temporaryVertexData == NULL) {
                    if (!HasFlag(m_flags, kBufferFlagSystemMemory)) {
                        glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
                        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
                    }
                    idLib::common->Error("Ran out of memory trying to allocate temporary vertex storage");
                    return;
                }

                CopyData(temporaryVertexData, m_loadFormat.m_size, m_loadFormat, 0, m_numVerticesWritten, NULL);
                vertexData = temporaryVertexData;
            }

            memcpy(tokenSubTypes, m_loadFormat.m_tokenSubTypes, sizeof(int) * m_loadFormat.m_numValues);
            WriteNumericStructArray(outFile, m_loadFormat.m_numValues, tokenSubTypes, m_numVerticesWritten, vertexData, dataIndent);

            if (temporaryVertexData != NULL) {
                Mem_Free16(temporaryVertexData);
            }
            if (!HasFlag(m_flags, kBufferFlagSystemMemory)) {
                glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
            }
        }
    }

    outFile.WriteFloatString("%s}\n", indent);
    outFile.WriteFloatString("%s}\n", baseIndent);
}


void rvVertexBuffer::Init(Lexer& lex) {
    idToken token;

    if (m_format.m_flags != 0) {
        Shutdown();
    }

    lex.ExpectTokenString("{");
    lex.ExpectTokenString("VertexFormat");
    m_format.Init(lex);
    m_loadFormat.Init(m_format);

    m_flags = 0;

    while (lex.ReadToken(&token)) {
        if (idStr::Icmp(token, "LoadVertexFormat") == 0) {
            m_loadFormat.Init(lex);
        }
        else if (idStr::Icmp(token, "SystemMemory") == 0) {
            m_flags |= kBufferFlagSystemMemory;
        }
        else if (idStr::Icmp(token, "VideoMemory") == 0) {
            m_flags |= kBufferFlagVideoMemory;
        }
        else {
            break;
        }
    }

    m_format.BuildDataTypes(0);

    const bool useLoadVertexFormat =
        FormatLayoutDiffers(m_loadFormat, m_format) ||
        !m_loadFormat.HasSameDataTypes(m_format);

    if (m_flags == 0) {
        m_flags = kBufferFlagVideoMemory;
    }

    if (idStr::Icmp(token, "SoA") == 0) {
        if (useLoadVertexFormat) {
            lex.Error("SoA is currently not supported with LoadVertexFormat");
        }

        m_flags |= kBufferFlagSoA;
        lex.ReadToken(&token);
    }

    if (idStr::Icmp(token, "Vertex") != 0) {
        lex.Error("Expected vertex header");
    }

    lex.ExpectTokenString("[");
    m_numVertices = lex.ParseInt();
    lex.ExpectTokenString("]");

    if (m_numVertices <= 0) {
        lex.Error("Invalid vertex count");
        return;
    }

    CreateVertexStorage();

    lex.ExpectTokenString("{");
    lex.ReadToken(&token);

    if (idStr::Icmp(token, "}") == 0) {
        m_flags |= kBufferFlagNoData;
        lex.ExpectTokenString("}");
        return;
    }

    lex.UnreadToken(&token);

    if (useLoadVertexFormat) {
        unsigned char* temporaryVertexData = AllocElements<unsigned char>((size_t)m_loadFormat.m_size * (size_t)m_numVertices);
        int tokenSubTypes[72];
        memset(tokenSubTypes, 0, sizeof(tokenSubTypes));

        if (temporaryVertexData == NULL) {
            idLib::common->FatalError("Ran out of memory trying to allocate temporary vertex storage");
            return;
        }

        memcpy(tokenSubTypes, m_loadFormat.m_tokenSubTypes, sizeof(int) * m_loadFormat.m_numValues);
        lex.ParseNumericStructArray(m_loadFormat.m_numValues, tokenSubTypes, m_numVertices, temporaryVertexData);
        CopyData(0, m_numVertices, temporaryVertexData, m_loadFormat.m_size, m_loadFormat, NULL);
        Mem_Free16(temporaryVertexData);
    }
    else if (HasFlag(m_flags, kBufferFlagSoA)) {
        int tokenSubTypes[72];
        memset(tokenSubTypes, 0, sizeof(tokenSubTypes));
        int unit;

        if (!HasFlag(m_flags, kBufferFlagSystemMemory)) {
            lex.Error("SoA vertex buffers require system memory");
            Shutdown();
            return;
        }

        m_lockStatus = kLockWrite | kLockDiscard;
        m_lockVertexOffset = 0;
        m_lockVertexCount = m_numVertices;

        if (HasFlag(m_format.m_flags, 0x0002u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentSwizzledPosition), tokenSubTypes);
            lex.ParseNumericStructArray(3, tokenSubTypes, AlignUp4(m_numVertices), reinterpret_cast<unsigned char*>(m_swizzledPositionArray));
        }
        else if (HasFlag(m_format.m_flags, 0x0001u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentPosition), tokenSubTypes);
            lex.ParseNumericStructArray(GetPositionDimension(m_format), tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_positionArray));
        }

        if (HasFlag(m_format.m_flags, 0x0004u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentBlendIndex), tokenSubTypes);
            lex.ParseNumericStructArray(1, tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_blendIndexArray));
        }
        if (HasFlag(m_format.m_flags, 0x0008u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentBlendWeight), tokenSubTypes);
            lex.ParseNumericStructArray(GetBlendWeightDimension(m_format), tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_blendWeightArray));
        }
        if (HasFlag(m_format.m_flags, 0x0010u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentNormal), tokenSubTypes);
            lex.ParseNumericStructArray(3, tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_normalArray));
        }
        if (HasFlag(m_format.m_flags, 0x0020u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentTangent), tokenSubTypes);
            lex.ParseNumericStructArray(3, tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_tangentArray));
        }
        if (HasFlag(m_format.m_flags, 0x0040u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentBinormal), tokenSubTypes);
            lex.ParseNumericStructArray(3, tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_binormalArray));
        }
        if (HasFlag(m_format.m_flags, 0x0080u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentDiffuseColor), tokenSubTypes);
            lex.ParseNumericStructArray(1, tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_diffuseColorArray));
        }
        if (HasFlag(m_format.m_flags, 0x0100u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentSpecularColor), tokenSubTypes);
            lex.ParseNumericStructArray(1, tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_specularColorArray));
        }
        if (HasFlag(m_format.m_flags, 0x0200u)) {
            m_format.GetTokenSubTypes(ToComponent(kComponentPointSize), tokenSubTypes);
            lex.ParseNumericStructArray(1, tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_pointSizeArray));
        }
        if (HasFlag(m_format.m_flags, 0x0400u)) {
            for (unit = 0; unit < 7; ++unit) {
                const int dimension = GetTexCoordDimension(m_format, unit);
                if (dimension == 0) {
                    continue;
                }
                m_format.GetTokenSubTypes(ToComponent(kComponentTexCoord0 + unit), tokenSubTypes);
                lex.ParseNumericStructArray(dimension, tokenSubTypes, m_numVertices, reinterpret_cast<unsigned char*>(m_texCoordArrays[unit]));
            }
        }

        Unlock();
    }
    else {
        void* rawVertexData = NULL;
        int stride = 0;
        int tokenSubTypes[72];
        memset(tokenSubTypes, 0, sizeof(tokenSubTypes));

        if (!LockInterleaved(0, m_numVertices, kLockWrite | kLockDiscard, rawVertexData, stride)) {
            lex.Error("Vertex buffer cannot be mapped for access");
            Shutdown();
            return;
        }

        memcpy(tokenSubTypes, m_format.m_tokenSubTypes, sizeof(int) * m_format.m_numValues);
        lex.ParseNumericStructArray(m_format.m_numValues, tokenSubTypes, m_numVertices, static_cast<unsigned char*>(rawVertexData));
        Unlock();
    }

    lex.ExpectTokenString("}");
    lex.ExpectTokenString("}");
}
