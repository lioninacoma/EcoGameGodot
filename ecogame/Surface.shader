shader_type spatial;
render_mode blend_mix,depth_draw_opaque,cull_back,diffuse_burley,specular_schlick_ggx;
uniform vec4 albedo : hint_color;
uniform sampler2D texture_albedo : hint_albedo;
uniform float specular;
uniform float metallic;
uniform float roughness : hint_range(0,1);
uniform float point_size : hint_range(0,128);
varying vec3 uv1_triplanar_pos;
uniform float uv1_blend_sharpness;
varying vec3 uv1_power_normal;
uniform vec3 uv1_scale;
uniform vec3 uv1_offset;
uniform vec3 uv2_scale;
uniform vec3 uv2_offset;

uniform sampler2D Front: hint_albedo;
uniform sampler2D Right: hint_albedo;
uniform sampler2D Left: hint_albedo;
uniform sampler2D Back: hint_albedo;
uniform sampler2D Up: hint_albedo;
uniform sampler2D Down: hint_albedo;
uniform sampler2D Surface: hint_albedo;
uniform float height;

vec4 cubemap(in vec3 d) {
	vec3 a = abs(d);
	bvec3 ip = greaterThan(d, vec3(0.));
	vec2 uvc;

	if (ip.x && a.x >= a.y && a.x >= a.z) {
		uvc.x = -d.z;
		uvc.y = d.y;
		return texture(Front, 0.5 * (uvc / a.x + 1.));
	} else if (!ip.x && a.x >= a.y && a.x >= a.z) {
		uvc.x = d.z;
		uvc.y = d.y;
		return texture(Back, 0.5 * (uvc / a.x + 1.));
	} else if (ip.y && a.y >= a.x && a.y >= a.z) {
		uvc.x = d.x;
		uvc.y = -d.z;
		return texture(Up, 0.5 * (uvc / a.y + 1.));
	} else if (!ip.y && a.y >= a.x && a.y >= a.z) {
		uvc.x = d.x;
		uvc.y = d.z;
		return texture(Down, 0.5 * (uvc / a.y + 1.));
	} else if (ip.z && a.z >= a.x && a.z >= a.y) {
		uvc.x = d.x;
		uvc.y = d.y;
		return texture(Left, 0.5 * (uvc / a.z + 1.));
	} else if (!ip.z && a.z >= a.x && a.z >= a.y) {
		uvc.x = -d.x;
		uvc.y = d.y;
		return texture(Right, 0.5 * (uvc / a.z + 1.));
	}

	return vec4(0.);
}

void vertex() {
	TANGENT = vec3(0.0,0.0,-1.0) * abs(NORMAL.x);
	TANGENT+= vec3(1.0,0.0,0.0) * abs(NORMAL.y);
	TANGENT+= vec3(1.0,0.0,0.0) * abs(NORMAL.z);
	TANGENT = normalize(TANGENT);
	BINORMAL = vec3(0.0,-1.0,0.0) * abs(NORMAL.x);
	BINORMAL+= vec3(0.0,0.0,1.0) * abs(NORMAL.y);
	BINORMAL+= vec3(0.0,-1.0,0.0) * abs(NORMAL.z);
	BINORMAL = normalize(BINORMAL);
	uv1_power_normal=pow(abs(NORMAL),vec3(uv1_blend_sharpness));
	uv1_power_normal/=dot(uv1_power_normal,vec3(1.0));
	uv1_triplanar_pos = VERTEX * uv1_scale + uv1_offset;
	uv1_triplanar_pos *= vec3(1.0,-1.0, 1.0);
	
	vec4 nor = vec4(-NORMAL, 0.);
	vec3 vertex_color = cubemap(nor.xyz).xyz;
	float h = vertex_color.x * height;
	VERTEX += NORMAL * h;
}


vec4 triplanar_texture(sampler2D p_sampler,vec3 p_weights,vec3 p_triplanar_pos) {
	vec4 samp=vec4(0.0);
	samp+= texture(p_sampler,p_triplanar_pos.xy) * p_weights.z;
	samp+= texture(p_sampler,p_triplanar_pos.xz) * p_weights.y;
	samp+= texture(p_sampler,p_triplanar_pos.zy * vec2(-1.0,1.0)) * p_weights.x;
	return samp;
}


void fragment() {
	vec4 albedo_tex = triplanar_texture(texture_albedo,uv1_power_normal,uv1_triplanar_pos);
	ALBEDO = albedo.rgb * albedo_tex.rgb;
	METALLIC = metallic;
	ROUGHNESS = roughness;
	SPECULAR = specular;
}
