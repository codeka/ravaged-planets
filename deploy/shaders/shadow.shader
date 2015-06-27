<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 worldviewproj;
    layout (location = 0) in vec3 position;
    out vec2 val;

    void main() {
      gl_Position = worldviewproj * vec4(position, 1);
      val = gl_Position.zw;
    }
  ]]></source>
  <source name="fragment"><![CDATA[
    in vec2 val;
    out vec3 colour;

    void main() {
      colour = vec3(val.x / val.y);
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
