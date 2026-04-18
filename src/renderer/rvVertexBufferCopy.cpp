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

    static const float scaleOne[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    static const float scale32767[4] = { 32767.0f, 32767.0f, 32767.0f, 32767.0f };
    static const float scale65535[4] = { 65535.0f, 65535.0f, 65535.0f, 65535.0f };

    static const float scaleInv32767[4] = {
        1.0f / 32767.0f, 1.0f / 32767.0f, 1.0f / 32767.0f, 1.0f / 32767.0f
    };
    static const float scaleInv65535[4] = {
        1.0f / 65535.0f, 1.0f / 65535.0f, 1.0f / 65535.0f, 1.0f / 65535.0f
    };

    static const unsigned int mask16bit[2] = { 0xFFFFu, 0xFFFFu };
    static const int shift16Bit[2] = { 0, 16 };

    static const unsigned int mask10_10_10[3] = { 0x3FFu, 0x3FFu, 0x3FFu };
    static const int shift10_10_10[3] = { 0, 10, 20 };
    static const int leftShift10_10_10[3] = { 22, 22, 22 };
    static const float scale1023[4] = { 1023.0f, 1023.0f, 1023.0f, 1.0f };
    static const float scale511[4] = { 511.0f, 511.0f, 511.0f, 1.0f };
    static const float scaleInv1023[4] = {
        1.0f / 1023.0f, 1.0f / 1023.0f, 1.0f / 1023.0f, 1.0f
    };
    static const float scaleInv511[4] = {
        1.0f / 511.0f, 1.0f / 511.0f, 1.0f / 511.0f, 1.0f
    };

    static const unsigned int mask10_11_11[3] = { 0x3FFu, 0x7FFu, 0x7FFu };
    static const int shift10_11_11[3] = { 0, 10, 21 };
    static const int leftShift10_11_11[3] = { 22, 21, 21 };
    static const float scale1023_2047_2047[4] = { 1023.0f, 2047.0f, 2047.0f, 1.0f };
    static const float scale511_1023_1023[4] = { 511.0f, 1023.0f, 1023.0f, 1.0f };
    static const float scaleInv1023_2047_2047[4] = {
        1.0f / 1023.0f, 1.0f / 2047.0f, 1.0f / 2047.0f, 1.0f
    };
    static const float scaleInv511_1023_1023[4] = {
        1.0f / 511.0f, 1.0f / 1023.0f, 1.0f / 1023.0f, 1.0f
    };

    static const unsigned int mask11_11_10[3] = { 0x7FFu, 0x7FFu, 0x3FFu };
    static const int shift11_11_10[3] = { 0, 11, 22 };
    static const int leftShift11_11_10[3] = { 21, 21, 22 };
    static const float scale2047_2047_1023[4] = { 2047.0f, 2047.0f, 1023.0f, 1.0f };
    static const float scale1023_1023_511[4] = { 1023.0f, 1023.0f, 511.0f, 1.0f };
    static const float scaleInv2047_2047_1023[4] = {
        1.0f / 2047.0f, 1.0f / 2047.0f, 1.0f / 1023.0f, 1.0f
    };
    static const float scaleInv1023_1023_511[4] = {
        1.0f / 1023.0f, 1.0f / 1023.0f, 1.0f / 511.0f, 1.0f
    };

    static inline const unsigned char* GetSourceComponent(
        const unsigned char*& sequentialSrcPtr,
        int srcStride,
        int componentIndex,
        const unsigned int* copyMapping) {
        if (copyMapping != nullptr) {
            return sequentialSrcPtr + (srcStride * copyMapping[componentIndex]);
        }

        const unsigned char* result = sequentialSrcPtr;
        sequentialSrcPtr += srcStride;
        return result;
    }

    static inline unsigned char* GetDestComponent(unsigned char* base, int stride, int index) {
        return base + static_cast<std::ptrdiff_t>(stride) * index;
    }

    static inline float ReadSignedField(uint32_t packedValue, int leftShift, int rightShift) {
        return static_cast<float>(static_cast<int32_t>(packedValue << leftShift) >> (leftShift + rightShift));
    }

    static void ConvertFloat16PairToFloat(float* destFloats, unsigned int float16Pair) {
        for (int i = 0; i < 2; ++i) {
            const uint16_t half = static_cast<uint16_t>(float16Pair & 0xFFFFu);
            float16Pair >>= 16;

            uint32_t mantissa = half & 0x03FFu;
            int exponent;

            if ((half & 0x7C00u) != 0) {
                exponent = (half >> 10) & 0x1F;
            }
            else if (mantissa != 0) {
                exponent = 1;
                while ((mantissa & 0x0400u) == 0) {
                    mantissa <<= 1;
                    --exponent;
                }
                mantissa &= 0x03FFu;
            }
            else {
                exponent = -112;
            }

            union {
                uint32_t u;
                float f;
            } value;
            value.u = static_cast<uint32_t>((exponent + 112) << 23)
                | ((mantissa | (static_cast<uint32_t>(half & 0x8000u) << 16)) << 13);
            destFloats[i] = value.f;
        }
    }

    static void ConvertFloatToFloat16Pair(unsigned int* float16Pair, const float* srcFloats) {
        *float16Pair = 0u;

        for (int i = 0; i < 2; ++i) {
            union {
                float f;
                uint32_t u;
            } value;
            value.f = srcFloats[i];

            const uint32_t absValue = value.u & 0x7FFFFFFFu;
            const uint16_t sign = static_cast<uint16_t>((value.u >> 16) & 0x8000u);

            uint16_t half;
            if (absValue <= 0x47FFEFFFu) {
                uint32_t temp;
                if (absValue >= 0x38800000u) {
                    temp = absValue - 0x38000000u;
                }
                else {
                    temp = ((absValue & 0x7FFFFFu) | 0x800000u) >> (113 - (absValue >> 23));
                }
                half = static_cast<uint16_t>(sign | ((((temp >> 13) & 1u) + temp + 4095u) >> 13));
            }
            else {
                half = static_cast<uint16_t>(sign | 0x7FFFu);
            }

            if (i == 0) {
                *float16Pair = static_cast<unsigned int>(half) << 16;
            }
            else {
                *float16Pair |= half;
            }
        }
    }

    static void rvVtxCopyFloatToFloat(
        const unsigned char* srcPtr,
        int destStride,
        float* destPtr,
        int destNumValuesPerComponent,
        int srcStride,
        int srcNumValuesPerComponent,
        int numComponents,
        const unsigned int* copyMapping,
        const float* srcTailPtr,
        bool absFlag) {
        const int copyCount = (destNumValuesPerComponent < srcNumValuesPerComponent)
            ? destNumValuesPerComponent
            : srcNumValuesPerComponent;

        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const float* src = reinterpret_cast<const float*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));

            float* dst = reinterpret_cast<float*>(
                reinterpret_cast<unsigned char*>(destPtr) + static_cast<std::ptrdiff_t>(destStride) * component);

            for (int value = 0; value < copyCount; ++value) {
                dst[value] = absFlag ? fabs(src[value]) : src[value];
            }

            if (destNumValuesPerComponent > srcNumValuesPerComponent) {
                const int tailCount = destNumValuesPerComponent - srcNumValuesPerComponent;
                if (srcTailPtr != nullptr) {
                    for (int value = 0; value < tailCount; ++value) {
                        dst[srcNumValuesPerComponent + value] = srcTailPtr[srcNumValuesPerComponent + value];
                    }
                }
                else if (tailCount == 1) {
                    switch (srcNumValuesPerComponent) {
                    case 1: dst[1] = 1.0f - dst[0]; break;
                    case 2: dst[2] = 1.0f - (dst[0] + dst[1]); break;
                    case 3: dst[3] = 1.0f - (dst[0] + dst[1] + dst[2]); break;
                    default: break;
                    }
                }
            }
        }
    }

    static void rvVtxCopyFloatToInt(
        int destNumValuesPerComponent,
        int srcNumValuesPerComponent,
        int numComponents,
        unsigned char* destPtr,
        int destStride,
        const unsigned char* srcPtr,
        int srcStride,
        const unsigned int* copyMapping) {
        const int copyCount = (destNumValuesPerComponent < srcNumValuesPerComponent)
            ? destNumValuesPerComponent
            : srcNumValuesPerComponent;
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const float* src = reinterpret_cast<const float*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            int* dst = reinterpret_cast<int*>(GetDestComponent(destPtr, destStride, component));

            for (int value = 0; value < copyCount; ++value) {
                dst[value] = static_cast<int>(src[value]);
            }
        }
    }

    static void rvVtxCopyIntToFloat(
        int destStride,
        const unsigned char* srcPtr,
        const float* srcTailPtr,
        float* destPtr,
        int destNumValuesPerComponent,
        int srcStride,
        int srcNumValuesPerComponent,
        int numComponents,
        const unsigned int* copyMapping) {
        const int copyCount = (destNumValuesPerComponent < srcNumValuesPerComponent)
            ? destNumValuesPerComponent
            : srcNumValuesPerComponent;
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const int* src = reinterpret_cast<const int*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            float* dst = reinterpret_cast<float*>(
                reinterpret_cast<unsigned char*>(destPtr) + static_cast<std::ptrdiff_t>(destStride) * component);

            for (int value = 0; value < copyCount; ++value) {
                dst[value] = static_cast<float>(src[value]);
            }

            if (destNumValuesPerComponent > srcNumValuesPerComponent && srcTailPtr != nullptr) {
                for (int value = srcNumValuesPerComponent; value < destNumValuesPerComponent; ++value) {
                    dst[value] = srcTailPtr[value];
                }
            }
        }
    }

    static void rvVtxCopyIntToInt(
        const unsigned char* srcPtr,
        int srcStride,
        int srcNumValuesPerComponent,
        int numComponents,
        unsigned char* destPtr,
        int destStride,
        int destNumValuesPerComponent,
        const unsigned int* copyMapping) {
        const int copyCount = (destNumValuesPerComponent < srcNumValuesPerComponent)
            ? destNumValuesPerComponent
            : srcNumValuesPerComponent;
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const uint32_t* src = reinterpret_cast<const uint32_t*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            uint32_t* dst = reinterpret_cast<uint32_t*>(GetDestComponent(destPtr, destStride, component));

            for (int value = 0; value < copyCount; ++value) {
                dst[value] = src[value];
            }
        }
    }

    static void rvVtxCopyDWordToDWord(
        unsigned char* destPtr,
        const unsigned char* srcPtr,
        int srcStride,
        int numComponents,
        int destStride,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const uint32_t* src = reinterpret_cast<const uint32_t*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            uint32_t* dst = reinterpret_cast<uint32_t*>(GetDestComponent(destPtr, destStride, component));
            *dst = *src;
        }
    }

    static void rvVtxCopyFloatToDec16Pair(
        unsigned char* destPtr,
        const int* destShifts,
        int destStride,
        const unsigned char* srcPtr,
        int srcStride,
        const float* destScales,
        const unsigned int* destMasks,
        int numComponents,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const float* src = reinterpret_cast<const float*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));

            uint32_t packed = 0;
            packed |= (destMasks[0] & static_cast<uint32_t>(static_cast<int>(src[0] * destScales[0]))) << destShifts[0];
            packed |= (destMasks[1] & static_cast<uint32_t>(static_cast<int>(src[1] * destScales[1]))) << destShifts[1];

            *reinterpret_cast<uint32_t*>(GetDestComponent(destPtr, destStride, component)) = packed;
        }
    }

    static void rvVtxCopyDec16PairToFloat(
        float* destPtr,
        const float* srcScales,
        const unsigned int* srcMasks,
        const int* srcShifts,
        int destStride,
        const unsigned char* srcPtr,
        int srcStride,
        int numComponents,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const uint32_t packed = *reinterpret_cast<const uint32_t*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            float* dst = reinterpret_cast<float*>(
                reinterpret_cast<unsigned char*>(destPtr) + static_cast<std::ptrdiff_t>(destStride) * component);

            dst[0] = static_cast<float>((packed >> srcShifts[0]) & srcMasks[0]) * srcScales[0];
            dst[1] = static_cast<float>((packed >> srcShifts[1]) & srcMasks[1]) * srcScales[1];
        }
    }

    static void rvVtxCopySignedDec16PairToFloat(
        float* destPtr,
        const float* srcScales,
        const unsigned int* srcMasks,
        const int* srcShifts,
        int destStride,
        const unsigned char* srcPtr,
        int srcStride,
        int numComponents,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const uint32_t packed = *reinterpret_cast<const uint32_t*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            float* dst = reinterpret_cast<float*>(
                reinterpret_cast<unsigned char*>(destPtr) + static_cast<std::ptrdiff_t>(destStride) * component);

            dst[0] = static_cast<float>(static_cast<int16_t>((packed >> srcShifts[0]) & srcMasks[0])) * srcScales[0];
            dst[1] = static_cast<float>(static_cast<int16_t>((packed >> srcShifts[1]) & srcMasks[1])) * srcScales[1];
        }
    }

    static void rvVtxCopyFloatToFloat16Pair(
        unsigned char* destPtr,
        const unsigned char* srcPtr,
        int numComponents,
        int destStride,
        int srcStride,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const float* src = reinterpret_cast<const float*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            ConvertFloatToFloat16Pair(reinterpret_cast<unsigned int*>(GetDestComponent(destPtr, destStride, component)), src);
        }
    }

    static void rvVtxCopyFloat16PairToFloat(
        float* destPtr,
        const unsigned char* srcPtr,
        int destStride,
        int srcStride,
        int numComponents,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const uint32_t packed = *reinterpret_cast<const uint32_t*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            float* dst = reinterpret_cast<float*>(
                reinterpret_cast<unsigned char*>(destPtr) + static_cast<std::ptrdiff_t>(destStride) * component);
            ConvertFloat16PairToFloat(dst, packed);
        }
    }

    static void rvVtxCopyFloatToDecTriple(
        const unsigned int* destMasks,
        const int* destShifts,
        unsigned char* destPtr,
        int destStride,
        const unsigned char* srcPtr,
        int srcStride,
        const float* destScales,
        int numComponents,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const float* src = reinterpret_cast<const float*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));

            uint32_t packed = 0;
            packed |= (destMasks[0] & static_cast<uint32_t>(static_cast<int>(src[0] * destScales[0]))) << destShifts[0];
            packed |= (destMasks[1] & static_cast<uint32_t>(static_cast<int>(src[1] * destScales[1]))) << destShifts[1];
            packed |= (destMasks[2] & static_cast<uint32_t>(static_cast<int>(src[2] * destScales[2]))) << destShifts[2];

            *reinterpret_cast<uint32_t*>(GetDestComponent(destPtr, destStride, component)) = packed;
        }
    }

    static void rvVtxCopyDecTripleToFloat(
        float* destPtr,
        const float* srcScales,
        const unsigned int* srcMasks,
        const int* srcShifts,
        int destStride,
        const unsigned char* srcPtr,
        int srcStride,
        int numComponents,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const uint32_t packed = *reinterpret_cast<const uint32_t*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            float* dst = reinterpret_cast<float*>(
                reinterpret_cast<unsigned char*>(destPtr) + static_cast<std::ptrdiff_t>(destStride) * component);

            dst[0] = static_cast<float>((packed >> srcShifts[0]) & srcMasks[0]) * srcScales[0];
            dst[1] = static_cast<float>((packed >> srcShifts[1]) & srcMasks[1]) * srcScales[1];
            dst[2] = static_cast<float>((packed >> srcShifts[2]) & srcMasks[2]) * srcScales[2];
        }
    }

    static void rvVtxCopySignedDecTripleToFloat(
        float* destPtr,
        const float* srcScales,
        const int* srcLeftShifts,
        const int* srcShifts,
        int destStride,
        const unsigned char* srcPtr,
        int srcStride,
        int numComponents,
        const unsigned int* copyMapping) {
        const unsigned char* sequentialSrcPtr = srcPtr;

        for (int component = 0; component < numComponents; ++component) {
            const uint32_t packed = *reinterpret_cast<const uint32_t*>(
                GetSourceComponent(sequentialSrcPtr, srcStride, component, copyMapping));
            float* dst = reinterpret_cast<float*>(
                reinterpret_cast<unsigned char*>(destPtr) + static_cast<std::ptrdiff_t>(destStride) * component);

            dst[0] = ReadSignedField(packed, srcLeftShifts[0], srcShifts[0]) * srcScales[0];
            dst[1] = ReadSignedField(packed, srcLeftShifts[1], srcShifts[1]) * srcScales[1];
            dst[2] = ReadSignedField(packed, srcLeftShifts[2], srcShifts[2]) * srcScales[2];
        }
    }

} // namespace

