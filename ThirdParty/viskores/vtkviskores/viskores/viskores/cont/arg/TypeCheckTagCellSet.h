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
#ifndef viskores_cont_arg_TypeCheckTagCellSet_h
#define viskores_cont_arg_TypeCheckTagCellSet_h

#include <viskores/cont/arg/TypeCheck.h>

#include <viskores/cont/CellSet.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// Check for a CellSet-like object.
///
struct TypeCheckTagCellSet
{
};

template <typename CellSetType>
struct TypeCheck<TypeCheckTagCellSet, CellSetType>
{
  static constexpr bool value = viskores::cont::internal::CellSetCheck<CellSetType>::type::value;
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TypeCheckTagCellSet_h
