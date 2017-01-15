#version 150

out vec4 color;
in vec2 pos;
uniform sampler2D passes;

vec4 blur(vec2 center, float dist) {

	vec4 dst = vec4(0);
	int sum = 0;
	int n = 5;
	float z = texture(passes,center).a;
	for(int y = -n; y < n; y++) {
		for(int x = -n; x < n; x++) {
			vec2 p = center + dist * vec2(x,y)/n;
			vec4 v = texture(passes,p);
			if(v.a <= z) {
				dst += v;
				sum++;
			}
		}
	}
	return dst / sum;
}

float ao(vec2 center, float dist) {

	float dst = 0;
	int sum = 0;
	int n = 5;
	float z = texture(passes,center).a;
	for(int y = -n; y < n; y++) {
		for(int x = -n; x < n; x++) {
			vec2 p = center + dist * vec2(x,y)/n;
			vec4 v = texture(passes,p);
			if((abs(v.a-z)<0.1) && (abs(v.a-z)>0.003)) {
				if(v.a < z) {
					dst ++;
				}
			} else {
				dst++;
			}
			sum++;
		}
	}
	return z < 0.5 ? 1 : clamp(1*pow(dst / sum,1),0.0,1.0);
}

float randF(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 randV3(vec2 co) {
	vec3 dst;
	dst.x = randF(co);
	dst.y = randF(co*0.5);
	dst.z = randF(vec2(dst.x,dst.y));
	return dst;
}

float ao2(vec2 pos, float dist) {
	int n = 100;
	float sum = 0;
	vec3 offset = randV3(pos);
	float z = texture(passes,pos).a;
	for(int i = 0; i < n; i++) {

		float z2 = texture(passes,pos+dist*offset.xy).a;
		if(abs(z-z2) > 0.2) { sum++; }
		else if(z+offset.z>=z2) { sum += 2*(z+offset.z-z2); }
		offset = randV3(offset.xy);
	}
	return clamp(pow((1.0*sum)/n*0.9+0.1,5),0,1);
}

void main() {

	vec2 posN = (vec2(1)+pos)/2;
	vec4 pass = texture(passes,posN);

	/*{ // Depth of field
		float z = pass.a;
		//color = vec4(1.0) * 10*abs(z-0.7);
		float level = abs(z-0.7);
		color = blur(posN,0.2*level);
	}*/

	color = ao(posN, 0.01) * vec4(1.0);//pass;
}
