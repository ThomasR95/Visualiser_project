#version 440
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
uniform bool falloff;
void main()
{
	vec2 coOrd = gl_FragCoord.xy;
	vec4 colour = vec4(0,0,0,opacity);

	vec2 AtoB = coOrd - point;
	float dist1 = max(AtoB.x - AtoB.y, AtoB.y - AtoB.x);
	float dist2 = max(AtoB.x + AtoB.y, AtoB.y + AtoB.x);
	float dist3 = sqrt(AtoB.x*AtoB.x + AtoB.y*AtoB.y) + waveWidth;

	float dist4 = max(dist1, abs(dist2));
	float dist = max(dist4, dist3);

	dist = dist*(0.01*dist);


	if(speed < 0)
		colour.xyz += inColour.xyz * sin(dist/waveWidth+(time*abs(speed)))/amplitude;
	else
		colour.xyz += inColour.xyz * sin(dist/waveWidth+(-time*abs(speed)))/amplitude;
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