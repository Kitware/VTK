//VTK::System::Dec


// Fragment shader used by the second pass of the Sobel filter render pass.

in vec2 tcoordVC;

uniform sampler2D gx1;
uniform sampler2D gy1;
uniform float stepSize; // 1/H

// the output of this shader
//VTK::Output::Dec

void main(void)
{
  vec2 offset=vec2(0.0,stepSize);

  // Gx

  vec4 tx1=texture2D(gx1,tcoordVC-offset);
  vec4 tx2=texture2D(gx1,tcoordVC);
  vec4 tx3=texture2D(gx1,tcoordVC+offset);

  // if clamped textures, rescale values from [0,1] to [-1,1]
  tx1=tx1*2.0-1.0;
  tx2=tx2*2.0-1.0;
  tx3=tx3*2.0-1.0;

  vec4 gx=(tx1+2.0*tx2+tx3)/4.0;

  // Gy

  vec4 ty1=texture2D(gy1,tcoordVC-offset);
  vec4 ty3=texture2D(gy1,tcoordVC+offset);

  vec4 gy=ty3-ty1;

  // the maximum gradient magnitude is sqrt(2.0) when for example gx=1 and
  // gy=1
//  gl_FragData[0]=sqrt((gx*gx+gy*gy)/2.0);
  gl_FragData[0].rgb=sqrt((gx.rgb*gx.rgb+gy.rgb*gy.rgb)/2.0);
  gl_FragData[0].a=1.0; // arbitrary choice.
}
