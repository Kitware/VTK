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

#ifndef viskores_worklet_PointElevation_h
#define viskores_worklet_PointElevation_h

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/Math.h>

namespace viskores
{
namespace worklet
{

namespace internal
{

template <typename T>
VISKORES_EXEC T clamp(const T& val, const T& min, const T& max)
{
  return viskores::Min(max, viskores::Max(min, val));
}
}

class PointElevation : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  VISKORES_CONT
  PointElevation(const viskores::Vec3f_64& lp,
                 const viskores::Vec3f_64& hp,
                 viskores::Float64 low,
                 viskores::Float64 hi)
    : LowPoint(lp)
    , HighPoint(hp)
    , RangeLow(low)
    , RangeHigh(hi)
  {
  }

  VISKORES_EXEC
  viskores::Float64 operator()(const viskores::Vec3f_64& vec) const
  {
    viskores::Vec3f_64 direction = this->HighPoint - this->LowPoint;
    viskores::Float64 lengthSqr = viskores::Dot(direction, direction);
    viskores::Float64 rangeLength = this->RangeHigh - this->RangeLow;
    viskores::Float64 s = viskores::Dot(vec - this->LowPoint, direction) / lengthSqr;
    s = internal::clamp(s, 0.0, 1.0);
    return this->RangeLow + (s * rangeLength);
  }

  template <typename T>
  VISKORES_EXEC viskores::Float64 operator()(const viskores::Vec<T, 3>& vec) const
  {
    return (*this)(viskores::make_Vec(static_cast<viskores::Float64>(vec[0]),
                                      static_cast<viskores::Float64>(vec[1]),
                                      static_cast<viskores::Float64>(vec[2])));
  }

private:
  const viskores::Vec3f_64 LowPoint, HighPoint;
  const viskores::Float64 RangeLow, RangeHigh;
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_PointElevation_h
