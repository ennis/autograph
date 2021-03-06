#version 450
#include "scene.glsl"

layout(std140, binding = 0) uniform U0 { SceneData sceneData; };
layout(std140, binding = 1) uniform U1 { mat4 modelMatrix; };

#ifdef _VERTEX_
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texcoords;
out vec3 wN;
out vec2 fTexcoords;
void main() {
  vec4 wPos_tmp = modelMatrix * vec4(position, 1.0);
  gl_Position = sceneData.projMatrix * sceneData.viewMatrix * wPos_tmp;
  // assume no scaling in modelMatrix
  wN = (modelMatrix * vec4(normal, 0.0)).xyz;
  fTexcoords = texcoords;
}
#endif

#ifdef _PIXEL_
in vec3 wN;
in vec2 fTexcoords;
layout(location = 0) out vec4 rtNormals;
layout(location = 1) out float rtStencil;

void main() {
  // normalize to [0;1]
  rtNormals = vec4(normalize(wN) / 2.0 + vec3(0.5), 1.0f);
  rtStencil = 1.0;
}
#endif
