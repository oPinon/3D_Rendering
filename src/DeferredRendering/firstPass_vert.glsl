#version 110

varying vec3 normal;
varying vec3 pos;

void main() {

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	pos = gl_Position.xyz/gl_Position.w;
	normal = normalize(gl_NormalMatrix * gl_Normal);
}
