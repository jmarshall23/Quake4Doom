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

const vertexStorageDesc_t vertexStorageDescArray[8192];
const vertexFormatDesc_t formatDescs[8192];
const char* outputDataTypeStrings[8192];

rvVertexFormat::rvVertexFormat() {
	m_flags = 0;
	m_dimensions = 0;
	m_size = 0;
	m_numValues = 0;
	memset(m_byteOffset, 0, sizeof(m_byteOffset));
	memset(m_dataTypes, 0, sizeof(m_dataTypes));
	memset(m_tokenSubTypes, 0, sizeof(m_tokenSubTypes));
	m_glVASMask = 0;
}

rvVertexFormat::rvVertexFormat(
	unsigned int vtxFmtFlags,
	int posDim,
	int numWeights,
	int numTransforms,
	const int* texDimArray) {
	m_flags = 0;
	m_dimensions = 0;
	m_size = 0;
	m_numValues = 0;
	memset(m_byteOffset, 0, sizeof(m_byteOffset));
	memset(m_dataTypes, 0, sizeof(m_dataTypes));
	memset(m_tokenSubTypes, 0, sizeof(m_tokenSubTypes));
	m_glVASMask = 0;

	Init(vtxFmtFlags, posDim, numWeights, numTransforms, texDimArray, nullptr);
}

void rvVertexFormat::Shutdown() {
	m_flags = 0;
	m_dimensions = 0;
	m_size = 0;
	m_numValues = 0;
	memset(m_byteOffset, 0, sizeof(m_byteOffset));
	memset(m_dataTypes, 0, sizeof(m_dataTypes));
	memset(m_tokenSubTypes, 0, sizeof(m_tokenSubTypes));
	m_glVASMask = 0;
}

