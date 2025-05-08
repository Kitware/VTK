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
#ifndef VISKORES_KERNEL_SPLINE_3RD_ORDER_H
#define VISKORES_KERNEL_SPLINE_3RD_ORDER_H

#include <viskores/worklet/splatkernels/KernelBase.h>

//
// Spline 3rd Order kernel.
//

namespace viskores
{
namespace worklet
{
namespace splatkernels
{

template <viskores::IdComponent Dim>
struct default_norm_value;

template <>
struct default_norm_value<2>
{
  const double value = 10.0 / (7.0 * M_PI);
};

template <>
struct default_norm_value<3>
{
  const double value = 1.0 / M_PI;
};


template <int Dimensions>
struct Spline3rdOrder : public KernelBase<Spline3rdOrder<Dimensions>>
{
  //---------------------------------------------------------------------
  // Constructor
  // Calculate coefficients used repeatedly when evaluating the kernel
  // value or gradient
  VISKORES_EXEC_CONT
  Spline3rdOrder(double smoothingLength)
    : KernelBase<Spline3rdOrder<Dimensions>>(smoothingLength)
  {
    Hinverse_ = 1.0 / smoothingLength;
    Hinverse2_ = Hinverse_ * Hinverse_;
    maxRadius_ = 2.0 * smoothingLength;
    maxRadius2_ = maxRadius_ * maxRadius_;
    //
    norm_ = default_norm_value<Dimensions>().value;

    scale_W_ = norm_ * PowerExpansion<Dimensions>(Hinverse_);
    scale_GradW_ = norm_ * PowerExpansion<Dimensions + 1>(Hinverse_);
  }
  //---------------------------------------------------------------------
  // Calculates the kernel value for the given distance
  VISKORES_EXEC_CONT
  double w(double distance) const
  {
    // compute Q=(r/h)
    double Q = distance * Hinverse_;
    if (Q < 1.0)
    {
      return scale_W_ * (1.0 - (3.0 / 2.0) * Q * Q + (3.0 / 4.0) * Q * Q * Q);
    }
    else if (Q < 2.0)
    {
      double q2 = (2.0 - Q);
      return scale_W_ * (1.0 / 4.0) * (q2 * q2 * q2);
    }
    else
    {
      return 0.0;
    }
  }

  //---------------------------------------------------------------------
  // Calculates the kernel value for the given squared distance
  VISKORES_EXEC_CONT
  double w2(double distance2) const
  {
    // compute Q
    double Q = sqrt(distance2) * Hinverse_;
    if (Q < 1.0)
    {
      return scale_W_ * (1.0 - (3.0 / 2.0) * Q * Q + (3.0 / 4.0) * Q * Q * Q);
    }
    else if (Q < 2.0)
    {
      double q2 = (2.0 - Q);
      return scale_W_ * (1.0 / 4.0) * (q2 * q2 * q2);
    }
    else
    {
      return 0.0;
    }
  }

  //---------------------------------------------------------------------
  // compute w(h) for a variable h kernel
  VISKORES_EXEC_CONT
  double w(double h, double distance) const
  {
    double Hinverse = 1.0 / h;
    double scale_W = norm_ * PowerExpansion<Dimensions>(Hinverse);
    double Q = distance * Hinverse;
    if (Q < 1.0)
    {
      return scale_W * (1.0 - (3.0 / 2.0) * Q * Q + (3.0 / 4.0) * Q * Q * Q);
    }
    else if (Q < 2.0)
    {
      double q2 = (2.0 - Q);
      return scale_W * (1.0 / 4.0) * (q2 * q2 * q2);
    }
    else
    {
      return 0.0;
    }
  }

  //---------------------------------------------------------------------
  // compute w(h) for a variable h kernel using distance squared
  VISKORES_EXEC_CONT
  double w2(double h, double distance2) const
  {
    double Hinverse = 1.0 / h;
    double scale_W = norm_ * PowerExpansion<Dimensions>(Hinverse);
    double Q = sqrt(distance2) * Hinverse;
    if (Q < 1.0)
    {
      return scale_W * (1.0 - (3.0 / 2.0) * Q * Q + (3.0 / 4.0) * Q * Q * Q);
    }
    else if (Q < 2.0)
    {
      double q2 = (2.0 - Q);
      return scale_W * (1.0 / 4.0) * (q2 * q2 * q2);
    }
    else
    {
      return 0.0;
    }
  }

  //---------------------------------------------------------------------
  // Calculates the kernel derivation for the given distance of two particles.
  // The used formula is the derivation of Speith (3.126) for the value
  // with (3.21) for the direction of the gradient vector.
  // Be careful: grad W is antisymmetric in r (3.25)!.
  VISKORES_EXEC_CONT
  vector_type gradW(double distance, const vector_type& pos) const
  {
    double Q = distance * Hinverse_;
    if (Q == 0.0)
    {
      return vector_type(0.0);
    }
    else if (Q < 1.0)
    {
      return scale_GradW_ * (-3.0 * Q + (9.0 / 4.0) * Q * Q) * pos;
    }
    else if (Q < 2.0)
    {
      double q2 = (2.0 - Q);
      return scale_GradW_ * (-3.0 / 4.0) * q2 * q2 * pos;
    }
    else
    {
      return vector_type(0.0);
    }
  }

  //---------------------------------------------------------------------
  VISKORES_EXEC_CONT
  vector_type gradW(double h, double distance, const vector_type& pos) const
  {
    double Hinverse = 1.0 / h;
    double scale_GradW = norm_ * PowerExpansion<Dimensions + 1>(Hinverse);
    double Q = distance * Hinverse;
    if (Q == 0.0)
    {
      return vector_type(0.0);
    }
    else if (Q < 1.0)
    {
      return scale_GradW * (-3.0 * Q + (9.0 / 4.0) * Q * Q) * pos;
    }
    else if (Q < 2.0)
    {
      double q2 = (2.0 - Q);
      return scale_GradW * (-3.0 / 4.0) * q2 * q2 * pos;
    }
    else
    {
      return vector_type(0.0);
    }
  }

  //---------------------------------------------------------------------
  // return the maximum distance at which this kernel is non zero
  VISKORES_EXEC_CONT
  double maxDistance() const { return maxRadius_; }

  //---------------------------------------------------------------------
  // return the maximum distance at which this variable h kernel is non zero
  VISKORES_EXEC_CONT
  double maxDistance(double h) const { return 2.0 * h; }

  //---------------------------------------------------------------------
  // return the maximum distance at which this kernel is non zero
  VISKORES_EXEC_CONT
  double maxSquaredDistance() const { return maxRadius2_; }

  //---------------------------------------------------------------------
  // return the maximum distance at which this kernel is non zero
  VISKORES_EXEC_CONT
  double maxSquaredDistance(double h) const { return 4.0 * h * h; }

  //---------------------------------------------------------------------
  // return the multiplier between smoothing length and max cutoff distance
  VISKORES_EXEC_CONT
  double getDilationFactor() const { return 2.0; }

private:
  double norm_;
  double Hinverse_;
  double Hinverse2_;
  double maxRadius_;
  double maxRadius2_;
  double scale_W_;
  double scale_GradW_;
};
}
}
}

#endif
