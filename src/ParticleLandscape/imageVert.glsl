#version 110

varying vec3 normal;
varying vec3 pos;

void main() {

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	pos = gl_Vertex.xyz;
}
