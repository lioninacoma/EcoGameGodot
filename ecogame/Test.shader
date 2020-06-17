shader_type spatial;
render_mode unshaded;

uniform sampler2D Front: hint_albedo;
uniform sampler2D Right: hint_albedo;
uniform sampler2D Left: hint_albedo;
uniform sampler2D Back: hint_albedo;
uniform sampler2D Up: hint_albedo;
uniform sampler2D Down: hint_albedo;
uniform sampler2D Surface: hint_albedo;

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

varying vec3 vertex_color;

void vertex() {
	vec4 nor = vec4(-NORMAL, 0.);
	vertex_color = cubemap(nor.xyz).xyz;
	float height = vertex_color.x * 2.;
	VERTEX += NORMAL * height;
}

void fragment() {
	ALPHA = 1.;
	ALBEDO = vec3(1, 0, 0);
}