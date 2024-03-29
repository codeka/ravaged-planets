<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 worldviewproj;
    uniform mat4 worldview;
    uniform mat4 view_to_light;

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
      vec4 view_pos = worldview * vec4(position, 1);
      light_pos = view_to_light * view_pos;
    }
  ]]></source>
  <source name="fragment"><![CDATA[
    in vec2 tex;
    in vec4 light_pos;
    in float NdotL;

    out vec4 color;

    uniform sampler2D entity_texture;
    uniform vec4 mesh_color;

    void main() {
      // work out how much this pixel is being affected by shadow(s)
      float light_amount = 1.0;//calculate_shadow_factor(light_pos);

      // get the "base" color from the texture
      vec4 base_color = texture(entity_texture, tex);

      // blend the color with the "mesh" color based on the texture's alpha channel
      base_color.rgb = (base_color.rgb * base_color.a) + (mesh_color.rgb * (1 - base_color.a));
      base_color.a   = 1.0;

      // then figure out the "real" color by applying the light calculation
      float ambient = 0.5;
      float diffuse = (clamp(NdotL, 0.0, 1.0) * light_amount * ambient) + ambient;

      color = base_color * diffuse;
      color.a = 1.0;
    }
  ]]></source>
  <program name="default">
    <vertex-shader source="vertex" />
    <fragment-shader source="fragment" />
    <state name="z-write" value="on" />
    <state name="z-test" value="on" />
    <state name="blend" value="off" />
  </program>
</shader>
