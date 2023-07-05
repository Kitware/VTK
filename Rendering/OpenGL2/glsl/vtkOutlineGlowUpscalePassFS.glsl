//VTK::System::Dec


// Fragment shader used by outline glow pass to combine the original scene and the blurred image to form an outline

in vec2 tcoordVC;
uniform sampler2D scene;
uniform sampler2D source;

uniform float outlineIntensity;

// the output of this shader
//VTK::Output::Dec

void main(void)
{
  vec4 color = texture2D(scene, tcoordVC);
  if( length(color.rgb) > 0.0)
  {
    // If the pixel was filled in the original scene it not part of the outline
    gl_FragData[0] = vec4(0.0, 0.0, 0.0, 0.0);
  }
  else
  {
    vec4 blurredColor = texture2D(source,tcoordVC);
    float brightness = max(blurredColor.r, max(blurredColor.g, blurredColor.b));
      gl_FragData[0].rgb = blurredColor.rgb * outlineIntensity;
      gl_FragData[0].a = brightness * outlineIntensity;
    }
}
