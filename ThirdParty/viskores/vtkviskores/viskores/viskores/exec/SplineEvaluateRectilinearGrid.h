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
#ifndef viskores_exec_SplineEvaluateRectilinearGrid_h
#define viskores_exec_SplineEvaluateRectilinearGrid_h

#include <viskores/Bounds.h>
#include <viskores/ErrorCode.h>

namespace viskores
{
namespace exec
{

class VISKORES_ALWAYS_EXPORT SplineEvaluateRectilinearGrid
{
private:
  using FieldType = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using AxisType = FieldType;
  using RectilinearType = viskores::cont::ArrayHandleCartesianProduct<AxisType, AxisType, AxisType>;

  using FieldPortalType = typename FieldType::ReadPortalType;
  using AxisPortalType = typename AxisType::ReadPortalType;

public:
  VISKORES_CONT SplineEvaluateRectilinearGrid() = default;

  VISKORES_CONT SplineEvaluateRectilinearGrid(const RectilinearType& rectCoords,
                                              const FieldType& field,
                                              const viskores::Bounds& bounds,
                                              viskores::cont::DeviceAdapterId device,
                                              viskores::cont::Token& token)
    : Bounds(bounds)
    , Field(field.PrepareForInput(device, token))
  {
    auto coordsExecPortal = rectCoords.PrepareForInput(device, token);

    this->AxisPortals[0] = coordsExecPortal.GetFirstPortal();
    this->AxisPortals[1] = coordsExecPortal.GetSecondPortal();
    this->AxisPortals[2] = coordsExecPortal.GetThirdPortal();

    this->NumX = this->AxisPortals[0].GetNumberOfValues();
    this->NumY = this->AxisPortals[1].GetNumberOfValues();
    this->NumZ = this->AxisPortals[2].GetNumberOfValues();
  }

  VISKORES_EXEC viskores::ErrorCode Evaluate(const viskores::Vec3f& point,
                                             viskores::FloatDefault& value) const
  {
    if (!this->Bounds.Contains(point))
      return viskores::ErrorCode::CellNotFound;

    auto x = point[0];
    auto y = point[1];
    auto z = point[2];

    viskores::Id iu = this->FindIndex(this->AxisPortals[0], this->NumX, point[0]);
    viskores::Id iv = this->FindIndex(this->AxisPortals[1], this->NumY, point[1]);
    viskores::Id iw = this->FindIndex(this->AxisPortals[2], this->NumZ, point[2]);

    VISKORES_ASSERT(this->NumX >= 4 && this->NumY >= 4);

    if (this->NumZ < 4)
    {
      // --- bicubic: gather a 4×4 patch in X–Y at single k = clamp(iw,0,ny−1) ---
      viskores::FloatDefault P2d[4 * 4];
      for (viskores::Id jj = 0; jj < 4; ++jj)
      {
        viskores::Id j = viskores::Clamp(iv - 1 + jj, 0, this->NumY - 1);
        for (viskores::Id ii = 0; ii < 4; ++ii)
        {
          viskores::Id i = viskores::Clamp(iu - 1 + ii, 0, this->NumX - 1);
          // flatten (i,j, 0) → data index
          P2d[jj * 4 + ii] = this->Field.Get((j * this->NumX - 1) + i);
        }
      }
      // 3) bicubic along X → C2[4]
      viskores::FloatDefault C2[4];
      for (int jj = 0; jj < 4; ++jj)
      {
        auto x0 = this->AxisPortals[0].Get(iu - 1);
        auto x1 = this->AxisPortals[0].Get(iu);
        auto x2 = this->AxisPortals[0].Get(iu + 1);
        auto x3 = this->AxisPortals[0].Get(iu + 2);
        C2[jj] = this->CubicInterpolateNonUniform(
          x0, x1, x2, x3, P2d[jj * 4 + 0], P2d[jj * 4 + 1], P2d[jj * 4 + 2], P2d[jj * 4 + 3], x);
      }
      // 4) bicubic along Y on C2 → final
      auto y0 = this->AxisPortals[1].Get(iv - 1);
      auto y1 = this->AxisPortals[1].Get(iv);
      auto y2 = this->AxisPortals[1].Get(iv + 1);
      auto y3 = this->AxisPortals[1].Get(iv + 2);
      value = this->CubicInterpolateNonUniform(y0, y1, y2, y3, C2[0], C2[1], C2[2], C2[3], y);
      return viskores::ErrorCode::Success;
    }

    viskores::FloatDefault P[4 * 4 * 4];
    for (viskores::Id kk = 0; kk < 4; ++kk)
    {
      viskores::Id k = viskores::Clamp(iw - 1 + kk, 0, this->NumZ - 1);
      for (viskores::Id jj = 0; jj < 4; ++jj)
      {
        viskores::Id j = viskores::Clamp(iv - 1 + jj, 0, this->NumY - 1);
        for (viskores::Id ii = 0; ii < 4; ++ii)
        {
          viskores::Id i = viskores::Clamp(iu - 1 + ii, 0, this->NumX - 1);
          auto pIndex = (kk * 4 + jj) * 4 + ii;
          auto dIndex = (k * this->NumY + j) * this->NumX + i;
          P[pIndex] = this->Field.Get(dIndex);
        }
      }
    }

    // interpolate in X for each (kk,jj) → Cbuf[16]
    viskores::FloatDefault Cbuf[4 * 4];
    for (int kk = 0; kk < 4; ++kk)
      for (int jj = 0; jj < 4; ++jj)
      {
        auto x0 = this->AxisPortals[0].Get(iu - 1);
        auto x1 = this->AxisPortals[0].Get(iu);
        auto x2 = this->AxisPortals[0].Get(iu + 1);
        auto x3 = this->AxisPortals[0].Get(iu + 2);
        auto p0 = P[(kk * 4 + jj) * 4 + 0];
        auto p1 = P[(kk * 4 + jj) * 4 + 1];
        auto p2 = P[(kk * 4 + jj) * 4 + 2];
        auto p3 = P[(kk * 4 + jj) * 4 + 3];
        Cbuf[kk * 4 + jj] = this->CubicInterpolateNonUniform(x0, x1, x2, x3, p0, p1, p2, p3, x);
      }

    // interpolate in Y → D[4]
    viskores::FloatDefault D[4];
    for (int kk = 0; kk < 4; ++kk)
    {
      auto y0 = this->AxisPortals[1].Get(iv - 1);
      auto y1 = this->AxisPortals[1].Get(iv);
      auto y2 = this->AxisPortals[1].Get(iv + 1);
      auto y3 = this->AxisPortals[1].Get(iv + 2);
      D[kk] = this->CubicInterpolateNonUniform(
        y0, y1, y2, y3, Cbuf[kk * 4 + 0], Cbuf[kk * 4 + 1], Cbuf[kk * 4 + 2], Cbuf[kk * 4 + 3], y);
    }

    // interpolate in Z
    auto z0 = this->AxisPortals[2].Get(iw - 1);
    auto z1 = this->AxisPortals[2].Get(iw);
    auto z2 = this->AxisPortals[2].Get(iw + 1);
    auto z3 = this->AxisPortals[2].Get(iw + 2);
    value = this->CubicInterpolateNonUniform(z0, z1, z2, z3, D[0], D[1], D[2], D[3], z);

    return viskores::ErrorCode::Success;
  }

private:
  VISKORES_EXEC viskores::Id FindIndex(const AxisPortalType& axis,
                                       const viskores::Id& N,
                                       viskores::FloatDefault val) const
  {
    // 1) Binary search for the largest index i with coords[i] <= val
    viskores::Id left = 0;
    viskores::Id right = N - 1;
    while (left <= right)
    {
      viskores::Id mid = left + (right - left) / 2;
      if (axis.Get(mid) <= val)
      {
        // mid is still ≤ val, so it might be our i
        left = mid + 1;
      }
      else
      {
        // coords[mid] > val, so the index we want is below mid
        right = mid - 1;
      }
    }
    // when loop ends, `right` is the last index where coords[right] <= val
    viskores::Id i = right;

    // 2) Clamp i into [1, N-3]
    return viskores::Max(viskores::Id(1), viskores::Min(i, N - 3));

    //return viskores::Clamp(viskores::Id(i), viskores::Id(1), viskores::Id(N - 3));
  }

