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
#ifndef VISKORES_KERNELBASE_HPP
#define VISKORES_KERNELBASE_HPP

#include <viskores/Math.h>
#include <viskores/Types.h>

namespace viskores
{
namespace worklet
{
namespace splatkernels
{

// Vector class used in the kernels
using vector_type = viskores::Vec3f_64;
// Pi compatibility
#ifndef M_PI
#define M_PI viskores::Pi()
#endif

// templated utility to generate expansions at compile time for x^N
template <viskores::IdComponent N>
inline VISKORES_EXEC_CONT viskores::Float64 PowerExpansion(viskores::Float64 x)
{
  return x * PowerExpansion<N - 1>(x);
}

template <>
inline VISKORES_EXEC_CONT viskores::Float64 PowerExpansion<1>(viskores::Float64 x)
{
  return x;
}

template <>
inline VISKORES_EXEC_CONT viskores::Float64 PowerExpansion<0>(viskores::Float64)
{
  return 1;
}

//---------------------------------------------------------------------
// Base class for Kernels
// We use CRTP to avoid virtual function calls.
template <typename Kernel>
struct KernelBase
{
  //---------------------------------------------------------------------
  // Constructor
  // Calculate coefficients used repeatedly when evaluating the kernel
  // value or gradient
  // The smoothing length is usually denoted as 'h' in SPH literature
  VISKORES_EXEC_CONT
  KernelBase(double smoothingLength)
    : smoothingLength_(smoothingLength)
  {
  }

  //---------------------------------------------------------------------
  // The functions below are placeholders which should be provided by
  // concrete implementations of this class.
  // The KernelBase versions will not be called when algorithms are
  // templated over a concrete implementation.
  //---------------------------------------------------------------------

  //---------------------------------------------------------------------
  // compute w(h) for the given distance
  VISKORES_EXEC_CONT
  double w(double distance) { return static_cast<Kernel*>(this)->w(distance); }

  //---------------------------------------------------------------------
  // compute w(h) for the given squared distance
  // this version takes the distance squared as a convenience/optimization
  // but not all implementations will benefit from it
  VISKORES_EXEC_CONT
  double w2(double distance2) { return static_cast<Kernel*>(this)->w2(distance2); }

  //---------------------------------------------------------------------
  // compute w(h) for a variable h kernel
  // this is less efficient than the fixed radius version as coefficients
  // must be calculatd on the fly, but it is required when all particles
  // have different smoothing lengths
  VISKORES_EXEC_CONT
  double w(double h, double distance) { return static_cast<Kernel*>(this)->w(h, distance); }

  //---------------------------------------------------------------------
  // compute w(h) for a variable h kernel using distance squared
  // this version takes the distance squared as a convenience/optimization
  VISKORES_EXEC_CONT
  double w2(double h, double distance2) { return static_cast<Kernel*>(this)->w2(h, distance2); }

  //---------------------------------------------------------------------
  // Calculates the kernel derivative for a distance {x,y,z} vector
  // from the centre.
  VISKORES_EXEC_CONT
  vector_type gradW(double distance, const vector_type& pos)
  {
    return static_cast<Kernel*>(this)->gradW(distance, pos);
  }

  // Calculates the kernel derivative at the given distance using a variable h value
  // this is less efficient than the fixed radius version as coefficients
  // must be calculatd on the fly
  VISKORES_EXEC_CONT
  vector_type gradW(double h, double distance, const vector_type& pos)
  {
    return static_cast<Kernel*>(this)->gradW(h, distance, pos);
  }

  // return the multiplier between smoothing length and max cutoff distance
  VISKORES_EXEC_CONT
  double getDilationFactor() const { return static_cast<Kernel*>(this)->getDilationFactor; }

  // return the maximum cutoff distance over which the kernel acts,
  // beyond this distance the kernel value is zero
  VISKORES_EXEC_CONT
  double maxDistance() { return static_cast<Kernel*>(this)->maxDistance(); }

  // return the maximum cutoff distance over which the kernel acts,
  // beyond this distance the kernel value is zero
  VISKORES_EXEC_CONT
  double maxDistanceSquared() { return static_cast<Kernel*>(this)->maxDistanceSquared(); }

protected:
  const double smoothingLength_;
};
}
}
}
#endif
