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

rvIndexBuffer::rvIndexBuffer() {
	m_flags = 0;
	m_lockStatus = 0;
	m_numIndices = 0;
	m_ibID = 0;
	m_lockIndexOffset = 0;
	m_lockIndexCount = 0;
	m_lockedBase = NULL;
	m_systemMemStorage = NULL;
	m_numIndicesWritten = 0;
}

rvIndexBuffer::~rvIndexBuffer() {
	Shutdown();
}

void rvIndexBuffer::CreateIndexStorage() {
	if (m_flags & IBF_SYSTEM_MEMORY) {
		const int bytesPerIndex = (m_flags & IBF_INDEX16) ? 2 : 4;
		m_systemMemStorage = reinterpret_cast<unsigned char*>(Mem_Alloc16(bytesPerIndex * m_numIndices, 0x11));
		if (m_systemMemStorage == NULL) {
			idLib::common->FatalError("Ran out of memory trying to allocate system memory index storage");
			return;
		}
	}

	if (m_flags & IBF_VIDEO_MEMORY) {
		qglGenBuffersARB(1, &m_ibID);
		if (!m_ibID) {
			idLib::common->FatalError("rvIndexBuffer: Unable to gen index buffer id");
			return;
		}

		if ((m_flags & IBF_DISCARDABLE) == 0) {
			const int bytesPerIndex = (m_flags & IBF_INDEX16) ? 2 : 4;
			const int totalBytes = bytesPerIndex * m_numIndices;

			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibID);
			qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, totalBytes, NULL, GL_STATIC_DRAW_ARB);

			if (qglGetError() != GL_NO_ERROR) {
				idLib::common->FatalError("Unable to allocate index storage");
			}

			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		}
	}
}

void rvIndexBuffer::Init(int numIndices, unsigned int flagMask) {
	if (m_flags) {
		Shutdown();
	}

	m_numIndices = numIndices;
	m_flags = flagMask;
	CreateIndexStorage();
}

void rvIndexBuffer::Init(Lexer& lex) {
	idToken token;

	if (m_flags) {
		Shutdown();
	}

	lex.ExpectTokenString("{");

	m_flags = 0;

	while (lex.ReadToken(&token)) {
		if (!idStr::Icmp(token.c_str(), "SystemMemory")) {
			m_flags |= IBF_SYSTEM_MEMORY;
			continue;
		}
		if (!idStr::Icmp(token.c_str(), "VideoMemory")) {
			m_flags |= IBF_VIDEO_MEMORY;
			continue;
		}
		break;
	}

	if (!m_flags) {
		m_flags = IBF_VIDEO_MEMORY;
	}

	if (!idStr::Icmp(token.c_str(), "BitDepth")) {
		if (lex.ParseInt() == 16) {
			m_flags |= IBF_INDEX16;
		}
		lex.ReadToken(&token);
	}

	if (idStr::Icmp(token.c_str(), "Index")) {
		lex.Error("Expected index header");
		return;
	}

	lex.ExpectTokenString("[");
	m_numIndices = lex.ParseInt();
	lex.ExpectTokenString("]");
	lex.ExpectTokenString("{");

	if (m_numIndices <= 0) {
		lex.Error("Invalid index count");
		return;
	}

	CreateIndexStorage();

	void* indexBasePtr = NULL;
	if (!Lock(0, m_numIndices, IBLOCK_WRITE | IBLOCK_DISCARD, &indexBasePtr)) {
		lex.Error("Index buffer cannot be mapped for access");
		Shutdown();
		return;
	}

	if (m_flags & IBF_INDEX16) {
		unsigned short* dst = reinterpret_cast<unsigned short*>(indexBasePtr);
		for (int i = 0; i < m_numIndices; ++i) {
			dst[i] = static_cast<unsigned short>(lex.ParseInt());
		}
	}
	else {
		unsigned int* dst = reinterpret_cast<unsigned int*>(indexBasePtr);
		for (int i = 0; i < m_numIndices; ++i) {
			dst[i] = static_cast<unsigned int>(lex.ParseInt());
		}
	}

	Unlock();

	lex.ExpectTokenString("}");
	lex.ExpectTokenString("}");
}

