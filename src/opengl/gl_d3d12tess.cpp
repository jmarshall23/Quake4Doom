#include <vector>
#include <cstring>
#include "tess/tess.h"

struct GLVertex
{
	float px, py, pz;
	float nx, ny, nz;
	float u0, v0;
	float u1, v1;
	float r, g, b, a;
};

struct TessVertex
{
	GLdouble coords[3];
	GLVertex  vtx;
};

struct TessContext
{
	std::vector<GLVertex>* out = nullptr;
	std::vector<TessVertex*>* allocated = nullptr;

	GLenum currentPrim = 0;
	std::vector<GLVertex> primVerts;

	bool failed = false;
};

static void TessBeginCB(GLenum type, void* userData)
{
	TessContext* ctx = reinterpret_cast<TessContext*>(userData);
	ctx->currentPrim = type;
	ctx->primVerts.clear();
}

static void TessEndCB(void* userData)
{
	TessContext* ctx = reinterpret_cast<TessContext*>(userData);

	if (ctx->failed)
		return;

	switch (ctx->currentPrim)
	{
	case GL_TRIANGLES:
		if ((ctx->primVerts.size() % 3) != 0)
		{
			ctx->failed = true;
			return;
		}

		for (size_t i = 0; i < ctx->primVerts.size(); i += 3)
		{
			ctx->out->push_back(ctx->primVerts[i + 0]);
			ctx->out->push_back(ctx->primVerts[i + 1]);
			ctx->out->push_back(ctx->primVerts[i + 2]);
		}
		break;

	case GL_TRIANGLE_FAN:
		if (ctx->primVerts.size() < 3)
			break;

		for (size_t i = 1; i + 1 < ctx->primVerts.size(); ++i)
		{
			ctx->out->push_back(ctx->primVerts[0]);
			ctx->out->push_back(ctx->primVerts[i]);
			ctx->out->push_back(ctx->primVerts[i + 1]);
		}
		break;

	case GL_TRIANGLE_STRIP:
		if (ctx->primVerts.size() < 3)
			break;

		for (size_t i = 0; i + 2 < ctx->primVerts.size(); ++i)
		{
			if ((i & 1) == 0)
			{
				ctx->out->push_back(ctx->primVerts[i + 0]);
				ctx->out->push_back(ctx->primVerts[i + 1]);
				ctx->out->push_back(ctx->primVerts[i + 2]);
			}
			else
			{
				ctx->out->push_back(ctx->primVerts[i + 1]);
				ctx->out->push_back(ctx->primVerts[i + 0]);
				ctx->out->push_back(ctx->primVerts[i + 2]);
			}
		}
		break;

	default:
		ctx->failed = true;
		break;
	}

	ctx->primVerts.clear();
	ctx->currentPrim = 0;
}

static void TessErrorCB(GLenum errorCode, void* userData)
{
	(void)errorCode;
	TessContext* ctx = reinterpret_cast<TessContext*>(userData);
	ctx->failed = true;
}

static void TessVertexCB(void* vertexData, void* userData)
{
	TessContext* ctx = reinterpret_cast<TessContext*>(userData);
	TessVertex* tv = reinterpret_cast<TessVertex*>(vertexData);

	if (!tv)
	{
		ctx->failed = true;
		return;
	}

	ctx->primVerts.push_back(tv->vtx);
}

static void TessCombineCB(GLdouble newVertex[3],
	void* neighborData[4],
	GLfloat neighborWeight[4],
	void** outData,
	void* userData)
{
	TessContext* ctx = reinterpret_cast<TessContext*>(userData);

	TessVertex* nv = new TessVertex{};
	nv->coords[0] = newVertex[0];
	nv->coords[1] = newVertex[1];
	nv->coords[2] = newVertex[2];

	std::memset(&nv->vtx, 0, sizeof(nv->vtx));
	nv->vtx.px = static_cast<float>(newVertex[0]);
	nv->vtx.py = static_cast<float>(newVertex[1]);
	nv->vtx.pz = static_cast<float>(newVertex[2]);

	for (int i = 0; i < 4; ++i)
	{
		if (!neighborData[i])
			continue;

		TessVertex* src = reinterpret_cast<TessVertex*>(neighborData[i]);
		const float w = neighborWeight[i];

		nv->vtx.nx += src->vtx.nx * w;
		nv->vtx.ny += src->vtx.ny * w;
		nv->vtx.nz += src->vtx.nz * w;

		nv->vtx.u0 += src->vtx.u0 * w;
		nv->vtx.v0 += src->vtx.v0 * w;
		nv->vtx.u1 += src->vtx.u1 * w;
		nv->vtx.v1 += src->vtx.v1 * w;

		nv->vtx.r += src->vtx.r * w;
		nv->vtx.g += src->vtx.g * w;
		nv->vtx.b += src->vtx.b * w;
		nv->vtx.a += src->vtx.a * w;
	}

	ctx->allocated->push_back(nv);
	*outData = nv;
}

void TessellatePolygon(const std::vector<GLVertex>& src, std::vector<GLVertex>& out)
{
	out.clear();

	if (src.size() < 3)
		return;

	GLUtesselator* tess = gluNewTess();
	if (!tess)
		return;

	std::vector<TessVertex*> allocated;
	allocated.reserve(src.size() + 8);

	TessContext ctx;
	ctx.out = &out;
	ctx.allocated = &allocated;

	gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
	gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);

	gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (_GLUfuncptr)&TessBeginCB);
	gluTessCallback(tess, GLU_TESS_END_DATA, (_GLUfuncptr)&TessEndCB);
	gluTessCallback(tess, GLU_TESS_ERROR_DATA, (_GLUfuncptr)&TessErrorCB);
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (_GLUfuncptr)&TessVertexCB);
	gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (_GLUfuncptr)&TessCombineCB);

	gluTessBeginPolygon(tess, &ctx);
	gluTessBeginContour(tess);

	for (const GLVertex& v : src)
	{
		TessVertex* tv = new TessVertex{};
		tv->coords[0] = static_cast<GLdouble>(v.px);
		tv->coords[1] = static_cast<GLdouble>(v.py);
		tv->coords[2] = static_cast<GLdouble>(v.pz);
		tv->vtx = v;

		allocated.push_back(tv);
		gluTessVertex(tess, tv->coords, tv);
	}

	gluTessEndContour(tess);
	gluTessEndPolygon(tess);

	gluDeleteTess(tess);

	for (TessVertex* p : allocated)
		delete p;

	if (ctx.failed)
		out.clear();
}