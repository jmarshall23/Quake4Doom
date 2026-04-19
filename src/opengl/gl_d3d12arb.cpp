#include "opengl.h"
#include "gl_d3d12arb.h"
#include <d3dcompiler.h>

#include <windows.h>
#include <wrl/client.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#undef max
#undef min

using Microsoft::WRL::ComPtr;

namespace
{
	struct ARBProgram
	{
		GLuint id = 0;
		GLenum target = 0;
		std::string source;
		std::string hlsl;
		std::string errorString;
		GLint errorPosition = -1;
		uint32_t revision = 1;
		bool compileAttempted = false;
		float local[QD3D12_ARB_MAX_PROGRAM_PARAMETERS][4] = {};
		ComPtr<ID3DBlob> blob;
	};

	static std::unordered_map<GLuint, ARBProgram> g_vertexPrograms;
	static std::unordered_map<GLuint, ARBProgram> g_fragmentPrograms;
	static GLuint g_nextProgramId = 1;
	static GLuint g_boundVertexProgram = 0;
	static GLuint g_boundFragmentProgram = 0;
	static bool g_vertexProgramEnabled = false;
	static bool g_fragmentProgramEnabled = false;

	static float g_env[QD3D12_ARB_MAX_PROGRAM_PARAMETERS][4] = {};
	static GLenum g_lastError = GL_NO_ERROR;
	static GLint g_lastProgramErrorPosition = -1;
	static std::string g_lastProgramErrorString;

	static void SetError(GLenum e)
	{
		if (g_lastError == GL_NO_ERROR)
			g_lastError = e;
	}

	static bool IsProgramTarget(GLenum target)
	{
		return target == GL_VERTEX_PROGRAM_ARB || target == GL_FRAGMENT_PROGRAM_ARB;
	}

	static std::unordered_map<GLuint, ARBProgram>& ProgramsForTarget(GLenum target)
	{
		return (target == GL_VERTEX_PROGRAM_ARB) ? g_vertexPrograms : g_fragmentPrograms;
	}

	static GLuint& BoundForTarget(GLenum target)
	{
		return (target == GL_VERTEX_PROGRAM_ARB) ? g_boundVertexProgram : g_boundFragmentProgram;
	}

	static std::string Trim(const std::string& s)
	{
		size_t b = 0;
		while (b < s.size() && std::isspace((unsigned char)s[b]))
			++b;

		size_t e = s.size();
		while (e > b && std::isspace((unsigned char)s[e - 1]))
			--e;

		return s.substr(b, e - b);
	}

	static std::string Upper(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
		return s;
	}

	static bool StartsWithNoCase(const std::string& s, const char* prefix)
	{
		const size_t n = std::strlen(prefix);
		if (s.size() < n)
			return false;

		for (size_t i = 0; i < n; ++i)
		{
			if (std::toupper((unsigned char)s[i]) != std::toupper((unsigned char)prefix[i]))
				return false;
		}

		return true;
	}

	static std::string StripComments(const std::string& source)
	{
		std::string out;
		out.reserve(source.size());

		for (size_t i = 0; i < source.size(); ++i)
		{
			if (source[i] == '#')
			{
				while (i < source.size() && source[i] != '\n' && source[i] != '\r')
					++i;
				out.push_back('\n');
			}
			else
			{
				out.push_back(source[i]);
			}
		}

		return out;
	}

	static std::vector<std::string> LogicalLines(const std::string& source)
	{
		std::string clean = StripComments(source);
		std::vector<std::string> lines;
		std::string cur;
		int braceDepth = 0;

		for (char c : clean)
		{
			if (c == '{') ++braceDepth;
			if (c == '}') --braceDepth;

			if ((c == '\n' || c == '\r' || c == ';') && braceDepth <= 0)
			{
				std::string t = Trim(cur);
				if (!t.empty())
					lines.push_back(t);
				cur.clear();
			}
			else
			{
				cur.push_back(c);
			}
		}

		std::string t = Trim(cur);
		if (!t.empty())
			lines.push_back(t);

		return lines;
	}

	static std::vector<std::string> SplitArgs(const std::string& s)
	{
		std::vector<std::string> args;
		std::string cur;
		int bracketDepth = 0;
		int braceDepth = 0;
		int parenDepth = 0;

		for (char c : s)
		{
			if (c == '[') ++bracketDepth;
			else if (c == ']') --bracketDepth;
			else if (c == '{') ++braceDepth;
			else if (c == '}') --braceDepth;
			else if (c == '(') ++parenDepth;
			else if (c == ')') --parenDepth;

			if (c == ',' && bracketDepth == 0 && braceDepth == 0 && parenDepth == 0)
			{
				args.push_back(Trim(cur));
				cur.clear();
			}
			else
			{
				cur.push_back(c);
			}
		}

		std::string t = Trim(cur);
		if (!t.empty())
			args.push_back(t);

		return args;
	}

