#version 330

in vec4 in_colour;
in vec2 in_uv;

uniform sampler2D particle_texture;
uniform sampler2D colour_texture;

out vec4 colour;

// this is the pixel shader used by the "additive" particle effect
void main() {
  colour = vec4(in_uv.x, in_uv.y, 1, 1);
/*
  // get the colour from the texture
  vec4 a = texture(particle_texture, in_uv);

  // the actual colour we use is based off the "colour_v" parameter and the 
  // "intensity" of the pixel
  vec4 c1 = texture(colour_texture, vec2(a.r, in_colour.r));
  vec4 c2 = texture(colour_texture, vec2(a.r, in_colour.g));
  colour = mix(c1, c2, vec4(in_colour.b, in_colour.b, in_colour.b, in_colour.b));

  // the final colour is multiplied by the intensity
  colour.rgb = colour.rgb * a.r * in_colour.a;
  colour.a = a.r * in_colour.a;
*/
}

