

//////////////// The properties of a splat
struct BrushSplat
{
	mat3x4 transform;	// 2D position and orientation
	vec2 center;	// center (also contained in transform)
	float width;
	float smoothness;
};

//////////////// Round brush kernel
float roundBrushKernel(vec2 p, vec2 c, float strokeWidth, float smoothness)
{
	//float falloff = strokeWidth/2.0f;
	//return 1.0/(falloff*sqrt(TWOPI))*exp(-0.5*sq(distance(p,c))/sq(falloff));
	return 1.0-smoothstep((1.0f-smoothness)*strokeWidth-1.0f, strokeWidth-1.0f, distance(p,c));
}
