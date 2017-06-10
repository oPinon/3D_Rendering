varying vec2 position;

void main() {

	gl_Position = gl_Vertex;
	position = gl_Vertex.xy;
}