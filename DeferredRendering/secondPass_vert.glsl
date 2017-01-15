#version 110

varying vec2 pos;

void main() {

	gl_Position = vec4(gl_Vertex.xy,0.0,1.0);
	pos = gl_Vertex.xy;
}
