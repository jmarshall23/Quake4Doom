// glu.cpp
//

#include "opengl.h"

static void qgluMultMatrixd(const GLdouble a[16], const GLdouble b[16], GLdouble r[16])
{
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            r[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
}

static void qgluMultMatrixVecd(const GLdouble m[16], const GLdouble in[4], GLdouble out[4])
{
    out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12] * in[3];
    out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13] * in[3];
    out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14] * in[3];
    out[3] = m[3] * in[0] + m[7] * in[1] + m[11] * in[2] + m[15] * in[3];
}

static int qgluInvertMatrixd(const GLdouble m[16], GLdouble invOut[16])
{
    GLdouble inv[16];

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

    GLdouble det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (det == 0.0)
        return GL_FALSE;

    det = 1.0 / det;
    for (int i = 0; i < 16; ++i)
        invOut[i] = inv[i] * det;

    return GL_TRUE;
}

extern "C" const GLubyte* APIENTRY gluErrorString(GLenum error)
{
    switch (error)
    {
    case 0:
        return (const GLubyte*)"no error";

#ifdef GLU_INVALID_ENUM
    case GLU_INVALID_ENUM:
        return (const GLubyte*)"invalid enum";
#endif
#ifdef GLU_INVALID_VALUE
    case GLU_INVALID_VALUE:
        return (const GLubyte*)"invalid value";
#endif
#ifdef GLU_OUT_OF_MEMORY
    case GLU_OUT_OF_MEMORY:
        return (const GLubyte*)"out of memory";
#endif
#ifdef GLU_INCOMPATIBLE_GL_VERSION
    case GLU_INCOMPATIBLE_GL_VERSION:
        return (const GLubyte*)"incompatible GL version";
#endif

#ifdef GLU_TESS_ERROR1
    case GLU_TESS_ERROR1: return (const GLubyte*)"gluTessBeginPolygon must precede a gluTessEndPolygon";
    case GLU_TESS_ERROR2: return (const GLubyte*)"gluTessBeginContour must precede a gluTessEndContour";
    case GLU_TESS_ERROR3: return (const GLubyte*)"gluTessEndPolygon must follow a gluTessBeginPolygon";
    case GLU_TESS_ERROR4: return (const GLubyte*)"gluTessEndContour must follow a gluTessBeginContour";
    case GLU_TESS_ERROR5: return (const GLubyte*)"a coordinate is too large";
    case GLU_TESS_ERROR6: return (const GLubyte*)"need combine callback";
    case GLU_TESS_ERROR7: return (const GLubyte*)"missing or bad polygon data";
    case GLU_TESS_ERROR8: return (const GLubyte*)"missing or bad contour data";
#endif

    default:
        return (const GLubyte*)"unknown GLU error";
    }
}

void APIENTRY gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    const GLdouble DEG2RAD = 3.14159265358979323846 / 180.0;

    GLfloat f = 1.0 / tan(fovy * 0.5 * DEG2RAD);

    GLfloat m[16] = { 0 };

    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0;
    m[14] = (2.0 * zFar * zNear) / (zNear - zFar);
    m[15] = 0.0;

    glMultMatrixf(m);
}

extern "C" GLint APIENTRY gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz,
    const GLdouble modelMatrix[16],
    const GLdouble projMatrix[16],
    const GLint viewport[4],
    GLdouble* objx, GLdouble* objy, GLdouble* objz)
{
    GLdouble finalMatrix[16];
    GLdouble invMatrix[16];
    GLdouble in[4];
    GLdouble out[4];

    if (!objx || !objy || !objz || !modelMatrix || !projMatrix || !viewport)
        return GL_FALSE;

    qgluMultMatrixd(projMatrix, modelMatrix, finalMatrix);

    if (!qgluInvertMatrixd(finalMatrix, invMatrix))
        return GL_FALSE;

    in[0] = (winx - (GLdouble)viewport[0]) / (GLdouble)viewport[2];
    in[1] = (winy - (GLdouble)viewport[1]) / (GLdouble)viewport[3];
    in[2] = winz;
    in[3] = 1.0;

    in[0] = in[0] * 2.0 - 1.0;
    in[1] = in[1] * 2.0 - 1.0;
    in[2] = in[2] * 2.0 - 1.0;

    qgluMultMatrixVecd(invMatrix, in, out);

    if (out[3] == 0.0)
        return GL_FALSE;

    out[3] = 1.0 / out[3];

    *objx = out[0] * out[3];
    *objy = out[1] * out[3];
    *objz = out[2] * out[3];

    return GL_TRUE;
}