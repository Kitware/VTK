//VTK::System::Dec


// This shader copies fragments and depths to the output buffer

// the output of this shader
//VTK::Output::Dec

uniform sampler2D texDepth;     // z values from vertex shader
uniform sampler2D texRGBColors; // final rgb LIC colors

in vec2 tcoordVC;

void main()
{
  gl_FragDepth = texture2D(texDepth, tcoordVC).x;
  gl_FragData[0] = texture2D(texRGBColors, tcoordVC);

  // since we render a screen aligned quad
  // we're going to be writing fragments
  // not touched by the original geometry
  // it's critical not to modify those
  // fragments.
  if (gl_FragDepth == 1.0)
    {
    discard;
    }
}
