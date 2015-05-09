#version 330

uniform mat4 worldviewproj;
uniform mat4 worldview;
uniform mat4 view_to_light;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec2 tex;
out vec4 light_pos;
out float NdotL;

void main() {
  gl_Position = worldviewproj * vec4(position, 1);
  NdotL = dot(normal, vec3(0.485, 0.485, 0.727));

  tex = vec2(position.x, position.z);

  // transform the position to light projection space
  vec4 view_pos = worldview * vec4(position, 1);
  light_pos = view_to_light * view_pos;
}

