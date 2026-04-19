#pragma once

#define __gl_h_

#include <stddef.h>
#include <stdint.h>
#include <d3d12.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <math.h>
#include <vector>

template<typename T>
static T ClampValue(T v, T lo, T hi)
{
	return (v < lo) ? lo : ((v > hi) ? hi : v);
}

struct Mat4
{
	float m[16];

	static Mat4 Identity()
	{
		Mat4 r{};
		r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
		return r;
	}

	static bool Invert(const Mat4* in, Mat4* out)
	{
		const float* m = in->m;
		float inv[16];

		inv[0] = m[5] * m[10] * m[15] -
			m[5] * m[11] * m[14] -
			m[9] * m[6] * m[15] +
			m[9] * m[7] * m[14] +
			m[13] * m[6] * m[11] -
			m[13] * m[7] * m[10];

		inv[4] = -m[4] * m[10] * m[15] +
			m[4] * m[11] * m[14] +
			m[8] * m[6] * m[15] -
			m[8] * m[7] * m[14] -
			m[12] * m[6] * m[11] +
			m[12] * m[7] * m[10];

		inv[8] = m[4] * m[9] * m[15] -
			m[4] * m[11] * m[13] -
			m[8] * m[5] * m[15] +
			m[8] * m[7] * m[13] +
			m[12] * m[5] * m[11] -
			m[12] * m[7] * m[9];

		inv[12] = -m[4] * m[9] * m[14] +
			m[4] * m[10] * m[13] +
			m[8] * m[5] * m[14] -
			m[8] * m[6] * m[13] -
			m[12] * m[5] * m[10] +
			m[12] * m[6] * m[9];

		inv[1] = -m[1] * m[10] * m[15] +
			m[1] * m[11] * m[14] +
			m[9] * m[2] * m[15] -
			m[9] * m[3] * m[14] -
			m[13] * m[2] * m[11] +
			m[13] * m[3] * m[10];

		inv[5] = m[0] * m[10] * m[15] -
			m[0] * m[11] * m[14] -
			m[8] * m[2] * m[15] +
			m[8] * m[3] * m[14] +
			m[12] * m[2] * m[11] -
			m[12] * m[3] * m[10];

		inv[9] = -m[0] * m[9] * m[15] +
			m[0] * m[11] * m[13] +
			m[8] * m[1] * m[15] -
			m[8] * m[3] * m[13] -
			m[12] * m[1] * m[11] +
			m[12] * m[3] * m[9];

		inv[13] = m[0] * m[9] * m[14] -
			m[0] * m[10] * m[13] -
			m[8] * m[1] * m[14] +
			m[8] * m[2] * m[13] +
			m[12] * m[1] * m[10] -
			m[12] * m[2] * m[9];

		inv[2] = m[1] * m[6] * m[15] -
			m[1] * m[7] * m[14] -
			m[5] * m[2] * m[15] +
			m[5] * m[3] * m[14] +
			m[13] * m[2] * m[7] -
			m[13] * m[3] * m[6];

		inv[6] = -m[0] * m[6] * m[15] +
			m[0] * m[7] * m[14] +
			m[4] * m[2] * m[15] -
			m[4] * m[3] * m[14] -
			m[12] * m[2] * m[7] +
			m[12] * m[3] * m[6];

		inv[10] = m[0] * m[5] * m[15] -
			m[0] * m[7] * m[13] -
			m[4] * m[1] * m[15] +
			m[4] * m[3] * m[13] +
			m[12] * m[1] * m[7] -
			m[12] * m[3] * m[5];

		inv[14] = -m[0] * m[5] * m[14] +
			m[0] * m[6] * m[13] +
			m[4] * m[1] * m[14] -
			m[4] * m[2] * m[13] -
			m[12] * m[1] * m[6] +
			m[12] * m[2] * m[5];

		inv[3] = -m[1] * m[6] * m[11] +
			m[1] * m[7] * m[10] +
			m[5] * m[2] * m[11] -
			m[5] * m[3] * m[10] -
			m[9] * m[2] * m[7] +
			m[9] * m[3] * m[6];

		inv[7] = m[0] * m[6] * m[11] -
			m[0] * m[7] * m[10] -
			m[4] * m[2] * m[11] +
			m[4] * m[3] * m[10] +
			m[8] * m[2] * m[7] -
			m[8] * m[3] * m[6];

		inv[11] = -m[0] * m[5] * m[11] +
			m[0] * m[7] * m[9] +
			m[4] * m[1] * m[11] -
			m[4] * m[3] * m[9] -
			m[8] * m[1] * m[7] +
			m[8] * m[3] * m[5];

		inv[15] = m[0] * m[5] * m[10] -
			m[0] * m[6] * m[9] -
			m[4] * m[1] * m[10] +
			m[4] * m[2] * m[9] +
			m[8] * m[1] * m[6] -
			m[8] * m[2] * m[5];

		float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
		if (det == 0.0f)
			return false;

		det = 1.0f / det;
		for (int i = 0; i < 16; ++i)
			out->m[i] = inv[i] * det;

		return true;
	}

	static Mat4 Multiply(const Mat4& a, const Mat4& b)
	{
		Mat4 r{};

		for (int col = 0; col < 4; ++col)
		{
			for (int row = 0; row < 4; ++row)
			{
				r.m[col * 4 + row] =
					a.m[0 * 4 + row] * b.m[col * 4 + 0] +
					a.m[1 * 4 + row] * b.m[col * 4 + 1] +
					a.m[2 * 4 + row] * b.m[col * 4 + 2] +
					a.m[3 * 4 + row] * b.m[col * 4 + 3];
			}
		}

		return r;
	}

	static Mat4 Translation(float x, float y, float z)
	{
		Mat4 r = Identity();
		r.m[12] = x;
		r.m[13] = y;
		r.m[14] = z;
		return r;
	}

	static Mat4 Scale(float x, float y, float z)
	{
		Mat4 r{};
		r.m[0] = x;
		r.m[5] = y;
		r.m[10] = z;
		r.m[15] = 1.0f;
		return r;
	}

	static Mat4 RotationAxisDeg(float angleDeg, float x, float y, float z)
	{
		float len = sqrtf(x * x + y * y + z * z);
		if (len <= 0.000001f)
			return Identity();

		x /= len;
		y /= len;
		z /= len;

		const float rad = angleDeg * 3.1415926535f / 180.0f;
		const float c = cosf(rad);
		const float s = sinf(rad);
		const float t = 1.0f - c;

		Mat4 r = Identity();
		r.m[0] = t * x * x + c;
		r.m[1] = t * x * y + s * z;
		r.m[2] = t * x * z - s * y;
		r.m[4] = t * x * y - s * z;
		r.m[5] = t * y * y + c;
		r.m[6] = t * y * z + s * x;
		r.m[8] = t * x * z + s * y;
		r.m[9] = t * y * z - s * x;
		r.m[10] = t * z * z + c;
		return r;
	}

	static Mat4 Ortho(double left, double right, double bottom, double top, double zNear, double zFar)
	{
		Mat4 r{};
		r.m[0] = (float)(2.0 / (right - left));
		r.m[5] = (float)(2.0 / (top - bottom));
		r.m[10] = (float)(1.0 / (zFar - zNear));
		r.m[12] = (float)(-(right + left) / (right - left));
		r.m[13] = (float)(-(top + bottom) / (top - bottom));
		r.m[14] = (float)(-zNear / (zFar - zNear));
		r.m[15] = 1.0f;
		return r;
	}
};

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

#ifndef WINGDIAPI
#define WINGDIAPI __declspec(dllimport)
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef ptrdiff_t GLsizeiptr;
typedef intptr_t GLintptr;
typedef char GLchar;

#ifndef GL_FALSE
#define GL_FALSE 0
#endif
#ifndef GL_TRUE
#define GL_TRUE 1
#endif

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif

#ifndef GL_VENDOR
#define GL_VENDOR 0x1F00
#endif
#ifndef GL_RENDERER
#define GL_RENDERER 0x1F01
#endif
#ifndef GL_VERSION
#define GL_VERSION 0x1F02
#endif
#ifndef GL_EXTENSIONS
#define GL_EXTENSIONS 0x1F03
#endif

#ifndef GL_DONT_CARE
#define GL_DONT_CARE 0x1100
#endif

#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif

