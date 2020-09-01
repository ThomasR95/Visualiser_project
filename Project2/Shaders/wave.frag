#version 130

uniform vec2 point;
uniform float time;
uniform float amplitude;
uniform float waveWidth;
uniform float fadeDist;
uniform float speed;
uniform float opacity;
uniform float scrw;
uniform float scrh;
uniform vec4 inColour;
void main()
{
	vec2 coOrd = gl_FragCoord.xy;
	vec4 colour = vec4(0,0,0,opacity);

	vec2 AtoB = coOrd - point;
	float dist = sqrt(AtoB.x*AtoB.x + AtoB.y*AtoB.y);

	dist = dist*(0.01*dist);

	colour.xyz += inColour.xyz * sin(dist/waveWidth+(-time*speed))/amplitude;

	float fade = clamp(dist / fadeDist, 0.0, 1.0);

	colour.xyz -= vec3(fade, fade, fade);

	if(coOrd.x == scrw)
		colour.xyz = vec3(0,0,0);

	if(coOrd.y == 0)
		colour.xyz = vec3(0,0,0);

	float fadeA = clamp((fade*scrw)/(scrw - fadeDist),0.0,1.0);
	colour.w = opacity + opacity*fadeA;

	gl_FragColor = colour;
}