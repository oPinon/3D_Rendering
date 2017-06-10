
varying vec3 posAbs;
varying vec3 posRel;

void main() {

	posAbs = gl_Vertex.xyz;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	posRel = gl_Position.xyz;
}