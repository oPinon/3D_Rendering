#version 130


uniform float width;
uniform float height;

uniform sampler2D backRender;

in vec3 posAbs;
in vec3 posRel;
out vec4 color;

void main() {

	vec4 back = texture2D( backRender, vec2( gl_FragCoord.x / width, gl_FragCoord.y / height ) );

	vec3 diff = (vec3(1.0)+posAbs)/2 - back.xyz;
	float depth = length(diff)/2;
	color = vec4( depth * vec3(1.0), 1.0 );
}