struct Canvas
{
	vec2 size;
};

vec2 toClipPos(in Canvas canvas, in vec2 pos) 
{
	return pos / canvas.size * vec2(2.0f) - vec2(1.0f);
} 