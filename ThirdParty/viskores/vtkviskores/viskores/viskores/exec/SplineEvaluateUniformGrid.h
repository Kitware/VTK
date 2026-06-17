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
#ifndef viskores_exec_SplineEvaluateUniformGrid_h
#define viskores_exec_SplineEvaluateUniformGrid_h

#include <viskores/Bounds.h>
#include <viskores/ErrorCode.h>

namespace viskores
{
namespace exec
{

class VISKORES_ALWAYS_EXPORT SplineEvaluateUniformGrid
{
private:
  using FieldType = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using FieldPortalType = typename FieldType::ReadPortalType;

public:
  VISKORES_CONT SplineEvaluateUniformGrid() = default;

  template <typename ArrayPortalType>
  VISKORES_CONT SplineEvaluateUniformGrid(const viskores::Vec3f origin,
                                          const viskores::Vec3f spacing,
                                          const viskores::Id3 pointDims,
                                          const ArrayPortalType& field)
    : Bounds(origin[0],
             origin[0] + static_cast<viskores::FloatDefault>((pointDims[0] - 1) * spacing[0]),
             origin[1],
             origin[1] + static_cast<viskores::FloatDefault>((pointDims[1] - 1) * spacing[1]),
             origin[2],
             origin[2] + static_cast<viskores::FloatDefault>((pointDims[2] - 1) * spacing[2]))
    , Dimensions(pointDims)
    , Field(field)
    , Origin(origin)
    , Spacing(spacing)
  {
  }

  VISKORES_EXEC viskores::ErrorCode Evaluate(const viskores::Vec3f& point,
                                             viskores::FloatDefault& value) const
  {
    if (!this->Bounds.Contains(point))
      return viskores::ErrorCode::CellNotFound;

    //map world to index space.
    viskores::Vec3f pointIndex;
    pointIndex[0] = (point[0] - this->Origin[0]) / this->Spacing[0];
    pointIndex[1] = (point[1] - this->Origin[1]) / this->Spacing[1];
    pointIndex[2] = (point[2] - this->Origin[2]) / this->Spacing[2];

    return this->TriCubicEvaluate(pointIndex, value);
  }

private:
  VISKORES_EXEC viskores::ErrorCode TriCubicEvaluate(const viskores::Vec3f& pointIndex,
                                                     viskores::FloatDefault& value) const
  {
    viskores::Id nx = this->Dimensions[0], ny = this->Dimensions[1], nz = this->Dimensions[2];
    viskores::FloatDefault x = pointIndex[0], y = pointIndex[1], z = pointIndex[2];
    // base integer coords
    viskores::Id ix = static_cast<viskores::Id>(viskores::Floor(x));
    viskores::Id iy = static_cast<viskores::Id>(viskores::Floor(y));
    viskores::Id iz = static_cast<viskores::Id>(viskores::Floor(z));

    // fractional offsets
    viskores::FloatDefault tx = x - ix;
    viskores::FloatDefault ty = y - iy;
    viskores::FloatDefault tz = z - iz;

    // Coefficients for tricubic interpolation
    viskores::FloatDefault P[4 * 4 * 4];
    viskores::FloatDefault C[4 * 4];
    viskores::FloatDefault D[4];

    // 1) Gather 4×4×4 neighborhood into P
    //    P[kk][jj][ii] -> P[(kk*4 + jj)*4 + ii]
    //    data[z0][y0][x0] -> data[(z0*ny + y0)*nx + x0]
    for (viskores::Id kk = 0; kk < 4; ++kk)
    {
      viskores::Id z0 = viskores::Clamp(iz - 1 + kk, 0, nz);
      for (viskores::Id jj = 0; jj < 4; ++jj)
      {
        viskores::Id y0 = viskores::Clamp(iy - 1 + jj, 0, ny);
        for (viskores::Id ii = 0; ii < 4; ++ii)
        {
          viskores::Id x0 = viskores::Clamp(ix - 1 + ii, 0, nx);
          // flatten 3D (kk,jj,ii) to 1D:
          viskores::Id pIndex = (kk * 4 + jj) * 4 + ii;
          // flatten volume coords: (x0,y0,z0) -> 1D index
          viskores::Id dIndex = (z0 * ny + y0) * nx + x0;
          P[pIndex] = this->Field.Get(dIndex);
        }
      }
    }

    // 2) Interpolate in X for each (kk, jj) → C[kk][jj]
    //    C[kk][jj] -> C[kk*4 + jj]
    for (viskores::Id kk = 0; kk < 4; ++kk)
    {
      for (viskores::Id jj = 0; jj < 4; ++jj)
      {
        // base offset for P row
        viskores::Id baseP = (kk * 4 + jj) * 4;
        viskores::Id cIndex = kk * 4 + jj;
        C[cIndex] =
          this->CubicInterpolate(P[baseP + 0], P[baseP + 1], P[baseP + 2], P[baseP + 3], tx);
      }
    }

    // 3) Interpolate in Y for each kk → D[kk]
    //    C[kk][0..3] -> C[kk*4 + 0..3]
    for (viskores::Id kk = 0; kk < 4; ++kk)
    {
      D[kk] =
        this->CubicInterpolate(C[kk * 4 + 0], C[kk * 4 + 1], C[kk * 4 + 2], C[kk * 4 + 3], ty);
    }

    // 4) Interpolate in Z across D[0..3]
    value = this->CubicInterpolate(D[0], D[1], D[2], D[3], tz);
    return viskores::ErrorCode::Success;
  }

  // 1D cubic‐convolution (Catmull–Rom) kernel
  // given four samples p0,p1,p2,p3 and relative t in [0,1]
  VISKORES_EXEC viskores::FloatDefault CubicInterpolate(viskores::FloatDefault p0,
                                                        viskores::FloatDefault p1,
                                                        viskores::FloatDefault p2,
                                                        viskores::FloatDefault p3,
                                                        viskores::FloatDefault t) const
  {
    // Catmull–Rom basis with no tension.
    viskores::FloatDefault t2 = t * t;
    viskores::FloatDefault t3 = t2 * t;

    viskores::FloatDefault m0 = (p2 - p0) * 0.5f;
    viskores::FloatDefault m1 = (p3 - p1) * 0.5f;
    viskores::FloatDefault d0 = p1;
    viskores::FloatDefault d1 = p2;

    // Hermite form: h00, h10, h01, h11
    viskores::FloatDefault h00 = 2 * t3 - 3 * t2 + 1;
    viskores::FloatDefault h10 = t3 - 2 * t2 + t;
    viskores::FloatDefault h01 = -2 * t3 + 3 * t2;
    viskores::FloatDefault h11 = t3 - t2;

    return h00 * d0 + h10 * m0 + h01 * d1 + h11 * m1;
  }

  viskores::Bounds Bounds;
  viskores::Id3 Dimensions;
  FieldPortalType Field;
  viskores::Vec3f Origin;
  viskores::Vec3f Spacing;
};

} //namespace exec
} //namespace viskores

#endif //viskores_exec_SplineEvaluateUniformGrid_h
