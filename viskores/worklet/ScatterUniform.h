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
#ifndef viskores_worklet_ScatterUniform_h
#define viskores_worklet_ScatterUniform_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/worklet/internal/ScatterBase.h>

namespace viskores
{
namespace worklet
{

namespace detail
{

template <viskores::IdComponent Modulus>
struct FunctorModulus
{
  VISKORES_EXEC_CONT
  viskores::IdComponent operator()(viskores::Id index) const
  {
    return static_cast<viskores::IdComponent>(index % Modulus);
  }
};

template <viskores::IdComponent Divisor>
struct FunctorDiv
{
  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id index) const { return index / Divisor; }
};
}

/// \brief A scatter that maps input to some constant numbers of output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterUniform establishes a 1 to N
/// mapping from input to output. That is, every input element generates N
/// elements associated with it where N is the same for every input. The output
/// elements are grouped by the input associated.
///
template <viskores::IdComponent NumOutputsPerInput>
struct ScatterUniform : internal::ScatterBase
{
  VISKORES_CONT ScatterUniform() = default;

  VISKORES_CONT
  viskores::Id GetOutputRange(viskores::Id inputRange) const
  {
    return inputRange * NumOutputsPerInput;
  }
  VISKORES_CONT
  viskores::Id GetOutputRange(viskores::Id3 inputRange) const
  {
    return this->GetOutputRange(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  using OutputToInputMapType =
    viskores::cont::ArrayHandleImplicit<detail::FunctorDiv<NumOutputsPerInput>>;
  template <typename RangeType>
  VISKORES_CONT OutputToInputMapType GetOutputToInputMap(RangeType inputRange) const
  {
    return OutputToInputMapType(detail::FunctorDiv<NumOutputsPerInput>(),
                                this->GetOutputRange(inputRange));
  }

  using VisitArrayType =
    viskores::cont::ArrayHandleImplicit<detail::FunctorModulus<NumOutputsPerInput>>;
  template <typename RangeType>
  VISKORES_CONT VisitArrayType GetVisitArray(RangeType inputRange) const
  {
    return VisitArrayType(detail::FunctorModulus<NumOutputsPerInput>(),
                          this->GetOutputRange(inputRange));
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_ScatterUniform_h
