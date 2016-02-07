#ifndef STATE_HPP
#define STATE_HPP

#include "gl_core_4_5.hpp"

namespace ag {
namespace opengl {
struct GLSamplerState {
  bool useDefault = true;
  GLenum minFilter = gl::NEAREST;
  GLenum magFilter = gl::NEAREST;
  GLenum addrU = gl::CLAMP_TO_EDGE;
  GLenum addrV = gl::CLAMP_TO_EDGE;
  GLenum addrW = gl::CLAMP_TO_EDGE;
};

struct GLBlendState {
  bool enabled = true;
  GLenum modeRGB = gl::FUNC_ADD;
  GLenum modeAlpha = gl::FUNC_ADD;
  GLenum funcSrcRGB = gl::SRC_ALPHA;
  GLenum funcDstRGB = gl::ONE_MINUS_SRC_ALPHA;
  GLenum funcSrcAlpha = gl::ONE;
  GLenum funcDstAlpha = gl::ZERO;
};

struct GLDepthStencilState {
  bool depthTestEnable = false;
  bool depthWriteEnable = false;
  bool stencilEnable = false;
  GLenum stencilFace = gl::FRONT_AND_BACK;
  GLenum stencilFunc;
  GLint stencilRef = 0;
  GLuint stencilMask = 0xFFFFFFFF;
  GLenum stencilOpSfail;
  GLenum stencilOpDPFail;
  GLenum stencilOpDPPass;
};

struct GLRasterizerState {
  GLenum fillMode = gl::FILL;
};
}
}

#endif // !STATE_HPP