#ifndef GL_SAMPLES_PASSED
#define GL_SAMPLES_PASSED 0x8914
#endif
#ifndef GL_QUERY_RESULT
#define GL_QUERY_RESULT 0x8866
#endif
#ifndef GL_QUERY_RESULT_AVAILABLE
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#endif
#ifndef GL_CURRENT_QUERY
#define GL_CURRENT_QUERY 0x8865
#endif

#ifndef GL_POINT_SIZE
#define GL_POINT_SIZE 0x0B11
#endif
#ifndef GL_POINT_SMOOTH
#define GL_POINT_SMOOTH 0x0B10
#endif
#ifndef GL_POINT_SIZE_MIN_EXT
#define GL_POINT_SIZE_MIN_EXT 0x8126
#endif
#ifndef GL_POINT_SIZE_MAX_EXT
#define GL_POINT_SIZE_MAX_EXT 0x8127
#endif
#ifndef GL_POINT_FADE_THRESHOLD_SIZE_EXT
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT 0x8128
#endif
#ifndef GL_DISTANCE_ATTENUATION_EXT
#define GL_DISTANCE_ATTENUATION_EXT 0x8129
#endif

#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D 0x0DE0
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D 0x806F
#endif

#ifndef GL_TEXTURE_ENV
#define GL_TEXTURE_ENV 0x2300
#endif
#ifndef GL_TEXTURE_ENV_MODE
#define GL_TEXTURE_ENV_MODE 0x2200
#endif
#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif
#ifndef GL_REPLACE
#define GL_REPLACE 0x1E01
#endif
#ifndef GL_DECAL
#define GL_DECAL 0x2101
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_ADD
#define GL_ADD 0x0104
#endif
#ifndef GL_COMBINE_EXT
#define GL_COMBINE_EXT 0x8570
#endif


#ifndef GL_TEXTURE_ENV_COLOR
#define GL_TEXTURE_ENV_COLOR 0x2201
#endif
#ifndef GL_COMBINE_ARB
#define GL_COMBINE_ARB 0x8570
#endif
#ifndef GL_COMBINE_RGB_ARB
#define GL_COMBINE_RGB_ARB 0x8571
#endif
#ifndef GL_COMBINE_ALPHA_ARB
#define GL_COMBINE_ALPHA_ARB 0x8572
#endif
#ifndef GL_RGB_SCALE_ARB
#define GL_RGB_SCALE_ARB 0x8573
#endif
#ifndef GL_ADD_SIGNED_ARB
#define GL_ADD_SIGNED_ARB 0x8574
#endif
#ifndef GL_INTERPOLATE_ARB
#define GL_INTERPOLATE_ARB 0x8575
#endif
#ifndef GL_CONSTANT_ARB
#define GL_CONSTANT_ARB 0x8576
#endif
#ifndef GL_PRIMARY_COLOR_ARB
#define GL_PRIMARY_COLOR_ARB 0x8577
#endif
#ifndef GL_PREVIOUS_ARB
#define GL_PREVIOUS_ARB 0x8578
#endif
#ifndef GL_SOURCE0_RGB_ARB
#define GL_SOURCE0_RGB_ARB 0x8580
#endif
#ifndef GL_SOURCE1_RGB_ARB
#define GL_SOURCE1_RGB_ARB 0x8581
#endif
#ifndef GL_SOURCE2_RGB_ARB
#define GL_SOURCE2_RGB_ARB 0x8582
#endif
#ifndef GL_SOURCE0_ALPHA_ARB
#define GL_SOURCE0_ALPHA_ARB 0x8588
#endif
#ifndef GL_SOURCE1_ALPHA_ARB
#define GL_SOURCE1_ALPHA_ARB 0x8589
#endif
#ifndef GL_SOURCE2_ALPHA_ARB
#define GL_SOURCE2_ALPHA_ARB 0x858A
#endif
#ifndef GL_OPERAND0_RGB_ARB
#define GL_OPERAND0_RGB_ARB 0x8590
#endif
#ifndef GL_OPERAND1_RGB_ARB
#define GL_OPERAND1_RGB_ARB 0x8591
#endif
#ifndef GL_OPERAND2_RGB_ARB
#define GL_OPERAND2_RGB_ARB 0x8592
#endif
#ifndef GL_OPERAND0_ALPHA_ARB
#define GL_OPERAND0_ALPHA_ARB 0x8598
#endif
#ifndef GL_OPERAND1_ALPHA_ARB
#define GL_OPERAND1_ALPHA_ARB 0x8599
#endif
#ifndef GL_OPERAND2_ALPHA_ARB
#define GL_OPERAND2_ALPHA_ARB 0x859A
#endif
#ifndef GL_ALPHA_SCALE
#define GL_ALPHA_SCALE 0x0D1C
#endif

#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_CULL_FACE
#define GL_CULL_FACE 0x0B44
#endif
#ifndef GL_SCISSOR_TEST
#define GL_SCISSOR_TEST 0x0C11
#endif
#ifndef GL_STENCIL_TEST
#define GL_STENCIL_TEST 0x0B90
#endif

#ifndef GL_DEPTH_BOUNDS_TEST_EXT
#define GL_DEPTH_BOUNDS_TEST_EXT 0x8890
#endif
#ifndef GL_DEPTH_BOUNDS_EXT
#define GL_DEPTH_BOUNDS_EXT 0x8891
#endif
#ifndef GL_STENCIL_TEST_TWO_SIDE_EXT
#define GL_STENCIL_TEST_TWO_SIDE_EXT 0x8910
#endif
#ifndef GL_ACTIVE_STENCIL_FACE_EXT
#define GL_ACTIVE_STENCIL_FACE_EXT 0x8911
#endif

#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif
#ifndef GL_POLYGON_OFFSET_LINE
#define GL_POLYGON_OFFSET_LINE 0x2A02
#endif
#ifndef GL_CLIP_PLANE0
#define GL_CLIP_PLANE0 0x3000
#endif

#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_FOG_INDEX
#define GL_FOG_INDEX 0x0B61
#endif
#ifndef GL_FOG_DENSITY
#define GL_FOG_DENSITY 0x0B62
#endif
#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#endif
#ifndef GL_FOG_END
#define GL_FOG_END 0x0B64
#endif
#ifndef GL_FOG_MODE
#define GL_FOG_MODE 0x0B65
#endif
#ifndef GL_FOG_COLOR
#define GL_FOG_COLOR 0x0B66
#endif
#ifndef GL_FOG_HINT
#define GL_FOG_HINT 0x0C54
#endif
#ifndef GL_EXP
#define GL_EXP 0x0800
#endif
#ifndef GL_EXP2
#define GL_EXP2 0x0801
#endif


#ifndef GL_CW
#define GL_CW 0x0900
#endif
#ifndef GL_CCW
#define GL_CCW 0x0901
#endif

#ifndef GL_FRONT
#define GL_FRONT 0x0404
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif
#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT 0x0402
#endif
#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT 0x0403
#endif

#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif
#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif
#ifndef GL_FLAT
#define GL_FLAT 0x1D00
#endif
#ifndef GL_SMOOTH
#define GL_SMOOTH 0x1D01
#endif

#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_STENCIL_BUFFER_BIT
#define GL_STENCIL_BUFFER_BIT 0x00000400
#endif
#ifndef GL_ACCUM_BUFFER_BIT
#define GL_ACCUM_BUFFER_BIT 0x00000200
#endif

#ifndef GL_POINTS
#define GL_POINTS 0x0000
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP 0x0002
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_QUAD_STRIP
#define GL_QUAD_STRIP 0x0008
#endif
#ifndef GL_POLYGON
#define GL_POLYGON 0x0009
#endif

#ifndef GL_NEVER
#define GL_NEVER 0x0200
#endif
#ifndef GL_LESS
#define GL_LESS 0x0201
#endif
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
#ifndef GL_LEQUAL
#define GL_LEQUAL 0x0203
#endif
#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif
#ifndef GL_NOTEQUAL
#define GL_NOTEQUAL 0x0205
#endif
#ifndef GL_GEQUAL
#define GL_GEQUAL 0x0206
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif

#ifndef GL_ZERO
#define GL_ZERO 0
#endif
#ifndef GL_ONE
#define GL_ONE 1
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif
#ifndef GL_DST_ALPHA
#define GL_DST_ALPHA 0x0304
#endif
#ifndef GL_ONE_MINUS_DST_ALPHA
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_SRC_ALPHA_SATURATE
#define GL_SRC_ALPHA_SATURATE 0x0308
#endif


