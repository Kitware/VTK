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

#ifndef viskores_worklet_CoordinateSystemTransform_h
#define viskores_worklet_CoordinateSystemTransform_h

#include <viskores/Math.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
struct CylToCar : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  //Functor
  template <typename T>
  VISKORES_EXEC viskores::Vec<T, 3> operator()(const viskores::Vec<T, 3>& vec) const
  {
    viskores::Vec<T, 3> res(vec[0] * static_cast<T>(viskores::Cos(vec[1])),
                            vec[0] * static_cast<T>(viskores::Sin(vec[1])),
                            vec[2]);
    return res;
  }
};

struct CarToCyl : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  //Functor
  template <typename T>
  VISKORES_EXEC viskores::Vec<T, 3> operator()(const viskores::Vec<T, 3>& vec) const
  {
    T R = viskores::Sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
    T Theta = 0;

    if (vec[0] == 0 && vec[1] == 0)
      Theta = 0;
    else if (vec[0] < 0)
      Theta = -viskores::ASin(vec[1] / R) + static_cast<T>(viskores::Pi());
    else
      Theta = viskores::ASin(vec[1] / R);

    viskores::Vec<T, 3> res(R, Theta, vec[2]);
    return res;
  }
};

struct SphereToCar : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  //Functor
  template <typename T>
  VISKORES_EXEC viskores::Vec<T, 3> operator()(const viskores::Vec<T, 3>& vec) const
  {
    T R = vec[0];
    T Theta = vec[1];
    T Phi = vec[2];

    T sinTheta = static_cast<T>(viskores::Sin(Theta));
    T cosTheta = static_cast<T>(viskores::Cos(Theta));
    T sinPhi = static_cast<T>(viskores::Sin(Phi));
    T cosPhi = static_cast<T>(viskores::Cos(Phi));

    T x = R * sinTheta * cosPhi;
    T y = R * sinTheta * sinPhi;
    T z = R * cosTheta;

    viskores::Vec<T, 3> r(x, y, z);
    return r;
  }
};

struct CarToSphere : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  //Functor
  template <typename T>
  VISKORES_EXEC viskores::Vec<T, 3> operator()(const viskores::Vec<T, 3>& vec) const
  {
    T R = viskores::Sqrt(viskores::Dot(vec, vec));
    T Theta = 0;
    if (R > 0)
      Theta = viskores::ACos(vec[2] / R);
    T Phi = viskores::ATan2(vec[1], vec[0]);
    if (Phi < 0)
      Phi += static_cast<T>(viskores::TwoPi());

    return viskores::Vec<T, 3>(R, Theta, Phi);
  }
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_CoordinateSystemTransform_h
