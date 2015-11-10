#version 130

in vec3 posAbs;
out vec4 color;

void main() {

	color = vec4(
		(vec3(1.0)+posAbs)/2.0,
		1.0
	);
}