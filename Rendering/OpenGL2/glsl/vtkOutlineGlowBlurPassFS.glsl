//VTK::System::Dec


// Fragment shader used by the outline glow blur render pass.

in vec2 tcoordVC;
uniform sampler2D source;

uniform float coef[3];
uniform float offsetx;
uniform float offsety;

// the output of this shader
//VTK::Output::Dec

void main(void)
{
  vec2 offset=vec2(offsetx,offsety);

  gl_FragData[0] = coef[0]*texture2D(source,tcoordVC-offset)
    +coef[1]*texture2D(source,tcoordVC)
    +coef[2]*texture2D(source,tcoordVC+offset);
}
