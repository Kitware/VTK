//VTK::System::Dec


// Fragment shader used by the gaussian blur filter render pass.

in vec2 tcoordVC;
uniform sampler2D source;

// the output of this shader
//VTK::Output::Dec

void main(void)
{
  gl_FragData[0] = texture2D(source,tcoordVC);
}
