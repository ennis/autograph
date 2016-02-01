struct SceneData
{
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 viewProjMatrix;
	vec2 viewportSize;
};

vec2 fragToTexCoord(vec2 viewportSize, vec2 fragCoord)
{
	return vec2(fragCoord.x, viewportSize.y-fragCoord.y) / viewportSize;
}
