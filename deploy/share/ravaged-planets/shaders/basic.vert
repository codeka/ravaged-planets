#version 330

uniform mat4 worldviewproj;
layout (location = 0) in vec3 position;

void main() {
  gl_Position = worldviewproj * vec4(position, 1);
}
