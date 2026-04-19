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

// This matches the logging style implied by the decompilation.
extern idCommon* common;

HDC   rvTexRenderTarget::m_hPrevDC = nullptr;
HGLRC rvTexRenderTarget::m_hPrevGLRC = nullptr;
int   rvTexRenderTarget::m_prevDrawBuffer = 0;
int   rvTexRenderTarget::m_prevReadBuffer = 0;
int   rvTexRenderTarget::m_prevViewport[4] = { 0, 0, 0, 0 };

rvTexRenderTarget::rvTexRenderTarget() {
	ResetValues();

	m_hGLRC = nullptr;

	m_resWidthSave = 0;
	m_resHeightSave = 0;
	m_flagsSave = 0;
	m_numColorBitsSave = 0;
	m_numRedBitsSave = 0;
	m_numGreenBitsSave = 0;
	m_numBlueBitsSave = 0;
	m_numAlphaBitsSave = 0;
	m_numDepthBitsSave = 0;
	m_numStencilBitsSave = 0;
}

rvTexRenderTarget::~rvTexRenderTarget() {
	Release();
}

void rvTexRenderTarget::ResetValues() {
	m_hPBuffer = nullptr;
	m_hDC = nullptr;
	m_hGLRC = nullptr;
	m_hPrevDC = nullptr;
	m_hPrevGLRC = nullptr;
	m_textureObjName = 0;

	m_resWidth = 0;
	m_resHeight = 0;
	m_flags = 0;

	m_numColorBits = 0;
	m_numRedBits = 0;
	m_numGreenBits = 0;
	m_numBlueBits = 0;
	m_numAlphaBits = 0;
	m_numDepthBits = 0;
	m_numStencilBits = 0;

	m_target = 3553; // GL_TEXTURE_2D
}