#ifndef GL_INVERT
#define GL_INVERT 0x150A
#endif
#ifndef GL_INCR_WRAP
#define GL_INCR_WRAP 0x8507
#endif
#ifndef GL_DECR_WRAP
#define GL_DECR_WRAP 0x8508
#endif
#ifndef GL_INCR_WRAP_EXT
#define GL_INCR_WRAP_EXT 0x8507
#endif
#ifndef GL_DECR_WRAP_EXT
#define GL_DECR_WRAP_EXT 0x8508
#endif
#ifndef GL_INCR_WRAP_ATI
#define GL_INCR_WRAP_ATI 0x8507
#endif
#ifndef GL_DECR_WRAP_ATI
#define GL_DECR_WRAP_ATI 0x8508
#endif

#ifndef GL_KEEP
#define GL_KEEP 0x1E00
#endif
#ifndef GL_INCR
#define GL_INCR 0x1E02
#endif
#ifndef GL_DECR
#define GL_DECR 0x1E03
#endif
#ifndef GL_INCR_WRAP_EXT
#define GL_INCR_WRAP_EXT 0x8507
#endif
#ifndef GL_DECR_WRAP_EXT
#define GL_DECR_WRAP_EXT 0x8508
#endif

#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_NEAREST_MIPMAP_NEAREST
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#endif
#ifndef GL_LINEAR_MIPMAP_NEAREST
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#endif
#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif
#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R 0x8072
#endif
#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_BYTE
#define GL_BYTE 0x1400
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_SHORT
#define GL_SHORT 0x1402
#endif
#ifndef GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT 0x1403
#endif
#ifndef GL_INT
#define GL_INT 0x1404
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif

#ifndef GL_COLOR_INDEX
#define GL_COLOR_INDEX 0x1900
#endif
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x1901
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT 0x1902
#endif
#ifndef GL_ALPHA
#define GL_ALPHA 0x1906
#endif
#ifndef GL_RGB
#define GL_RGB 0x1907
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif
#ifndef GL_LUMINANCE_ALPHA
#define GL_LUMINANCE_ALPHA 0x190A
#endif
#ifndef GL_INTENSITY
#define GL_INTENSITY 0x8049
#endif

#ifndef GL_RGB4_S3TC
#define GL_RGB4_S3TC 0x83A1
#endif
#ifndef GL_RGB5
#define GL_RGB5 0x8050
#endif
#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif
#ifndef GL_RGBA4
#define GL_RGBA4 0x8056
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_INTENSITY8
#define GL_INTENSITY8 0x804B
#endif
#ifndef GL_LUMINANCE8
#define GL_LUMINANCE8 0x8040
#endif
#ifndef GL_LUMINANCE8_ALPHA8
#define GL_LUMINANCE8_ALPHA8 0x8045
#endif
#ifndef GL_ALPHA8
#define GL_ALPHA8 0x803C
#endif

#ifndef GL_SHARED_TEXTURE_PALETTE_EXT
#define GL_SHARED_TEXTURE_PALETTE_EXT 0x81FB
#endif
#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT 0x80E5
#endif

#ifndef GL_BGR_EXT
#define GL_BGR_EXT 0x80E0
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

#ifndef GL_COMPRESSED_RGB_ARB
#define GL_COMPRESSED_RGB_ARB 0x84ED
#endif
#ifndef GL_COMPRESSED_RGBA_ARB
#define GL_COMPRESSED_RGBA_ARB 0x84EE
#endif
#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT 0x0001
#endif
#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
#endif
#ifndef GL_MAP_INVALIDATE_RANGE_BIT
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#endif
#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif
#ifndef GL_MAP_FLUSH_EXPLICIT_BIT
#define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
#endif
#ifndef GL_MAP_UNSYNCHRONIZED_BIT
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020
#endif

#ifndef GL_PERSPECTIVE_CORRECTION_HINT
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#endif
#ifndef GL_FASTEST
#define GL_FASTEST 0x1101
#endif
#ifndef GL_NICEST
#define GL_NICEST 0x1102
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
#ifndef GL_MAX_TEXTURE_SIZE
#define GL_MAX_TEXTURE_SIZE 0x0D33
#endif
#ifndef GL_VIEWPORT
#define GL_VIEWPORT 0x0BA2
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif

#ifndef GL_NO_ERROR
#define GL_NO_ERROR 0
#endif
#ifndef GL_INVALID_ENUM
#define GL_INVALID_ENUM 0x0500
#endif
#ifndef GL_INVALID_VALUE
#define GL_INVALID_VALUE 0x0501
#endif
#ifndef GL_INVALID_OPERATION
#define GL_INVALID_OPERATION 0x0502
#endif
#ifndef GL_STACK_OVERFLOW
#define GL_STACK_OVERFLOW 0x0503
#endif
#ifndef GL_STACK_UNDERFLOW
#define GL_STACK_UNDERFLOW 0x0504
#endif
#ifndef GL_OUT_OF_MEMORY
#define GL_OUT_OF_MEMORY 0x0505
#endif

#ifndef GL_VERTEX_ARRAY
#define GL_VERTEX_ARRAY 0x8074
#endif
#ifndef GL_NORMAL_ARRAY
#define GL_NORMAL_ARRAY 0x8075
#endif
#ifndef GL_COLOR_ARRAY
#define GL_COLOR_ARRAY 0x8076
#endif
#ifndef GL_TEXTURE_COORD_ARRAY
#define GL_TEXTURE_COORD_ARRAY 0x8078
#endif

#ifndef GL_TEXTURE0_ARB
#define GL_TEXTURE0_ARB 0x84C0
#endif
#ifndef GL_TEXTURE1_ARB
#define GL_TEXTURE1_ARB 0x84C1
#endif
#ifndef GL_TEXTURE2_ARB
#define GL_TEXTURE2_ARB 0x84C2
#endif
#ifndef GL_TEXTURE3_ARB
#define GL_TEXTURE3_ARB 0x84C3
#endif
#ifndef GL_TEXTURE4_ARB
#define GL_TEXTURE4_ARB 0x84C4
#endif
#ifndef GL_TEXTURE5_ARB
#define GL_TEXTURE5_ARB 0x84C5
#endif
#ifndef GL_TEXTURE6_ARB
#define GL_TEXTURE6_ARB 0x84C6
#endif
#ifndef GL_TEXTURE7_ARB
#define GL_TEXTURE7_ARB 0x84C7
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_TEXTURE3
#define GL_TEXTURE3 0x84C3
#endif
#ifndef GL_TEXTURE4
#define GL_TEXTURE4 0x84C4
#endif
#ifndef GL_TEXTURE5
#define GL_TEXTURE5 0x84C5
#endif
#ifndef GL_TEXTURE6
#define GL_TEXTURE6 0x84C6
#endif
#ifndef GL_TEXTURE7
#define GL_TEXTURE7 0x84C7
#endif

#ifndef GL_ACTIVE_TEXTURE_ARB
#define GL_ACTIVE_TEXTURE_ARB 0x84E0
#endif
#ifndef GL_CLIENT_ACTIVE_TEXTURE_ARB
#define GL_CLIENT_ACTIVE_TEXTURE_ARB 0x84E1
#endif
#ifndef GL_MAX_TEXTURE_UNITS_ARB
#define GL_MAX_TEXTURE_UNITS_ARB 0x84E2
#endif
#ifndef GL_MAX_ACTIVE_TEXTURES_ARB
#define GL_MAX_ACTIVE_TEXTURES_ARB 0x84E2
#endif

#ifndef GL_LINE_STIPPLE
#define GL_LINE_STIPPLE 0x0B24
#endif
#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_LIGHT_MODEL_AMBIENT
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#endif
#ifndef GL_CURRENT_RASTER_POSITION
#define GL_CURRENT_RASTER_POSITION 0x0B07
#endif
#ifndef GL_CURRENT_COLOR
#define GL_CURRENT_COLOR 0x0B00
#endif

#ifndef GL_CURRENT_BIT
#define GL_CURRENT_BIT 0x00000001
#endif
#ifndef GL_COMPILE
#define GL_COMPILE 0x1300
#endif
#ifndef GL_COMPILE_AND_EXECUTE
#define GL_COMPILE_AND_EXECUTE 0x1301
#endif
#ifndef GL_COLOR
#define GL_COLOR 0x1800
#endif
#ifndef GL_ALL_ATTRIB_BITS
#define GL_ALL_ATTRIB_BITS 0xFFFFFFFF
#endif

