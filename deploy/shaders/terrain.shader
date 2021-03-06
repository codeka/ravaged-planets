<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 worldviewproj;
    uniform mat4 worldview;
    uniform mat4 lightviewproj;

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 normal;

    out vec2 tex;
    out vec4 light_pos;
    out float NdotL;

    void main() {
      gl_Position = worldviewproj * vec4(position, 1);
      NdotL = dot(normal, vec3(0.485, 0.485, 0.727));

      tex = vec2(position.x, position.z);

      // transform the position to light projection space
      light_pos = lightviewproj * vec4(position, 1);
    }
  ]]></source>
  <source name="fragment"><![CDATA[
    in vec2 tex;
    in vec4 light_pos;
    in float NdotL;

    uniform sampler2D layer1;
    uniform sampler2D layer2;
    uniform sampler2D layer3;
    uniform sampler2D layer4;
    uniform sampler2D splatt;
    uniform sampler2DShadow shadow_map;

    out vec4 colour;

    float patch_size = 64;

    void main() {
      vec2 uv = 0.5 * (light_pos.xy / light_pos.w) + 0.5;
      float light_amount = 1.0;// texture(shadow_map, vec3(uv, light_pos.z / light_pos.w));

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
      uv = tex / 8.0;
      base_colour  = weights.r * texture(layer1, uv);
      base_colour += weights.g * texture(layer2, uv);
      base_colour += weights.b * texture(layer3, uv);
      base_colour += weights.a * texture(layer4, uv);

      colour = base_colour * diffuse;
      colour.a = 1.0;
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