bool rvIndexBuffer::Lock(int indexOffset, int numIndicesToLock, unsigned int lockFlags, void** indexPtr) {
	m_lockIndexOffset = indexOffset;
	m_lockIndexCount = (numIndicesToLock != 0) ? numIndicesToLock : (m_numIndices - indexOffset);

	const int bytesPerIndex = (m_flags & IBF_INDEX16) ? 2 : 4;

	if (m_flags & IBF_SYSTEM_MEMORY) {
		m_lockedBase = m_systemMemStorage;
		m_lockStatus = lockFlags;
		*indexPtr = m_lockedBase + (bytesPerIndex * m_lockIndexOffset);
		return true;
	}

	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibID);

	unsigned int mapAccess = GL_READ_WRITE_ARB;
	unsigned int effectiveLockFlags = lockFlags;

	if (lockFlags & IBLOCK_READ) {
		mapAccess = (lockFlags & IBLOCK_WRITE) ? GL_READ_WRITE_ARB : GL_READ_ONLY_ARB;
	}
	else if (lockFlags & IBLOCK_WRITE) {
		mapAccess = GL_WRITE_ONLY_ARB;

		if (m_flags & IBF_DISCARDABLE) {
			effectiveLockFlags |= IBLOCK_DISCARD;
			const unsigned int usage = (effectiveLockFlags & IBLOCK_DISCARD) ? GL_STREAM_DRAW_ARB : GL_STATIC_DRAW_ARB;

			if (effectiveLockFlags & IBLOCK_DISCARD) {
				const int lockBytes = bytesPerIndex * m_lockIndexCount;
				m_lockIndexOffset = 0;
				qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, lockBytes, NULL, usage);
			}
		}
	}

	m_lockedBase = reinterpret_cast<unsigned char*>(qglMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mapAccess));
	if (m_lockedBase == NULL) {
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		return false;
	}

	m_lockStatus = effectiveLockFlags;
	*indexPtr = m_lockedBase + (bytesPerIndex * m_lockIndexOffset);
	return true;
}

void rvIndexBuffer::Unlock() {
	if (m_lockStatus & IBLOCK_WRITE) {
		if (m_lockStatus & IBLOCK_DISCARD) {
			m_numIndicesWritten = m_lockIndexCount;
		}
		else {
			m_numIndicesWritten += m_lockIndexCount;
		}
	}

	const int bytesPerIndex = (m_flags & IBF_INDEX16) ? 2 : 4;

	if (m_flags & IBF_SYSTEM_MEMORY) {
		if ((m_lockStatus & IBLOCK_WRITE) && (m_flags & IBF_VIDEO_MEMORY)) {
			const int byteOffset = bytesPerIndex * m_lockIndexOffset;
			const int byteCount = bytesPerIndex * m_lockIndexCount;

			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibID);
			qglBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, byteOffset, byteCount, m_systemMemStorage + byteOffset);
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		}
	}
	else if (m_flags & IBF_VIDEO_MEMORY) {
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibID);
		qglUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	m_lockStatus = 0;
	m_lockIndexOffset = 0;
	m_lockIndexCount = 0;
	m_lockedBase = NULL;
}

void rvIndexBuffer::CopyRemappedData(unsigned int indexBufferOffset, int numIndices, const unsigned int* indexMapping, const int* indices, unsigned int indexBase) {
	if (static_cast<int>(indexBufferOffset) + numIndices > m_numIndices) {
		common->Error("rvIndexBuffer: attempt to copy index data out-of-bounds");
		return;
	}

	void* indexBasePtr = NULL;
	Lock(indexBufferOffset, numIndices, IBLOCK_WRITE, &indexBasePtr);

	if (m_flags & IBF_INDEX16) {
		unsigned short* dst = reinterpret_cast<unsigned short*>(indexBasePtr);
		for (int i = 0; i < numIndices; ++i) {
			dst[i] = static_cast<unsigned short>(indexBase + indexMapping[indices[i]]);
		}
	}
	else {
		unsigned int* dst = reinterpret_cast<unsigned int*>(indexBasePtr);
		for (int i = 0; i < numIndices; ++i) {
			dst[i] = indexBase + indexMapping[indices[i]];
		}
	}

	Unlock();
}

void rvIndexBuffer::CopyData(unsigned int indexBufferOffset, int numIndices, const int* indices, unsigned int indexBase) {
	if (static_cast<int>(indexBufferOffset) + numIndices > m_numIndices) {
		common->Error("rvIndexBuffer: attempt to copy index data out-of-bounds");
		return;
	}

	void* indexBasePtr = NULL;
	Lock(indexBufferOffset, numIndices, IBLOCK_WRITE, &indexBasePtr);

	if (m_flags & IBF_INDEX16) {
		unsigned short* dst = reinterpret_cast<unsigned short*>(indexBasePtr);
		for (int i = 0; i < numIndices; ++i) {
			dst[i] = static_cast<unsigned short>(indexBase + indices[i]);
		}
	}
	else {
		unsigned int* dst = reinterpret_cast<unsigned int*>(indexBasePtr);
		for (int i = 0; i < numIndices; ++i) {
			dst[i] = indexBase + indices[i];
		}
	}

	Unlock();
}