bool rvTexRenderTarget::Init(
	int resWidth,
	int resHeight,
	int numColorBits,
	int numRedBits,
	int numGreenBits,
	int numBlueBits,
	int numAlphaBits,
	int numDepthBits,
	int numStencilBits,
	int flags
) {
	HDC   currentDC = wglGetCurrentDC();
	HGLRC currentGLRC = (HGLRC)wglGetCurrentContext();

	int pixelFormatIndex = 0;
	UINT formatCount = 0;
	int attribs[24] = { 0 };

	attribs[0] = 8237; // WGL_DRAW_TO_PBUFFER_ARB
	attribs[1] = 1;

	int attribIndex;

	if (flags & 2) {
		attribs[2] = 8368; // floating point related format selector
		attribs[3] = 1;
		attribIndex = 5;

		if (numAlphaBits) {
			attribs[4] = 8372;
		}
		else if (numBlueBits) {
			attribs[4] = 8371;
		}
		else if (numGreenBits) {
			attribs[4] = 8370;
		}
		else {
			attribs[4] = 8369;
		}
	}
	else {
		const int hasAlpha = (numAlphaBits != 0) ? 1 : 0;
		attribs[2] = (flags & 0xC) ? (8304 + hasAlpha) : (8352 + hasAlpha);
		attribIndex = 3;
	}

	attribs[attribIndex++] = 1;

	attribs[attribIndex++] = 8212; // WGL_COLOR_BITS_ARB
	attribs[attribIndex++] = numColorBits;
	attribs[attribIndex++] = 8213; // WGL_RED_BITS_ARB
	attribs[attribIndex++] = numRedBits;
	attribs[attribIndex++] = 8215; // WGL_GREEN_BITS_ARB
	attribs[attribIndex++] = numGreenBits;
	attribs[attribIndex++] = 8217; // WGL_BLUE_BITS_ARB
	attribs[attribIndex++] = numBlueBits;
	attribs[attribIndex++] = 8219; // WGL_ALPHA_BITS_ARB
	attribs[attribIndex++] = numAlphaBits;

	if (flags & 1) {
		attribs[attribIndex++] = 8226; // WGL_DEPTH_BITS_ARB
		attribs[attribIndex++] = numDepthBits;
		attribs[attribIndex++] = 8227; // WGL_STENCIL_BITS_ARB
		attribs[attribIndex++] = numStencilBits;
	}

	attribs[attribIndex++] = 0;
	attribs[attribIndex++] = 0;

	if (!wglChoosePixelFormatARB(currentDC, attribs, nullptr, 1, &pixelFormatIndex, &formatCount)) {
		common->Warning("wglChoosePixelFormatARB() failed, last error %x\n", GetLastError());
		return false;
	}

	if (formatCount == 0) {
		common->Warning("wglChoosePixelFormatARB() couldn't find any matches\n");
		return false;
	}

	// Build pbuffer attributes.
	memset(attribs, 0, sizeof(attribs));
	attribs[0] = 8307; // WGL_TEXTURE_FORMAT_ARB

	if (flags & 4) {
		attribs[1] = 8312;   // cube map
		m_target = 34067;    // GL_TEXTURE_CUBE_MAP
	}
	else if (flags & 8) {
		attribs[1] = 8314;   // texture 2D
		m_target = 3553;     // GL_TEXTURE_2D
	}
	else {
		attribs[1] = 8354;   // rectangle texture
		m_target = 34037;    // GL_TEXTURE_RECTANGLE_ARB
	}

	attribs[2] = 8306; // WGL_TEXTURE_TARGET_ARB

	int texAttribIndex = 4;

	if (flags & 2) {
		if (numAlphaBits) {
			attribs[3] = 8376;
		}
		else if (numBlueBits) {
			attribs[3] = 8375;
		}
		else if (numGreenBits) {
			attribs[3] = 8374;
		}
		else {
			attribs[3] = 8373;
		}
	}
	else {
		attribs[3] = 8309 + ((numAlphaBits != 0) ? 1 : 0);
	}

	if (flags & 8) {
		attribs[4] = 8308; // mipmap texture?
		attribs[5] = 1;
		texAttribIndex = 6;
	}

	attribs[texAttribIndex++] = 0;
	attribs[texAttribIndex++] = 0;

	m_hPBuffer = wglCreatePbufferARB(currentDC, pixelFormatIndex, resWidth, resHeight, attribs);
	if (!m_hPBuffer) {
		common->Warning("wglCreatePbufferARB() failed, last error %x\n", GetLastError());
		return false;
	}

	m_hDC = wglGetPbufferDCARB(m_hPBuffer);
	if (!m_hDC) {
		wglDestroyPbufferARB(m_hPBuffer);
		ResetValues();
		return false;
	}

	m_hGLRC = (HGLRC)wglCreateContext(m_hDC);
	if (!m_hGLRC) {
		wglReleasePbufferDCARB(m_hPBuffer, m_hDC);
		wglDestroyPbufferARB(m_hPBuffer);
		ResetValues();
		return false;
	}

	if (currentGLRC && !wglShareLists(currentGLRC, m_hGLRC)) {
		common->Warning("wglShareLists() couldn't share display list resources\n");
		wglDeleteContext(m_hGLRC);
		wglReleasePbufferDCARB(m_hPBuffer, m_hDC);
		wglDestroyPbufferARB(m_hPBuffer);
		ResetValues();
		return false;
	}

	m_flagsSave = flags;
	m_flags = flags;

	m_numColorBitsSave = numColorBits;
	m_numColorBits = numColorBits;

	m_numRedBitsSave = numRedBits;
	m_numRedBits = numRedBits;

	m_numGreenBitsSave = numGreenBits;
	m_numGreenBits = numGreenBits;

	m_numBlueBitsSave = numBlueBits;
	m_numBlueBits = numBlueBits;

	m_numAlphaBitsSave = numAlphaBits;
	m_numAlphaBits = numAlphaBits;

	m_numDepthBitsSave = numDepthBits;
	m_numDepthBits = numDepthBits;

	m_numStencilBitsSave = numStencilBits;
	m_numStencilBits = numStencilBits;

	m_resWidthSave = resWidth;
	m_resWidth = resWidth;

	m_resHeightSave = resHeight;
	m_resHeight = resHeight;

	return true;
}

void rvTexRenderTarget::Release() {
	if (m_hPBuffer) {
		if (m_hDC) {
			if (m_hGLRC) {
				wglDeleteContext(m_hGLRC);
				m_hGLRC = nullptr;
			}
			wglReleasePbufferDCARB(m_hPBuffer, m_hDC);
			m_hDC = nullptr;
		}

		wglDestroyPbufferARB(m_hPBuffer);
		m_hPBuffer = nullptr;
	}

	ResetValues();
}

bool rvTexRenderTarget::Restore() {
	return Init(
		m_resWidthSave,
		m_resHeightSave,
		m_numColorBitsSave,
		m_numRedBitsSave,
		m_numGreenBitsSave,
		m_numBlueBitsSave,
		m_numAlphaBitsSave,
		m_numDepthBitsSave,
		m_numStencilBitsSave,
		m_flagsSave
	);
}

