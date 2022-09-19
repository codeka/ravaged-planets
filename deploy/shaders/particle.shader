<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 worldviewproj;

    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec4 in_color;
    layout (location = 2) in vec2 in_uv;

    out vec4 color;
    out vec2 uv;

    void main() {
      gl_Position = worldviewproj * vec4(in_position, 1);
      color = in_color;
      uv = in_uv;
    }
  ]]></source>
  <source name="fragment-normal"><![CDATA[
    in vec4 color;
    in vec2 uv;

    uniform sampler2D particle_texture;
    uniform sampler2D color_texture;

    out vec4 out_color;

    // this is the pixel shader used by the "normal" particle effect
    void main() {
      // get the color from the texture
      vec4 c = texture(particle_texture, uv);

      // and blend the texture color with the vertex color
      //c.rgb = (c.rgb * color.a) + (color.rgb * (1 - color.a));
      c.a = c.a * color.a;

      out_color = c;
    }
  ]]></source>
  <source name="fragment-additive"><![CDATA[
    in vec4 color;
    in vec2 uv;

    uniform sampler2D particle_texture;
    uniform sampler2D color_texture;

    out vec4 out_color;

    // this is the pixel shader used by the "additive" particle effect
    void main() {
      // get the "intensity" from the particle texture
      vec4 a = texture(particle_texture, uv);

      // look up the color for that intensity for the two colors we are blending, then
      // combine the two colors based on whatever value of color.b we have.
      vec4 c1 = texture(color_texture, vec2(a.r + 0.0078125, color.r + 0.0078125));
      vec4 c2 = texture(color_texture, vec2(a.r + 0.0078125, color.g + 0.0078125));
      out_color = mix(c1, c2, vec4(color.b, color.b, color.b, color.b));

      // the final color is multiplied by the intensity
      out_color.rgb = out_color.rgb * a.r * color.a;
      out_color.a = a.r * color.b;
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
