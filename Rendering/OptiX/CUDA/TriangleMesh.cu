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

rtBuffer<float3>    vertices;
rtBuffer<float3>    normals;
rtBuffer<int3>      triangles;
rtBuffer<float2>    texcoords;
rtBuffer<float4>    vertexcolors;

rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

rtDeclareVariable( IntersectionRecord, irec, attribute irec, );

RT_PROGRAM void TriangleMeshIntersection( int prim_idx )
{
  const int3 v_idx = triangles[prim_idx];

  const float3 p0 = vertices[ v_idx.x ];
  const float3 p1 = vertices[ v_idx.y ];
  const float3 p2 = vertices[ v_idx.z ];

  // Intersect ray with triangle
  float3 n;
  float  t, beta, gamma;
  if( intersect_triangle( ray, p0, p1, p2, n, t, beta, gamma ) )
  {
    if(  rtPotentialIntersection( t ) )
    {
      irec.PrimIdx = prim_idx;

      irec.Ng = normalize( n );
      if( normals.size() == 0 )
      {
        irec.N = irec.Ng;
      }
      else
      {
        const float3 n0 = normals[ v_idx.x ];
        const float3 n1 = normals[ v_idx.y ];
        const float3 n2 = normals[ v_idx.z ];
        irec.N = normalize( n1*beta + n2*gamma + n0*(1.0f-beta-gamma) );
      }

      if(texcoords.size() == 0)
      {
        irec.TexCoord = make_float2(0.0);
      }
      else
      {
        const float2 t0 = texcoords[ v_idx.x ];
        const float2 t1 = texcoords[ v_idx.y ];
        const float2 t2 = texcoords[ v_idx.z ];
        irec.TexCoord = t1*beta + t2*gamma + t0*(1.0f-beta-gamma);
      }

      if(vertexcolors.size() == 0)
      {
        irec.VertexColor = make_float3(1.0f, 1.0f, 1.0f);
      }
      else
      {
        const float4 c0 = vertexcolors[ v_idx.x ];
        const float4 c1 = vertexcolors[ v_idx.y ];
        const float4 c2 = vertexcolors[ v_idx.z ];
        float4 temp = c1*beta + c2*gamma + c0*(1.0f-beta-gamma);
        irec.VertexColor = make_float3(temp.x, temp.y, temp.z);
      }

      rtReportIntersection( 0 );
    }
  }
}


RT_PROGRAM void TriangleMeshBoundingBox( int prim_idx, float result[6] )
{
    const int3 v_idx = triangles[prim_idx];

    const float3 v0   = vertices[ v_idx.x ];
    const float3 v1   = vertices[ v_idx.y ];
    const float3 v2   = vertices[ v_idx.z ];
    const float  area = length(cross(v1-v0, v2-v0));

    optix::Aabb* aabb = (optix::Aabb*)result;

    if( area > 0.0f && !isinf(area) )
    {
      aabb->m_min = fminf( fminf( v0, v1), v2 );
      aabb->m_max = fmaxf( fmaxf( v0, v1), v2 );
    }
    else
    {
      aabb->invalidate();
    }
}
