#include "parserbase.hpp"

#include <cassert>
#include <unordered_map>
#include <string>

namespace ag {
	namespace opengl {
		namespace fx {
			
			constexpr auto kFxMaxTextureUnits = 8;
			constexpr auto kFxMaxVertexAttributes = 8;

			enum class GLAttributeType
			{
				Float,
				Float2,
				Float3,
				Float4,
				Unorm16x2
			};

			struct GLSLSnippet
			{
				std::string name;
				std::string body;
				std::vector<GLSLSnippet*> snippets;

				void paste(std::ostream& os)
				{
					for (auto snip : snippets)
						snip->paste(os);
					os << body << std::endl;
				}
			};

			struct GLSLProgram
			{
				std::string name;
				GLSLSnippet* VS_snippet = nullptr;
				GLSLSnippet* GS_snippet = nullptr;
				GLSLSnippet* FS_snippet = nullptr;
				GLSLSnippet* TCS_snippet = nullptr;
				GLSLSnippet* TES_snippet = nullptr;
				GLSLSnippet* CS_snippet = nullptr;
				int VS_version = 330;
				int GS_version = 330;
				int FS_version = 330;
				int TCS_version = 330;
				int TES_version = 330;
				int CS_version = 330;

				std::string samplerIds[kFxMaxTextureUnits];
				std::string blendStateId;
				std::string rasterizerStateId;
				std::string depthStencilStateId;
				std::string vertexArrayId;
			};

			struct GLSLVertexArray
			{
				VertexAttribute attributes[kFxMaxVertexAttributes];
				int numAttributes;
			};

			constexpr auto TK_GLSL_PROGRAM = "GLSLProgram";
			constexpr auto TK_GLSL_SNIPPET = "GLSLSnippet";
			constexpr auto TK_GLSL_VERTEX_ARRAY = "GLVertexArray";
			constexpr auto TK_SAMPLER_STATE = "SamplerState";
			constexpr auto TK_BLEND_STATE = "BlendState";
			constexpr auto TK_RASTERIZER_STATE = "RasterizerState";
			constexpr auto TK_DEPTH_STENCIL_STATE = "DepthStencilState";

			class FxLibraryParser : public parse::ParserBase
			{
			public:
				FxLibraryParser(const char *filePath) : parse::ParserBase(filePath)
				{
				}

				// parse #include directive
				bool parseInclude()
				{
					std::string path;
					// must be at the beginning of a line
					if (currentContext().col != 1)
						return false;
					pushctx();
					PARSE_TRY(pchar('#'));
					skipws(); PARSE_TRY(ptoken("include"));
					skipws(); PARSE_TRY(pchar('<'));
					skipws(); puntil('>', path);
					PARSE_TRY(pchar('>'));
					skipws(); PARSE_TRY('\n');
					// success
					insertInclude(path.c_str());
					commitctx();
					return true;
				}

				// parse a shader snippet (GLSL)
				bool parseGlslSnippet()
				{
					std::string name;
					std::string body;
					std::vector<std::string> snippet_refs;
					pushctx();
					// whitespace already skipped
					PARSE_TRY(pstring("GLSLSnippet"));
					PARSE_TRY(skipws1());
					PARSE_TRY(pident(name));
					skipws();
					if (!pchar(':')) {
						skipws();
						// No requirement list
						PARSE_TRY(pchar('{'));
						PARSE_TRY(parseUntilMatchingBrace('{', body));
						PARSE_TRY(pchar('}'));
					}
					else {
						std::string ref;
						skipws();
						PARSE_TRY(pident(ref));
						snippet_refs.emplace_back(std::move(ref));
						for (;;) {
							skipws();
							if (pchar('{')) {
								break;
							}
							PARSE_TRY(pchar(','));
							skipws();
							PARSE_TRY(pident(ref));
							snippet_refs.emplace_back(std::move(ref));
						}

						PARSE_TRY(parseUntilMatchingBrace('{', body));
						PARSE_TRY(pchar('}'));
					}

					std::vector<GLSLSnippet*> refs;
					for (auto &s : snippet_refs)
					{
						if (snippets.find(s) == snippets.end()) {
							// snippet ref not found
							parseError() << "snippet not declared: " << s;
							// This is not a syntax error: continue parsing anyway
							commitctx();
							return true;
						}
						refs.push_back(snippets[s].get());
					}

					snippets[name] = std::unique_ptr<GLSLSnippet>(new GLSLSnippet{ name, std::move(body), std::move(refs) });
					commitctx();
					return true;
				}

