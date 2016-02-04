

//////////////// The properties of a splat
struct BrushSplat
{
	mat2 transform;	// 2D position and orientation
	vec4 color;		// color and opacity
	vec2 center;	// center (also contained in transform)
	float width:	// width (for the round brush)
};


//////////////// Round brush kernel
float roundBrushKernel(vec2 p, vec2 c, float strokeWidth)
{
	//float falloff = strokeWidth/2.0f;
	//return 1.0/(falloff*sqrt(TWOPI))*exp(-0.5*sq(distance(p,c))/sq(falloff));
	return 1.0-smoothstep(strokeWidth-6.0f, strokeWidth+6.0f, distance(p,c));
}
