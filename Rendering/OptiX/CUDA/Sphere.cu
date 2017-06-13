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

#include "Common.h"

using namespace optix;

rtBuffer<float4>    spheres;
rtBuffer<float2>    texcoords;
rtBuffer<float4>    vertexcolors;

rtDeclareVariable( optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable( IntersectionRecord, irec, attribute irec, );

rtDeclareVariable( float, sphere_radius, , );

template<bool use_robust_method>
static __device__
void intersect_sphere(int prim_idx)
{
  float4 sphereAndRadius = spheres[prim_idx];

  const float3 center = make_float3( sphereAndRadius.x, sphereAndRadius.y, sphereAndRadius.z );
  const float radius = sphereAndRadius.w * sphere_radius;

  float3 O = ray.origin - center;
  float3 D = ray.direction;

  float b = dot(O, D);
  float c = dot(O, O)-radius*radius;
  float b2 = b*b;
  if(b2 > c)
  {
    float sdisc = sqrtf(b2 - c);
    float root1 = (-b - sdisc);

    bool do_refine = false;

    float root11 = 0.0f;

    if(use_robust_method && /*fabsf(root1) > 10.f * radius*/c > b2*0.999f )
    {
      do_refine = true;
    }

    if(do_refine)
    {
      // refine root1
      float3 O1 = O + root1 * ray.direction;
      b = dot(O1, D);
      c = dot(O1, O1) - radius*radius;
      float disc = b*b - c;

      if(disc > 0.0f)
      {
        sdisc = sqrtf(disc);
        root11 = (-b - sdisc);
      }
    }

    bool check_second = true;
    if( rtPotentialIntersection( root1 + root11 ) )
    {
      irec.PrimIdx = prim_idx;
      irec.N = irec.Ng = (O + (root1 + root11)*D)/radius;

      irec.TexCoord = texcoords[prim_idx];

      if(vertexcolors.size() == 0)
      {
        irec.VertexColor = make_float3(1.0f, 1.0f, 1.0f);
      }
      else
      {
        float4 vertexColor = vertexcolors[prim_idx];
        irec.VertexColor = make_float3(vertexColor.x, vertexColor.y, vertexColor.z);
      }

      if(rtReportIntersection(0))
      {
        check_second = false;
      }
    }

    if(check_second)
    {
      float root2 = (-b + sdisc) + (do_refine ? root1 : 0);

      if( rtPotentialIntersection( root2 ) )
      {
        irec.PrimIdx = prim_idx;
        irec.N = irec.Ng = (O + root2*D)/radius;
        irec.TexCoord = texcoords[prim_idx];

        if(vertexcolors.size() == 0)
        {
          irec.VertexColor = make_float3(1.0f, 1.0f, 1.0f);
        }
        else
        {
          float4 vertexColor = vertexcolors[prim_idx];
          irec.VertexColor = make_float3(vertexColor.x, vertexColor.y, vertexColor.z);
        }

        rtReportIntersection(0);
      }
    }
  }
}

RT_PROGRAM void SphereIntersect( int prim_idx )
{
  intersect_sphere<true>(prim_idx);
}

RT_PROGRAM void SphereIntersect_robust(int prim_idx)
{
  intersect_sphere<true>(prim_idx);
}

RT_PROGRAM void SphereBounds (int prim_idx, float result[6])
{
  float4 sphereAndRadius = spheres[prim_idx];

  const float3 center = make_float3( sphereAndRadius.x, sphereAndRadius.y, sphereAndRadius.z );
  const float3 radius = make_float3( sphereAndRadius.w * sphere_radius );

  optix::Aabb* aabb = (optix::Aabb*)result;

  if( radius.x > 0.0f  && !isinf(radius.x) )
  {
    aabb->m_min = center - radius;
    aabb->m_max = center + radius;
  }
  else
  {
    aabb->invalidate();
  }
}
