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
#ifndef viskores_cont_arg_TypeCheckTagCellSetStructured_h
#define viskores_cont_arg_TypeCheckTagCellSetStructured_h

#include <viskores/cont/arg/TypeCheck.h>

#include <viskores/cont/CellSetStructured.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// Check for a Structured CellSet-like object.
///
struct TypeCheckTagCellSetStructured
{
};


template <typename CellSetType>
struct TypeCheck<TypeCheckTagCellSetStructured, CellSetType>
{
  using is_3d_cellset = std::is_same<CellSetType, viskores::cont::CellSetStructured<3>>;
  using is_2d_cellset = std::is_same<CellSetType, viskores::cont::CellSetStructured<2>>;
  using is_1d_cellset = std::is_same<CellSetType, viskores::cont::CellSetStructured<1>>;

  static constexpr bool value =
    is_3d_cellset::value || is_2d_cellset::value || is_1d_cellset::value;
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TypeCheckTagCellSetStructured_h
