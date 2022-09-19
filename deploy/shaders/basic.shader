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
  <source name="vertex-notexture"><![CDATA[
    uniform mat4 worldviewproj;

    out vec4 color;

    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec4 in_color;

    void main() {
      gl_Position = worldviewproj * vec4(in_position, 1);
      color = in_color;
    }
  ]]></source>
  <source name="fragment"><![CDATA[
    in vec2 tex;
    in vec4 light_pos;
    in float NdotL;
    out vec4 color;
    uniform sampler2D tex_sampler;
    uniform sampler2DShadow shadow_map;

    void main() {
      vec2 uv = 0.5 * (light_pos.xy / light_pos.w) + 0.5;
      float light_amount = texture(shadow_map, vec3(uv, light_pos.z / light_pos.w));

      // get the "base" color from the texture
      vec4 base_color = texture(tex_sampler, tex);

      // then figure out the "real" color by applying the light calculation
      float diffuse = clamp(NdotL, 0.0, 1.0) * light_amount;

      color = base_color * diffuse;
    }
  ]]></source>
  <source name="fragment-notexture"><![CDATA[
    in vec4 color;
    out vec4 out_color;

    void main() {
      out_color = color;
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
    <vertex-shader source="vertex-notexture" />
    <fragment-shader source="fragment-notexture" />
    <state name="z-write" value="on" />
    <state name="z-test" value="on" />
    <state name="blend" value="off" />
  </program>
</shader>
