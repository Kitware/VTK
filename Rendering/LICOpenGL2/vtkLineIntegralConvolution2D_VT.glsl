//VTK::System::Dec


// move vector field to normalized image space
// pre-processing for vtkLineIntegralConvolution2D

// the output of this shader
//VTK::Output::Dec

// Fragment shader used by the gaussian blur filter render pass.

uniform sampler2D texVectors; // input texture
uniform vec2      uTexSize;   // size of texture

in vec2 tcoordVC;

void main(void)
{
  //VTK::LICComponentSelection::Impl
  V = V/uTexSize;
  gl_FragData[0] = vec4(V, 0.0, 1.0);
}
