//VTK::System::Dec


// the output of this shader
//VTK::Output::Dec

/**
This shader finalizes the convolution for the LIC computation
applying the normalization. eg. if box kernel is used the this
is the number of steps taken.
*/

uniform sampler2D texLIC;

in vec2 tcoordVC;

void main(void)
{
  vec4 conv = texture2D(texLIC, tcoordVC.st);
  conv.r = conv.r/conv.a;
  // lic => (convolution, mask, 0, 1)
  gl_FragData[0] = vec4(conv.rg , 0.0, 1.0);
}
