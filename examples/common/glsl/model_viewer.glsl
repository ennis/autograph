#version 440

////////////////////////////// VERTEX SHADER
#ifdef _VERTEX_
layout (std140, binding=0) uniform SceneUniforms {
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 viewProjMatrix;
	vec2 viewportSize;
};
layout (std140, binding=0) uniform ObjectUniforms {
	mat4 modelMatrix;
};

layout(location=0) in vec3 position;

out vec2 texcoords;

void main() {
	gl_Position = vec4((modelMatrix*vec4(position, 1.0f)).xyz, 1.0f);
        texcoords = position.xy;
}

#endif 

////////////////////////////// FRAGMENT SHADER
#ifdef _PIXEL_
layout (binding=0) uniform sampler2D mainTex;

in vec2 texcoords;
out vec4 color;

void main()
{
    color = texture(mainTex, texcoords);
}
#endif