#ifndef GL_TEXTURE0_SGIS
#define GL_TEXTURE0_SGIS 0x835E
#endif
#ifndef GL_TEXTURE1_SGIS
#define GL_TEXTURE1_SGIS 0x835F
#endif
#ifndef GL_TEXTURE2_SGIS
#define GL_TEXTURE2_SGIS 0x8360
#endif
#ifndef GL_TEXTURE3_SGIS
#define GL_TEXTURE3_SGIS 0x8361
#endif
#ifndef GL_SELECTED_TEXTURE_SGIS
#define GL_SELECTED_TEXTURE_SGIS 0x835C
#endif
#ifndef GL_MAX_TEXTURES_SGIS
#define GL_MAX_TEXTURES_SGIS 0x83B9
#endif
#ifndef GL_SGIS_multitexture
#define GL_SGIS_multitexture 1
#endif

#ifndef GL_S
#define GL_S 0x2000
#endif
#ifndef GL_T
#define GL_T 0x2001
#endif
#ifndef GL_R
#define GL_R 0x2002
#endif
#ifndef GL_Q
#define GL_Q 0x2003
#endif

#ifndef GL_TEXTURE_GEN_S
#define GL_TEXTURE_GEN_S 0x0C60
#endif
#ifndef GL_TEXTURE_GEN_T
#define GL_TEXTURE_GEN_T 0x0C61
#endif
#ifndef GL_TEXTURE_GEN_R
#define GL_TEXTURE_GEN_R 0x0C62
#endif
#ifndef GL_TEXTURE_GEN_Q
#define GL_TEXTURE_GEN_Q 0x0C63
#endif

#ifndef GL_TEXTURE_GEN_MODE
#define GL_TEXTURE_GEN_MODE 0x2500
#endif
#ifndef GL_OBJECT_PLANE
#define GL_OBJECT_PLANE 0x2501
#endif
#ifndef GL_EYE_PLANE
#define GL_EYE_PLANE 0x2502
#endif
#ifndef GL_EYE_LINEAR
#define GL_EYE_LINEAR 0x2400
#endif
#ifndef GL_OBJECT_LINEAR
#define GL_OBJECT_LINEAR 0x2401
#endif
#ifndef GL_SPHERE_MAP
#define GL_SPHERE_MAP 0x2402
#endif

#ifndef GL_TEXTURE_CUBE_MAP_EXT
#define GL_TEXTURE_CUBE_MAP_EXT 0x8513
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT 0x8515
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT 0x8516
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT 0x8517
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT 0x8518
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT 0x8519
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT 0x851A
#endif
#ifndef GL_REFLECTION_MAP_EXT
#define GL_REFLECTION_MAP_EXT 0x8512
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_TEXTURE_LOD_BIAS_EXT
#define GL_TEXTURE_LOD_BIAS_EXT 0x8501
#endif

#ifndef GL_ARRAY_BUFFER_ARB
#define GL_ARRAY_BUFFER_ARB 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER_ARB
#define GL_ELEMENT_ARRAY_BUFFER_ARB 0x8893
#endif
#ifndef GL_STATIC_DRAW_ARB
#define GL_STATIC_DRAW_ARB 0x88E4
#endif
#ifndef GL_STREAM_DRAW_ARB
#define GL_STREAM_DRAW_ARB 0x88E0
#endif

#ifndef GL_READ_ONLY
#define GL_READ_ONLY   0x88B8
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY  0x88B9
#endif

#ifndef GL_READ_WRITE
#define GL_READ_WRITE  0x88BA
#endif

#ifndef GLsizeiptrARB
typedef intptr_t GLsizeiptrARB;
#endif
#ifndef GLintptrARB
typedef intptr_t GLintptrARB;
#endif


#ifndef GL_VERTEX_PROGRAM_ARB
#define GL_VERTEX_PROGRAM_ARB 0x8620
#endif
#ifndef GL_FRAGMENT_PROGRAM_ARB
#define GL_FRAGMENT_PROGRAM_ARB 0x8804
#endif
#ifndef GL_PROGRAM_FORMAT_ASCII_ARB
#define GL_PROGRAM_FORMAT_ASCII_ARB 0x8875
#endif
#ifndef GL_PROGRAM_LENGTH_ARB
#define GL_PROGRAM_LENGTH_ARB 0x8627
#endif
#ifndef GL_PROGRAM_FORMAT_ARB
#define GL_PROGRAM_FORMAT_ARB 0x8876
#endif
#ifndef GL_PROGRAM_BINDING_ARB
#define GL_PROGRAM_BINDING_ARB 0x8677
#endif
#ifndef GL_PROGRAM_ERROR_POSITION_ARB
#define GL_PROGRAM_ERROR_POSITION_ARB 0x864B
#endif
#ifndef GL_PROGRAM_ERROR_STRING_ARB
#define GL_PROGRAM_ERROR_STRING_ARB 0x8874
#endif
#ifndef GL_PROGRAM_STRING_ARB
#define GL_PROGRAM_STRING_ARB 0x8628
#endif
#ifndef GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#endif
#ifndef GL_MAX_PROGRAM_ENV_PARAMETERS_ARB
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#endif
#ifndef GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#endif
#ifndef GL_MAX_VERTEX_ATTRIBS_ARB
#define GL_MAX_VERTEX_ATTRIBS_ARB 0x8869
#endif
#ifndef GL_MAX_TEXTURE_COORDS_ARB
#define GL_MAX_TEXTURE_COORDS_ARB 0x8871
#endif
#ifndef GL_MAX_TEXTURE_IMAGE_UNITS_ARB
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB 0x8872
#endif
#ifndef GL_VERTEX_PROGRAM_BINDING_ARB
#define GL_VERTEX_PROGRAM_BINDING_ARB 0x864A
#endif
#ifndef GL_FRAGMENT_PROGRAM_BINDING_ARB
#define GL_FRAGMENT_PROGRAM_BINDING_ARB 0x8873
#endif
#ifndef GL_MAX_PROGRAM_MATRICES_ARB
#define GL_MAX_PROGRAM_MATRICES_ARB 0x862F
#endif
#ifndef GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#endif
#ifndef GL_MAX_PROGRAM_INSTRUCTIONS_ARB
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB 0x88A1
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#endif
#ifndef GL_MAX_PROGRAM_TEMPORARIES_ARB
#define GL_MAX_PROGRAM_TEMPORARIES_ARB 0x88A4
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#endif
#ifndef GL_MAX_PROGRAM_PARAMETERS_ARB
#define GL_MAX_PROGRAM_PARAMETERS_ARB 0x88A8
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AA
#endif
#ifndef GL_MAX_PROGRAM_ATTRIBS_ARB
#define GL_MAX_PROGRAM_ATTRIBS_ARB 0x88AD
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#endif
#ifndef GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#endif
#ifndef GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#endif

typedef void* QD3D12_HGLRC;

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
typedef BOOL(WINAPI* PFNWGLGETDEVICEGAMMARAMP3DFXPROC)(HDC hDC, LPVOID ramp);
typedef BOOL(WINAPI* PFNWGLSETDEVICEGAMMARAMP3DFXPROC)(HDC hDC, LPVOID ramp);

typedef void (APIENTRY* PFNGLMULTITEXCOORD2FARBPROC)(GLenum texture, GLfloat s, GLfloat t);
typedef void (APIENTRY* PFNGLACTIVETEXTUREARBPROC)(GLenum texture);
typedef void (APIENTRY* PFNGLCLIENTACTIVETEXTUREARBPROC)(GLenum texture);
typedef void (APIENTRY* PFNGLLOCKARRAYSEXTPROC)(GLint first, GLsizei count);
typedef void (APIENTRY* PFNGLUNLOCKARRAYSEXTPROC)(void);
typedef void (APIENTRY* PFNGLDEPTHBOUNDSEXTPROC)(GLclampd zmin, GLclampd zmax);
typedef void (APIENTRY* PFNGLACTIVESTENCILFACEEXTPROC)(GLenum face);
typedef void (APIENTRY* PFNGLSTENCILOPSEPARATEATIPROC)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
typedef void (APIENTRY* PFNGLSTENCILFUNCSEPARATEATIPROC)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);

