float PI = 3.14159265359;
float TWOPI = 2.0*3.14159265359;

// Generalized blending ('over' operator)
vec4 blend(in vec4 S, in vec4 D)
{
	vec4 O;
	O.rgb = S.rgb * S.a + D.rgb * D.a * (1.0 - S.a);
	O.a = S.a + D.a * (1.0 - S.a);
	O.rgb /= O.a;
	return O;
}

float sq(float x)
{
	return x*x;
}


vec3 evalNormalMap(vec4 v)
{
	return v.xyz * 2.0 - vec3(1.0); 
}

float shadingTerm(in sampler2D normalMap, ivec2 texelCoords, vec3 lightPos)
{
	vec4 c = texelFetch(normalMap, texelCoords, 0);
	vec3 N = evalNormalMap(c);
	return dot(N, -lightPos);
}

