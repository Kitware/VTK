//VTK::System::Dec

// This shader composites for surface lic
// it expects float depth values encoded
// in alpha channel.

// The following line handles system declarations such as
// default precisions, or defining precisions to null
// the output of this shader
//VTK::Output::Dec

uniform sampler2D texData;

in vec2 tcoordVC;

void main()
{
  vec4 newData = texture2D(texData, tcoordVC.st);
  float newDepth = newData.a;
  if (newDepth == 0.0)
    {
    discard;
    }
  gl_FragDepth = newDepth;
  gl_FragData[0] = newData;
}
