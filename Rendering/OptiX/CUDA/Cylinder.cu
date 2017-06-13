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

rtBuffer<float4>    vertices;
rtBuffer<int2>      lines;
rtBuffer<float2>    texcoords;
rtBuffer<float4>    vertexcolors;

rtDeclareVariable( optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable( IntersectionRecord, irec, attribute irec, );

rtDeclareVariable( float, cylinder_radius, , );

template<bool use_robust_method>
static __device__
void intersect_cylinder(int prim_idx)
{
  int2 vertexIdxs = lines[prim_idx];
  float4 posAndRadius0 = vertices[vertexIdxs.x];
  float4 posAndRadius1 = vertices[vertexIdxs.y];

  float3 pos0 = make_float3(posAndRadius0.x, posAndRadius0.y, posAndRadius0.z);
  float3 pos1 = make_float3(posAndRadius1.x, posAndRadius1.y, posAndRadius1.z);

  const float radius = (posAndRadius0.w + posAndRadius1.w) * 0.5 * cylinder_radius;

  float3 O = ray.origin - pos0;
  float3 D = ray.direction; //Assumed to be of length 1
  float3 C = pos1 - pos0;

  float dotCC = dot(C,C);
  float dotOO = dot(O,O);
  float dotDC = dot(D,C);
  float dotOC = dot(O,C);
  float dotOD = dot(O,D);
  float dotCCInv = (dotCC == 0.0f ? 1.0f : 1.0f/dotCC);

  float dotDCdotCCInv = dotDC*dotCCInv;
  float dotOCdotCCInv = dotOC*dotCCInv;

  float a = 1-dotDC*dotDCdotCCInv;
  float b = dotOD - dotOC*dotDCdotCCInv;
  float c = dotOO - dotOC*dotOCdotCCInv - radius*radius;

  //A factor 2 falls away in b, and in the second term of discriminant
  float disc = b*b-a*c;

  if(disc > 0.0f)
  {
    float sdisc = sqrtf(disc);
    float root1 = (-b - sdisc)/a;

    //bool do_refine = false;

    float root11 = 0.0f;

    bool check_second = true;

    float dC1 = dotOCdotCCInv + dotDCdotCCInv * root1;

    if( dC1 >= 0.0f && dC1 <= 1.0f && rtPotentialIntersection( root1 + root11 ) )
    {
      float3 pC = dC1 * C;
      float3 pR = O + root1 * D;

      irec.PrimIdx = prim_idx;
      irec.N = irec.Ng = (pR - pC)/radius;

      irec.TexCoord = (1-dC1)*texcoords[vertexIdxs.x] + dC1*texcoords[vertexIdxs.y];


      if(vertexcolors.size() == 0)
      {
        irec.VertexColor = make_float3(1.0f, 1.0f, 1.0f);
      }
      else
      {
        float4 vertexColor = (1-dC1)*vertexcolors[vertexIdxs.x] + dC1*vertexcolors[vertexIdxs.y];
        irec.VertexColor = make_float3(vertexColor.x, vertexColor.y, vertexColor.z);
      }

      if(rtReportIntersection(0))
      {
        check_second = false;
      }
    }

    if(check_second)
    {
      float root2 = (-b + sdisc) / a; //+ (do_refine ? root1 : 0);

      float dC2 = dotOCdotCCInv + dotDCdotCCInv * root2;

      if( dC2 >= 0.0f && dC2 <= 1.0f && rtPotentialIntersection( root2 ) )
      {
        float3 pC = dC2 * C;
        float3 pR = O + root2 * D;

        irec.PrimIdx = prim_idx;
        irec.N = irec.Ng = (pR - pC)/radius;

        irec.TexCoord = (1-dC2)*texcoords[vertexIdxs.x] + dC2*texcoords[vertexIdxs.y];


        if(vertexcolors.size() == 0)
        {
          irec.VertexColor = make_float3(1.0f, 1.0f, 1.0f);
        }
        else
        {
          float4 vertexColor = (1-dC2)*vertexcolors[vertexIdxs.x] + dC2*vertexcolors[vertexIdxs.y];

          irec.VertexColor = make_float3(vertexColor.x, vertexColor.y, vertexColor.z);
        }

        rtReportIntersection(0);
      }
    }
  }
}

RT_PROGRAM void CylinderIntersect( int prim_idx )
{
  intersect_cylinder<false>(prim_idx);
}

RT_PROGRAM void CylinderIntersect_robust(int prim_idx)
{
  intersect_cylinder<true>(prim_idx);
}

RT_PROGRAM void CylinderBounds (int prim_idx, float result[6])
{
  int2 vertexIdxs = lines[prim_idx];
  float4 posAndRadius0 = vertices[vertexIdxs.x];
  float4 posAndRadius1 = vertices[vertexIdxs.y];

  float3 pos0 = make_float3(posAndRadius0.x, posAndRadius0.y, posAndRadius0.z);
  float3 pos1 = make_float3(posAndRadius1.x, posAndRadius1.y, posAndRadius1.z);

  const float radius = (posAndRadius0.w + posAndRadius1.w) * 0.5 * cylinder_radius;

  optix::Aabb* aabb = (optix::Aabb*)result;

  if( radius > 0.0f  && !isinf(radius) )
  {
    aabb->m_min = fminf(pos0, pos1) - radius;
    aabb->m_max = fmaxf(pos0, pos1) + radius;
  }
  else
  {
    aabb->invalidate();
  }
}
