//VTK::System::Dec


// Fragment shader used by the composite z render pass.

// the output of this shader
//VTK::Output::Dec

in vec2 tcoordVC;
uniform sampler2D depth;

void main(void)
{
  gl_FragDepth = texture2D(depth,tcoordVC).x;
}
