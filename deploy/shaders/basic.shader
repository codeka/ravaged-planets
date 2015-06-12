<?xml version="1.0" ?>
<shader version="1">
  <source name="vertex"><![CDATA[
    uniform mat4 worldviewproj;
    layout (location = 0) in vec3 position;

    void main() {
      gl_Position = worldviewproj * vec4(position, 1);
    }
  ]]></source>
  <source name="fragment"><![CDATA[
    out vec4 color;

    void main() {
      color = vec4(1, 1, 1, 0.3);
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