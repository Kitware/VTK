attribute vec4 vertex;
attribute vec3 normal;

uniform vec3 color;
uniform mat4 modelView;
uniform mat4 projection;
uniform mat3 normalMatrix;

varying vec3 fnormal;

void main()
{
  gl_FrontColor = vec4(color, 1.0);
  gl_Position = projection * modelView * vertex;
  fnormal = normalize(normalMatrix * normal);
}