	static std::string ComponentMap(std::string s)
	{
		for (char& c : s)
		{
			switch (c)
			{
			case 'r': case 'R': c = 'x'; break;
			case 'g': case 'G': c = 'y'; break;
			case 'b': case 'B': c = 'z'; break;
			case 'a': case 'A': c = 'w'; break;
			default: c = (char)std::tolower((unsigned char)c); break;
			}
		}
		return s;
	}

	static bool IsComponentSuffix(const std::string& s)
	{
		if (s.empty() || s.size() > 4)
			return false;

		for (char c : s)
		{
			switch (c)
			{
			case 'x': case 'X':
			case 'y': case 'Y':
			case 'z': case 'Z':
			case 'w': case 'W':
			case 'r': case 'R':
			case 'g': case 'G':
			case 'b': case 'B':
			case 'a': case 'A':
				break;
			default:
				return false;
			}
		}

		return true;
	}

	static void SplitARBComponentSuffix(std::string& tokenBase, std::string& outSwizzle)
	{
		outSwizzle.clear();

		const size_t dot = tokenBase.find_last_of('.');
		if (dot == std::string::npos)
			return;

		const std::string suffix = tokenBase.substr(dot + 1);
		if (!IsComponentSuffix(suffix))
			return;

		outSwizzle = ComponentMap(suffix);
		tokenBase = tokenBase.substr(0, dot);
	}

	static bool IsNumberToken(const std::string& t)
	{
		if (t.empty())
			return false;
		char* end = nullptr;
		std::strtod(t.c_str(), &end);
		return end && *end == 0;
	}

	static std::string HlslFloat(float f)
	{
		char buf[64];
		std::snprintf(buf, sizeof(buf), "%.9gf", f);
		return buf;
	}

	static bool ParseFloat4Initializer(const std::string& text, float v[4])
	{
		size_t b = text.find('{');
		size_t e = text.find('}', b == std::string::npos ? 0 : b);
		if (b == std::string::npos || e == std::string::npos || e <= b)
			return false;

		std::vector<std::string> args = SplitArgs(text.substr(b + 1, e - b - 1));
		for (int i = 0; i < 4; ++i)
			v[i] = (i < (int)args.size()) ? (float)std::atof(args[i].c_str()) : 0.0f;

		return true;
	}

	class ARBTranslator
	{
	public:
		explicit ARBTranslator(GLenum target) : m_target(target) {}

		bool Translate(const std::string& source, std::string& outHlsl, std::string& outError, GLint& outErrorPosition)
		{
			m_tempDecls.clear();
			m_paramDecls.clear();
			m_instructions.clear();
			m_error.clear();
			m_errorPosition = -1;

			std::vector<std::string> lines = LogicalLines(source);
			for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
			{
				if (!ParseLine(lines[lineIndex], (GLint)lineIndex))
				{
					outError = m_error;
					outErrorPosition = m_errorPosition;
					return false;
				}
			}

			outHlsl = (m_target == GL_VERTEX_PROGRAM_ARB) ? BuildVertexHLSL() : BuildFragmentHLSL();
			outError.clear();
			outErrorPosition = -1;
			return true;
		}

	private:
		GLenum m_target = 0;
		std::vector<std::string> m_tempDecls;
		std::vector<std::string> m_paramDecls;
		std::vector<std::string> m_instructions;
		std::string m_error;
		GLint m_errorPosition = -1;

		bool Fail(const std::string& msg, GLint pos)
		{
			m_error = msg;
			m_errorPosition = pos;
			return false;
		}

