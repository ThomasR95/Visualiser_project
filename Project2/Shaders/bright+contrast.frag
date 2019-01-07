
uniform sampler2D texture;
uniform float contrast;
uniform float brightness;
void main()
{
	vec2 coOrd = gl_TexCoord[0].xy;
	vec4 colour = texture2D(texture, coOrd);
	colour = colour * gl_Color;
	colour.xyz = clamp((colour.xyz - 0.5)*contrast + 0.5 + brightness, 0.0, 1.0);

	gl_FragColor = colour;
}