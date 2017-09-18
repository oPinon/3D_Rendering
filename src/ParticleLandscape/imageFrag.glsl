#version 150

in vec3 normal;
uniform sampler2D image;
in vec3 pos;
out vec4 color;

vec3 lightDir = normalize(vec3(0.2, 0.2, 1.0));

vec4 light(vec3 normal) {
	float v = dot(lightDir, normal)*0.5 + 0.5;
	return 0.5*pow(v, 64)*vec4(1.0) + 0.5*v*v*vec4(0.9, 0.85, 0.7, 1.0) + v*vec4(0.1, 0.15, 0.3, 1.0);
}

void main() {

	color = light(normalize(normal)) * texture(image, (vec2(1, 1) + pos.xy) / 2);
}
