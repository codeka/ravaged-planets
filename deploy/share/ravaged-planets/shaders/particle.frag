#version 330

in vec4 colour;
in vec2 uv;

uniform sampler2D particle_texture;
uniform sampler2D colour_texture;

out vec4 out_colour;

// this is the pixel shader used by the "additive" particle effect
void main() {
/* Non-additive:
  // get the colour from the texture
  vec4 c = texture(particle_texture, uv);

  // and blend the texture colour with the vertex colour
  c.rgb = (c.rgb * colour.a) + (colour.rgb * (1 - colour.a));
  c.a = c.a * colour.a;

  out_colour = c;
*/
  // get the "intensity" from the particle texture
  vec4 a = texture(particle_texture, uv);

  // look up the colour for that intensity for the two colours we are blending, then
  // combine the two colours based on whatever value of colour.b we have.
  vec4 c1 = texture(colour_texture, vec2(a.r, colour.r));
  vec4 c2 = texture(colour_texture, vec2(a.r, colour.g));
  out_colour = mix(c1, c2, vec4(colour.b, colour.b, colour.b, colour.b));

  // the final colour is multiplied by the intensity
  out_colour.rgb = out_colour.rgb * a.r * colour.a;
  out_colour.a = a.r * colour.b;
}