void rvVertexFormat::CalcSize() {
	m_size = 0;
	m_numValues = 0;
	m_glVASMask = 0;

	m_byteOffset[0] = 0;
	m_byteOffset[1] = 0;

	// Position / swizzled position
	if (m_flags & 0x2) {
		m_size = 12;
	}
	else if (m_flags & 0x1) {
		unsigned int remaining = m_dimensions & 0x7;
		const vertexStorageDesc_t& desc = vertexStorageDescArray[m_dataTypes[0]];

		while (remaining > 0) {
			m_tokenSubTypes[m_numValues++] = static_cast<unsigned char>(desc.m_tokenSubType);
			m_size += 4;
			if (remaining <= static_cast<unsigned int>(desc.m_numComponents)) {
				break;
			}
			remaining -= desc.m_numComponents;
		}

		m_glVASMask |= 0x00000001u;
	}

	// Blend index
	m_byteOffset[2] = m_size;
	if (m_flags & 0x4) {
		m_tokenSubTypes[m_numValues++] =
			static_cast<unsigned char>(vertexStorageDescArray[m_dataTypes[2]].m_tokenSubType);
		m_size += 4;
		m_glVASMask |= 0x00020000u;
	}

	// Blend weight
	m_byteOffset[3] = m_size;
	if (m_flags & 0x8) {
		unsigned int remaining = (m_dimensions >> 3) & 0x7;
		const vertexStorageDesc_t& desc = vertexStorageDescArray[m_dataTypes[3]];

		while (remaining > 0) {
			m_tokenSubTypes[m_numValues++] = static_cast<unsigned char>(desc.m_tokenSubType);
			m_size += 4;
			if (remaining <= static_cast<unsigned int>(desc.m_numComponents)) {
				break;
			}
			remaining -= desc.m_numComponents;
		}

		m_glVASMask |= 0x00200000u;
	}

	// Normal
	m_byteOffset[4] = m_size;
	if (m_flags & 0x10) {
		const vertexStorageDesc_t& desc = vertexStorageDescArray[m_dataTypes[4]];
		int remaining = 3;

		while (remaining > 0) {
			m_tokenSubTypes[m_numValues++] = static_cast<unsigned char>(desc.m_tokenSubType);
			m_size += 4;
			if (remaining <= desc.m_numComponents) {
				break;
			}
			remaining -= desc.m_numComponents;
		}

		m_glVASMask |= 0x00000002u;
	}

	// Tangent
	m_byteOffset[5] = m_size;
	if (m_flags & 0x20) {
		const vertexStorageDesc_t& desc = vertexStorageDescArray[m_dataTypes[5]];
		int remaining = 3;

		while (remaining > 0) {
			m_tokenSubTypes[m_numValues++] = static_cast<unsigned char>(desc.m_tokenSubType);
			m_size += 4;
			if (remaining <= desc.m_numComponents) {
				break;
			}
			remaining -= desc.m_numComponents;
		}

		m_glVASMask |= 0x00400000u;
	}

	// Binormal
	m_byteOffset[6] = m_size;
	if (m_flags & 0x40) {
		const vertexStorageDesc_t& desc = vertexStorageDescArray[m_dataTypes[6]];
		int remaining = 3;

		while (remaining > 0) {
			m_tokenSubTypes[m_numValues++] = static_cast<unsigned char>(desc.m_tokenSubType);
			m_size += 4;
			if (remaining <= desc.m_numComponents) {
				break;
			}
			remaining -= desc.m_numComponents;
		}

		m_glVASMask |= 0x00800000u;
	}

	// Diffuse color
	m_byteOffset[7] = m_size;
	if (m_flags & 0x80) {
		m_tokenSubTypes[m_numValues++] =
			static_cast<unsigned char>(vertexStorageDescArray[m_dataTypes[7]].m_tokenSubType);
		m_size += 4;
		m_glVASMask |= 0x00000004u;
	}

	// Specular color
	m_byteOffset[8] = m_size;
	if (m_flags & 0x100) {
		m_tokenSubTypes[m_numValues++] =
			static_cast<unsigned char>(vertexStorageDescArray[m_dataTypes[8]].m_tokenSubType);
		m_size += 4;
		m_glVASMask |= 0x00100000u;
	}

	// Point size
	m_byteOffset[9] = m_size;
	if (m_flags & 0x200) {
		m_tokenSubTypes[m_numValues++] =
			static_cast<unsigned char>(vertexStorageDescArray[m_dataTypes[9]].m_tokenSubType);
		m_size += 4;
		m_glVASMask |= 0x80000000u;
	}

	// Texcoords 0..6
	if (m_flags & 0x400) {
		for (int tc = 0; tc < 7; ++tc) {
			const int componentIndex = 10 + tc;
			const int dimShift = 9 + tc * 3;
			const unsigned int dim = (m_dimensions >> dimShift) & 0x7;

			m_byteOffset[componentIndex] = m_size;

			if (dim == 0) {
				continue;
			}

			const vertexStorageDesc_t& desc = vertexStorageDescArray[m_dataTypes[componentIndex]];
			unsigned int remaining = dim;

			while (remaining > 0) {
				m_tokenSubTypes[m_numValues++] = static_cast<unsigned char>(desc.m_tokenSubType);
				m_size += 4;
				if (remaining <= static_cast<unsigned int>(desc.m_numComponents)) {
					break;
				}
				remaining -= desc.m_numComponents;
			}

			m_glVASMask |= (0x01000000u << tc);
		}
	}
}