void rvVertexBuffer::ComponentCopy(
    unsigned char* destPtr,
    int destStride,
    Rv_Vertex_Data_Type_t destDataType,
    int destNumValuesPerComponent,
    const unsigned char* srcPtr,
    int srcStride,
    Rv_Vertex_Data_Type_t srcDataType,
    int srcNumValuesPerComponent,
    int numComponents,
    const unsigned int* copyMapping,
    const float* srcTailPtr,
    bool absFlag) {
    switch (destDataType) {
    case RV_VERTEX_DATA_TYPE_FLOAT:
        switch (srcDataType) {
        case RV_VERTEX_DATA_TYPE_FLOAT:
            rvVtxCopyFloatToFloat(
                srcPtr, destStride, reinterpret_cast<float*>(destPtr),
                destNumValuesPerComponent, srcStride, srcNumValuesPerComponent,
                numComponents, copyMapping, srcTailPtr, absFlag);
            return;

        case RV_VERTEX_DATA_TYPE_FLOAT16:
            rvVtxCopyFloat16PairToFloat(
                reinterpret_cast<float*>(destPtr), srcPtr,
                destStride, srcStride, numComponents, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyFloat16PairToFloat(
                    reinterpret_cast<float*>(destPtr) + 2, srcPtr + 4,
                    destStride, srcStride, numComponents, copyMapping);
            }
            return;

        case RV_VERTEX_DATA_TYPE_INT:
            rvVtxCopyIntToFloat(
                destStride, srcPtr, srcTailPtr, reinterpret_cast<float*>(destPtr),
                destNumValuesPerComponent, srcStride, srcNumValuesPerComponent,
                numComponents, copyMapping);
            return;

        case RV_VERTEX_DATA_TYPE_SHORT:
            rvVtxCopySignedDec16PairToFloat(
                reinterpret_cast<float*>(destPtr), scaleOne, mask16bit, shift16Bit,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopySignedDec16PairToFloat(
                    reinterpret_cast<float*>(destPtr) + 2, scaleOne, mask16bit, shift16Bit,
                    destStride, srcPtr + 4, srcStride, numComponents, copyMapping);
            }
            return;

        case RV_VERTEX_DATA_TYPE_USHORT:
            rvVtxCopyDec16PairToFloat(
                reinterpret_cast<float*>(destPtr), scaleOne, mask16bit, shift16Bit,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyDec16PairToFloat(
                    reinterpret_cast<float*>(destPtr) + 2, scaleOne, mask16bit, shift16Bit,
                    destStride, srcPtr + 4, srcStride, numComponents, copyMapping);
            }
            return;

        case RV_VERTEX_DATA_TYPE_SHORTN:
            rvVtxCopySignedDec16PairToFloat(
                reinterpret_cast<float*>(destPtr), scaleInv32767, mask16bit, shift16Bit,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopySignedDec16PairToFloat(
                    reinterpret_cast<float*>(destPtr) + 2, scaleInv32767, mask16bit, shift16Bit,
                    destStride, srcPtr + 4, srcStride, numComponents, copyMapping);
            }
            return;

        case RV_VERTEX_DATA_TYPE_USHORTN:
            rvVtxCopyDec16PairToFloat(
                reinterpret_cast<float*>(destPtr), scaleInv65535, mask16bit, shift16Bit,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyDec16PairToFloat(
                    reinterpret_cast<float*>(destPtr) + 2, scaleInv65535, mask16bit, shift16Bit,
                    destStride, srcPtr + 4, srcStride, numComponents, copyMapping);
            }
            return;

        case RV_VERTEX_DATA_TYPE_UDEC_10_10_10:
            rvVtxCopyDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleOne, mask10_10_10, shift10_10_10,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_DEC_10_10_10:
            rvVtxCopySignedDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleOne, leftShift10_10_10, shift10_10_10,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_UDEC_10_10_10N:
            rvVtxCopyDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleInv1023, mask10_10_10, shift10_10_10,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_DEC_10_10_10N:
            rvVtxCopySignedDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleInv511, leftShift10_10_10, shift10_10_10,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;

        case RV_VERTEX_DATA_TYPE_UDEC_10_11_11:
            rvVtxCopyDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleOne, mask10_11_11, shift10_11_11,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_DEC_10_11_11:
            rvVtxCopySignedDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleOne, leftShift10_11_11, shift10_11_11,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_UDEC_10_11_11N:
            rvVtxCopyDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleInv1023_2047_2047, mask10_11_11, shift10_11_11,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_DEC_10_11_11N:
            rvVtxCopySignedDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleInv511_1023_1023, leftShift10_11_11, shift10_11_11,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;

        case RV_VERTEX_DATA_TYPE_UDEC_11_11_10:
            rvVtxCopyDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleOne, mask11_11_10, shift11_11_10,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_DEC_11_11_10:
            rvVtxCopySignedDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleOne, leftShift11_11_10, shift11_11_10,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_UDEC_11_11_10N:
            rvVtxCopyDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleInv2047_2047_1023, mask11_11_10, shift11_11_10,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;
        case RV_VERTEX_DATA_TYPE_DEC_11_11_10N:
            rvVtxCopySignedDecTripleToFloat(reinterpret_cast<float*>(destPtr), scaleInv1023_1023_511, leftShift11_11_10, shift11_11_10,
                destStride, srcPtr, srcStride, numComponents, copyMapping);
            return;

        default:
            return;
        }

    case RV_VERTEX_DATA_TYPE_FLOAT16:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToFloat16Pair(destPtr, srcPtr, numComponents, destStride, srcStride, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyFloatToFloat16Pair(destPtr + 4, srcPtr + 8, numComponents, destStride, srcStride, copyMapping);
            }
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyDWordToDWord(destPtr + 4, srcPtr + 4, srcStride, numComponents, destStride, copyMapping);
            }
        }
        return;

    case RV_VERTEX_DATA_TYPE_INT:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToInt(destNumValuesPerComponent, srcNumValuesPerComponent, numComponents,
                destPtr, destStride, srcPtr, srcStride, copyMapping);
        }
        else {
            rvVtxCopyIntToInt(srcPtr, srcStride, srcNumValuesPerComponent, numComponents,
                destPtr, destStride, destNumValuesPerComponent, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_SHORT:
    case RV_VERTEX_DATA_TYPE_USHORT:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDec16Pair(destPtr, shift16Bit, destStride, srcPtr, srcStride, scaleOne, mask16bit,
                numComponents, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyFloatToDec16Pair(destPtr + 4, shift16Bit, destStride, srcPtr + 8, srcStride, scaleOne, mask16bit,
                    numComponents, copyMapping);
            }
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyDWordToDWord(destPtr + 4, srcPtr + 4, srcStride, numComponents, destStride, copyMapping);
            }
        }
        return;

    case RV_VERTEX_DATA_TYPE_SHORTN:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDec16Pair(destPtr, shift16Bit, destStride, srcPtr, srcStride, scale32767, mask16bit,
                numComponents, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyFloatToDec16Pair(destPtr + 4, shift16Bit, destStride, srcPtr + 8, srcStride, scale32767, mask16bit,
                    numComponents, copyMapping);
            }
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyDWordToDWord(destPtr + 4, srcPtr + 4, srcStride, numComponents, destStride, copyMapping);
            }
        }
        return;

    case RV_VERTEX_DATA_TYPE_USHORTN:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDec16Pair(destPtr, shift16Bit, destStride, srcPtr, srcStride, scale65535, mask16bit,
                numComponents, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyFloatToDec16Pair(destPtr + 4, shift16Bit, destStride, srcPtr + 8, srcStride, scale65535, mask16bit,
                    numComponents, copyMapping);
            }
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
            if (srcNumValuesPerComponent == 4) {
                rvVtxCopyDWordToDWord(destPtr + 4, srcPtr + 4, srcStride, numComponents, destStride, copyMapping);
            }
        }
        return;

    case RV_VERTEX_DATA_TYPE_UDEC_10_10_10:
    case RV_VERTEX_DATA_TYPE_DEC_10_10_10:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask10_10_10, shift10_10_10, destPtr, destStride, srcPtr, srcStride, scaleOne,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_UDEC_10_10_10N:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask10_10_10, shift10_10_10, destPtr, destStride, srcPtr, srcStride, scale1023,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_DEC_10_10_10N:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask10_10_10, shift10_10_10, destPtr, destStride, srcPtr, srcStride, scale511,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_UDEC_10_11_11:
    case RV_VERTEX_DATA_TYPE_DEC_10_11_11:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask10_11_11, shift10_11_11, destPtr, destStride, srcPtr, srcStride, scaleOne,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_UDEC_10_11_11N:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask10_11_11, shift10_11_11, destPtr, destStride, srcPtr, srcStride, scale1023_2047_2047,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_DEC_10_11_11N:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask10_11_11, shift10_11_11, destPtr, destStride, srcPtr, srcStride, scale511_1023_1023,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_UDEC_11_11_10:
    case RV_VERTEX_DATA_TYPE_DEC_11_11_10:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask11_11_10, shift11_11_10, destPtr, destStride, srcPtr, srcStride, scaleOne,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_UDEC_11_11_10N:
        if (srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask11_11_10, shift11_11_10, destPtr, destStride, srcPtr, srcStride, scale2047_2047_1023,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    case RV_VERTEX_DATA_TYPE_DEC_11_11_10N:
    case RV_VERTEX_DATA_TYPE_COLOR:
    case RV_VERTEX_DATA_TYPE_UBYTE:
    case RV_VERTEX_DATA_TYPE_BYTE:
    case RV_VERTEX_DATA_TYPE_UBYTEN:
    case RV_VERTEX_DATA_TYPE_BYTEN:
        if (destDataType == RV_VERTEX_DATA_TYPE_DEC_11_11_10N && srcDataType == RV_VERTEX_DATA_TYPE_FLOAT) {
            rvVtxCopyFloatToDecTriple(mask11_11_10, shift11_11_10, destPtr, destStride, srcPtr, srcStride, scale1023_1023_511,
                numComponents, copyMapping);
        }
        else {
            rvVtxCopyDWordToDWord(destPtr, srcPtr, srcStride, numComponents, destStride, copyMapping);
        }
        return;

    default:
        return;
    }
}