typedef void (APIENTRY* PFNGLGENPROGRAMSARBPROC)(GLsizei n, GLuint* programs);
typedef void (APIENTRY* PFNGLDELETEPROGRAMSARBPROC)(GLsizei n, const GLuint* programs);
typedef void (APIENTRY* PFNGLBINDPROGRAMARBPROC)(GLenum target, GLuint program);
typedef void (APIENTRY* PFNGLPROGRAMSTRINGARBPROC)(GLenum target, GLenum format, GLsizei len, const GLvoid* string);
typedef void (APIENTRY* PFNGLPROGRAMENVPARAMETER4FARBPROC)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY* PFNGLPROGRAMENVPARAMETER4FVARBPROC)(GLenum target, GLuint index, const GLfloat* params);
typedef void (APIENTRY* PFNGLPROGRAMLOCALPARAMETER4FARBPROC)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY* PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)(GLenum target, GLuint index, const GLfloat* params);
typedef void (APIENTRY* PFNGLGETPROGRAMENVPARAMETERFVARBPROC)(GLenum target, GLuint index, GLfloat* params);
typedef void (APIENTRY* PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC)(GLenum target, GLuint index, GLfloat* params);
typedef void (APIENTRY* PFNGLGETPROGRAMIVARBPROC)(GLenum target, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETPROGRAMSTRINGARBPROC)(GLenum target, GLenum pname, GLvoid* string);
typedef GLboolean(APIENTRY* PFNGLISPROGRAMARBPROC)(GLuint program);

QD3D12_HGLRC WINAPI qd3d12_wglCreateContext(HDC hdc);
BOOL         WINAPI qd3d12_wglMakeCurrent(HDC hdc, QD3D12_HGLRC hglrc);
PROC         WINAPI qd3d12_wglGetProcAddress(LPCSTR name);
HDC          WINAPI qd3d12_wglGetCurrentDC(void);
QD3D12_HGLRC WINAPI qd3d12_wglGetCurrentContext(void);
BOOL         WINAPI qd3d12_wglDeleteContext(QD3D12_HGLRC hglrc);
void               QD3D12_SwapBuffers(HDC hdc);

int  WINAPI qd3d12_wglDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd);
BOOL WINAPI qd3d12_wglSetPixelFormat(HDC hdc, int format, const PIXELFORMATDESCRIPTOR* ppfd);
BOOL WINAPI qd3d12_wglSwapIntervalEXT(int interval);
BOOL WINAPI qd3d12_wglGetDeviceGammaRamp3DFX(HDC hdc, LPVOID ramp);
BOOL WINAPI qd3d12_wglSetDeviceGammaRamp3DFX(HDC hdc, LPVOID ramp);

#ifndef QD3D12_NO_WGL_REDIRECTS
#define wglCreateContext          qd3d12_wglCreateContext
#define wglMakeCurrent            qd3d12_wglMakeCurrent
#define wglGetProcAddress         qd3d12_wglGetProcAddress
#define wglGetCurrentDC           qd3d12_wglGetCurrentDC
#define wglGetCurrentContext      qd3d12_wglGetCurrentContext
#define wglDeleteContext          qd3d12_wglDeleteContext
#define wglDescribePixelFormat    qd3d12_wglDescribePixelFormat
#define wglSetPixelFormat         qd3d12_wglSetPixelFormat
#define wglSwapIntervalEXT        qd3d12_wglSwapIntervalEXT
#define wglGetDeviceGammaRamp3DFX qd3d12_wglGetDeviceGammaRamp3DFX
#define wglSetDeviceGammaRamp3DFX qd3d12_wglSetDeviceGammaRamp3DFX
#define SwapBuffers(x)            QD3D12_SwapBuffers(x)
#endif

#ifndef QD3D12_NO_GL_PROTOTYPES
BOOL WINAPI wglSwapBuffers(HDC hdc);

void APIENTRY glFlush(void);
void APIENTRY glFinish(void);

void APIENTRY glPixelZoom(GLfloat xfactor, GLfloat yfactor);
void APIENTRY glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);

void APIENTRY glFrontFace(GLenum mode);
void APIENTRY glCullFace(GLenum mode);
void APIENTRY glPolygonMode(GLenum face, GLenum mode);
void APIENTRY glShadeModel(GLenum mode);
void APIENTRY glHint(GLenum target, GLenum mode);
void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units);
void APIENTRY glPolygonStipple(const GLubyte* mask);

void APIENTRY glEnable(GLenum cap);
void APIENTRY glDisable(GLenum cap);
GLboolean APIENTRY glIsEnabled(GLenum cap);

void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor);
void APIENTRY glAlphaFunc(GLenum func, GLclampf ref);
void APIENTRY glDepthFunc(GLenum func);
void APIENTRY glDepthMask(GLboolean flag);
void APIENTRY glDepthRange(GLclampd zNear, GLclampd zFar);
void APIENTRY glClearDepth(GLclampd depth);
void APIENTRY glClearStencil(GLint s);
void APIENTRY glStencilMask(GLuint mask);
void APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask);
void APIENTRY glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void APIENTRY glDepthBoundsEXT(GLclampd zmin, GLclampd zmax);
void APIENTRY glActiveStencilFaceEXT(GLenum face);
void APIENTRY glStencilOpSeparateATI(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
void APIENTRY glStencilFuncSeparateATI(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
void APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask);
void APIENTRY glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);

void APIENTRY glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void APIENTRY glClear(GLbitfield mask);
void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
GLenum APIENTRY glGetError(void);
const GLubyte* APIENTRY glGetString(GLenum name);
void APIENTRY glGetFloatv(GLenum pname, GLfloat* params);
void APIENTRY glGetDoublev(GLenum pname, GLdouble* params);
void APIENTRY glGetIntegerv(GLenum pname, GLint* params);

void APIENTRY glMatrixMode(GLenum mode);
void APIENTRY glLoadIdentity(void);
void APIENTRY glPushMatrix(void);
void APIENTRY glPopMatrix(void);
void APIENTRY glLoadMatrixf(const GLfloat* m);
void APIENTRY glLoadModelMatrixf(const float* m16);
void APIENTRY glMultMatrixf(const GLfloat* m);
void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void APIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z);
void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void APIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z);
void APIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void APIENTRY glClipPlane(GLenum plane, const GLdouble* equation);

void APIENTRY glBegin(GLenum mode);
void APIENTRY glEnd(void);
void APIENTRY glVertex2f(GLfloat x, GLfloat y);
void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void APIENTRY glVertex3fv(const GLfloat* v);
void APIENTRY glNormal3f(GLfloat x, GLfloat y, GLfloat z);
void APIENTRY glNormal3fv(const GLfloat* v);
void APIENTRY glTexCoord2f(GLfloat s, GLfloat t);
void APIENTRY glTexCoord2fv(const GLfloat* v);
void APIENTRY glColor3f(GLfloat r, GLfloat g, GLfloat b);
void APIENTRY glColor3fv(const GLfloat* v);
void APIENTRY glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void APIENTRY glColor4fv(const GLfloat* v);
void APIENTRY glColor4ubv(const GLubyte* v);
void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void APIENTRY glRasterPos2f(GLfloat x, GLfloat y);
void APIENTRY glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);
void APIENTRY glRasterPos3fv(const GLfloat* v);

void APIENTRY glPushAttrib(GLbitfield mask);
void APIENTRY glPopAttrib(void);

GLuint APIENTRY glGenLists(GLsizei range);
void APIENTRY glNewList(GLuint list, GLenum mode);
void APIENTRY glEndList(void);
void APIENTRY glCallList(GLuint list);
void APIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid* lists);
void APIENTRY glDeleteLists(GLuint list, GLsizei range);
void APIENTRY glListBase(GLuint base);

void APIENTRY glLineWidth(GLfloat width);
void APIENTRY glLineStipple(GLint factor, GLushort pattern);

void APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat* params);
void APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param);
void APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat* params);
void APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param);
void APIENTRY glLightModelfv(GLenum pname, const GLfloat* params);

void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);
void APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params);
void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params);

void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param);
void APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint* params);
void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat* params);

void APIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param);
void APIENTRY glTexGeniv(GLenum coord, GLenum pname, const GLint* params);
void APIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param);
void APIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat* params);

void APIENTRY glGenTextures(GLsizei n, GLuint* textures);
void APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures);
void APIENTRY glBindTexture(GLenum target, GLuint texture);
void APIENTRY glBindTextureEXT(GLenum target, GLuint texture);

void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalFormat,
	GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
	GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
void APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat,
	GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void APIENTRY glCopyTexSubImage2D(GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);

void APIENTRY glTexImage3D(GLenum target, GLint level, GLint internalFormat,
	GLsizei width, GLsizei height, GLsizei depth, GLint border,
	GLenum format, GLenum type, const GLvoid* pixels);
