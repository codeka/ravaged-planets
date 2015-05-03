#version 330

uniform mat4 worldviewproj;
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

void main() {
  gl_Position = worldviewproj * vec4(position, 1.0);
}
