#version 330

uniform mat4 worldviewproj;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec4 in_colour;
layout (location = 2) in vec2 in_uv;

out vec4 out_colour;
out vec2 out_uv;

void main() {
  gl_Position = worldviewproj * vec4(in_position, 1);
  out_colour = in_colour;
  out_uv = in_uv;
}
