<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 worldviewproj;
    uniform mat4 worldview;
    uniform mat4 lightviewproj;

    out vec2 tex;
    out vec4 light_pos;
    out float NdotL;

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec2 uv;

    void main() {
      gl_Position = worldviewproj * vec4(position, 1);

      NdotL = dot(normal, vec3(0.485, 0.485, 0.727));

      tex = uv;

      // transform the position to light projection space
      light_pos = lightviewproj * vec4(position, 1);
    }
  ]]></source>
  <source name="fragment"><![CDATA[
    in vec2 tex;
    in vec4 light_pos;
    in float NdotL;
    out vec4 colour;
    uniform sampler2D tex_sampler;
    uniform sampler2DShadow shadow_map;

    void main() {
      vec2 uv = 0.5 * (light_pos.xy / light_pos.w) + 0.5;
      float light_amount = texture(shadow_map, vec3(uv, light_pos.z / light_pos.w));

      // get the "base" colour from the texture
      vec4 base_colour = texture(tex_sampler, tex);

      // then figure out the "real" colour by applying the light calculation
      float diffuse = clamp(NdotL, 0.0, 1.0) * light_amount;

      colour = base_colour * diffuse;
    }
  ]]></source>
  <source name="fragment-notexture"><![CDATA[
    out vec4 colour;

    void main() {
      colour = vec4(1, 1, 1, 1);
    }
  ]]></source>
  <program name="default">
    <vertex-shader source="vertex" />
    <fragment-shader source="fragment" />
    <state name="z-write" value="on" />
    <state name="z-test" value="on" />
    <state name="blend" value="off" />
  </program>
  <program name="notexture">
    <vertex-shader source="vertex" />
    <fragment-shader source="fragment-notexture" />
    <state name="z-write" value="on" />
    <state name="z-test" value="on" />
    <state name="blend" value="off" />
  </program>
</shader>
