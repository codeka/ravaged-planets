#version 330

in vec3 in_colour;
in vec2 in_uv;

uniform sampler2D particle_texture;
uniform sampler2D colour_texture;

out vec4 colour;

// this is the pixel shader used by the "additive" particle effect
void main() {
  // get the colour from the texture
  float4 a = texture(particle_texture, in_uv);

  // the actual colour we use is based off the "colour_v" parameter and the 
  // "intensity" of the pixel
  float4 c1 = texture(colour_texture, float2(a.r, in_colour.r));
  float4 c2 = texture(colour_texture, float2(a.r, in_colour.g));
  float4 c = lerp(c1, c2, float4(in_colour.b, in_colour.b, in_colour.b, in_colour.b));

  // the final colour is multiplied by the intensity
  c.rgb = c.rgb * a.r * in_colour.a;
  c.a = a.r * in_colour.a;

  return c;
}

