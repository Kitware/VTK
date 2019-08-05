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

in vec2 texCoord;

uniform mat3 invNormalMatrix;
uniform mat4 DCVCMatrix;
uniform mat4 VCDCMatrix;
uniform mat4 MCVCMatrix;

uniform sampler2D fluidZTexture;
uniform sampler2D fluidThicknessTexture;
uniform sampler2D fluidNormalTexture;
uniform sampler2D fluidColorTexture;

uniform sampler2D opaqueRGBATexture;

//VTK::UseIBL::Dec
#ifdef UseIBL
uniform samplerCube prefilterTex;
#endif

uniform int displayModeOpaqueSurface = 0;
uniform int displayModeSurfaceNormal = 0;
uniform int hasVertexColor           = 0;
uniform float vertexColorPower = 0.1f;
uniform float vertexColorScale = 1.0f;

uniform float refractionScale      = 1.0f;
uniform float attenuationScale    = 1.0f;
uniform float additionalReflection = 0.1f;
uniform float refractiveIndex      = 1.33f;

uniform vec3 fluidOpaqueColor       = vec3(0.4, 0.4, 0.95);
uniform vec3 fluidAttenuationColor;

uniform float farZValue;

// Texture maps
//VTK::TMap::Dec

// the output of this shader
//VTK::Output::Dec


// Include lighting uniforms
//VTK::Light::Dec

uniform float  ambientValue;

// These are fluid material, can be changed by user
const vec3  fluidSpecularColor = vec3(1, 1, 1);
const float fluidShininess     = 150.0f;

// This should not be changed
const float fresnelPower = 5.0f;

vec3 uvToEye(float vcDepth)
{
  // need to convert depth back to DC, to use
  vec4 tmp = vec4(0.0, 0.0, vcDepth, 1.0);
  tmp = VCDCMatrix*tmp;
  tmp.x = texCoord.x * 2.0 - 1.0;
  tmp.y = texCoord.y * 2.0 - 1.0;
  tmp.z = tmp.z/tmp.w;
  tmp.w = 1.0;
  vec4 viewPos = DCVCMatrix * tmp;
  return viewPos.xyz / viewPos.w;
}

vec3 computeAttenuation(float thickness)
{
  return vec3(exp(-fluidAttenuationColor.r * thickness),
              exp(-fluidAttenuationColor.g * thickness),
              exp(-fluidAttenuationColor.b * thickness));
}

void main()
{
  float fdepth = texture(fluidZTexture, texCoord).r;
  if (fdepth <= farZValue || fdepth >= 0) { discard; }

  gl_FragDepth = fdepth;
  vec3  N      = texture(fluidNormalTexture, texCoord).xyz;
  if(displayModeSurfaceNormal == 1)
  {
    gl_FragData[0] = vec4(N, 1);
    return;
  }

  vec3  position = uvToEye(fdepth);
  vec3  viewer   = normalize(-position.xyz);

  vec3 accumulatedLightDiffuseColor = vec3(0.0,0.0,0.0);
  vec3 accumulatedLightSpecularColor = vec3(0.0,0.0,0.0);

  //VTK::Light::Impl

  if(displayModeOpaqueSurface == 1)
  {
    if(hasVertexColor == 0)
    {
      gl_FragData[0] = vec4(ambientValue*fluidOpaqueColor
          + fluidOpaqueColor * accumulatedLightDiffuseColor
          + fluidSpecularColor * accumulatedLightSpecularColor, 1.0);
    }
    else
    {
      vec3 tmp = texture(fluidColorTexture, texCoord).xyz;
      tmp.r = 1.0 - pow(tmp.r, vertexColorPower) * vertexColorScale;
      tmp.g = 1.0 - pow(tmp.g, vertexColorPower) * vertexColorScale;
      tmp.b = 1.0 - pow(tmp.b, vertexColorPower) * vertexColorScale;
      gl_FragData[0] = vec4(ambientValue*tmp + tmp * accumulatedLightDiffuseColor + fluidSpecularColor * accumulatedLightSpecularColor, 1.0);
    }
    return;
  }

#ifdef UseIBL
  vec3 worldReflect = normalize(invNormalMatrix*reflect(-viewer, N));
  vec3  reflectionColor = texture(prefilterTex, worldReflect).rgb;
#else
  vec3  reflectionColor = vec3(1.0,1.0,1.0);
#endif

  float eta = 1.0 / refractiveIndex;          // Ratio of indices of refraction
  float F   = ((1.0 - eta) * (1.0 - eta)) / ((1.0 + eta) * (1.0 + eta));

  //Fresnel Reflection
  float fresnelRatio  = clamp(F + (1.0 - F) * pow((1.0 - dot(viewer, N)), fresnelPower), 0, 1);
  vec3  reflectionDir = reflect(-viewer, N);

  float fthick = texture(fluidThicknessTexture, texCoord).r * attenuationScale;
  vec3 volumeColor;
  if(hasVertexColor == 0)
  {
    //Color Attenuation from Thickness (Beer's Law)
    volumeColor = computeAttenuation(fthick);
  }
  else
  {
    vec3 tmp = texture(fluidColorTexture, texCoord).xyz;
    tmp.r = 1.0 - pow(tmp.r, vertexColorPower) * vertexColorScale;
    tmp.g = 1.0 - pow(tmp.g, vertexColorPower) * vertexColorScale;
    tmp.b = 1.0 - pow(tmp.b, vertexColorPower) * vertexColorScale;

    volumeColor = vec3(exp(-tmp.r * fthick),
                        exp(-tmp.g * fthick),
                        exp(-tmp.b * fthick));
  }

  vec3 refractionDir   = refract(-viewer, N, eta);
  vec3 refractionColor = volumeColor * texture(opaqueRGBATexture, texCoord + refractionDir.xy * refractionScale).xyz;

  fresnelRatio = mix(fresnelRatio, 1.0, additionalReflection);
  vec3 finalColor = mix(refractionColor, reflectionColor, fresnelRatio) + fluidSpecularColor * accumulatedLightSpecularColor;
  gl_FragData[0] = vec4(finalColor, 1);
}