		bool ParseLine(const std::string& rawLine, GLint pos)
		{
			std::string line = Trim(rawLine);
			if (line.empty())
				return true;

			if (StartsWithNoCase(line, "!!ARBvp") || StartsWithNoCase(line, "!!ARBfp")) return true;
			if (StartsWithNoCase(line, "OPTION")) return true;
			if (StartsWithNoCase(line, "END")) return true;
			if (StartsWithNoCase(line, "OUTPUT") || StartsWithNoCase(line, "ATTRIB")) return true;
			if (StartsWithNoCase(line, "ALIAS") || StartsWithNoCase(line, "ADDRESS")) return true;

			size_t sp = line.find_first_of(" \t");
			std::string op = (sp == std::string::npos) ? Upper(line) : Upper(line.substr(0, sp));
			std::string rest = (sp == std::string::npos) ? std::string() : Trim(line.substr(sp + 1));

			if (op == "TEMP")
			{
				std::vector<std::string> names = SplitArgs(rest);
				for (const std::string& nRaw : names)
				{
					std::string n = Trim(nRaw);
					if (!n.empty())
						m_tempDecls.push_back("    float4 " + n + " = float4(0.0, 0.0, 0.0, 0.0);\n");
				}
				return true;
			}

			if (op == "PARAM")
				return ParseParam(rest, pos);

			std::vector<std::string> args = SplitArgs(rest);

			if (op == "MOV" && args.size() == 2)
			{
				m_instructions.push_back(Assign(args[0], Src(args[1])));
				return true;
			}

			if ((op == "ADD" || op == "SUB" || op == "MUL" || op == "MIN" || op == "MAX") && args.size() == 3)
			{
				std::string expr;
				if (op == "ADD") expr = "(" + Src(args[1]) + " + " + Src(args[2]) + ")";
				else if (op == "SUB") expr = "(" + Src(args[1]) + " - " + Src(args[2]) + ")";
				else if (op == "MUL") expr = "(" + Src(args[1]) + " * " + Src(args[2]) + ")";
				else if (op == "MIN") expr = "min(" + Src(args[1]) + ", " + Src(args[2]) + ")";
				else expr = "max(" + Src(args[1]) + ", " + Src(args[2]) + ")";

				m_instructions.push_back(Assign(args[0], expr));
				return true;
			}

			if (op == "MAD" && args.size() == 4)
			{
				m_instructions.push_back(Assign(args[0], "(" + Src(args[1]) + " * " + Src(args[2]) + " + " + Src(args[3]) + ")"));
				return true;
			}

			if (op == "DP3" && args.size() == 3)
			{
				std::string d = "dot((" + Src(args[1]) + ").xyz, (" + Src(args[2]) + ").xyz)";
				m_instructions.push_back(Assign(args[0], "float4(" + d + ", " + d + ", " + d + ", " + d + ")"));
				return true;
			}

			if (op == "DP4" && args.size() == 3)
			{
				std::string d = "dot((" + Src(args[1]) + "), (" + Src(args[2]) + "))";
				m_instructions.push_back(Assign(args[0], "float4(" + d + ", " + d + ", " + d + ", " + d + ")"));
				return true;
			}

			if (op == "RSQ" && args.size() == 2)
			{
				std::string r = "rsqrt(max(abs((" + Src(args[1]) + ").x), 1e-20))";
				m_instructions.push_back(Assign(args[0], "float4(" + r + ", " + r + ", " + r + ", " + r + ")"));
				return true;
			}

			if (op == "RCP" && args.size() == 2)
			{
				std::string r = "(1.0 / max(abs((" + Src(args[1]) + ").x), 1e-20))";
				m_instructions.push_back(Assign(args[0], "float4(" + r + ", " + r + ", " + r + ", " + r + ")"));
				return true;
			}

			if (op == "ABS" && args.size() == 2)
			{
				m_instructions.push_back(Assign(args[0], "abs(" + Src(args[1]) + ")"));
				return true;
			}

			if (op == "LRP" && args.size() == 4)
			{
				m_instructions.push_back(Assign(args[0], "lerp(" + Src(args[3]) + ", " + Src(args[2]) + ", " + Src(args[1]) + ")"));
				return true;
			}

			if ((op == "TEX" || op == "TXP") && args.size() >= 4)
			{
				if (m_target != GL_FRAGMENT_PROGRAM_ARB)
					return Fail("TEX/TXP only supported in ARBfp for this shim", pos);

				m_instructions.push_back(EmitTex(op == "TXP", args[0], args[1], args[2], args[3]));
				return true;
			}

			if (op == "KIL" && args.size() == 1)
			{
				m_instructions.push_back("    clip((" + Src(args[0]) + ").x);\n");
				return true;
			}

			m_instructions.push_back("    // Unsupported ARB op ignored by Doom3 subset translator: " + line + "\n");
			return true;
		}

		bool ParseParam(const std::string& rest, GLint pos)
		{
			size_t eq = rest.find('=');
			if (eq == std::string::npos)
				return Fail("PARAM without initializer is not supported", pos);

			std::string name = Trim(rest.substr(0, eq));
			std::string init = Trim(rest.substr(eq + 1));
			float v[4] = {};
			if (!ParseFloat4Initializer(init, v))
				return Fail("Only PARAM name = {x,y,z,w} is supported", pos);

			std::ostringstream ss;
			ss << "    const float4 " << name << " = float4("
				<< HlslFloat(v[0]) << ", "
				<< HlslFloat(v[1]) << ", "
				<< HlslFloat(v[2]) << ", "
				<< HlslFloat(v[3]) << ");\n";
			m_paramDecls.push_back(ss.str());
			return true;
		}

		std::string Dest(const std::string& token, std::string* outMask)
		{
			std::string t = Trim(token);
			std::string mask;
			SplitARBComponentSuffix(t, mask);

			if (outMask)
				*outMask = mask;

			if (StartsWithNoCase(t, "result.color")) return "result_color";
			if (StartsWithNoCase(t, "result.position")) return "o.pos";

			if (StartsWithNoCase(t, "result.texcoord["))
			{
				int idx = std::atoi(t.c_str() + std::strlen("result.texcoord["));
				idx = std::max(0, std::min((int)QD3D12_ARB_MAX_TEXTURE_UNITS - 1, idx));
				char buf[32];
				std::snprintf(buf, sizeof(buf), "o.tc%d", idx);
				return buf;
			}

			return t;
		}

