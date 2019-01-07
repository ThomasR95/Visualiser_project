
uniform sampler2D texture;
uniform float minOpacity = 0.1;
void main()
{
	vec2 coOrd = gl_TexCoord[0].xy;
	vec4 colour = texture2D(texture, coOrd) * gl_Color;

	float originalAlpha = colour.w;
	colour.w *= max(max(colour.r, colour.g), colour.b);
	colour.w = clamp(colour.w, min(minOpacity,originalAlpha), 0.8);

	gl_FragColor = colour;
}