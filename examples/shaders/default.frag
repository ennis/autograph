#version 440
layout (binding=0) uniform sampler2D mainTex;

in vec2 texcoords;
out vec4 color;

void main()
{
    color = texture(mainTex, texcoords);
}