				bool parseShaderStage(GLenum& stage)
				{
					if (ptoken("VS")) { stage = gl::VERTEX_SHADER; return true; }
					if (ptoken("FS")) { stage = gl::FRAGMENT_SHADER; return true; }
					if (ptoken("GS")) { stage = gl::GEOMETRY_SHADER; return true; }
					if (ptoken("TCS")) { stage = gl::TESS_CONTROL_SHADER; return true; }
					if (ptoken("TES")) { stage = gl::TESS_EVALUATION_SHADER; return true; }
					if (ptoken("CS")) { stage = gl::COMPUTE_SHADER; return true; }
					return false;
				}

				bool parseVertexArrayAttributeType(GLAttributeType& type)
				{
					if (ptoken("float")) { type = GLAttributeType::Float; return true; }
					if (ptoken("float2")) { type = GLAttributeType::Float2; return true; }
					if (ptoken("float3")) { type = GLAttributeType::Float3; return true; }
					if (ptoken("float4")) { type = GLAttributeType::Float4; return true; }
					if (ptoken("unorm16x2")) { type = GLAttributeType::Unorm16x2; return true; }
					return false;
				}

				bool parseVertexArrayAttribute(GLAttributeType& type, int& bufSlot)
				{
					pushctx();
					std::string name;
					skipws(); PARSE_TRY(parseVertexArrayAttributeType(type));
					PARSE_TRY(skipws1()); PARSE_TRY(pident(name));
					skipws(); PARSE_TRY(pchar(':'));
					skipws(); PARSE_TRY(pnum(bufSlot));
					skipws(); PARSE_TRY(pchar(';'));
					commitctx();
					return true;
				}

				//============================================================
				//============================================================
				// Vertex array block
				bool parseVertexArrayBlock()
				{
					pushctx();
					std::string name;
					PARSE_TRY(pstring(TK_GLSL_VERTEX_ARRAY));
					skipws(); PARSE_TRY(pident(name));
					skipws(); PARSE_TRY(pchar('{'));
					GLSLVertexArray va;
					int attribindex = 0;
					for (;;)
					{
						skipws();
						GLAttributeType type;
						int size, bufSlot;
						if (!parseVertexArrayAttribute(type, bufSlot))
							break;
						assert(bufSlot < 8);
						// XXX TODO TODO TODO
						//va.attributes[attribindex].type = type;
						va.attributes[attribindex].slot = bufSlot;
						++attribindex;
						assert(attribindex < 8);
					}
					skipws(); PARSE_TRY(pchar('}'));
					va.numAttributes = attribindex;
					vertexArrayBlocks[name] = va;
					commitctx();
					return true;
				}

				// parse VS|GS|FS|TCS|TES(int, string)
				bool parseProgramShaderLine(GLenum& stage_, int& version_, GLSLSnippet*& snip_)
				{
					pushctx();
					GLenum stage;
					int version;
					std::string src;
					PARSE_TRY(parseShaderStage(stage));
					skipws(); PARSE_TRY(pchar('('));
					skipws(); PARSE_TRY(pnum(version));
					skipws(); PARSE_TRY(pchar(','));
					skipws(); PARSE_TRY(pident(src));
					skipws(); PARSE_TRY(pchar(')'));
					skipws(); PARSE_TRY(pchar(';'));

					stage_ = stage;
					version_ = version;

					if (snippets.find(src) == snippets.end()) {
						parseError() << "snippet not declared: " << src;
						snip_ = nullptr;
						// Not a syntax error
						commitctx();
						return true;
					}

					snip_ = snippets[src].get();
					commitctx();
					return true;
				}

				// Parse a blend state declaration