void rvVertexFormat::BuildDataTypes(Rv_Vertex_Data_Type_t* dataTypes) {
	if ((m_flags & 0x2) || (m_flags & 0x1)) {
		m_dataTypes[0] = (dataTypes && dataTypes[0]) ? static_cast<unsigned char>(dataTypes[0]) : RV_VERTEX_DATA_TYPE_FLOAT;
	}

	if (m_flags & 0x4) {
		m_dataTypes[2] = (dataTypes && dataTypes[2]) ? static_cast<unsigned char>(dataTypes[2]) : RV_VERTEX_DATA_TYPE_UBYTEN;
	}

	if (m_flags & 0x8) {
		m_dataTypes[3] = (dataTypes && dataTypes[3]) ? static_cast<unsigned char>(dataTypes[3]) : RV_VERTEX_DATA_TYPE_FLOAT;
	}

	if (m_flags & 0x10) {
		m_dataTypes[4] = (dataTypes && dataTypes[4]) ? static_cast<unsigned char>(dataTypes[4]) : RV_VERTEX_DATA_TYPE_FLOAT;
	}

	if (m_flags & 0x20) {
		m_dataTypes[5] = (dataTypes && dataTypes[5]) ? static_cast<unsigned char>(dataTypes[5]) : RV_VERTEX_DATA_TYPE_FLOAT;
	}

	if (m_flags & 0x40) {
		m_dataTypes[6] = (dataTypes && dataTypes[6]) ? static_cast<unsigned char>(dataTypes[6]) : RV_VERTEX_DATA_TYPE_FLOAT;
	}

	if (m_flags & 0x80) {
		m_dataTypes[7] = (dataTypes && dataTypes[7]) ? static_cast<unsigned char>(dataTypes[7]) : RV_VERTEX_DATA_TYPE_COLOR;
	}

	if (m_flags & 0x100) {
		m_dataTypes[8] = (dataTypes && dataTypes[8]) ? static_cast<unsigned char>(dataTypes[8]) : RV_VERTEX_DATA_TYPE_COLOR;
	}

	if (m_flags & 0x400) {
		for (int i = 0; i < 7; ++i) {
			const int idx = 10 + i;
			m_dataTypes[idx] = (dataTypes && dataTypes[idx]) ? static_cast<unsigned char>(dataTypes[idx]) : RV_VERTEX_DATA_TYPE_FLOAT;
		}
	}

	CalcSize();
}

void rvVertexFormat::GetTokenSubTypes(
	Rv_Vertex_Component_t vertexComponent,
	int* tokenSubTypes) const {
	const int tokenSubType = vertexStorageDescArray[m_dataTypes[vertexComponent]].m_tokenSubType;
	tokenSubTypes[0] = tokenSubType;
	tokenSubTypes[1] = tokenSubType;
	tokenSubTypes[2] = tokenSubType;
	tokenSubTypes[3] = tokenSubType;
}

