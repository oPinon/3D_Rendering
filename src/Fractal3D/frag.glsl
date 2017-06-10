#version 130

in vec2 position; // pixel position in [-1;1]

// camera position
uniform vec3 camPos = vec3(0.0, -1.0, 0.0);
uniform vec3 camDir = vec3(0.0, 1.0, 0.0);
uniform vec3 camUp = vec3(0.0, 0.0, 1.0);
uniform vec3 camLeft = vec3(-1.0, 0.0, 0.0);
uniform float ratio = 1.0;
float focal = 1.0;

// fractal order
uniform float order = 8.0;

// MatCap shading
uniform sampler2D matCap;

out vec4 color;

// Nylander's power formula
void powN(float p, inout vec3 z, float zr0, inout float dr)
{
    float zo0 = asin(z.z / zr0);
    float zi0 = atan(z.y, z.x);
    float zr = pow(zr0, p - 1.0);
    float zo = zo0 * p;
    float zi = zi0 * p;
    float czo = cos(zo);

    dr = zr * dr * p + 1.0;
    zr *= zr0;

    z = zr * vec3(czo * cos(zi), czo * sin(zi), sin(zo));
}

// distance to the fractal
float dist(vec3 c) {

	vec3 z = c;
	float dr = 1.0;
	float r = length(z);

	for(int i = 0; i < 16; i++) {

		powN(order, z, r, dr);

		z += c;

		r = length(z);

		if(r > 2) { break; }
	}

	return 0.5 * log(r) * r / dr;
}

// normal on a point 'z' of the surface
// step : precision of the approximation (acts like anti-aliasing)
vec3 fracNormal(vec3 z, float step) {

	return normalize(vec3(
		dist(z + step * vec3(1,0,0)) - dist(z + step * vec3(-1,0,0)),
		dist(z + step * vec3(0,1,0)) - dist(z + step * vec3(0,-1,0)),
		dist(z + step * vec3(0,0,1)) - dist( z + step * vec3(0,0,-1))
	));
}

void main() {

	// direction of the ray
	vec3 dir = normalize(
		focal * camDir // fisheye -> * (1-0.5*length(vec2( ratio * position.x, position.y)))
		+ ratio * position.x * camLeft
		+ position.y * camUp
	);
	vec3 pos = camPos; // variable position of the ray marching
	float depth = 0.0; // distance from surface

	float minStep = 0.000001; // minimal step before stopping ray-marching
	float step; // adaptive step for ray-marching

	float maxDist = 32 * dist(camPos); // distance before stopping ray-marching

	bool background = false; // is it a background pixel

	for(int i = 0; i < 32; i++) {

		step = dist(pos);
		depth += step;
		pos += step * dir;
		background = depth > maxDist;
		if(background || step < minStep) { break; }
	}

	float fog = depth/maxDist; // fog intensity
	vec3 normal = fracNormal(pos,0.01*depth); // surface normal

	vec3 locNormal = vec3(
		dot(normal, camLeft),
		-dot(normal, camUp),
		-dot(normal, camLeft)
	);

	//vec3 shade = max(0,0.5+0.5*dot(normal,dir)) * vec3(1.0);
	vec3 shade = texture( matCap, (1+locNormal.xy)/2 ).rgb;

	color = vec4( fog * vec3(1.0) + (1-fog) * shade, 1.0 ); // output color
}