				// UNTESTED
				/*template <typename Parser>
				bool look(Parser p)
				{
				pushctx();
				if (p()) {
				popctx();
				return true;
				}
				return false;
				}

				void passthrough()
				{
				while (!peof()) {
				if (look([this]() {return pstring(TK_GLSL_PROGRAM);})) return;
				if (look([this]() {return pstring(TK_GLSL_SNIPPET);})) return;
				global += getch();
				}
				}*/

				bool parseSetSamplerDeclaration(int& unit, std::string& sampler)
				{
					pushctx();
					PARSE_TRY(pstring("SetSampler"));
					skipws(); PARSE_TRY(pchar('('));
					skipws(); PARSE_TRY(pnum(unit));
					skipws(); PARSE_TRY(pchar(','));
					skipws(); PARSE_TRY(pident(sampler));
					skipws(); PARSE_TRY(pchar(')'));
					skipws(); PARSE_TRY(pchar(';'));
					commitctx();
					return true;
				}

				bool parseSetBlendStateDecl(std::string& blendStateId)
				{
					pushctx();
					PARSE_TRY(pstring("SetBlendState"));
					skipws(); PARSE_TRY(pchar('('));
					skipws(); PARSE_TRY(pident(blendStateId));
					skipws(); PARSE_TRY(pchar(')'));
					skipws(); PARSE_TRY(pchar(';'));
					commitctx();
					return true;
				}

				bool parseSetDepthStencilStateDecl(std::string& depthStencilStateId)
				{
					pushctx();
					PARSE_TRY(pstring("SetDepthStencilState"));
					skipws(); PARSE_TRY(pchar('('));
					skipws(); PARSE_TRY(pident(depthStencilStateId));
					skipws(); PARSE_TRY(pchar(')'));
					skipws(); PARSE_TRY(pchar(';'));
					commitctx();
					return true;
				}

				bool parseSetVertexArray(std::string& vertexArrayId)
				{
					pushctx();
					PARSE_TRY(pstring("SetVertexArray"));
					skipws(); PARSE_TRY(pchar('('));
					skipws(); PARSE_TRY(pident(vertexArrayId));
					skipws(); PARSE_TRY(pchar(')'));
					skipws(); PARSE_TRY(pchar(';'));
					commitctx();
					return true;
				}

				// parse a program block
				bool parseProgramBlock()
				{
					pushctx();
					std::string name;
					PARSE_TRY(pstring(TK_GLSL_PROGRAM));
					skipws();
					PARSE_TRY(pident(name));
					skipws();
					PARSE_TRY(pchar('{'));

					std::unique_ptr<GLSLProgram> prog = std::unique_ptr<GLSLProgram>(new GLSLProgram());
					prog->name = name;

					for (;;)
					{
						skipws();

						// SetShader
						GLenum stage;
						int version;
						GLSLSnippet* snippet;

						// SetSampler
						int texUnit;
						std::string id;

						if (parseProgramShaderLine(stage, version, snippet))
						{
							if (!snippet)
								continue;

							switch (stage)
							{
							case gl::VERTEX_SHADER:
								prog->VS_snippet = snippet;
								prog->VS_version = version;
								break;
							case gl::FRAGMENT_SHADER:
								prog->FS_snippet = snippet;
								prog->FS_version = version;
								break;
							case gl::GEOMETRY_SHADER:
								prog->GS_snippet = snippet;
								prog->GS_version = version;
								break;
							case gl::TESS_CONTROL_SHADER:
								prog->TCS_snippet = snippet;
								prog->TCS_version = version;
								break;
							case gl::TESS_EVALUATION_SHADER:
								prog->TES_snippet = snippet;
								prog->TES_version = version;
								break;
							case gl::COMPUTE_SHADER:
								prog->CS_snippet = snippet;
								prog->CS_version = version;
								break;
							}
						}
						else if (parseSetSamplerDeclaration(texUnit, id))
						{
							assert(texUnit < kFxMaxTextureUnits);
							prog->samplerIds[texUnit] = id;
						}
						else if (parseSetBlendStateDecl(id)) {
							prog->blendStateId = id;
						}
						else if (parseSetDepthStencilStateDecl(id)) {
							prog->depthStencilStateId = id;
						}
						else if (parseSetVertexArray(id)) {
							prog->vertexArrayId = id;
						}
						else {
							break;
						}
					}

					skipws(); PARSE_TRY(pchar('}'));
					programs[name] = std::move(prog);
					commitctx();
					return true;
				}