void rvVertexFormat::SetVertexDeclaration(int vertexStartOffset) const {
	const int baseOffset = vertexStartOffset * m_size;

	if (m_flags & 0x4) {
		const int type = m_dataTypes[2];
		const vertexFormatDesc_t& desc = formatDescs[type];
		const int count =
			((desc.m_countMask + desc.m_countAdd) & 4) +
			((~desc.m_countMask) & desc.m_compressedCount);

		qglVertexAttribPointerARB(
			1,
			count,
			desc.m_glStorage,
			desc.m_normalized,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[2]));
	}

	if (m_flags & 0x8) {
		const int type = m_dataTypes[3];
		const vertexFormatDesc_t& desc = formatDescs[type];
		const unsigned int dim = (m_dimensions >> 3) & 0x7;
		const int count =
			((~desc.m_countMask) & desc.m_compressedCount) +
			(dim & (desc.m_countMask + desc.m_countAdd));

		qglVertexAttribPointerARB(
			5,
			count,
			desc.m_glStorage,
			desc.m_normalized,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[3]));
	}

	if (m_flags & 0x10) {
		qglNormalPointer(
			formatDescs[m_dataTypes[4]].m_glStorage,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[4]));
	}

	if (m_flags & 0x20) {
		const int type = m_dataTypes[5];
		const vertexFormatDesc_t& desc = formatDescs[type];
		const int count =
			((desc.m_countMask + desc.m_countAdd) & 3) +
			((~desc.m_countMask) & desc.m_compressedCount);

		qglVertexAttribPointerARB(
			6,
			count,
			desc.m_glStorage,
			desc.m_normalized,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[5]));
	}

	if (m_flags & 0x40) {
		const int type = m_dataTypes[6];
		const vertexFormatDesc_t& desc = formatDescs[type];
		const int count =
			((desc.m_countMask + desc.m_countAdd) & 3) +
			((~desc.m_countMask) & desc.m_compressedCount);

		qglVertexAttribPointerARB(
			7,
			count,
			desc.m_glStorage,
			desc.m_normalized,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[6]));
	}

	if (m_flags & 0x80) {
		const int type = m_dataTypes[7];
		const vertexFormatDesc_t& desc = formatDescs[type];
		const int count =
			((desc.m_countMask + desc.m_countAdd) & 4) +
			((~desc.m_countMask) & desc.m_compressedCount);

		qglColorPointer(
			count,
			desc.m_glStorage,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[7]));
	}

	if (m_flags & 0x100) {
		const int type = m_dataTypes[8];
		const vertexFormatDesc_t& desc = formatDescs[type];
		const int count =
			((desc.m_countMask + desc.m_countAdd) & 4) +
			((~desc.m_countMask) & desc.m_compressedCount);

		qglVertexAttribPointerARB(
			4,
			count,
			desc.m_glStorage,
			desc.m_normalized,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[8]));
	}

	if (m_flags & 0x200) {
		const int type = m_dataTypes[9];
		const vertexFormatDesc_t& desc = formatDescs[type];
		const int count =
			((desc.m_countMask + desc.m_countAdd) & 1) +
			((~desc.m_countMask) & desc.m_compressedCount);

		qglVertexAttribPointerARB(
			15,
			count,
			desc.m_glStorage,
			desc.m_normalized,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[9]));
	}

	if (m_flags & 0x400) {
		for (int tc = 0; tc < 7; ++tc) {
			const int componentIndex = 10 + tc;
			const int dimShift = 9 + tc * 3;
			const unsigned int dim = (m_dimensions >> dimShift) & 0x7;

			if (dim == 0) {
				continue;
			}

			const int type = m_dataTypes[componentIndex];
			const vertexFormatDesc_t& desc = formatDescs[type];
			const int count =
				((~desc.m_countMask) & desc.m_compressedCount) +
				(dim & (desc.m_countMask + desc.m_countAdd));

			qglVertexAttribPointerARB(
				8 + tc,
				count,
				desc.m_glStorage,
				desc.m_normalized,
				m_size,
				reinterpret_cast<const void*>(baseOffset + m_byteOffset[componentIndex]));
		}
	}

	if (m_flags & 0x1) {
		const int type = m_dataTypes[0];
		const vertexFormatDesc_t& desc = formatDescs[type];
		const int count =
			((~desc.m_countMask) & desc.m_compressedCount) +
			((m_dimensions & 0x7) & (desc.m_countMask + desc.m_countAdd));

		qglVertexPointer(
			count,
			desc.m_glStorage,
			m_size,
			reinterpret_cast<const void*>(baseOffset + m_byteOffset[0]));
	}
}

void rvVertexFormat::EnableVertexDeclaration() const {
	//GL_VertexAttribState(m_glVASMask); // jmarshall
}

bool rvVertexFormat::HasSameComponents(const rvVertexFormat& vf) const {
	if ((m_flags & vf.m_flags) != vf.m_flags) {
		return false;
	}

	if ((vf.m_flags & 0x1) && (((m_dimensions ^ vf.m_dimensions) & 0x7) != 0)) {
		return false;
	}

	if ((vf.m_flags & 0x8) && (((m_dimensions ^ vf.m_dimensions) & 0x38) != 0)) {
		return false;
	}

	if ((vf.m_flags & 0x400) == 0) {
		return true;
	}

	for (int shift = 9; shift < 30; shift += 3) {
		const unsigned int theirDim = (vf.m_dimensions >> shift) & 0x7;
		const unsigned int ourDim = (m_dimensions >> shift) & 0x7;

		if (theirDim != 0 && theirDim != ourDim) {
			return false;
		}
	}

	return true;
}

