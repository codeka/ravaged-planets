<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 pos_transform;
    uniform mat4 uv_transform;

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 uv;

    out vec2 tex;

    void main() {
      gl_Position = pos_transform * vec4(position, 1);
      vec4 transformed_uv = uv_transform * vec4(uv, 0, 1);
      tex = transformed_uv.xy;
    }
  ]]></source>
  <source name="fragment-normal"><![CDATA[
    uniform sampler2D texsampler;
    in vec2 tex;
    out vec4 color;

    void main() {
      color = texture(texsampler, tex);
    }
  ]]></source>
  <source name="fragment-ninepatch"><![CDATA[
    in vec2 tex;
    uniform sampler2D texsampler;

    uniform float inner_top;
    uniform float inner_left;
    uniform float inner_right;
    uniform float inner_bottom;
    uniform float inner_top_v;
    uniform float inner_left_u;
    uniform float inner_right_u;
    uniform float inner_bottom_v;
    uniform float fraction_width;
    uniform float fraction_height;
    uniform float fraction_width2;
    uniform float fraction_height2;
    uniform float pixel_width;
    uniform float pixel_height;

    out vec4 color;

    void main() {
      float new_x;
      float new_y;

      if (tex.y < inner_top) {
        // top row
        new_y = inner_top_v - ((inner_top - tex.y) * fraction_height);

        if (tex.x < inner_left) {
          // top-left
          new_x = inner_left_u - ((inner_left - tex.x) * fraction_width);
        } else if (tex.x > inner_right) {
          // top-right
          new_x = inner_right_u + ((tex.x - inner_right) * fraction_width);
        } else {
          // top-middle
          new_x = inner_left_u + ((tex.x - inner_left) / (fraction_width2 / fraction_width));
        }
      } else if (tex.y >= inner_bottom) {
        // bottom row
        new_y = inner_bottom_v + ((tex.y - inner_bottom) * fraction_height);

        if (tex.x < inner_left) {
          // bottom-left
          new_x = inner_left_u - ((inner_left - tex.x) * fraction_width);
        } else if (tex.x >= inner_right) {
          // bottom-right
          new_x = inner_right_u + ((tex.x - inner_right) * fraction_width);
        } else {
          // bottom-middle
          new_x = inner_left_u + ((tex.x - inner_left) / (fraction_width2 / fraction_width));
        }
      } else {
        // middle row
        new_y = inner_top_v + ((tex.y - inner_top) / (fraction_height2 / fraction_height));

        if (tex.x < inner_left) {
          // middle-left
          new_x = inner_left_u - ((inner_left - tex.x) * fraction_width);
        } else if (tex.x >= inner_right) {
          // middle-right
          new_x = inner_right_u + ((tex.x - inner_right) * fraction_width);
        } else {
          // middle
          new_x = inner_left_u + ((tex.x - inner_left) / (fraction_width2 / fraction_width));
        }
      }

      color = texture(texsampler, vec2(new_x, new_y));
    }
  ]]></source>
  <program name="default">
    <vertex-shader source="vertex" />
    <fragment-shader source="fragment-normal" />
    <state name="z-write" value="off" />
    <state name="z-test" value="off" />
    <state name="blend" value="alpha" />
  </program>
  <program name="ninepatch">
    <vertex-shader source="vertex" />
    <fragment-shader source="fragment-ninepatch" />
    <state name="z-write" value="off" />
    <state name="z-test" value="off" />
    <state name="blend" value="alpha" />
  </program>
</shader>