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
uniform float     sigmaDepth   = 10.0f;
uniform float     farZValue;

in vec2 texCoord;

// the output of this shader
//VTK::Output::Dec

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define MAX_ADAPTIVE_RADIUS 32
#define PI_OVER_8           0.392699082f

float compute_weight2D(vec2 r, float two_sigma2)
{
  return exp(-dot(r, r) / two_sigma2);
}

float compute_weight1D(float r, float two_sigma2)
{
  return exp(-r * r / two_sigma2);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
void main()
{
  vec2 blurRadius  = vec2(1.0 / float(viewportWidth), 1.0 / float(viewportHeight));
  float pixelDepth = texture(fluidZTexture, texCoord).r;
  float finalDepth;

  if(pixelDepth >= 0.0f || pixelDepth <= farZValue)
  {
    finalDepth = pixelDepth;
  }
  else
  {
    float ratio      = viewportHeight / 2.0 / tan(PI_OVER_8);
    float K          = -filterRadius * ratio * particleRadius * 0.1f;
    int   filterSize = min(MAX_ADAPTIVE_RADIUS, int(ceil(K / pixelDepth)));
    float sigma      = filterSize / 3.0f;
    float two_sigma2 = 2.0f * sigma * sigma;

    float threshold       = particleRadius * sigmaDepth;
    float sigmaDepth      = threshold / 3.0f;
    float two_sigmaDepth2 = 2.0f * sigmaDepth * sigmaDepth;

    vec4 f_tex = texCoord.xyxy;
    vec2 r     = vec2(0, 0);
    vec4 sum4  = vec4(pixelDepth, 0, 0, 0);
    vec4 wsum4 = vec4(1, 0, 0, 0);
    vec4 sampleDepth;
    vec4 w4_r;
    vec4 w4_depth;
    vec4 rDepth;

    for(int x = 1; x <= filterSize; ++x)
    {
      r.x     += blurRadius.x;
      f_tex.x += blurRadius.x;
      f_tex.z -= blurRadius.x;
      vec4 f_tex1 = f_tex.xyxy;
      vec4 f_tex2 = f_tex.zwzw;

      for(int y = 1; y <= filterSize; ++y)
      {
        r.y += blurRadius.y;

        f_tex1.y += blurRadius.y;
        f_tex1.w -= blurRadius.y;
        f_tex2.y += blurRadius.y;
        f_tex2.w -= blurRadius.y;

        sampleDepth.x = texture(fluidZTexture, f_tex1.xy).r;
        sampleDepth.y = texture(fluidZTexture, f_tex1.zw).r;
        sampleDepth.z = texture(fluidZTexture, f_tex2.xy).r;
        sampleDepth.w = texture(fluidZTexture, f_tex2.zw).r;

        rDepth     = sampleDepth - vec4(pixelDepth);
        w4_r       = vec4(compute_weight2D(blurRadius * r, two_sigma2));
        w4_depth.x = compute_weight1D(rDepth.x, two_sigmaDepth2);
        w4_depth.y = compute_weight1D(rDepth.y, two_sigmaDepth2);
        w4_depth.z = compute_weight1D(rDepth.z, two_sigmaDepth2);
        w4_depth.w = compute_weight1D(rDepth.w, two_sigmaDepth2);

        sum4  += sampleDepth * w4_r * w4_depth;
        wsum4 += w4_r * w4_depth;
      }
    }

    vec2 filterVal;
    filterVal.x = dot(sum4, vec4(1, 1, 1, 1));
    filterVal.y = dot(wsum4, vec4(1, 1, 1, 1));

    finalDepth = filterVal.x / filterVal.y;
  }

  gl_FragData[0] = vec4(finalDepth, 0, 0, 1.0);
}