		std::string Src(const std::string& token)
		{
			std::string t = Trim(token);
			if (t.empty())
				return "float4(0.0, 0.0, 0.0, 0.0)";

			bool negate = false;
			if (t[0] == '-')
			{
				negate = true;
				t = Trim(t.substr(1));
			}

			std::string swizzle;
			SplitARBComponentSuffix(t, swizzle);

			std::string base;
			if (StartsWithNoCase(t, "vertex.position")) base = "vertex_position";
			else if (StartsWithNoCase(t, "vertex.color")) base = "vertex_color";
			else if (StartsWithNoCase(t, "vertex.attrib["))
			{
				int idx = std::atoi(t.c_str() + std::strlen("vertex.attrib["));
				idx = std::max(0, std::min((int)QD3D12_ARB_MAX_VERTEX_ATTRIBS - 1, idx));
				char buf[32];
				std::snprintf(buf, sizeof(buf), "vertex_attrib%d", idx);
				base = buf;
			}
			else if (StartsWithNoCase(t, "program.env["))
			{
				int idx = std::atoi(t.c_str() + std::strlen("program.env["));
				idx = std::max(0, std::min((int)QD3D12_ARB_MAX_PROGRAM_PARAMETERS - 1, idx));
				char buf[48];
				std::snprintf(buf, sizeof(buf), "gARBEnv[%d]", idx);
				base = buf;
			}
			else if (StartsWithNoCase(t, "program.local["))
			{
				int idx = std::atoi(t.c_str() + std::strlen("program.local["));
				idx = std::max(0, std::min((int)QD3D12_ARB_MAX_PROGRAM_PARAMETERS - 1, idx));
				char buf[64];
				std::snprintf(buf, sizeof(buf), (m_target == GL_VERTEX_PROGRAM_ARB) ? "gARBLocalVP[%d]" : "gARBLocalFP[%d]", idx);
				base = buf;
			}
			else if (StartsWithNoCase(t, "fragment.color")) base = "fragment_color";
			else if (StartsWithNoCase(t, "fragment.texcoord["))
			{
				int idx = std::atoi(t.c_str() + std::strlen("fragment.texcoord["));
				idx = std::max(0, std::min((int)QD3D12_ARB_MAX_TEXTURE_UNITS - 1, idx));
				char buf[32];
				std::snprintf(buf, sizeof(buf), "fragment_tc%d", idx);
				base = buf;
			}
			else if (StartsWithNoCase(t, "result.color")) base = "result_color";
			else if (IsNumberToken(t)) base = "float4(" + t + ", " + t + ", " + t + ", " + t + ")";
			else base = t;

			if (!swizzle.empty())
			{
				// ARB one-component source swizzles are scalar operands that are
				// commonly used as vector splats, e.g.MUL R0, R0, R1.x.
					// Emit xxxx so follow-on HLSL code can safely take .x/.xyz too.
					if (swizzle.size() == 1)
						base += "." + swizzle + swizzle + swizzle + swizzle;
					else
						base += "." + swizzle;
			}
			if (negate)
				base = "(-" + base + ")";

			return base;
		}

		std::string Assign(const std::string& dstToken, const std::string& rhs)
		{
			std::string mask;
			std::string d = Dest(dstToken, &mask);
			std::ostringstream ss;

			if (mask.empty())
				ss << "    " << d << " = " << rhs << ";\n";
			else
				ss << "    " << d << "." << mask << " = (" << rhs << ")." << mask << ";\n";

			return ss.str();
		}

		int TextureIndex(const std::string& token)
		{
			int idx = 0;
			if (StartsWithNoCase(token, "texture["))
				idx = std::atoi(token.c_str() + std::strlen("texture["));
			return std::max(0, std::min((int)QD3D12_ARB_MAX_TEXTURE_UNITS - 1, idx));
		}

		std::string EmitTex(bool projected, const std::string& dst, const std::string& coordToken, const std::string& texToken, const std::string& targetToken)
		{
			std::string coord = Src(coordToken);
			std::string target = Upper(targetToken);
			std::string sample;

			if (target.find("CUBE") != std::string::npos)
			{
				// Doom 3 uses texture[0] as a normalization cube map in the common
				// interaction program.  The shim does not create real cubemaps, so
				// synthesize the normalized-vector lookup result.
				sample = "float4(QD3D12ARB_NormalizeSafe((" + coord + ").xyz) * 0.5 + 0.5, 1.0)";
			}
			else
			{
				const int idx = TextureIndex(texToken);
				char texName[32];
				char sampName[32];
				std::snprintf(texName, sizeof(texName), "gTex%d", idx);
				std::snprintf(sampName, sizeof(sampName), "gSamp%d", idx);

				std::string uv;
				if (projected)
					uv = "((" + coord + ").xy / max(abs((" + coord + ").w), 1e-6))";
				else
					uv = "(" + coord + ").xy";

				sample = std::string(texName) + ".Sample(" + sampName + ", " + uv + ")";
			}

			return Assign(dst, sample);
		}

