/*
* Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <optix.h>
#include <optixu/optixu_math_namespace.h>
#include <math_constants.h>

#include "Random.h"
#include "Common.h"
#include "Light.h"

#include <stdio.h>

using namespace optix;

rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint2, launch_dim,   rtLaunchDim, );

rtDeclareVariable(rtObject, top_object, , );

rtBuffer<vtkopt::Light> lights;
rtBuffer<float3> cellcolors;

rtDeclareVariable( RadiancePRD, prd,   rtPayload, );
rtDeclareVariable( optix::Ray,  ray,   rtCurrentRay, );
rtDeclareVariable( float,       t_hit, rtIntersectionDistance, );

rtDeclareVariable( float3, Ks, , );
rtDeclareVariable( float3, Kd, , );
rtDeclareVariable( float,  Ns, , );

rtDeclareVariable( float3, bg_color, , );
rtDeclareVariable( int, shadows_enabled, , );
rtDeclareVariable( int, texture_enabled, , );
rtDeclareVariable( int, vertexcolors_enabled, , );
rtDeclareVariable( int, cellcolors_enabled, , );
rtDeclareVariable( int, num_ambient_samples, , );
rtDeclareVariable( float, ambient_occlusion_dist, , );
rtDeclareVariable( float, occlusion_epsilon, , );

rtDeclareVariable( IntersectionRecord, irec, attribute irec, );

rtTextureSampler<uchar4, 2, cudaReadModeNormalizedFloat> colorTexture;

static __device__
float3 cartesianCoords(float phi, float sinTheta, float cosTheta)
{
  float sinPhi, cosPhi;
  sincos(phi, &sinPhi, &cosPhi);
  return make_float3(cosPhi * sinTheta,
                     sinPhi * sinTheta,
                     cosTheta);
}

static __device__
float3 cosineSampleHemisphere(float2 s_uni)
{
  float cosTheta = sqrt(s_uni.y);
  float sinTheta = sqrt(1.0f - s_uni.y);
  float phi = 2 * CUDART_PI_F * s_uni.x;
  return cartesianCoords(phi, sinTheta, cosTheta);
}

static __device__
void buildFrame( float3 N, float3* mat )
{
  float3 tan0 = make_float3( 0.0f, N.z, -N.y );
  float3 tan1 = make_float3( -N.z, 0.0f, N.x );
  float3 tan = normalize( abs(N.x) < abs(N.y) ? tan0 : tan1 );
  float3 bitan = cross( N, tan );
  mat[0] = tan;
  mat[1] = bitan;
  mat[2] = N;
}

RT_PROGRAM void LambertianClosestHit()
{
  const float3 N = faceforward( irec.N, -ray.direction, irec.Ng );
  const float3 P  = ray.origin + t_hit * ray.direction;
  //const float3 Kd = make_float3( 0.7f, 0.7f, 0.7f );

  float3 Kdiffuse = Kd;
  if( texture_enabled )
  {
    float4 temp = tex2D( colorTexture, irec.TexCoord.x, irec.TexCoord.y );
    Kdiffuse = make_float3(temp.x, temp.y, temp.z);
  }
  else if( vertexcolors_enabled )
  {
    Kdiffuse = irec.VertexColor;
  }
  else if( cellcolors_enabled )
  {
    Kdiffuse = cellcolors[irec.PrimIdx];
  }

  // light loop
  float3 color = make_float3( 0.0f );
  const int num_lights = lights.size();
  for( int i =0; i < num_lights; ++i )
  {
    const vtkopt::Light light = lights[i];
    float3 L;
    float  Ldist;
    float3  Lcolor;
    if( light.type == vtkopt::Light::DIRECTIONAL )
    {
      L = -light.dir;
      Ldist = 1e8f;
      Lcolor = light.color;
    }
    else
    {
      Ldist = optix::length( light.pos - P );
      L = ( light.pos-P ) / Ldist;
      Lcolor = light.color/(Ldist*Ldist);
    }

    const float N_dot_L = optix::dot( L, N );

    float3 light_attenuation = make_float3( 1.0f );
    if( N_dot_L > 0.0f )
    {
      //
      // Calculation occlusion
      //
      if( shadows_enabled )
      {
        OcclusionPRD shadow_prd;
        shadow_prd.occlusion = make_float3( 1.0f );

        optix::Ray shadow_ray =
          optix::make_Ray(
            P,
            L,
            OCCLUSION_RAY_TYPE,
            occlusion_epsilon,
            Ldist );
        rtTrace( top_object, shadow_ray, shadow_prd );

        light_attenuation = shadow_prd.occlusion;
      }

      //
      // Calculate local lighting
      //
      if( fmaxf(light_attenuation) > 0.0f )
      {
        //const float3 H = optix::normalize( L - ray.direction );
        //const float  N_dot_H = optix::dot( N, H );
        const float3 R = optix::reflect( ray.direction, N );
        //clamp, as normal can be slightly inaccurate, causing aliasing with large Ns
        const float L_dot_R = fminf(fmaxf( optix::dot( L, R ), 0.0f), 1.0f);
        color += ( Kdiffuse*N_dot_L + Ks*powf( L_dot_R, Ns ) ) * Lcolor * light_attenuation;
      }
    }
  }

  if( num_ambient_samples > 0 )
  {
    float3 localToWorld[3];
    buildFrame(N, localToWorld);

    int Pi0 = *((int*)(&P.x));
    int Pi1 = *((int*)(&P.y));
    int Pi2 = *((int*)(&P.z));

    unsigned int seed = tea<16>( Pi0^Pi1^Pi2, 0 );

    float3 attenuation = make_float3(0.0f);
    for( int ambSampleIdx = 0; ambSampleIdx < num_ambient_samples; ++ambSampleIdx )
    {
      float2 uniSamples = make_float2( rnd(seed), rnd(seed) );
      float3 ambDir = cosineSampleHemisphere( uniSamples );

      ambDir = localToWorld[0]*ambDir.x + localToWorld[1]*ambDir.y + localToWorld[2]*ambDir.z;

      if( dot( ambDir, N ) < 0.05f )
      {
        continue;
      }

      OcclusionPRD ambient_prd;
      ambient_prd.occlusion = make_float3( 1.0f );

      optix::Ray ambient_ray =
        optix::make_Ray(
          P,
          ambDir,
          OCCLUSION_RAY_TYPE,
          occlusion_epsilon,
          ambient_occlusion_dist );
      rtTrace( top_object, ambient_ray, ambient_prd );

      attenuation += ambient_prd.occlusion;
    }
    attenuation = attenuation/num_ambient_samples;

    color *= attenuation;
  }

  prd.result = color;
  prd.depth = t_hit;
}

rtDeclareVariable( OcclusionPRD, shadow_prd, rtPayload, );

RT_PROGRAM void LambertianAnyHit()
{
  shadow_prd.occlusion = make_float3( 0.0f );
}

RT_PROGRAM void Miss()
{
  prd.result = bg_color;
}
