shader_type spatial;

uniform float height_scale = 1.5;
uniform sampler2D noise;

varying vec2 vertex_position;

void vertex() {
  vertex_position = VERTEX.xz / 32.0;
  float height = texture(noise, vertex_position).x * height_scale;
  VERTEX.y += height * height_scale;
}