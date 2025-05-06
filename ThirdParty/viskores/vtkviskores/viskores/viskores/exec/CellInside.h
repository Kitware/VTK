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
#ifndef viskores_exec_CellInside_h
#define viskores_exec_CellInside_h

#include <viskores/CellShape.h>
#include <viskores/Types.h>

#include <lcl/lcl.h>

namespace viskores
{
namespace exec
{

template <typename T, typename CellShapeTag>
static inline VISKORES_EXEC bool CellInside(const viskores::Vec<T, 3>& pcoords, CellShapeTag)
{
  using VtkcTagType = typename viskores::internal::CellShapeTagViskoresToVtkc<CellShapeTag>::Type;
  return lcl::cellInside(VtkcTagType{}, pcoords);
}

template <typename T>
static inline VISKORES_EXEC bool CellInside(const viskores::Vec<T, 3>&, viskores::CellShapeTagEmpty)
{
  return false;
}

template <typename T>
static inline VISKORES_EXEC bool CellInside(const viskores::Vec<T, 3>& pcoords,
                                            viskores::CellShapeTagPolyLine)
{
  return pcoords[0] >= T(0) && pcoords[0] <= T(1);
}

/// Checks if the parametric coordinates `pcoords` are on the inside for the
/// specified cell type.
///
template <typename T>
static inline VISKORES_EXEC bool CellInside(const viskores::Vec<T, 3>& pcoords,
                                            viskores::CellShapeTagGeneric shape)
{
  bool result = false;
  switch (shape.Id)
  {
    viskoresGenericCellShapeMacro(result = CellInside(pcoords, CellShapeTag()));
    default:
      break;
  }

  return result;
}
}
} // viskores::exec

#endif // viskores_exec_CellInside_h