bool rvVertexFormat::HasSameDataTypes(const rvVertexFormat& vf) const {
	if ((m_flags & 0x1) && m_dataTypes[0] != vf.m_dataTypes[0]) return false;
	if ((m_flags & 0x2) && m_dataTypes[1] != vf.m_dataTypes[1]) return false;
	if ((m_flags & 0x4) && m_dataTypes[2] != vf.m_dataTypes[2]) return false;
	if ((m_flags & 0x8) && m_dataTypes[3] != vf.m_dataTypes[3]) return false;
	if ((m_flags & 0x10) && m_dataTypes[4] != vf.m_dataTypes[4]) return false;
	if ((m_flags & 0x20) && m_dataTypes[5] != vf.m_dataTypes[5]) return false;
	if ((m_flags & 0x40) && m_dataTypes[6] != vf.m_dataTypes[6]) return false;
	if ((m_flags & 0x80) && m_dataTypes[7] != vf.m_dataTypes[7]) return false;
	if ((m_flags & 0x100) && m_dataTypes[8] != vf.m_dataTypes[8]) return false;
	if ((m_flags & 0x200) && m_dataTypes[9] != vf.m_dataTypes[9]) return false;

	if ((m_flags & 0x400) == 0) {
		return true;
	}

	for (int tc = 0; tc < 7; ++tc) {
		const int shift = 9 + tc * 3;
		if (((vf.m_dimensions >> shift) & 0x7) != 0) {
			if (m_dataTypes[10 + tc] != vf.m_dataTypes[10 + tc]) {
				return false;
			}
		}
	}

	return true;
}