		std::string CommonHLSLPrefix()
		{
			return R"HLSL(
cbuffer DrawCB : register(b0)
{
    float4x4 gMVP;
    float4x4 gPrevMVP;
    float4x4 gModelMatrix;

    float gAlphaRef;
    float gUseTex0;
    float gUseTex1;
    float gTex1IsLightmap;
    float gTexEnvMode0;
    float gTexEnvMode1;
    float geometryFlag;
    float gRoughness;

    float gFogEnabled;
    float gFogMode;
    float gFogDensity;
    float gFogStart;

    float gFogEnd;
    float gPointSize;
    float gMaterialType;
    float gFogPad2;

    float4 gFogColor;

    float2 gRenderSize;
    float2 gInvRenderSize;
    float2 gJitterPixels;
    float2 gPrevJitterPixels;
    float4 gMotionPad;

    float4 gARBEnv[128];
    float4 gARBLocalVP[128];
    float4 gARBLocalFP[128];
};

float3 QD3D12ARB_NormalizeSafe(float3 v)
{
    return v * rsqrt(max(dot(v, v), 1e-20));
}

)HLSL";
		}

		std::string BuildVertexHLSL()
		{
			std::ostringstream ss;
			ss << CommonHLSLPrefix();
			ss << R"HLSL(
struct VSIn
{
    float3 pos       : POSITION;
    float3 normal    : NORMAL;
    float2 uv0       : TEXCOORD0;
    float2 uv1       : TEXCOORD1;
    float4 col       : COLOR0;
    float3 tangent   : TANGENT;
    float3 bitangent : BINORMAL;
};

struct VSOut
{
    float4 pos : SV_Position;
    float4 tc0 : TEXCOORD0;
    float4 tc1 : TEXCOORD1;
    float4 tc2 : TEXCOORD2;
    float4 tc3 : TEXCOORD3;
    float4 tc4 : TEXCOORD4;
    float4 tc5 : TEXCOORD5;
    float4 tc6 : TEXCOORD6;
    float4 tc7 : TEXCOORD7;
    float4 col : COLOR0;
};

VSOut VSMain(VSIn i)
{
    VSOut o = (VSOut)0;
    o.pos = mul(gMVP, float4(i.pos, 1.0));
    o.tc0 = float4(i.uv0, 0.0, 1.0);
    o.tc1 = float4(i.uv1, 0.0, 1.0);
    o.tc2 = float4(0.0, 0.0, 0.0, 1.0);
    o.tc3 = float4(0.0, 0.0, 0.0, 1.0);
    o.tc4 = float4(0.0, 0.0, 0.0, 1.0);
    o.tc5 = float4(0.0, 0.0, 0.0, 1.0);
    o.tc6 = float4(0.0, 0.0, 0.0, 1.0);
    o.tc7 = float4(0.0, 0.0, 0.0, 1.0);
    o.col = i.col;

    float4 vertex_position = float4(i.pos, 1.0);
    float4 vertex_color = i.col;
    float4 vertex_attrib0 = vertex_position;
    float4 vertex_attrib1 = float4(0.0, 0.0, 0.0, 1.0);
    float4 vertex_attrib2 = float4(i.normal, 0.0);
    float4 vertex_attrib3 = i.col;
    float4 vertex_attrib4 = float4(0.0, 0.0, 0.0, 1.0);
    float4 vertex_attrib5 = float4(0.0, 0.0, 0.0, 1.0);
    float4 vertex_attrib6 = float4(0.0, 0.0, 0.0, 1.0);
    float4 vertex_attrib7 = float4(0.0, 0.0, 0.0, 1.0);
    float4 vertex_attrib8 = float4(i.uv0, 0.0, 1.0);
    float4 vertex_attrib9 = float4(i.normal, 0.0);
    float4 vertex_attrib10 = float4(i.tangent, 0.0);
    float4 vertex_attrib11 = float4(i.bitangent, 0.0);
    float4 vertex_attrib12 = float4(0.0, 0.0, 0.0, 1.0);
    float4 vertex_attrib13 = float4(0.0, 0.0, 0.0, 1.0);
    float4 vertex_attrib14 = float4(0.0, 0.0, 0.0, 1.0);
    float4 vertex_attrib15 = float4(0.0, 0.0, 0.0, 1.0);
)HLSL";
			for (const std::string& d : m_tempDecls) ss << d;
			for (const std::string& d : m_paramDecls) ss << d;
			for (const std::string& i : m_instructions) ss << i;
			ss << "    return o;\n}\n";
			return ss.str();
		}

