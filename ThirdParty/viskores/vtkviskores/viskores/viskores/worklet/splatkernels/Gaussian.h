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
#ifndef VISKORES_KERNEL_GAUSSIAN_H
#define VISKORES_KERNEL_GAUSSIAN_H

#include <viskores/worklet/splatkernels/KernelBase.h>

//
// Gaussian kernel.
// Compact support is achieved by truncating the kernel beyond the cutoff radius
// This implementation uses a factor of 5 between smoothing length and cutoff
//

namespace viskores
{
namespace worklet
{
namespace splatkernels
{

template <int Dimensions>
struct Gaussian : public KernelBase<Gaussian<Dimensions>>
{
  //---------------------------------------------------------------------
  // Constructor
  // Calculate coefficients used repeatedly when evaluating the kernel
  // value or gradient
  VISKORES_EXEC_CONT
  Gaussian(double smoothingLength)
    : KernelBase<Gaussian<Dimensions>>(smoothingLength)
  {
    Hinverse_ = 1.0 / smoothingLength;
    Hinverse2_ = Hinverse_ * Hinverse_;
    maxRadius_ = 5.0 * smoothingLength;
    maxRadius2_ = maxRadius_ * maxRadius_;
    //
    norm_ = 1.0 / viskores::Pow(M_PI, static_cast<double>(Dimensions) / 2.0);
    scale_W_ = norm_ * PowerExpansion<Dimensions>(Hinverse_);
    scale_GradW_ = -2.0 * PowerExpansion<Dimensions + 1>(Hinverse_) / norm_;
  }

  //---------------------------------------------------------------------
  // return the multiplier between smoothing length and max cutoff distance
  VISKORES_EXEC_CONT
  constexpr double getDilationFactor() const { return 5.0; }

  //---------------------------------------------------------------------
  // compute w(h) for the given distance
  VISKORES_EXEC_CONT
  double w(double distance) const
  {
    if (distance < maxDistance())
    {
      // compute r/h
      double normedDist = distance * Hinverse_;
      // compute w(h)
      return scale_W_ * viskores::Exp(-normedDist * normedDist);
    }
    return 0.0;
  }

  //---------------------------------------------------------------------
  // compute w(h) for the given squared distance
  VISKORES_EXEC_CONT
  double w2(double distance2) const
  {
    if (distance2 < maxSquaredDistance())
    {
      // compute (r/h)^2
      double normedDist = distance2 * Hinverse2_;
      // compute w(h)
      return scale_W_ * viskores::Exp(-normedDist);
    }
    return 0.0;
  }

  //---------------------------------------------------------------------
  // compute w(h) for a variable h kernel
  VISKORES_EXEC_CONT
  double w(double h, double distance) const
  {
    if (distance < maxDistance(h))
    {
      double Hinverse = 1.0 / h;
      double scale_W = norm_ * PowerExpansion<Dimensions>(Hinverse);
      double Q = distance * Hinverse;

      return scale_W * viskores::Exp(-Q * Q);
    }
    return 0;
  }

  //---------------------------------------------------------------------
  // compute w(h) for a variable h kernel using distance squared
  VISKORES_EXEC_CONT
  double w2(double h, double distance2) const
  {
    if (distance2 < maxSquaredDistance(h))
    {
      double Hinverse = 1.0 / h;
      double scale_W = norm_ * PowerExpansion<Dimensions>(Hinverse);
      double Q = distance2 * Hinverse * Hinverse;

      return scale_W * viskores::Exp(-Q);
    }
    return 0;
  }

  //---------------------------------------------------------------------
  // Calculates the kernel derivative for a distance {x,y,z} vector
  // from the centre
  VISKORES_EXEC_CONT
  vector_type gradW(double distance, const vector_type& pos) const
  {
    double Q = distance * Hinverse_;
    if (Q != 0.0)
    {
      return scale_GradW_ * viskores::Exp(-Q * Q) * pos;
    }
    else
    {
      return vector_type(0.0);
    }
  }

  //---------------------------------------------------------------------
  // Calculates the kernel derivative for a distance {x,y,z} vector
  // from the centre using a variable h
  VISKORES_EXEC_CONT
  vector_type gradW(double h, double distance, const vector_type& pos) const
  {
    double Hinverse = 1.0 / h;
    double scale_GradW = -2.0 * PowerExpansion<Dimensions + 1>(Hinverse) /
      viskores::Pow(M_PI, static_cast<double>(Dimensions) / 2.0);
    double Q = distance * Hinverse;

    //!!! check this due to the fitting offset
    if (distance != 0.0)
    {
      return scale_GradW * viskores::Exp(-Q * Q) * pos;
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
  double maxDistance(double h) const { return getDilationFactor() * h; }

  //---------------------------------------------------------------------
  // return the maximum distance at which this kernel is non zero
  VISKORES_EXEC_CONT
  double maxSquaredDistance() const { return maxRadius2_; }

  //---------------------------------------------------------------------
  // return the maximum distance at which this kernel is non zero
  VISKORES_EXEC_CONT
  double maxSquaredDistance(double h) const
  {
    return PowerExpansion<2>(getDilationFactor()) * h * h;
  }

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