void rvVertexFormat::ParseComponentDataType(
	Rv_Vertex_Component_t vertexComponent,
	Rv_Vertex_Data_Type_t defaultDataType,
	Lexer& lex)
{
	idToken token;

	if (!lex.ReadToken(&token)) {
		m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		return;
	}

	const char c = static_cast<char>(toupper(token[0]));

	switch (c) {
	case 'B':
		if (!idStr::Icmp(token.c_str(), "Byte")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_BYTE;
		}
		else if (!idStr::Icmp(token.c_str(), "ByteN")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_BYTEN;
		}
		else {
			lex.UnreadToken(&token);
			m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		}
		break;

	case 'C':
		if (!idStr::Icmp(token.c_str(), "Color")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_COLOR;
		}
		else {
			lex.UnreadToken(&token);
			m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		}
		break;

	case 'D':
		if (!idStr::Icmp(token.c_str(), "DEC_10_10_10")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_DEC_10_10_10;
		}
		else if (!idStr::Icmp(token.c_str(), "DEC_10_10_10N")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_DEC_10_10_10N;
		}
		else if (!idStr::Icmp(token.c_str(), "DEC_10_11_11")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_DEC_10_11_11;
		}
		else if (!idStr::Icmp(token.c_str(), "DEC_10_11_11N")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_DEC_10_11_11N;
		}
		else if (!idStr::Icmp(token.c_str(), "DEC_11_11_10")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_DEC_11_11_10;
		}
		else if (!idStr::Icmp(token.c_str(), "DEC_11_11_10N")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_DEC_11_11_10N;
		}
		else {
			lex.UnreadToken(&token);
			m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		}
		break;

	case 'F':
		if (!idStr::Icmp(token.c_str(), "Float")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else if (!idStr::Icmp(token.c_str(), "Float16")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_FLOAT16;
		}
		else {
			lex.UnreadToken(&token);
			m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		}
		break;

	case 'I':
		if (!idStr::Icmp(token.c_str(), "Int")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_INT;
		}
		else if (!idStr::Icmp(token.c_str(), "IntN")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_INTN;
		}
		else {
			lex.UnreadToken(&token);
			m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		}
		break;

	case 'S':
		if (!idStr::Icmp(token.c_str(), "Short")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_SHORT;
		}
		else if (!idStr::Icmp(token.c_str(), "ShortN")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_SHORTN;
		}
		else {
			lex.UnreadToken(&token);
			m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		}
		break;

	case 'U':
		if (!idStr::Icmp(token.c_str(), "UInt")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UINT;
		}
		else if (!idStr::Icmp(token.c_str(), "UIntN")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UINTN;
		}
		else if (!idStr::Icmp(token.c_str(), "UByte")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UBYTE;
		}
		else if (!idStr::Icmp(token.c_str(), "UByteN")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UBYTEN;
		}
		else if (!idStr::Icmp(token.c_str(), "UShort")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_USHORT;
		}
		else if (!idStr::Icmp(token.c_str(), "UShortN")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_USHORTN;
		}
		else if (!idStr::Icmp(token.c_str(), "UDec_10_10_10")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UDEC_10_10_10;
		}
		else if (!idStr::Icmp(token.c_str(), "UDec_10_10_10N")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UDEC_10_10_10N;
		}
		else if (!idStr::Icmp(token.c_str(), "UDec_10_11_11")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UDEC_10_11_11;
		}
		else if (!idStr::Icmp(token.c_str(), "UDec_10_11_11N")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UDEC_10_11_11N;
		}
		else if (!idStr::Icmp(token.c_str(), "UDec_11_11_10")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UDEC_11_11_10;
		}
		else if (!idStr::Icmp(token.c_str(), "UDec_11_11_10N")) {
			m_dataTypes[vertexComponent] = RV_VERTEX_DATA_TYPE_UDEC_11_11_10N;
		}
		else {
			lex.UnreadToken(&token);
			m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		}
		break;

	default:
		lex.UnreadToken(&token);
		m_dataTypes[vertexComponent] = static_cast<unsigned char>(defaultDataType);
		break;
	}
}

void rvVertexFormat::Write(idFile& outFile, const char* /*name*/) {
	outFile.WriteFloatString("{ ");

	if (m_flags & 0x2) {
		outFile.WriteFloatString("Pos3Swizzled ");
		if (m_dataTypes[1] != RV_VERTEX_DATA_TYPE_FLOAT) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[1]]);
		}
	}
	else if (m_flags & 0x1) {
		outFile.WriteFloatString("Position %d ", m_dimensions & 0x7);
		if (m_dataTypes[0] != RV_VERTEX_DATA_TYPE_FLOAT) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[0]]);
		}
	}

	if (m_flags & 0x4) {
		outFile.WriteFloatString("BlendIndex ");
		if (m_dataTypes[2] != RV_VERTEX_DATA_TYPE_UBYTEN) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[2]]);
		}
	}

	if (m_flags & 0x8) {
		outFile.WriteFloatString("BlendWeight %d %d ", (m_dimensions >> 3) & 0x7, (m_dimensions >> 6) & 0x7);
		if (m_dataTypes[3] != RV_VERTEX_DATA_TYPE_FLOAT) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[3]]);
		}
	}

	if (m_flags & 0x10) {
		outFile.WriteFloatString("Normal ");
		if (m_dataTypes[4] != RV_VERTEX_DATA_TYPE_FLOAT) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[4]]);
		}
	}

	if (m_flags & 0x20) {
		outFile.WriteFloatString("Tangent ");
		if (m_dataTypes[5] != RV_VERTEX_DATA_TYPE_FLOAT) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[5]]);
		}
	}

	if (m_flags & 0x40) {
		outFile.WriteFloatString("Binormal ");
		if (m_dataTypes[6] != RV_VERTEX_DATA_TYPE_FLOAT) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[6]]);
		}
	}

	if (m_flags & 0x80) {
		outFile.WriteFloatString("DiffuseColor ");
		if (m_dataTypes[7] != RV_VERTEX_DATA_TYPE_COLOR) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[7]]);
		}
	}

	if (m_flags & 0x100) {
		outFile.WriteFloatString("SpecularColor ");
		if (m_dataTypes[8] != RV_VERTEX_DATA_TYPE_COLOR) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[8]]);
		}
	}

	if (m_flags & 0x200) {
		outFile.WriteFloatString("PointSize ");
		if (m_dataTypes[9] != RV_VERTEX_DATA_TYPE_FLOAT) {
			outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[9]]);
		}
	}

	if (m_flags & 0x400) {
		for (int tc = 0; tc < 7; ++tc) {
			const int dim = (m_dimensions >> (9 + tc * 3)) & 0x7;
			if (dim != 0) {
				outFile.WriteFloatString("TexCoord %d %d ", dim, tc);
			}
			if (m_dataTypes[10 + tc] != RV_VERTEX_DATA_TYPE_FLOAT) {
				outFile.WriteFloatString("%s ", outputDataTypeStrings[m_dataTypes[10 + tc]]);
			}
		}
	}

	outFile.WriteFloatString("}\n");
}