		std::string BuildFragmentHLSL()
		{
			std::ostringstream ss;
			ss << CommonHLSLPrefix();

			for (UINT i = 0; i < QD3D12_ARB_MAX_TEXTURE_UNITS; ++i)
				ss << "Texture2D gTex" << i << " : register(t" << i << ");\n";
			for (UINT i = 0; i < QD3D12_ARB_MAX_TEXTURE_UNITS; ++i)
				ss << "SamplerState gSamp" << i << " : register(s" << i << ");\n";

			ss << R"HLSL(
struct PSIn
{
    float4 pos : SV_Position;
    float4 tc0 : TEXCOORD0;
    float4 tc1 : TEXCOORD1;
    float4 tc2 : TEXCOORD2;
    float4 tc3 : TEXCOORD3;
    float4 tc4 : TEXCOORD4;
    float4 tc5 : TEXCOORD5;
    float4 tc6 : TEXCOORD6;
    float4 tc7 : TEXCOORD7;
    float4 col : COLOR0;
};

float4 PSMain(PSIn i) : SV_Target0
{
    float4 result_color = float4(0.0, 0.0, 0.0, 1.0);
    float4 fragment_color = i.col;
    float4 fragment_tc0 = i.tc0;
    float4 fragment_tc1 = i.tc1;
    float4 fragment_tc2 = i.tc2;
    float4 fragment_tc3 = i.tc3;
    float4 fragment_tc4 = i.tc4;
    float4 fragment_tc5 = i.tc5;
    float4 fragment_tc6 = i.tc6;
    float4 fragment_tc7 = i.tc7;
)HLSL";
			for (const std::string& d : m_tempDecls) ss << d;
			for (const std::string& d : m_paramDecls) ss << d;
			for (const std::string& i : m_instructions) ss << i;
			ss << "    return result_color;\n}\n";
			return ss.str();
		}
	};

	static ComPtr<ID3DBlob> CompileHLSL(const char* source, const char* entry, const char* target, std::string& error)
	{
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ComPtr<ID3DBlob> blob;
		ComPtr<ID3DBlob> err;
		HRESULT hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry, target, flags, 0, &blob, &err);
		if (FAILED(hr))
		{
			error = err ? (const char*)err->GetBufferPointer() : "unknown HLSL compile error";
			return nullptr;
		}

		error.clear();
		return blob;
	}

	static ARBProgram* GetBoundProgram(GLenum target)
	{
		if (!IsProgramTarget(target))
			return nullptr;

		const GLuint id = BoundForTarget(target);
		if (id == 0)
			return nullptr;

		auto& map = ProgramsForTarget(target);
		auto it = map.find(id);
		if (it == map.end())
			return nullptr;

		return &it->second;
	}

	static ID3DBlob* EnsureCompiled(GLenum target)
	{
		ARBProgram* p = GetBoundProgram(target);
		if (!p)
			return nullptr;

		if (p->blob)
			return p->blob.Get();

		if (p->compileAttempted && !p->blob)
			return nullptr;

		p->compileAttempted = true;
		p->hlsl.clear();
		p->errorString.clear();
		p->errorPosition = -1;

		ARBTranslator translator(target);
		std::string hlsl;
		std::string translateError;
		GLint translateErrorPosition = -1;
		if (!translator.Translate(p->source, hlsl, translateError, translateErrorPosition))
		{
			p->errorString = translateError;
			p->errorPosition = translateErrorPosition;
			g_lastProgramErrorString = translateError;
			g_lastProgramErrorPosition = translateErrorPosition;
			SetError(GL_INVALID_OPERATION);
			return nullptr;
		}

		p->hlsl = hlsl;
		std::string compileError;
		p->blob = CompileHLSL(
			hlsl.c_str(),
			(target == GL_VERTEX_PROGRAM_ARB) ? "VSMain" : "PSMain",
			(target == GL_VERTEX_PROGRAM_ARB) ? "vs_5_0" : "ps_5_0",
			compileError);

		if (!p->blob)
		{
			p->errorString = compileError;
			p->errorPosition = -1;
			g_lastProgramErrorString = compileError;
			g_lastProgramErrorPosition = -1;
			SetError(GL_INVALID_OPERATION);
			return nullptr;
		}

		return p->blob.Get();
	}

	static void ResetEnvDefaults()
	{
		for (UINT i = 0; i < QD3D12_ARB_MAX_PROGRAM_PARAMETERS; ++i)
		{
			g_env[i][0] = 0.0f;
			g_env[i][1] = 0.0f;
			g_env[i][2] = 0.0f;
			g_env[i][3] = 0.0f;
		}
	}
}

