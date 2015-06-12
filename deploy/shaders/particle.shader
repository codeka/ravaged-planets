<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 worldviewproj;

    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec4 in_colour;
    layout (location = 2) in vec2 in_uv;

    out vec4 colour;
    out vec2 uv;

    void main() {
      gl_Position = worldviewproj * vec4(in_position, 1);
      colour = in_colour;
      uv = in_uv;
    }
  ]]></source>
  <source name="fragment-normal"><![CDATA[
    in vec4 colour;
    in vec2 uv;

    uniform sampler2D particle_texture;
    uniform sampler2D colour_texture;

    out vec4 out_colour;

    // this is the pixel shader used by the "additive" particle effect
    void main() {
      // get the colour from the texture
      vec4 c = texture(particle_texture, uv);

      // and blend the texture colour with the vertex colour
      c.rgb = (c.rgb * colour.a) + (colour.rgb * (1 - colour.a));
      c.a = c.a * colour.a;

      out_colour = c;
    }
  ]]></source>
  <source name="fragment-additive"><![CDATA[
    in vec4 colour;
    in vec2 uv;

    uniform sampler2D particle_texture;
    uniform sampler2D colour_texture;

    out vec4 out_colour;

    // this is the pixel shader used by the "additive" particle effect
    void main() {
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
  ]]></source>
  <program name="particle-additive">
    <vertex-shader source="vertex" />
    <fragment-shader source="fragment-additive" />
    <state name="z-write" value="off" />
    <state name="z-test" value="on" />
    <state name="blend" value="additive" />
  </program>
  <program name="particle-normal">
    <vertex-shader source="vertex" />
    <fragment-shader source="fragment-normal" />
    <state name="z-write" value="off" />
    <state name="z-test" value="on" />
    <state name="blend" value="alpha" />
  </program>
</shader>