void rvVertexFormat::Init(
	unsigned int vtxFmtFlags,
	int posDim,
	int numWeights,
	int numTransforms,
	const int* texDimArray,
	Rv_Vertex_Data_Type_t* dataTypes) {
	if (m_flags != 0) {
		Shutdown();
	}

	m_flags = vtxFmtFlags;

	int finalPosDim = posDim;
	if (m_flags & 0x2) {
		m_flags |= 0x1;
		finalPosDim = 3;
	}
	else if (posDim > 4) {
		common->Error("Vertex format was initialized with an unsupported position dimension");
	}

	if (numWeights > 4) {
		common->Error("Vertex format was initialized with an unsupported number of blend weights");
	}

	int finalNumTransforms = numTransforms;
	if (finalNumTransforms < 1) {
		finalNumTransforms = 1;
	}
	else if (finalNumTransforms > numWeights + 1) {
		common->Error("Vertex format was initialized with an unsupported number of blend transforms");
	}

	m_dimensions = (finalPosDim & 0x7) |
		((numWeights & 0x7) << 3) |
		((finalNumTransforms & 0x7) << 6);

	if (m_flags & 0x400) {
		for (int i = 0; i < 7; ++i) {
			if (texDimArray[i] > 4) {
				common->Error("Vertex format was initialized with an unsupported texture dimension");
			}

			m_dimensions |= ((texDimArray[i] & 0x7) << (9 + i * 3));
		}
	}

	BuildDataTypes(dataTypes);
}

