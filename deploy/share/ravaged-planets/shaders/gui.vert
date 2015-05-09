#version 330

uniform mat4 transform;
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

out vec2 tex;

void main() {
  gl_Position = transform * vec4(position, 1);
  tex = uv;
}
