//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_raytracing_Sampler_h
#define viskores_rendering_raytracing_Sampler_h
#include <viskores/Math.h>
#include <viskores/VectorAnalysis.h>
namespace viskores
{
namespace rendering
{
namespace raytracing
{

template <viskores::Int32 Base>
VISKORES_EXEC void Halton2D(const viskores::Int32& sampleNum, viskores::Vec2f_32& coord)
{
  //generate base2 halton
  viskores::Float32 x = 0.0f;
  viskores::Float32 xadd = 1.0f;
  viskores::UInt32 b2 = 1 + static_cast<viskores::UInt32>(sampleNum);
  while (b2 != 0)
  {
    xadd *= 0.5f;
    if ((b2 & 1) != 0)
      x += xadd;
    b2 >>= 1;
  }

  viskores::Float32 y = 0.0f;
  viskores::Float32 yadd = 1.0f;
  viskores::Int32 bn = 1 + sampleNum;
  while (bn != 0)
  {
    yadd *= 1.0f / (viskores::Float32)Base;
    y += (viskores::Float32)(bn % Base) * yadd;
    bn /= Base;
  }

  coord[0] = x;
  coord[1] = y;
} // Halton2D

VISKORES_EXEC
viskores::Vec3f_32 CosineWeightedHemisphere(const viskores::Int32& sampleNum,
                                            const viskores::Vec3f_32& normal)
{
  //generate orthoganal basis about normal
  int kz = 0;
  if (viskores::Abs(normal[0]) > viskores::Abs(normal[1]))
  {
    if (viskores::Abs(normal[0]) > viskores::Abs(normal[2]))
      kz = 0;
    else
      kz = 2;
  }
  else
  {
    if (viskores::Abs(normal[1]) > viskores::Abs(normal[2]))
      kz = 1;
    else
      kz = 2;
  }
  viskores::Vec3f_32 notNormal;
  notNormal[0] = 0.f;
  notNormal[1] = 0.f;
  notNormal[2] = 0.f;
  notNormal[kz] = 1.f;

  viskores::Vec3f_32 xAxis = viskores::Cross(normal, notNormal);
  viskores::Normalize(xAxis);
  viskores::Vec3f_32 yAxis = viskores::Cross(normal, xAxis);
  viskores::Normalize(yAxis);

  viskores::Vec2f_32 xy;
  Halton2D<3>(sampleNum, xy);
  const viskores::Float32 r = Sqrt(xy[0]);
  const viskores::Float32 theta = 2 * static_cast<viskores::Float32>(viskores::Pi()) * xy[1];

  viskores::Vec3f_32 direction(0.f, 0.f, 0.f);
  direction[0] = r * viskores::Cos(theta);
  direction[1] = r * viskores::Sin(theta);
  direction[2] = viskores::Sqrt(viskores::Max(0.0f, 1.f - xy[0]));

  viskores::Vec3f_32 sampleDir;
  sampleDir[0] = viskores::dot(direction, xAxis);
  sampleDir[1] = viskores::dot(direction, yAxis);
  sampleDir[2] = viskores::dot(direction, normal);
  return sampleDir;
}
}
}
} // namespace viskores::rendering::raytracing
#endif
