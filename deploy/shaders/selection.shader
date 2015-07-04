<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 worldviewproj;

    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec2 in_uv;

    out vec2 uv;

    void main() {
      gl_Position = worldviewproj * vec4(in_position, 1);
      uv = in_uv;
    }
  ]]></source>
  <source name="fragment"><![CDATA[
    uniform vec4 selection_colour;
    in vec2 uv;
    out vec4 colour;

    void main() {
      // get the "base" colour, which is the current selection colour
      vec4 base_colour = selection_colour;

      // work out our distance from the centre of the quad, radius goes from 0 to 1 (0 at the centre, 1 at the edges)
      float radius = distance(uv, vec2(0.5,0.5)) * 2;
      if (radius > 1) { // it'll be > 1 in the area outside the main circle
        base_colour = vec4(0, 0, 0, 0);
      }

      // we want to fade from full colour at radius = 1 to completely transparent at radius = 0.75
      radius = clamp(radius - 0.75, 0, 1) * 4;

      // and just lerp between totally transparent to opaque
      colour = mix(vec4(0, 0, 0, 0), base_colour, radius);
    }
  ]]></source>
  <program name="default">
    <vertex-shader source="vertex" />
    <fragment-shader source="fragment" />
    <state name="z-write" value="on" />
    <state name="z-test" value="on" />
    <state name="blend" value="alpha" />
  </program>
</shader>
