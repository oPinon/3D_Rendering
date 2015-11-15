#version 130

uniform float width;
uniform float height;
uniform int offset;

uniform sampler2D backRender;
uniform sampler3D voxels;

in vec3 posAbs;
in vec3 posRel;
out vec4 color;

// sphere :
/*float getDensity( vec3 pos ) {
	float dist = length( pos - vec3(0.5,0.5,0.5) );
	return dist < 0.5 ? 1 : 0;
}*/

// cube
/*float getDensity( vec3 pos ) { return 1; }*/

// 3d sampler
float getDensity( vec3 pos ) {

	return max(0,exp(5*texture(voxels, pos.yzx).r)-1);
}

void main() {

	float step = 0.01; // precision of the ray marching

	vec3 end = texture( backRender,
		vec2(
			gl_FragCoord.x / width,
			gl_FragCoord.y / height
		)
	).xyz;

	vec3 start = (vec3(1.0)+posAbs)/2;
	float distance = length(start - end);

	float nbSteps = distance / step;
	float sum = 0;
	for(int i = 0; i < nbSteps; i++) { // offset by 0.5 ?
		vec3 pos = ((nbSteps-i) * start + i * end ) / nbSteps;
		sum += step * getDensity(pos);
	}

	//color = vec4( vec3(sum)/2, 1.0);
	sum *= exp(0.1*offset);

	color = vec4( // blue color ramp

		sum/3,
		sum/2,
		sum,
		1.0
	);
}