void APIENTRY glTexSubImage3D(GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLint zoffset,
	GLsizei width, GLsizei height, GLsizei depth,
	GLenum format, GLenum type, const GLvoid* pixels);

void APIENTRY glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width,
	GLenum format, GLenum type, const GLvoid* data);

void APIENTRY glDrawBuffer(GLenum mode);
void APIENTRY glReadBuffer(GLenum mode);
void APIENTRY glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data);

void APIENTRY glPointSize(GLfloat size);
void APIENTRY glPointParameterfEXT(GLenum pname, GLfloat param);
void APIENTRY glPointParameterfvEXT(GLenum pname, const GLfloat* params);

void APIENTRY glEnableClientState(GLenum array);
void APIENTRY glDisableClientState(GLenum array);
void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr);
void APIENTRY glNormalPointer(GLenum type, GLsizei stride, const void* pointer);
void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr);
void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr);
void APIENTRY glArrayElement(GLint i);
void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count);
void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);

void APIENTRY glVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
void APIENTRY glEnableVertexAttribArrayARB(GLuint index);
void APIENTRY glDisableVertexAttribArrayARB(GLuint index);

void APIENTRY glGenProgramsARB(GLsizei n, GLuint* programs);
void APIENTRY glDeleteProgramsARB(GLsizei n, const GLuint* programs);
void APIENTRY glBindProgramARB(GLenum target, GLuint program);
void APIENTRY glProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid* string);
void APIENTRY glProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void APIENTRY glProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat* params);
void APIENTRY glProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void APIENTRY glProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat* params);
void APIENTRY glGetProgramEnvParameterfvARB(GLenum target, GLuint index, GLfloat* params);
void APIENTRY glGetProgramLocalParameterfvARB(GLenum target, GLuint index, GLfloat* params);
void APIENTRY glGetProgramivARB(GLenum target, GLenum pname, GLint* params);
void APIENTRY glGetProgramStringARB(GLenum target, GLenum pname, GLvoid* string);
GLboolean APIENTRY glIsProgramARB(GLuint program);

void APIENTRY glGenBuffers(GLsizei n, GLuint* buffers);
void APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers);
void APIENTRY glBindBuffer(GLenum target, GLuint buffer);
void APIENTRY glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags);
void APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
void* APIENTRY glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
GLboolean APIENTRY glUnmapBuffer(GLenum target);

void APIENTRY glGenBuffersARB(GLsizei n, GLuint* buffers);
void APIENTRY glDeleteBuffersARB(GLsizei n, const GLuint* buffers);
void APIENTRY glBindBufferARB(GLenum target, GLuint buffer);
void APIENTRY glBufferDataARB(GLenum target, GLsizeiptrARB size, const void* data, GLenum usage);
void APIENTRY glBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void* data);
void* APIENTRY glMapBufferARB(GLenum target, GLenum access);
GLboolean APIENTRY glUnmapBufferARB(GLenum target);

void APIENTRY glActiveTexture(GLenum texture);
void APIENTRY glClientActiveTexture(GLenum texture);
void APIENTRY glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t);

void APIENTRY glActiveTextureARB(GLenum texture);
void APIENTRY glClientActiveTextureARB(GLenum texture);
void APIENTRY glMultiTexCoord2fARB(GLenum texture, GLfloat s, GLfloat t);
void APIENTRY glSelectTextureSGIS(GLenum texture);
void APIENTRY glMTexCoord2fSGIS(GLenum texture, GLfloat s, GLfloat t);

void APIENTRY glLockArraysEXT(GLint first, GLsizei count);
void APIENTRY glUnlockArraysEXT(void);

void APIENTRY glFogiv(GLenum pname, const GLint* params);
void APIENTRY glFogfv(GLenum pname, const GLfloat* params);
void APIENTRY glFogi(GLenum pname, GLint param);
void APIENTRY glFogf(GLenum pname, GLfloat param);

void APIENTRY glGenQueries(GLsizei n, GLuint* ids);
void APIENTRY glDeleteQueries(GLsizei n, const GLuint* ids);
GLboolean APIENTRY glIsQuery(GLuint id);
void APIENTRY glBeginQuery(GLenum target, GLuint id);
void APIENTRY glEndQuery(GLenum target);
void APIENTRY glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params);
void APIENTRY glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params);

enum GeometryFlag_t
{
	GEOMETRY_FLAG_NONE = 0,
	GEOMETRY_FLAG_SKELETAL,
	GEOMETRY_FLAG_UNLIT
};

void APIENTRY glGeometryFlagf(GLfloat flag);

void glUpdateBottomAccelStructure(bool opaque, uint32_t& meshHandle);
void glUpdateTopLevelAceelStructure(uint32_t mesh, float* transform, uint32_t& topLevelHandle);
#endif

struct GLVertex
{
	float px, py, pz;
	float nx, ny, nz;
	float u0, v0;
	float u1, v1;
	float r, g, b, a;

	// Doom 3 ARB vertex programs conventionally pass tangent-space basis in
	// generic attributes 9/10/11. Keep these at the end so existing fixed
	// function offsets remain stable.
	float tx, ty, tz;
	float bx, by, bz;
};

struct glRaytracingVertex_t
{
	float xyz[3];
	float normal[3];
	float st[2];

	glRaytracingVertex_t() = default;

	glRaytracingVertex_t(const GLVertex& v)
	{
		xyz[0] = v.px;
		xyz[1] = v.py;
		xyz[2] = v.pz;

		normal[0] = v.nx;
		normal[1] = v.ny;
		normal[2] = v.nz;

		st[0] = v.u0;
		st[1] = v.v0;
	}

	glRaytracingVertex_t& operator=(const GLVertex& v)
	{
		xyz[0] = v.px;
		xyz[1] = v.py;
		xyz[2] = v.pz;

		normal[0] = v.nx;
		normal[1] = v.ny;
		normal[2] = v.nz;

		st[0] = v.u0;
		st[1] = v.v0;

		return *this;
	}
};

typedef struct glRaytracingMeshDesc_s
{
	const glRaytracingVertex_t* vertices;
	uint32_t                    vertexCount;
	const uint32_t* indices;
	uint32_t                    indexCount;
	int                         allowUpdate;
	int                         opaque;
} glRaytracingMeshDesc_t;

typedef struct glRaytracingInstanceDesc_s
{
	uint32_t meshHandle;
	uint32_t instanceID;
	uint32_t mask;
	float    transform[12];
} glRaytracingInstanceDesc_t;

typedef uint32_t glRaytracingMeshHandle_t;
typedef uint32_t glRaytracingInstanceHandle_t;

static const uint32_t GL_RAYTRACING_MAX_LIGHTS = 256;

typedef struct glRaytracingVec3_s
{
	float x, y, z;
} glRaytracingVec3_t;

typedef struct glRaytracingLight_s
{
	glRaytracingVec3_t position;
	float              radius;

	glRaytracingVec3_t color;
	float              intensity;

	glRaytracingVec3_t normal;
	uint32_t           type;

	glRaytracingVec3_t axisU;
	float              halfWidth;

	glRaytracingVec3_t axisV;
	float              halfHeight;

	uint32_t           samples;
	uint32_t           twoSided;
	float              persistant;
	float              pad1;
} glRaytracingLight_t;

typedef struct glRaytracingLightingPassDesc_s
{
	ID3D12Resource* albedoTexture;
	DXGI_FORMAT     albedoFormat;

	ID3D12Resource* depthTexture;
	DXGI_FORMAT     depthFormat;

	ID3D12Resource* normalTexture;
	DXGI_FORMAT     normalFormat;

	ID3D12Resource* positionTexture;
	DXGI_FORMAT     positionFormat;

	ID3D12Resource* outputTexture;
	DXGI_FORMAT     outputFormat;

	ID3D12Resource* topLevelAS;

	uint32_t        width;
	uint32_t        height;
} glRaytracingLightingPassDesc_t;

int                        glRaytracingInit(void);
void                       glRaytracingShutdown(void);
void                       glRaytracingClear(void);

glRaytracingMeshHandle_t   glRaytracingCreateMesh(const glRaytracingMeshDesc_t* desc);
int                        glRaytracingUpdateMesh(glRaytracingMeshHandle_t meshHandle, const glRaytracingMeshDesc_t* desc);
void                       glRaytracingDeleteMesh(glRaytracingMeshHandle_t meshHandle);

