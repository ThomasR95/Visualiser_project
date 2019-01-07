#version 140

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
uniform bool blackBetweenWaves = false;
void main()
{
	vec2 coOrd = gl_FragCoord.xy;
	vec4 colour = vec4(0,0,0,opacity);

	vec2 AtoB = coOrd - point;
	float dist = max(abs(AtoB.x), abs(AtoB.y));

	dist = dist*(0.01*dist);
	
	float mult = sin(dist/waveWidth+(-time*speed))/amplitude;

	colour.xyz += inColour.xyz * mult;
	
	
	float fade = clamp(dist / fadeDist, 0.0, 1.0);

	colour.xyz -= vec3(fade, fade, fade);

	if(coOrd.x == scrw)
		colour.xyz = vec3(0,0,0);

	if(coOrd.y == 0)
		colour.xyz = vec3(0,0,0);

	float fadeA = clamp((fade*scrw)/(scrw - fadeDist),0.0,1.0);
	colour.w = opacity + opacity*fadeA;
	if(blackBetweenWaves)
	{
		
		float blackfade = clamp(dist / (fadeDist*0.5), 0.0, 1.0);
		colour.w += max(0,(1-blackfade)*opacity*2);

	}

	gl_FragColor = colour;
}