				bool parseKeyValuePair(std::string& outKey, std::string& outValue)
				{
					pushctx();
					PARSE_TRY(pident(outKey));
					skipws(); PARSE_TRY(pchar('='));
					skipws(); PARSE_TRY(pident(outValue));
					skipws(); PARSE_TRY(pchar(';'));
					commitctx();
					return true;
				}

				// always succeeds
				std::unordered_map<std::string, std::string> parseKeyValueDict()
				{
					std::unordered_map<std::string, std::string> dict;
					for (;;)
					{
						skipws();
						std::string param;
						std::string value;
						if (!parseKeyValuePair(param, value))
							break;
						dict[param] = value;
					}

					return std::move(dict);
				}

				bool parseTextureAddressMode(const std::string& mode, GLenum& out, const std::string& samplerName = "" /* for debugging */)
				{
					if (mode == "REPEAT") { out = gl::REPEAT; return true; }
					else if (mode == "CLAMP_TO_EDGE") { out = gl::CLAMP_TO_EDGE; return true; }
					else {
						std::clog << "WARNING: SamplerState " << samplerName << ": unrecognized address mode: " << mode << std::endl;
						return false;
					}
				}

				//============================================================
				//============================================================
				// parse a sampler state declaration
				bool parseSamplerState()
				{
					pushctx();
					std::string name;
					PARSE_TRY(ptoken(TK_SAMPLER_STATE));
					skipws(); PARSE_TRY(pident(name));
					skipws(); PARSE_TRY(pchar('{'));

					GLSamplerState samplerState;
					samplerState.useDefault = false;

					auto dict = parseKeyValueDict();

					//------- MinFilter -------
					{
						auto minFilterStr = dict["MinFilter"];
						if (minFilterStr == "NEAREST") { samplerState.minFilter = gl::NEAREST; }
						else if (minFilterStr == "LINEAR") { samplerState.minFilter = gl::LINEAR; }
						else if (minFilterStr == "") { samplerState.minFilter = gl::NEAREST; }
						else {
							std::clog << "WARNING: SamplerState " << name << ": unrecognized filter value: " << minFilterStr << std::endl;
						}
					}

					//------- MagFilter -------
					{
						auto magFilterStr = dict["MagFilter"];
						if (magFilterStr == "NEAREST") { samplerState.magFilter = gl::NEAREST; }
						else if (magFilterStr == "LINEAR") { samplerState.magFilter = gl::LINEAR; }
						else if (magFilterStr == "") { samplerState.magFilter = gl::NEAREST; }
						else {
							std::clog << "WARNING: SamplerState " << name << ": unrecognized filter value: " << magFilterStr << std::endl;
						}
					}

					//------- addrU -------
					parseTextureAddressMode(dict["AddressU"], samplerState.addrU, name);
					parseTextureAddressMode(dict["AddressV"], samplerState.addrV, name);
					//parseTextureAddressMode(dict["AddressW"], samplerState.addrW, name);

					skipws(); PARSE_TRY(pchar('}'));

					samplers[name] = samplerState;
					commitctx();
					return true;
				}

				bool parseBlendFunc(const std::string& mode, GLenum& out, const std::string& blendStateName = "" /* for debugging */)
				{
					if (mode == "FUNC_ADD") { out = gl::FUNC_ADD; return true; }
					else if (mode == "FUNC_SUBTRACT") { out = gl::FUNC_SUBTRACT; return true; }
					else if (mode == "FUNC_REVERSE_SUBTRACT") { out = gl::FUNC_REVERSE_SUBTRACT; return true; }
					else if (mode == "MIN") { out = gl::MIN; return true; }
					else if (mode == "MAX") { out = gl::MAX; return true; }
					else if (mode == "") { return true; }
					else {
						std::clog << "WARNING: BlendState " << blendStateName << ": unrecognized blend function: " << mode << std::endl;
						return false;
					}
				}