glRaytracingInstanceHandle_t glRaytracingCreateInstance(const glRaytracingInstanceDesc_t* desc);
int                          glRaytracingUpdateInstance(glRaytracingInstanceHandle_t instanceHandle, const glRaytracingInstanceDesc_t* desc);
void                         glRaytracingDeleteInstance(glRaytracingInstanceHandle_t instanceHandle);

int                        glRaytracingBuildMesh(glRaytracingMeshHandle_t meshHandle);
int                        glRaytracingBuildAllMeshes(void);
int                        glRaytracingBuildScene(void);
int                        glRaytracingBuildSceneIfDirty(void);

uint32_t                   glRaytracingGetMeshCount(void);
uint32_t                   glRaytracingGetInstanceCount(void);

bool                       glRaytracingLightingInit(void);
void                       glRaytracingLightingShutdown(void);
bool                       glRaytracingLightingIsInitialized(void);

void                       glRaytracingLightingClearLights(bool clearPersistant);
bool                       glRaytracingLightingAddLight(const glRaytracingLight_t* light);

void                       glRaytracingLightingSetAmbient(float r, float g, float b, float intensity);
void                       glRaytracingLightingSetCameraPosition(float x, float y, float z);
void                       glRaytracingLightingSetInvViewProjMatrix(const float* m16);
void                       glRaytracingLightingSetInvViewMatrix(const float* m16);
void                       glRaytracingLightingSetNormalReconstructSign(float signValue);
void                       glRaytracingLightingEnableSpecular(int enable);
void                       glRaytracingLightingEnableHalfLambert(int enable);
void                       glRaytracingLightingSetShadowBias(float bias);

bool                       glRaytracingLightingExecute(const glRaytracingLightingPassDesc_t* pass);

glRaytracingLight_t        glRaytracingLightingMakePointLight(
	float px, float py, float pz,
	float radius,
	float r, float g, float b,
	float intensity);

glRaytracingLight_t        glRaytracingLightingMakeRectLight(
	float px, float py, float pz,
	float nx, float ny, float nz,
	float ux, float uy, float uz,
	float vx, float vy, float vz,
	float halfWidth, float halfHeight,
	float r, float g, float b,
	float intensity,
	uint32_t samples,
	uint32_t twoSided);

uint32_t                   glRaytracingLightingGetLightCount(void);

void                       glLightScene(void);

ID3D12Device* QD3D12_GetDevice(void);
ID3D12CommandQueue* QD3D12_GetQueue(void);
ID3D12GraphicsCommandList* QD3D12_GetCommandList(void);
ID3D12Resource* QD3D12_GetCurrentBackBuffer(void);

static float glRaytracingSqrtf(float x)
{
	return (x > 0.0f) ? sqrtf(x) : 0.0f;
}

static void glRaytracingNormalize3(float& x, float& y, float& z)
{
	const float lenSq = x * x + y * y + z * z;
	if (lenSq > 1e-20f)
	{
		const float invLen = 1.0f / glRaytracingSqrtf(lenSq);
		x *= invLen;
		y *= invLen;
		z *= invLen;
	}
	else
	{
		x = 0.0f;
		y = 0.0f;
		z = 1.0f;
	}
}

static void glRaytracingCross3(
	float ax, float ay, float az,
	float bx, float by, float bz,
	float& outX, float& outY, float& outZ)
{
	outX = ay * bz - az * by;
	outY = az * bx - ax * bz;
	outZ = ax * by - ay * bx;
}

static const int GL_RAYTRACING_LIGHT_TYPE_POINT = 0;
static const int GL_RAYTRACING_LIGHT_TYPE_RECT = 1;

void TessellatePolygon(const std::vector<GLVertex>& src, std::vector<GLVertex>& out);

// ============================================================
// Pbuffer / WGL render-texture additions
// ============================================================

#ifndef GL_DRAW_BUFFER
#define GL_DRAW_BUFFER 0x0C01
#endif
#ifndef GL_READ_BUFFER
#define GL_READ_BUFFER 0x0C02
#endif
#ifndef GL_FRONT_LEFT
#define GL_FRONT_LEFT 0x0400
#endif
#ifndef GL_FRONT_RIGHT
#define GL_FRONT_RIGHT 0x0401
#endif

#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif
#ifndef GL_TEXTURE_RECTANGLE_NV
#define GL_TEXTURE_RECTANGLE_NV 0x84F5
#endif
#ifndef GL_TEXTURE_RECTANGLE_EXT
#define GL_TEXTURE_RECTANGLE_EXT 0x84F5
#endif
#ifndef GL_TEXTURE_BINDING_RECTANGLE_ARB
#define GL_TEXTURE_BINDING_RECTANGLE_ARB 0x84F6
#endif
#ifndef GL_TEXTURE_BINDING_RECTANGLE_NV
#define GL_TEXTURE_BINDING_RECTANGLE_NV 0x84F6
#endif
#ifndef GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB 0x84F8
#endif

#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#endif
#ifndef GL_TEXTURE_CUBE_MAP_ARB
#define GL_TEXTURE_CUBE_MAP_ARB 0x8513
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_X
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_X
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_Y
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_Z
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#endif
#ifndef GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#endif

#ifndef WGL_ARB_pbuffer
#define WGL_ARB_pbuffer 1
DECLARE_HANDLE(HPBUFFERARB);
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_ARB_pixel_format 1
#endif
#ifndef WGL_ARB_render_texture
#define WGL_ARB_render_texture 1
#endif
#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string 1
#endif
#ifndef WGL_EXT_extensions_string
#define WGL_EXT_extensions_string 1
#endif
#ifndef WGL_NV_render_texture_rectangle
#define WGL_NV_render_texture_rectangle 1
#endif
#ifndef WGL_NV_float_buffer
#define WGL_NV_float_buffer 1
#endif

#ifndef WGL_DRAW_TO_WINDOW_ARB
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#endif
#ifndef WGL_DRAW_TO_BITMAP_ARB
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#endif
#ifndef WGL_ACCELERATION_ARB
#define WGL_ACCELERATION_ARB 0x2003
#endif
#ifndef WGL_SUPPORT_GDI_ARB
#define WGL_SUPPORT_GDI_ARB 0x200F
#endif
#ifndef WGL_SUPPORT_OPENGL_ARB
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#endif
#ifndef WGL_DOUBLE_BUFFER_ARB
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#endif
#ifndef WGL_STEREO_ARB
#define WGL_STEREO_ARB 0x2012
#endif
#ifndef WGL_PIXEL_TYPE_ARB
#define WGL_PIXEL_TYPE_ARB 0x2013
#endif
#ifndef WGL_COLOR_BITS_ARB
#define WGL_COLOR_BITS_ARB 0x2014
#endif
#ifndef WGL_RED_BITS_ARB
#define WGL_RED_BITS_ARB 0x2015
#endif
#ifndef WGL_RED_SHIFT_ARB
#define WGL_RED_SHIFT_ARB 0x2016
#endif
#ifndef WGL_GREEN_BITS_ARB
#define WGL_GREEN_BITS_ARB 0x2017
#endif
#ifndef WGL_GREEN_SHIFT_ARB
#define WGL_GREEN_SHIFT_ARB 0x2018
#endif
#ifndef WGL_BLUE_BITS_ARB
#define WGL_BLUE_BITS_ARB 0x2019
#endif
#ifndef WGL_BLUE_SHIFT_ARB
#define WGL_BLUE_SHIFT_ARB 0x201A
#endif
#ifndef WGL_ALPHA_BITS_ARB
#define WGL_ALPHA_BITS_ARB 0x201B
#endif
#ifndef WGL_ALPHA_SHIFT_ARB
#define WGL_ALPHA_SHIFT_ARB 0x201C
#endif
#ifndef WGL_DEPTH_BITS_ARB
#define WGL_DEPTH_BITS_ARB 0x2022
#endif
#ifndef WGL_STENCIL_BITS_ARB
#define WGL_STENCIL_BITS_ARB 0x2023
#endif
#ifndef WGL_FULL_ACCELERATION_ARB
#define WGL_FULL_ACCELERATION_ARB 0x2027
#endif
#ifndef WGL_TYPE_RGBA_ARB
#define WGL_TYPE_RGBA_ARB 0x202B
#endif
#ifndef WGL_DRAW_TO_PBUFFER_ARB
#define WGL_DRAW_TO_PBUFFER_ARB 0x202D
#endif
#ifndef WGL_PBUFFER_WIDTH_ARB
#define WGL_PBUFFER_WIDTH_ARB 0x2034
#endif
#ifndef WGL_PBUFFER_HEIGHT_ARB
#define WGL_PBUFFER_HEIGHT_ARB 0x2035
#endif
#ifndef WGL_PBUFFER_LOST_ARB
#define WGL_PBUFFER_LOST_ARB 0x2036
#endif