  VISKORES_EXEC
  viskores::FloatDefault CubicInterpolateNonUniform(viskores::FloatDefault x0,
                                                    viskores::FloatDefault x1,
                                                    viskores::FloatDefault x2,
                                                    viskores::FloatDefault x3,
                                                    viskores::FloatDefault p0,
                                                    viskores::FloatDefault p1,
                                                    viskores::FloatDefault p2,
                                                    viskores::FloatDefault p3,
                                                    viskores::FloatDefault x) const
  {
    // 1) Compute interval lengths
    viskores::FloatDefault h0 = x1 - x0;
    viskores::FloatDefault h1 = x2 - x1;
    viskores::FloatDefault h2 = x3 - x2;
    VISKORES_ASSERT(h0 > 0.0f && h1 > 0.0f && h2 > 0.0f);


    // 2) Compute right‐hand sides for second‐derivative system
    viskores::FloatDefault rhs1 = 6.0f * ((p2 - p1) / h1 - (p1 - p0) / h0);
    viskores::FloatDefault rhs2 = 6.0f * ((p3 - p2) / h2 - (p2 - p1) / h1);

    // 3) Build and solve the 2×2 system:
    //     [2(h0+h1)   h1      ][d2_1] = [rhs1]
    //     [  h1     2(h1+h2)  ][d2_2]   [rhs2]
    viskores::FloatDefault a11 = 2.0f * (h0 + h1);
    viskores::FloatDefault a12 = h1;
    viskores::FloatDefault a21 = h1;
    viskores::FloatDefault a22 = 2.0f * (h1 + h2);
    viskores::FloatDefault det = a11 * a22 - a12 * a21;
    VISKORES_ASSERT(det != 0.0f);

    viskores::FloatDefault d2_1 = (rhs1 * a22 - a12 * rhs2) / det;
    viskores::FloatDefault d2_2 = (a11 * rhs2 - rhs1 * a21) / det;

    // 4) Map x into local parameter t ∈ [0,1] on [x1,x2]
    viskores::FloatDefault t = (x - x1) / h1;

    // 5) Hermite form of the natural cubic on [x1, x2]
    viskores::FloatDefault A = 1.0f - t;
    viskores::FloatDefault B = t;
    viskores::FloatDefault h1_sq_6 = h1 * h1 / 6.f;
    viskores::FloatDefault term1 = (A * A * A - A) * h1_sq_6 * d2_1;
    viskores::FloatDefault term2 = (B * B * B - B) * h1_sq_6 * d2_2;

    // 6) Combine the linear and curvature parts
    return A * p1 + B * p2 + term1 + term2;
  }

  AxisPortalType AxisPortals[3];
  viskores::Bounds Bounds;
  FieldPortalType Field;
  viskores::Id NumX;
  viskores::Id NumY;
  viskores::Id NumZ;
};
} //namespace exec
} //namespace viskores

#endif //viskores_exec_SplineEvaluateRectilinearGrid_h
