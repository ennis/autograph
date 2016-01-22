#version 440

////////////////////////////// VERTEX SHADER
#ifdef _VERTEX_
layout (std140, binding=0) uniform SceneUniforms {
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 viewProjMatrix;
	vec2 viewportSize;
};
layout (std140, binding=1) uniform ObjectUniforms {
	mat4 modelMatrix;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normals;

out vec3 wPosition;
out vec3 wNormals;

void main() {
	gl_Position = viewProjMatrix*modelMatrix*vec4(position, 1.0f);
	wPosition = (modelMatrix*vec4(position, 1.0f)).xyz;
	wNormals = (modelMatrix*vec4(normals, 0.0f)).xyz;
}

#endif 

////////////////////////////// FRAGMENT SHADER
#ifdef _PIXEL_

in vec3 wPosition;
in vec3 wNormals;
out vec4 color;

void main()
{
	color = vec4(normalize(wNormals), 1.0f);
}
#endif
