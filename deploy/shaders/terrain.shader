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

    uniform sampler2DArray textures;
    uniform usampler2D splatt;
    uniform sampler2DShadow shadow_map;

    out vec4 color;

    float patch_size = 64;

    vec4 splattQuery(in usampler2D splatt, in vec2 splattUv, in vec2 splattSize, in sampler2DArray textures, in vec2 textureUv) {
      uint layer_tl = texture(splatt, splattUv).r;
      uint layer_tr = texture(splatt, splattUv + vec2(1.0 / splattSize.x, 0.0)).r;
      uint layer_bl = texture(splatt, splattUv + vec2(0.0, 1.0 / splattSize.y)).r;
      uint layer_br = texture(splatt, splattUv + vec2(1.0 / splattSize.x, 1.0 / splattSize.y)).r;

      vec4 tl = texture(textures, vec3(textureUv, layer_tl));
      vec4 tr = texture(textures, vec3(textureUv, layer_tr));
      vec4 bl = texture(textures, vec3(textureUv, layer_bl));
      vec4 br = texture(textures, vec3(textureUv, layer_br));

      vec2 f = fract(splattUv * vec2(128.0, 128.0));
      //vec2 f = fract(textureUv * vec2(512.0, 512.0));
      vec4 tA = mix(tl, tr, f.x);
      vec4 tB = mix(bl, br, f.x);
      return mix(tA, tB, f.y);
    }

    void main() {
      vec2 uv = 0.5 * (light_pos.xy / light_pos.w) + 0.5;
      float light_amount = 1.0;// texture(shadow_map, vec3(uv, light_pos.z / light_pos.w));

      // calculate the diffuse light
      float ambient = 0.5;
      float diffuse = (clamp(NdotL, 0.0, 1.0) * light_amount * ambient) + ambient;

      // TODO: make the 8.0 somehow configurable?
      color = splattQuery(splatt, tex / patch_size, vec2(128.0), textures, tex / 8.0);
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
