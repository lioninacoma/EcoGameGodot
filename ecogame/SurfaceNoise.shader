shader_type spatial;

uniform float height_scale = 16.0;

float hash(vec2 p) {
	return fract(sin(dot(p * 17.17, vec2(14.91, 67.31))) * 4791.9511);
}

float noise(vec2 x) {
	vec2 p = floor(x);
	vec2 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	vec2 a = vec2(1.0, 0.0);
	return mix(mix(hash(p + a.yy), hash(p + a.xy), f.x),
		mix(hash(p + a.yx), hash(p + a.xx), f.x), f.y);
}

float fbm(vec2 x) {
	float height = 0.0;
	float amplitude = 0.5;
	float frequency = 0.01;
	for (int i = 0; i < 6; i++){
		height += noise(x * frequency) * amplitude;
		amplitude *= 0.5;
		frequency *= 2.0;
	}
	return height;
}

void vertex() {
	float height = fbm(VERTEX.xz * 4.0);
	VERTEX.y += height * height_scale;
	
	vec2 e = vec2(0.01, 0.0);
	vec3 normal = normalize(vec3(fbm(VERTEX.xz - e) - fbm(VERTEX.xz + e), 2.0 * e.x, fbm(VERTEX.xz - e.yx) - fbm(VERTEX.xz + e.yx)));
	NORMAL = normal;
	
	COLOR.xyz = vec3(height);
}

void fragment(){
	ALBEDO = COLOR.xyz;
}