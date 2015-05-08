#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
out vec2 tex;

void main() {
  gl_Position.xyz = position;
  gl_Position.w = 1.0;
  tex = uv;
}