#ifndef WGL_BIND_TO_TEXTURE_RGB_ARB
#define WGL_BIND_TO_TEXTURE_RGB_ARB 0x2070
#endif
#ifndef WGL_BIND_TO_TEXTURE_RGBA_ARB
#define WGL_BIND_TO_TEXTURE_RGBA_ARB 0x2071
#endif
#ifndef WGL_TEXTURE_FORMAT_ARB
#define WGL_TEXTURE_FORMAT_ARB 0x2072
#endif
#ifndef WGL_TEXTURE_TARGET_ARB
#define WGL_TEXTURE_TARGET_ARB 0x2073
#endif
#ifndef WGL_MIPMAP_TEXTURE_ARB
#define WGL_MIPMAP_TEXTURE_ARB 0x2074
#endif
#ifndef WGL_TEXTURE_RGB_ARB
#define WGL_TEXTURE_RGB_ARB 0x2075
#endif
#ifndef WGL_TEXTURE_RGBA_ARB
#define WGL_TEXTURE_RGBA_ARB 0x2076
#endif
#ifndef WGL_NO_TEXTURE_ARB
#define WGL_NO_TEXTURE_ARB 0x2077
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_ARB
#define WGL_TEXTURE_CUBE_MAP_ARB 0x2078
#endif
#ifndef WGL_TEXTURE_1D_ARB
#define WGL_TEXTURE_1D_ARB 0x2079
#endif
#ifndef WGL_TEXTURE_2D_ARB
#define WGL_TEXTURE_2D_ARB 0x207A
#endif
#ifndef WGL_MIPMAP_LEVEL_ARB
#define WGL_MIPMAP_LEVEL_ARB 0x207B
#endif
#ifndef WGL_CUBE_MAP_FACE_ARB
#define WGL_CUBE_MAP_FACE_ARB 0x207C
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x207D
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x207E
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x207F
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x2080
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x2081
#endif
#ifndef WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x2082
#endif
#ifndef WGL_FRONT_LEFT_ARB
#define WGL_FRONT_LEFT_ARB 0x2083
#endif
#ifndef WGL_FRONT_RIGHT_ARB
#define WGL_FRONT_RIGHT_ARB 0x2084
#endif
#ifndef WGL_BACK_LEFT_ARB
#define WGL_BACK_LEFT_ARB 0x2085
#endif
#ifndef WGL_BACK_RIGHT_ARB
#define WGL_BACK_RIGHT_ARB 0x2086
#endif

#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV 0x20A0
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV 0x20A1
#endif
#ifndef WGL_TEXTURE_RECTANGLE_NV
#define WGL_TEXTURE_RECTANGLE_NV 0x20A2
#endif

#ifndef WGL_FLOAT_COMPONENTS_NV
#define WGL_FLOAT_COMPONENTS_NV 0x20B0
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV 0x20B1
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV 0x20B2
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV 0x20B3
#endif
#ifndef WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV 0x20B4
#endif
#ifndef WGL_TEXTURE_FLOAT_R_NV
#define WGL_TEXTURE_FLOAT_R_NV 0x20B5
#endif
#ifndef WGL_TEXTURE_FLOAT_RG_NV
#define WGL_TEXTURE_FLOAT_RG_NV 0x20B6
#endif
#ifndef WGL_TEXTURE_FLOAT_RGB_NV
#define WGL_TEXTURE_FLOAT_RGB_NV 0x20B7
#endif
#ifndef WGL_TEXTURE_FLOAT_RGBA_NV
#define WGL_TEXTURE_FLOAT_RGBA_NV 0x20B8
#endif

typedef BOOL(WINAPI* PFNWGLSHARELISTSPROC)(HGLRC hglrc1, HGLRC hglrc2);
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void);
typedef BOOL(WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
typedef BOOL(WINAPI* PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int* piAttributes, int* piValues);
typedef HPBUFFERARB(WINAPI* PFNWGLCREATEPBUFFERARBPROC)(HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList);
typedef HDC(WINAPI* PFNWGLGETPBUFFERDCARBPROC)(HPBUFFERARB hPbuffer);
typedef int         (WINAPI* PFNWGLRELEASEPBUFFERDCARBPROC)(HPBUFFERARB hPbuffer, HDC hdc);
typedef BOOL(WINAPI* PFNWGLDESTROYPBUFFERARBPROC)(HPBUFFERARB hPbuffer);
typedef BOOL(WINAPI* PFNWGLQUERYPBUFFERARBPROC)(HPBUFFERARB hPbuffer, int iAttribute, int* piValue);
typedef BOOL(WINAPI* PFNWGLSETPBUFFERATTRIBARBPROC)(HPBUFFERARB hPbuffer, const int* piAttribList);
typedef BOOL(WINAPI* PFNWGLBINDTEXIMAGEARBPROC)(HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL(WINAPI* PFNWGLRELEASETEXIMAGEARBPROC)(HPBUFFERARB hPbuffer, int iBuffer);

BOOL        WINAPI qd3d12_wglShareLists(QD3D12_HGLRC hglrc1, QD3D12_HGLRC hglrc2);
const char* WINAPI qd3d12_wglGetExtensionsStringARB(HDC hdc);
const char* WINAPI qd3d12_wglGetExtensionsStringEXT(void);
BOOL        WINAPI qd3d12_wglChoosePixelFormatARB(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
BOOL        WINAPI qd3d12_wglGetPixelFormatAttribivARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int* piAttributes, int* piValues);
HPBUFFERARB WINAPI qd3d12_wglCreatePbufferARB(HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList);
HDC         WINAPI qd3d12_wglGetPbufferDCARB(HPBUFFERARB hPbuffer);
int         WINAPI qd3d12_wglReleasePbufferDCARB(HPBUFFERARB hPbuffer, HDC hdc);
BOOL        WINAPI qd3d12_wglDestroyPbufferARB(HPBUFFERARB hPbuffer);
BOOL        WINAPI qd3d12_wglQueryPbufferARB(HPBUFFERARB hPbuffer, int iAttribute, int* piValue);
BOOL        WINAPI qd3d12_wglSetPbufferAttribARB(HPBUFFERARB hPbuffer, const int* piAttribList);
BOOL        WINAPI qd3d12_wglBindTexImageARB(HPBUFFERARB hPbuffer, int iBuffer);
BOOL        WINAPI qd3d12_wglReleaseTexImageARB(HPBUFFERARB hPbuffer, int iBuffer);

//#define wglShareLists                  qd3d12_wglShareLists
#define wglGetExtensionsStringARB      qd3d12_wglGetExtensionsStringARB
#define wglGetExtensionsStringEXT      qd3d12_wglGetExtensionsStringEXT
#define wglChoosePixelFormatARB        qd3d12_wglChoosePixelFormatARB
#define wglGetPixelFormatAttribivARB   qd3d12_wglGetPixelFormatAttribivARB
#define wglCreatePbufferARB            qd3d12_wglCreatePbufferARB
#define wglGetPbufferDCARB             qd3d12_wglGetPbufferDCARB
#define wglReleasePbufferDCARB         qd3d12_wglReleasePbufferDCARB
#define wglDestroyPbufferARB           qd3d12_wglDestroyPbufferARB
#define wglQueryPbufferARB             qd3d12_wglQueryPbufferARB
#define wglSetPbufferAttribARB         qd3d12_wglSetPbufferAttribARB
#define wglBindTexImageARB             qd3d12_wglBindTexImageARB
#define wglReleaseTexImageARB          qd3d12_wglReleaseTexImageARB

#ifndef GL_READ_ONLY_ARB
#define GL_READ_ONLY_ARB GL_READ_ONLY
#endif

#ifndef GL_WRITE_ONLY_ARB
#define GL_WRITE_ONLY_ARB GL_WRITE_ONLY
#endif

#ifndef GL_READ_WRITE_ARB
#define GL_READ_WRITE_ARB GL_READ_WRITE
#endif

void APIENTRY glCompressedTexImage2DARB(GLenum target, GLint level, GLenum internalFormat,
	GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);