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

uniform sampler2D fluidZTexture;
uniform int       viewportWidth;
uniform int       viewportHeight;
uniform float     particleRadius;
uniform int       filterRadius = 5;
uniform float     lambda       = 10.0f;
uniform float     mu           = 1.0f;
uniform float     farZValue;

in vec2 texCoord;

// the output of this shader
//VTK::Output::Dec

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define MAX_ADAPTIVE_RADIUS 32
#define PI_OVER_8           0.392699082f
#define FIX_OTHER_WEIGHT
#define RANGE_EXTENSION

float compute_weight2D(vec2 r, float two_sigma2)
{
  return exp(-dot(r, r) / two_sigma2);
}

void modifiedGaussianFilter2D(inout float sampleDepth, inout float weight, inout float weight_other,
                              inout float upper, inout float lower, float lower_clamp, float threshold)
{
  if(sampleDepth > upper)
  {
    weight = 0;
#ifdef FIX_OTHER_WEIGHT
    weight_other = 0;
#endif
  }
  else
  {
    if(sampleDepth < lower)
    {
      sampleDepth = lower_clamp;
    }
#ifdef RANGE_EXTENSION
    else
    {
      upper = max(upper, sampleDepth + threshold);
      lower = min(lower, sampleDepth - threshold);
    }
#endif
  }
}

float filter2D(float pixelDepth)
{
  if(filterRadius == 0) {
      return pixelDepth;
  }

  vec2  blurRadius = vec2(1.0 / viewportWidth, 1.0 / viewportHeight);
  float threshold  = particleRadius * lambda;
  float ratio      = viewportHeight / 2.0 / tan(PI_OVER_8);
  float K          = -filterRadius * ratio * particleRadius * 0.1f;
  int   filterSize = min(MAX_ADAPTIVE_RADIUS, int(ceil(K / pixelDepth)));

  float upper       = pixelDepth + threshold;
  float lower       = pixelDepth - threshold;
  float lower_clamp = pixelDepth - particleRadius * mu;

  float sigma      = filterSize / 3.0f;
  float two_sigma2 = 2.0f * sigma * sigma;

  vec4 f_tex = texCoord.xyxy;

  vec2 r     = vec2(0, 0);
  vec4 sum4  = vec4(pixelDepth, 0, 0, 0);
  vec4 wsum4 = vec4(1, 0, 0, 0);
  vec4 sampleDepth;
  vec4 w4;

  for(int x = 1; x <= filterSize; ++x)
  {
    r.x     += blurRadius.x;
    f_tex.x += blurRadius.x;
    f_tex.z -= blurRadius.x;
    vec4 f_tex1 = f_tex.xyxy;
    vec4 f_tex2 = f_tex.zwzw;

    for(int y = 1; y <= filterSize; ++y)
    {
      f_tex1.y += blurRadius.y;
      f_tex1.w -= blurRadius.y;
      f_tex2.y += blurRadius.y;
      f_tex2.w -= blurRadius.y;

      sampleDepth.x = texture(fluidZTexture, f_tex1.xy).r;
      sampleDepth.y = texture(fluidZTexture, f_tex1.zw).r;
      sampleDepth.z = texture(fluidZTexture, f_tex2.xy).r;
      sampleDepth.w = texture(fluidZTexture, f_tex2.zw).r;

      r.y += blurRadius.y;
      w4   = vec4(compute_weight2D(blurRadius * r, two_sigma2));

      modifiedGaussianFilter2D(sampleDepth.x, w4.x, w4.w, upper, lower, lower_clamp, threshold);
      modifiedGaussianFilter2D(sampleDepth.y, w4.y, w4.z, upper, lower, lower_clamp, threshold);
      modifiedGaussianFilter2D(sampleDepth.z, w4.z, w4.y, upper, lower, lower_clamp, threshold);
      modifiedGaussianFilter2D(sampleDepth.w, w4.w, w4.x, upper, lower, lower_clamp, threshold);

      sum4  += sampleDepth * w4;
      wsum4 += w4;
    }
  }

  vec2 filterVal;
  filterVal.x = dot(sum4, vec4(1, 1, 1, 1));
  filterVal.y = dot(wsum4, vec4(1, 1, 1, 1));
  return filterVal.x / filterVal.y;
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
void main()
{
  float pixelDepth = texture(fluidZTexture, texCoord).r;
  float finalDepth;
  if (pixelDepth > 0.0 || pixelDepth <= farZValue)
  {
    finalDepth = pixelDepth;
  }
  else
  {
    finalDepth = filter2D(pixelDepth);
  }
  gl_FragData[0] = vec4(finalDepth, 0, 0, 1.0);
}