void QD3D12ARB_Init(void)
{
	ResetEnvDefaults();
}

void QD3D12ARB_Shutdown(void)
{
	g_vertexPrograms.clear();
	g_fragmentPrograms.clear();
	g_nextProgramId = 1;
	g_boundVertexProgram = 0;
	g_boundFragmentProgram = 0;
	g_vertexProgramEnabled = false;
	g_fragmentProgramEnabled = false;
	g_lastError = GL_NO_ERROR;
	g_lastProgramErrorPosition = -1;
	g_lastProgramErrorString.clear();
	ResetEnvDefaults();
}

void QD3D12ARB_DeviceLost(void)
{
	for (auto& kv : g_vertexPrograms) kv.second.blob.Reset();
	for (auto& kv : g_fragmentPrograms) kv.second.blob.Reset();
}

void QD3D12ARB_SetEnabled(GLenum cap, bool enabled)
{
	if (cap == GL_VERTEX_PROGRAM_ARB)
		g_vertexProgramEnabled = enabled;
	else if (cap == GL_FRAGMENT_PROGRAM_ARB)
		g_fragmentProgramEnabled = enabled;
}

bool QD3D12ARB_IsVertexEnabled(void) { return g_vertexProgramEnabled; }
bool QD3D12ARB_IsFragmentEnabled(void) { return g_fragmentProgramEnabled; }

bool QD3D12ARB_IsActive(void)
{
	return g_vertexProgramEnabled && g_fragmentProgramEnabled && g_boundVertexProgram != 0 && g_boundFragmentProgram != 0;
}

GLuint QD3D12ARB_GetBoundVertexProgram(void) { return g_boundVertexProgram; }
GLuint QD3D12ARB_GetBoundFragmentProgram(void) { return g_boundFragmentProgram; }

uint32_t QD3D12ARB_GetBoundVertexRevision(void)
{
	ARBProgram* p = GetBoundProgram(GL_VERTEX_PROGRAM_ARB);
	return p ? p->revision : 0;
}

uint32_t QD3D12ARB_GetBoundFragmentRevision(void)
{
	ARBProgram* p = GetBoundProgram(GL_FRAGMENT_PROGRAM_ARB);
	return p ? p->revision : 0;
}

ID3DBlob* QD3D12ARB_GetVertexShaderBlob(void)
{
	return EnsureCompiled(GL_VERTEX_PROGRAM_ARB);
}

ID3DBlob* QD3D12ARB_GetFragmentShaderBlob(void)
{
	return EnsureCompiled(GL_FRAGMENT_PROGRAM_ARB);
}

void QD3D12ARB_FillDrawConstantArrays(QD3D12ARBDrawConstantArrays* outArrays)
{
	if (!outArrays)
		return;

	std::memset(outArrays, 0, sizeof(*outArrays));
	std::memcpy(outArrays->env, g_env, sizeof(outArrays->env));

	if (ARBProgram* vp = GetBoundProgram(GL_VERTEX_PROGRAM_ARB))
		std::memcpy(outArrays->vertexLocal, vp->local, sizeof(outArrays->vertexLocal));

	if (ARBProgram* fp = GetBoundProgram(GL_FRAGMENT_PROGRAM_ARB))
		std::memcpy(outArrays->fragmentLocal, fp->local, sizeof(outArrays->fragmentLocal));
}

GLenum QD3D12ARB_ConsumeError(void)
{
	GLenum e = g_lastError;
	g_lastError = GL_NO_ERROR;
	return e;
}

GLint QD3D12ARB_GetProgramErrorPosition(void)
{
	return g_lastProgramErrorPosition;
}

const char* QD3D12ARB_GetProgramErrorString(void)
{
	return g_lastProgramErrorString.c_str();
}

void APIENTRY glGenProgramsARB(GLsizei n, GLuint* programs)
{
	if (n < 0)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	if (!programs)
		return;

	for (GLsizei i = 0; i < n; ++i)
		programs[i] = g_nextProgramId++;
}

void APIENTRY glDeleteProgramsARB(GLsizei n, const GLuint* programs)
{
	if (n < 0)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	if (!programs)
		return;

	for (GLsizei i = 0; i < n; ++i)
	{
		const GLuint id = programs[i];
		if (id == 0)
			continue;

		if (g_boundVertexProgram == id) g_boundVertexProgram = 0;
		if (g_boundFragmentProgram == id) g_boundFragmentProgram = 0;

		g_vertexPrograms.erase(id);
		g_fragmentPrograms.erase(id);
	}
}

void APIENTRY glBindProgramARB(GLenum target, GLuint program)
{
	if (!IsProgramTarget(target))
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	BoundForTarget(target) = program;
	if (program == 0)
		return;

	auto& map = ProgramsForTarget(target);
	auto it = map.find(program);
	if (it == map.end())
	{
		ARBProgram p{};
		p.id = program;
		p.target = target;
		map.emplace(program, std::move(p));
	}
}

