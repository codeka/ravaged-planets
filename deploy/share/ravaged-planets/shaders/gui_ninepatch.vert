#version 330

uniform mat4 pos_transform;
uniform mat4 uv_transform;
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

out vec2 tex;

void main() {
  gl_Position = pos_transform * vec4(position, 1);
  vec4 transformed_uv = uv_transform * vec4(uv, 0, 1);
  tex = transformed_uv.xy;
}
