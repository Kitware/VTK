//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
uniform sampler2D fluidThicknessTexture;
uniform int       viewportWidth;
uniform int       viewportHeight;
uniform int       filterRadius = 5;

uniform int       hasVertexColor = 0;
uniform sampler2D fluidColorTexture;

uniform float minThickness;

in vec2 texCoord;
// the output of this shader
//VTK::Output::Dec

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
float compute_weight2D(vec2 r, float two_sigma2)
{
  return exp(-dot(r, r) / two_sigma2);
}

void main()
{
  vec2  blurRadius = vec2(1.0 / float(viewportWidth), 1.0 / float(viewportHeight));
  float sigma      = float(filterRadius) / 3.0;
  float two_sigma2 = 2.0 * sigma * sigma;
  float fthick     = texture2D(fluidThicknessTexture, texCoord).r;
  if (fthick < minThickness)
  {
    discard;
  }

  vec4 f_tex = texCoord.xyxy;
  vec2 r     = vec2(0, 0);
  vec4 sum4  = vec4(fthick, 0, 0, 0);
  vec4 wsum4 = vec4(1, 0, 0, 0);
  vec4 sampleThick;
  vec4 w4_r;

  for(int x = 1; x <= filterRadius; ++x)
  {
    r.x     += blurRadius.x;
    f_tex.x += blurRadius.x;
    f_tex.z -= blurRadius.x;
    vec4 f_tex1 = f_tex.xyxy;
    vec4 f_tex2 = f_tex.zwzw;

    for(int y = 1; y <= filterRadius; ++y)
    {
      r.y += blurRadius.y;
      w4_r = vec4(compute_weight2D(blurRadius * r, two_sigma2));

      f_tex1.y += blurRadius.y;
      f_tex1.w -= blurRadius.y;
      f_tex2.y += blurRadius.y;
      f_tex2.w -= blurRadius.y;

      sampleThick.x = texture(fluidThicknessTexture, f_tex1.xy).r;
      sampleThick.y = texture(fluidThicknessTexture, f_tex1.zw).r;
      sampleThick.z = texture(fluidThicknessTexture, f_tex2.xy).r;
      sampleThick.w = texture(fluidThicknessTexture, f_tex2.zw).r;
      sum4         += sampleThick * w4_r;
      wsum4        += w4_r;
    }
  }

  vec2 filteredThickness;
  filteredThickness.x = dot(sum4, vec4(1, 1, 1, 1));
  filteredThickness.y = dot(wsum4, vec4(1, 1, 1, 1));
  gl_FragData[0]      = vec4(filteredThickness.x / filteredThickness.y, 0, 0, 1);

  if(hasVertexColor == 0)
  {
    return;
  }

  f_tex = texCoord.xyxy;
  r     = vec2(0, 0);
  vec3 sumColor = texture2D(fluidColorTexture, texCoord).rgb;
  wsum4 = vec4(1, 0, 0, 0);

  for(int x = 1; x <= filterRadius; ++x)
  {
    r.x     += blurRadius.x;
    f_tex.x += blurRadius.x;
    f_tex.z -= blurRadius.x;
    vec4 f_tex1 = f_tex.xyxy;
    vec4 f_tex2 = f_tex.zwzw;

    for(int y = 1; y <= filterRadius; ++y)
    {
      r.y   += blurRadius.y;
      w4_r   = vec4(compute_weight2D(blurRadius * r, two_sigma2));
      wsum4 += w4_r;

      f_tex1.y += blurRadius.y;
      f_tex1.w -= blurRadius.y;
      f_tex2.y += blurRadius.y;
      f_tex2.w -= blurRadius.y;

      vec3 sampleColor = texture(fluidColorTexture, f_tex1.xy).rgb;
      sumColor += sampleColor * w4_r[0];

      sampleColor = texture(fluidColorTexture, f_tex1.zw).rgb;
      sumColor   += sampleColor * w4_r[1];

      sampleColor = texture(fluidColorTexture, f_tex2.xy).rgb;
      sumColor   += sampleColor * w4_r[2];

      sampleColor = texture(fluidColorTexture, f_tex2.zw).rgb;
      sumColor   += sampleColor * w4_r[3];
    }
  }

  float wsum = dot(wsum4, vec4(1, 1, 1, 1));
  gl_FragData[1] = vec4(sumColor / wsum, 1);
}