void APIENTRY glProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid* string)
{
	if (!IsProgramTarget(target))
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	if (format != GL_PROGRAM_FORMAT_ASCII_ARB)
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	if (len < 0 || !string)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	GLuint id = BoundForTarget(target);
	if (id == 0)
	{
		id = g_nextProgramId++;
		BoundForTarget(target) = id;
	}

	ARBProgram& p = ProgramsForTarget(target)[id];
	p.id = id;
	p.target = target;
	p.source.assign((const char*)string, (size_t)len);
	p.hlsl.clear();
	p.errorString.clear();
	p.errorPosition = -1;
	p.compileAttempted = false;
	p.blob.Reset();
	++p.revision;

	EnsureCompiled(target);
}

void APIENTRY glProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	if (!IsProgramTarget(target))
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	if (index >= QD3D12_ARB_MAX_PROGRAM_PARAMETERS)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	g_env[index][0] = x;
	g_env[index][1] = y;
	g_env[index][2] = z;
	g_env[index][3] = w;
}

void APIENTRY glProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat* params)
{
	if (!params)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	glProgramEnvParameter4fARB(target, index, params[0], params[1], params[2], params[3]);
}

void APIENTRY glProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	if (!IsProgramTarget(target))
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	if (index >= QD3D12_ARB_MAX_PROGRAM_PARAMETERS)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	ARBProgram* p = GetBoundProgram(target);
	if (!p)
	{
		SetError(GL_INVALID_OPERATION);
		return;
	}

	p->local[index][0] = x;
	p->local[index][1] = y;
	p->local[index][2] = z;
	p->local[index][3] = w;
}

void APIENTRY glProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat* params)
{
	if (!params)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	glProgramLocalParameter4fARB(target, index, params[0], params[1], params[2], params[3]);
}

void APIENTRY glGetProgramEnvParameterfvARB(GLenum target, GLuint index, GLfloat* params)
{
	if (!params)
		return;

	if (!IsProgramTarget(target))
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	if (index >= QD3D12_ARB_MAX_PROGRAM_PARAMETERS)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	std::memcpy(params, g_env[index], sizeof(float) * 4);
}

void APIENTRY glGetProgramLocalParameterfvARB(GLenum target, GLuint index, GLfloat* params)
{
	if (!params)
		return;

	if (!IsProgramTarget(target))
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	if (index >= QD3D12_ARB_MAX_PROGRAM_PARAMETERS)
	{
		SetError(GL_INVALID_VALUE);
		return;
	}

	ARBProgram* p = GetBoundProgram(target);
	if (!p)
	{
		SetError(GL_INVALID_OPERATION);
		return;
	}

	std::memcpy(params, p->local[index], sizeof(float) * 4);
}

void APIENTRY glGetProgramivARB(GLenum target, GLenum pname, GLint* params)
{
	if (!params)
		return;

	if (!IsProgramTarget(target))
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	ARBProgram* p = GetBoundProgram(target);
	switch (pname)
	{
	case GL_PROGRAM_ERROR_POSITION_ARB:
		*params = p ? p->errorPosition : g_lastProgramErrorPosition;
		break;

	case GL_PROGRAM_LENGTH_ARB:
		*params = p ? (GLint)p->source.size() : 0;
		break;

	case GL_PROGRAM_BINDING_ARB:
	case GL_VERTEX_PROGRAM_BINDING_ARB:
	case GL_FRAGMENT_PROGRAM_BINDING_ARB:
		*params = (GLint)BoundForTarget(target);
		break;

	case GL_PROGRAM_FORMAT_ARB:
		*params = GL_PROGRAM_FORMAT_ASCII_ARB;
		break;

	case GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
		*params = GL_TRUE;
		break;

	default:
		*params = 0;
		break;
	}
}

void APIENTRY glGetProgramStringARB(GLenum target, GLenum pname, GLvoid* string)
{
	if (!string)
		return;

	if (!IsProgramTarget(target))
	{
		SetError(GL_INVALID_ENUM);
		return;
	}

	ARBProgram* p = GetBoundProgram(target);
	if (!p)
	{
		((char*)string)[0] = 0;
		return;
	}

	if (pname == GL_PROGRAM_STRING_ARB)
		std::memcpy(string, p->source.c_str(), p->source.size() + 1);
	else
		SetError(GL_INVALID_ENUM);
}

GLboolean APIENTRY glIsProgramARB(GLuint program)
{
	if (program == 0)
		return GL_FALSE;

	return (g_vertexPrograms.find(program) != g_vertexPrograms.end() ||
		g_fragmentPrograms.find(program) != g_fragmentPrograms.end()) ? GL_TRUE : GL_FALSE;
}