void rvIndexBuffer::Write(idFile& outFile, const char* prepend) {
	idStr indent = prepend;
	idStr innerIndent = indent + "\t";
	idStr dataIndent = innerIndent + "\t";

	outFile.WriteFloatString("%sIndexBuffer\n", prepend);
	outFile.WriteFloatString("%s{\n", prepend);

	if (m_flags & IBF_SYSTEM_MEMORY) {
		outFile.WriteFloatString("%sSystemMemory\n", innerIndent.c_str());
	}
	if (m_flags & IBF_VIDEO_MEMORY) {
		outFile.WriteFloatString("%sVideoMemory\n", innerIndent.c_str());
	}
	if (m_flags & IBF_INDEX16) {
		outFile.WriteFloatString("%sBitDepth 16\n", innerIndent.c_str());
	}

	outFile.WriteFloatString("%sIndex[ %d ]\n", innerIndent.c_str(), m_numIndicesWritten);
	outFile.WriteFloatString("%s{\n", innerIndent.c_str());

	void* indexBasePtr = NULL;
	if (!Lock(0, m_numIndicesWritten, IBLOCK_READ, &indexBasePtr)) {
		idLib::common->FatalError("Index buffer cannot be mapped for access");
		return;
	}

	outFile.WriteFloatString("%s", dataIndent.c_str());

	if (m_flags & IBF_INDEX16) {
		const unsigned short* src = reinterpret_cast<const unsigned short*>(indexBasePtr);
		for (int i = 0; i < m_numIndicesWritten; ++i) {
			outFile.WriteFloatString("%d ", src[i]);
			if ((i & 15) == 15) {
				outFile.WriteFloatString("\n%s", dataIndent.c_str());
			}
		}
	}
	else {
		const unsigned int* src = reinterpret_cast<const unsigned int*>(indexBasePtr);
		for (int i = 0; i < m_numIndicesWritten; ++i) {
			outFile.WriteFloatString("%d ", src[i]);
			if ((i & 15) == 15) {
				outFile.WriteFloatString("\n%s", dataIndent.c_str());
			}
		}
	}

	outFile.WriteFloatString("\n");
	Unlock();

	outFile.WriteFloatString("%s}\n", innerIndent.c_str());
	outFile.WriteFloatString("%s}\n", prepend);
}

void rvIndexBuffer::Shutdown() {
	if (m_lockStatus) {
		Unlock();
	}

	if (m_systemMemStorage) {
		Mem_Free16(m_systemMemStorage);
		m_systemMemStorage = NULL;
	}

	if (m_ibID) {
		qglDeleteBuffersARB(1, &m_ibID);
		m_ibID = 0;
	}

	m_flags = 0;
	m_lockStatus = 0;
	m_numIndices = 0;
	m_lockIndexOffset = 0;
	m_lockIndexCount = 0;
	m_lockedBase = NULL;
	m_numIndicesWritten = 0;
}

void rvIndexBuffer::Resize(int numIndices) {
	if (numIndices == m_numIndices) {
		return;
	}

	if ((m_flags & IBF_SYSTEM_MEMORY) == 0) {
		return;
	}

	int copyCount = m_numIndices;
	if (copyCount > numIndices) {
		copyCount = numIndices;
	}

	const int bytesPerIndex = (m_flags & IBF_INDEX16) ? 2 : 4;
	const int newSizeBytes = bytesPerIndex * numIndices;
	const int copySizeBytes = bytesPerIndex * copyCount;

	void* newStorage = Mem_Alloc16(newSizeBytes, 0x11);
	if (newStorage == NULL) {
		idLib::common->FatalError("Ran out of memory trying to allocate system memory index storage");
		Shutdown();
		return;
	}

	SIMDProcessor->Memcpy(newStorage, m_systemMemStorage, copySizeBytes);
	Mem_Free16(m_systemMemStorage);
	m_systemMemStorage = reinterpret_cast<unsigned char*>(newStorage);

	if (m_flags & IBF_VIDEO_MEMORY) {
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibID);
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, newSizeBytes, m_systemMemStorage, GL_STATIC_DRAW_ARB);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	m_numIndices = numIndices;
}