				bool parseBlendFactor(const std::string& mode, GLenum& out, const std::string& blendStateName = "" /* for debugging */)
				{
					if (mode == "ZERO") { out = gl::ZERO; return true; }
					else if (mode == "ONE") { out = gl::ONE; return true; }
					else if (mode == "SRC_COLOR") { out = gl::SRC_COLOR; return true; }
					else if (mode == "ONE_MINUS_SRC_COLOR") { out = gl::ONE_MINUS_SRC_COLOR; return true; }
					else if (mode == "DST_COLOR") { out = gl::DST_COLOR; return true; }
					else if (mode == "ONE_MINUS_DST_COLOR") { out = gl::ONE_MINUS_DST_COLOR; return true; }
					else if (mode == "SRC_ALPHA") { out = gl::SRC_ALPHA; return true; }
					else if (mode == "ONE_MINUS_SRC_ALPHA") { out = gl::ONE_MINUS_SRC_ALPHA; return true; }
					else if (mode == "DST_ALPHA") { out = gl::DST_ALPHA; return true; }
					else if (mode == "ONE_MINUS_DST_ALPHA") { out = gl::ONE_MINUS_DST_ALPHA; return true; }
					else if (mode == "CONSTANT_COLOR") { out = gl::CONSTANT_COLOR; return true; }
					else if (mode == "ONE_MINUS_CONSTANT_COLOR") { out = gl::ONE_MINUS_CONSTANT_COLOR; return true; }
					else if (mode == "CONSTANT_ALPHA") { out = gl::CONSTANT_ALPHA; return true; }
					else if (mode == "ONE_MINUS_CONSTANT_ALPHA") { out = gl::ONE_MINUS_CONSTANT_ALPHA; return true; }
					else if (mode == "SRC_ALPHA_SATURATE") { out = gl::SRC_ALPHA_SATURATE; return true; }
					else if (mode == "SRC1_COLOR") { out = gl::SRC1_COLOR; return true; }
					else if (mode == "ONE_MINUS_SRC1_COLOR") { out = gl::ONE_MINUS_SRC1_COLOR; return true; }
					else if (mode == "SRC1_ALPHA") { out = gl::SRC1_ALPHA; return true; }
					else if (mode == "ONE_MINUS_SRC1_ALPHA") { out = gl::ONE_MINUS_SRC1_ALPHA; return true; }
					else if (mode == "") { return true; }
					else {
						std::clog << "WARNING: BlendState " << blendStateName << ": unrecognized blend factor: " << mode << std::endl;
						return false;
					}
				}

				bool parseStencilFunc(const std::string& mode, GLenum& out, const std::string& dsStateName = "" /* for debugging */)
				{
					if (mode == "NEVER") { out = gl::NEVER; return true; }
					else if (mode == "LESS") { out = gl::LESS; return true; }
					else if (mode == "GREATER") { out = gl::GREATER; return true; }
					else if (mode == "EQUAL") { out = gl::EQUAL; return true; }
					else if (mode == "ALWAYS") { out = gl::ALWAYS; return true; }
					else if (mode == "LEQUAL") { out = gl::LEQUAL; return true; }
					else if (mode == "GEQUAL") { out = gl::GEQUAL; return true; }
					else if (mode == "NOTEQUAL") { out = gl::NOTEQUAL; return true; }
					else if (mode == "") { return true; }
					else {
						std::clog << "WARNING: DepthStencilState " << dsStateName << ": unrecognized stencil function: " << mode << std::endl;
						return false;
					}
				}

				bool parseStencilOp(const std::string& mode, GLenum& out, const std::string& dsStateName = "" /* for debugging */)
				{
					if (mode == "KEEP") { out = gl::KEEP; return true; }
					else if (mode == "ZERO") { out = gl::ZERO; return true; }
					else if (mode == "INCR") { out = gl::INCR; return true; }
					else if (mode == "DECR") { out = gl::DECR; return true; }
					else if (mode == "INVERT") { out = gl::INVERT; return true; }
					else if (mode == "REPLACE") { out = gl::REPLACE; return true; }
					else if (mode == "INCR_WRAP") { out = gl::INCR_WRAP; return true; }
					else if (mode == "DECR_WRAP") { out = gl::DECR_WRAP; return true; }
					else if (mode == "") { return true; }
					else {
						std::clog << "WARNING: DepthStencilState " << dsStateName << ": unrecognized stencil operation: " << mode << std::endl;
						return false;
					}
				}

