#version 130

in vec2 position; // pixel position in [-1;1]

// camera position
uniform vec3 camPos = vec3(0.0, -1.0, 0.0);
uniform vec3 camDir = vec3(0.0, 1.0, 0.0);
uniform vec3 camUp = vec3(0.0, 0.0, 1.0);
uniform vec3 camLeft = vec3(-1.0, 0.0, 0.0);
uniform float ratio = 1.0;
float focal = 1.0;

out vec4 color;

vec3 powWN3( vec3 src ) {

	return vec3(
		src.x*src.x - src.y*src.y - src.z*src.z,
		2.0 * src.x * src.z,
		2.0 * src.x * src.y
	);
}

vec3 powWN4( vec3 src ) {

	float x = src.x, y = src.y, z = src.z;
	return vec3(
		x*x*x*x*x - 10 * x*x*x*(y*y + z*z) + 5 * x*(y*y*y*y + z*z*z*z),
		y*y*y*y*y - 10 * y*y*y*(z*z + x*x) + 5 * y*(z*z*z*z + x*x*x*x),
		z*z*z*z*z - 10 * z*z*z*(x*x + y*y) + 5 * z*(x*x*x*x + y*y*y*y)
	);
}

float length2( vec3 pos ) {

	return pos.x*pos.x + pos.y*pos.y + pos.z*pos.z;
}

bool fracValue( vec3 pos ) {

	vec3 p = pos;
	for(int i = 0; i < 30; i++) {
		if(length2(p) > 1) { return false; }
		p = powWN4(p) + pos;
	}
	return true;
}

void main() {

	// direction of the ray
	vec3 dir = normalize(
		focal * camDir
		+ ratio * position.x * camLeft
		+ position.y * camUp
	);
	// step used for the ray marching
	vec3 step = 0.03 * dir;
	// variable position of the ray marching
	vec3 pos = camPos;
	// resulting depth
	float depth = 1.0;

	for(int i = 0; i < 50; i++) {

		pos += step;
		if(fracValue(pos)) {
			depth = i / 50.0;
			break;
		}
	}

	color = vec4( vec3(1-depth), 1.0 );
	//color = vec4((vec2(1.0)+position)/2,0.0,1.0);
}