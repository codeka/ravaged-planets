#version 330

in vec2 tex;
in vec4 light_pos;
in float NdotL;

uniform sampler2D layer1;
uniform sampler2D layer2;
uniform sampler2D layer3;
uniform sampler2D layer4;
uniform sampler2D splatt;

out vec4 colour;

float patch_size = 64;

void main() {
  // work out how much this pixel is being affected by shadow(s)
  float light_amount = 1.0;//calculate_shadow_factor(light_pos);

  // calculate the diffuse light
  float ambient = 0.5;
  float diffuse = (clamp(NdotL, 0.0, 1.0) * light_amount * ambient) + ambient;

  // get the colour information from the texture
  vec4 weights = texture(splatt, tex / patch_size);
  float weight_sum = dot(weights, vec4(1.0, 1.0, 1.0, 1.0)); // weights[0] + weight[1] + ...
  weights /= weight_sum;

  // calculate the base texture by splatting the various other textures together.
  // this only allows for four different kinds of texture, but maybe there's something
  // we can do about this (encoding the splatt texture differently, perhaps?)
  vec4 base_colour;
  vec2 uv = tex / 8.0;
  base_colour  = weights.r * texture(layer1, uv);
  base_colour += weights.g * texture(layer2, uv);
  base_colour += weights.b * texture(layer3, uv);
  base_colour += weights.a * texture(layer4, uv);

  colour = base_colour * diffuse;
}