void rvVertexFormat::Init(Lexer& lex)
{
	idToken token;
	Rv_Vertex_Component_t currentComponent = RV_VERTEX_COMPONENT_POSITION;
	Rv_Vertex_Data_Type_t defaultType = RV_VERTEX_DATA_TYPE_FLOAT;

	if (m_flags != 0) {
		Shutdown();
	}

	lex.ExpectTokenString("{");

	if (!lex.ReadToken(&token)) {
		lex.Error("Unexpected end-of-token stream");
		return;
	}

	while (idStr::Icmp(token.c_str(), "}") != 0) {
		if (!idStr::Icmp(token.c_str(), "Position")) {
			m_flags |= 0x1;

			const int dim = lex.ParseInt();
			if (dim > 4) {
				lex.Error("Vertex format was initialized with an unsupported position dimension");
			}

			m_dimensions |= dim;
			currentComponent = RV_VERTEX_COMPONENT_POSITION;
			defaultType = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else if (!idStr::Icmp(token.c_str(), "PositionSwizzled")) {
			m_flags |= 0x2;
			m_dimensions |= 0x3;
			currentComponent = RV_VERTEX_COMPONENT_SWIZZLED_POSITION;
			defaultType = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else if (!idStr::Icmp(token.c_str(), "BlendIndex")) {
			m_flags |= 0x4;
			currentComponent = RV_VERTEX_COMPONENT_BLEND_INDEX;
			defaultType = RV_VERTEX_DATA_TYPE_UBYTEN;
		}
		else if (!idStr::Icmp(token.c_str(), "BlendWeight")) {
			m_flags |= 0x8;

			const int numWeights = lex.ParseInt();
			if (numWeights > 4) {
				lex.Error("Vertex format was initialized with an unsupported number of blend weights");
			}

			int numTransforms = lex.ParseInt();
			if (numTransforms < 1) {
				numTransforms = 1;
			}
			else if (numTransforms > numWeights + 1) {
				lex.Error("Vertex format was initialized with an unsupported number of blend transforms");
			}

			m_dimensions |= ((numWeights & 7) << 3);
			m_dimensions |= ((numTransforms & 7) << 6);

			currentComponent = RV_VERTEX_COMPONENT_BLEND_WEIGHT;
			defaultType = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else if (!idStr::Icmp(token.c_str(), "Normal")) {
			m_flags |= 0x10;
			currentComponent = RV_VERTEX_COMPONENT_NORMAL;
			defaultType = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else if (!idStr::Icmp(token.c_str(), "Tangent")) {
			m_flags |= 0x20;
			currentComponent = RV_VERTEX_COMPONENT_TANGENT;
			defaultType = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else if (!idStr::Icmp(token.c_str(), "Binormal")) {
			m_flags |= 0x40;
			currentComponent = RV_VERTEX_COMPONENT_BINORMAL;
			defaultType = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else if (!idStr::Icmp(token.c_str(), "DiffuseColor")) {
			m_flags |= 0x80;
			currentComponent = RV_VERTEX_COMPONENT_DIFFUSE_COLOR;
			defaultType = RV_VERTEX_DATA_TYPE_COLOR;
		}
		else if (!idStr::Icmp(token.c_str(), "SpecularColor")) {
			m_flags |= 0x100;
			currentComponent = RV_VERTEX_COMPONENT_SPECULAR_COLOR;
			defaultType = RV_VERTEX_DATA_TYPE_COLOR;
		}
		else if (!idStr::Icmp(token.c_str(), "PointSize")) {
			m_flags |= 0x200;
			currentComponent = RV_VERTEX_COMPONENT_POINT_SIZE;
			defaultType = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else if (!idStr::Icmp(token.c_str(), "TexCoord")) {
			m_flags |= 0x400;

			const int dim = lex.ParseInt();
			if (dim > 4) {
				lex.Error("Vertex format was initialized with an unsupported texture coordinate dimension");
			}

			const int set = lex.ParseInt();
			if (set >= 7) {
				lex.Error("Vertex format was initialized with an unsupported texture coordinate set");
			}

			m_dimensions |= (dim << (9 + set * 3));
			currentComponent = static_cast<Rv_Vertex_Component_t>(RV_VERTEX_COMPONENT_TEXCOORD0 + set);
			defaultType = RV_VERTEX_DATA_TYPE_FLOAT;
		}
		else {
			lex.Error("Expected vertex format keyword");
		}

		ParseComponentDataType(currentComponent, defaultType, lex);

		if (!lex.ReadToken(&token)) {
			lex.Error("Unexpected end-of-token stream");
			return;
		}
	}

	CalcSize();

	if (((m_dimensions >> 6) & 7) == 0) {
		m_dimensions |= (1 << 6);
	}
}

void rvVertexFormat::Init(const rvVertexFormat& vf) {
	if (m_flags != 0) {
		Shutdown();
	}

	m_flags = vf.m_flags;
	m_dimensions = vf.m_dimensions;
	m_size = vf.m_size;
	m_numValues = vf.m_numValues;
	memcpy(m_byteOffset, vf.m_byteOffset, sizeof(m_byteOffset));
	memcpy(m_dataTypes, vf.m_dataTypes, sizeof(m_dataTypes));
	memcpy(m_tokenSubTypes, vf.m_tokenSubTypes, sizeof(m_tokenSubTypes));
	m_glVASMask = vf.m_glVASMask;
}