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
