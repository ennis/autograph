#version 440

// uniform buffer bindings:
// 0: scene and pass global (includes lights)
// 1: per-material data
// 2: per-object data 

layout (std140, binding=0) uniform ObjectData {
	mat4 modelMatrix;
};

layout(location=0) in vec3 position;

void main() {
	gl_Position = vec4((modelMatrix*vec4(position, 1.0f)).xyz, 1.0f);
}