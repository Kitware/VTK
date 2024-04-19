
// Fragment shader used by the composite z render pass.

#version 110

uniform sampler2D depth;

void main(void)
{
  vec2 tcoord=gl_TexCoord[0].st;
  gl_FragDepth=texture2D(depth,tcoord).x;
}
