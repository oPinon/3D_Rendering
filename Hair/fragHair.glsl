#version 150

in vec3 normal;
out vec4 color;

vec3 lightDir = normalize(vec3(0.2,0.2,1.0));

vec4 light(vec3 normal) {
	float v = abs(dot(lightDir,normal)); v = pow(v,32);
	return 0.5*pow(v,1)*vec4(1.0) + 0.5*v*v*vec4( 1.0,0.9,0.7,1.0 ) + v*vec4(0.1,0.15,0.3,1.0);
}

void main() {

	//color = vec4( vec3(0.5) + 0.5 * normal, 1.0);
	vec3 eye = vec3(0.0,0.0,-1.0);
	vec3 normal2 = cross(cross(eye,lightDir),normal);
	float ao = pow(length(cross(eye,normal)),4);
	vec4 lightI = light(normalize(normal2));
	color = ao * ( lightI * vec4( 1.0,0.9,0.7,1.0 ) + vec4(0.3,0.1,0.0,1.0));
}