void rvTexRenderTarget::BeginRender(int cubeFace) {
	if (m_textureObjName) {
		wglReleaseTexImageARB(m_hPBuffer, 8323);
		m_textureObjName = 0;
	}

	if (!m_hPrevDC && !m_hPrevGLRC) {
		m_hPrevDC = wglGetCurrentDC();
		m_hPrevGLRC = (HGLRC)wglGetCurrentContext();

		glGetIntegerv(0x0C01, &m_prevDrawBuffer);    // GL_DRAW_BUFFER
		glGetIntegerv(0x0C02, &m_prevReadBuffer);    // GL_READ_BUFFER
		glGetIntegerv(0x0BA2, m_prevViewport);       // GL_VIEWPORT
	}

	if (m_flags & 4) {
		int attribs[4];
		attribs[0] = 8316;
		attribs[1] = cubeFace;
		attribs[2] = 0;
		attribs[3] = 0;
		wglSetPbufferAttribARB(m_hPBuffer, attribs);
	}

	if (!wglMakeCurrent(m_hDC, m_hGLRC)) {
		common->Printf("wglMakeCurrent(m_hDC, m_hGLRC) failed, last error %x\n", GetLastError());
	}

	glDrawBuffer(0x0404); // GL_FRONT
	glReadBuffer(0x0404); // GL_FRONT
	glViewport(0, 0, m_resWidth, m_resHeight);
}

void rvTexRenderTarget::EndRender(bool restorePreviousDC) {
	if (!restorePreviousDC) {
		return;
	}

	if (!wglMakeCurrent(m_hPrevDC, m_hPrevGLRC)) {
		common->Printf("wglMakeCurrent(m_hPrevDC, m_hPrevGLRC) failed, last error %x\n", GetLastError());
	}

	glDrawBuffer(m_prevDrawBuffer);
	glReadBuffer(m_prevReadBuffer);
	glViewport(
		m_prevViewport[0],
		m_prevViewport[1],
		m_prevViewport[2],
		m_prevViewport[3]
	);

	m_hPrevDC = nullptr;
	m_hPrevGLRC = nullptr;
}

void rvTexRenderTarget::BeginTexture(
	unsigned int textureObjName,
	int minFilter,
	int magFilter,
	int wrap
) {
	if (m_textureObjName) {
		wglReleaseTexImageARB(m_hPBuffer, 8323);
		m_textureObjName = 0;
	}

	glBindTexture(m_target, textureObjName);
	glTexParameteri(m_target, 0x2801, minFilter); // GL_TEXTURE_MIN_FILTER
	glTexParameteri(m_target, 0x2800, magFilter); // GL_TEXTURE_MAG_FILTER
	glTexParameteri(m_target, 0x2802, wrap);      // GL_TEXTURE_WRAP_S
	glTexParameteri(m_target, 0x2803, wrap);      // GL_TEXTURE_WRAP_T

	wglBindTexImageARB(m_hPBuffer, 8323);
	m_textureObjName = textureObjName;
}

void rvTexRenderTarget::EndTexture() {
	wglReleaseTexImageARB(m_hPBuffer, 8323);
	m_textureObjName = 0;
}

void rvTexRenderTarget::DefaultD3GL() {
	BeginRender(0);

	glClearDepth(1.0);
	glClear(0x4100); // GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT

	glEnable(0x0B71);               // GL_DEPTH_TEST
	glDisable(0x0BC0);              // GL_ALPHA_TEST
	glDepthFunc(0x0203);            // GL_LEQUAL

	glMatrixMode(0x1700);           // GL_MODELVIEW
	glLoadIdentity();
	glMatrixMode(0x1701);           // GL_PROJECTION
	glLoadIdentity();

	glEnableClientState(0x8074);    // GL_VERTEX_ARRAY
	glDisableClientState(0x8075);   // GL_NORMAL_ARRAY
	glEnableClientState(0x8078);    // GL_TEXTURE_COORD_ARRAY
	glDisableClientState(0x8076);   // GL_COLOR_ARRAY

	glShadeModel(0x1D01);           // GL_SMOOTH
	glCullFace(0x0405);             // GL_BACK
	glEnable(0x0B44);               // GL_CULL_FACE
	glDisable(0x0BE2);              // GL_BLEND
	glDisable(0x0B50);              // GL_LIGHTING
	glDisable(0x0DE1);              // GL_TEXTURE_2D

	EndRender(true);
}