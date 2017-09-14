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

using namespace optix;

rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint2, launch_dim,   rtLaunchDim, );

rtBuffer<uchar4, 2>   frame_buffer;
rtBuffer<float, 2>    depth_buffer;

rtDeclareVariable(rtObject,      top_object, , );
rtDeclareVariable(float3,        pos, , );
rtDeclareVariable(float3,        U, , );
rtDeclareVariable(float3,        V, , );
rtDeclareVariable(float3,        W, , );
rtDeclareVariable(int,           sqrt_num_samples, , );

RT_PROGRAM void PerspectiveCameraRayGen()
{
  size_t2 screen = frame_buffer.size();
  float2 inv_screen = 1.0f/make_float2(screen) * 2.f;
  float2 pixel = (make_float2(launch_index)) * inv_screen - 1.f;

  float2 jitter_scale = inv_screen / (float) sqrt_num_samples;
  int samples_per_pixel = sqrt_num_samples*sqrt_num_samples;

  const float3 ray_origin    = pos;

  float3 result = make_float3(0.0f);

  unsigned int seed = tea<16>(screen.x*launch_index.y+launch_index.x, 0);
  float minDepth = CUDART_INF_F;

  do
  {
    int x = samples_per_pixel%sqrt_num_samples;
    int y = samples_per_pixel/sqrt_num_samples;

    float2 jitter = (samples_per_pixel > 1) ? make_float2(x-rnd(seed), y-rnd(seed)) : make_float2(x, y);
    float2 d = pixel + jitter*jitter_scale;
    const float3 ray_direction = normalize(d.x*U + d.y*V + W);

    RadiancePRD prd;
    prd.result = make_float3( 0.0f ); //ray_direction*0.5f + make_float3( 0.5f );
    prd.depth = CUDART_INF_F;

    optix::Ray ray = optix::make_Ray(
      ray_origin,
      ray_direction,
      RADIANCE_RAY_TYPE,
      0.001f,
      RT_DEFAULT_MAX
      );

    rtTrace( top_object, ray, prd );
    result += prd.result;
    minDepth = min(prd.depth, minDepth);

  } while(--samples_per_pixel);


  result = result / (sqrt_num_samples * sqrt_num_samples);

  const float3 c = fminf( result, make_float3( 1.0f ) );
  frame_buffer[launch_index] = make_uchar4( c.x*255.99f, c.y*255.99f, c.z*255.99f, 255 );
  depth_buffer[launch_index] = minDepth;
}
