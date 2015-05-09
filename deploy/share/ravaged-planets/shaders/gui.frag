#version 330

in vec2 tex;
uniform sampler2D texsampler;

out vec4 color;

void main() {
//  color = vec4(tex, 0, 1);
  color = texture(texsampler, tex);
}