				bool parseBoolean(const std::string& value, bool& out)
				{
					if (value == "TRUE") { out = true; return true; }
					else if (value == "FALSE") { out = false; return true; }
					else if (value == "") { return true; }
					else { std::clog << "Invalid boolean value, expected TRUE or FALSE" << std::endl; return false; }
				}

				//============================================================
				//============================================================
				// parse a blend state declaration
				bool parseBlendState()
				{
					pushctx();
					std::string name;
					PARSE_TRY(ptoken(TK_BLEND_STATE));
					skipws(); PARSE_TRY(pident(name));
					skipws(); PARSE_TRY(pchar('{'));
					GLBlendState blendState;
					auto dict = parseKeyValueDict();
					skipws(); PARSE_TRY(pchar('}'));

					parseBoolean(dict["Enabled"], blendState.enabled);
					parseBlendFunc(dict["ModeRGB"], blendState.modeRGB, name);
					parseBlendFunc(dict["ModeAlpha"], blendState.modeAlpha, name);
					parseBlendFactor(dict["FuncSrcRGB"], blendState.funcSrcRGB, name);
					parseBlendFactor(dict["FuncSrcAlpha"], blendState.funcSrcAlpha, name);
					parseBlendFactor(dict["FuncDstRGB"], blendState.funcDstRGB, name);
					parseBlendFactor(dict["FuncDstAlpha"], blendState.funcDstAlpha, name);

					std::clog << "Parsed blend state!" << std::endl;

					blendStates[name] = blendState;
					commitctx();
					return true;
				}

				//============================================================
				//============================================================
				// parse a depth stencil state declaration
				bool parseDepthStencilState()
				{
					pushctx();
					std::string name;
					PARSE_TRY(ptoken(TK_DEPTH_STENCIL_STATE));
					skipws(); PARSE_TRY(pident(name));
					skipws(); PARSE_TRY(pchar('{'));
					GLDepthStencilState depthStencilState;
					auto dict = parseKeyValueDict();
					skipws(); PARSE_TRY(pchar('}'));
					parseBoolean(dict["DepthTestEnable"], depthStencilState.depthTestEnable);
					parseBoolean(dict["DepthWriteEnable"], depthStencilState.depthWriteEnable);
					//depthStencilState.stencilFace = gl::FRONT_AND_BACK;
					parseBoolean(dict["StencilEnable"], depthStencilState.stencilEnable);
					parseStencilFunc(dict["StencilFunc"], depthStencilState.stencilFunc);
					parseStencilOp(dict["StencilOpSFail"], depthStencilState.stencilOpSfail);
					parseStencilOp(dict["StencilOpDPFail"], depthStencilState.stencilOpDPFail);
					parseStencilOp(dict["StencilOpDPPass"], depthStencilState.stencilOpDPPass);
					//depthStencilState.stencilRef = 1;
					//depthStencilState.stencilMask = 0xFF;
					std::clog << "Parsed depth stencil state!" << std::endl;
					depthStencilStates[name] = depthStencilState;
					commitctx();
					return true;
				}


				// MAIN
				bool parse()
				{
					while (!peof())
					{
						if (parseGlslSnippet()) {
							continue;
						}
						else if (parseVertexArrayBlock()) {
							continue;
						}
						else if (parseInclude()) {
							continue;
						}
						else if (parseProgramBlock()) {
							continue;
						}
						else if (parseSamplerState()) {
							continue;
						}
						else if (parseBlendState()) {
							continue;
						}
						else if (parseDepthStencilState()) {
							continue;
						}
						else {
							// passthrough
							global += getch();
						}
					}

					return true;
				}

		

				std::unordered_map<std::string, std::unique_ptr<GLSLProgram> > programs;
				std::unordered_map<std::string, std::unique_ptr<GLSLSnippet> > snippets;
				std::unordered_map<std::string, GLSamplerState > samplers;
				std::unordered_map<std::string, GLBlendState > blendStates;
				std::unordered_map<std::string, GLDepthStencilState > depthStencilStates;
				std::unordered_map<std::string, GLSLVertexArray > vertexArrayBlocks;
				std::string global;
			};
		}
	}
}