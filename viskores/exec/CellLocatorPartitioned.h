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
#ifndef viskores_exec_CellLocatorPartitioned_h
#define viskores_exec_CellLocatorPartitioned_h

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellLocatorGeneral.h>

namespace viskores
{
namespace exec
{
class VISKORES_ALWAYS_EXPORT CellLocatorPartitioned
{
private:
  viskores::cont::ArrayHandle<viskores::cont::CellLocatorGeneral::ExecObjType>::ReadPortalType
    Locators;
  viskores::cont::ArrayHandle<
    viskores::cont::ArrayHandleStride<viskores::UInt8>::ReadPortalType>::ReadPortalType Ghosts;

public:
  VISKORES_CONT CellLocatorPartitioned() = default;
  VISKORES_CONT CellLocatorPartitioned(
    const viskores::cont::ArrayHandle<
      viskores::cont::CellLocatorGeneral::ExecObjType>::ReadPortalType& locators,
    viskores::cont::ArrayHandle<
      viskores::cont::ArrayHandleStride<viskores::UInt8>::ReadPortalType>::ReadPortalType ghosts)
    : Locators(locators)
    , Ghosts(ghosts)
  {
  }

  VISKORES_EXEC
  viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                               viskores::Id& partitionId,
                               viskores::Id& cellId,
                               viskores::Vec3f& parametric) const
  {
    bool found = 0;
    for (viskores::Id partitionIndex = 0; partitionIndex < this->Locators.GetNumberOfValues();
         ++partitionIndex)
    {
      viskores::Id cellIndex;
      viskores ::Vec3f parametricLocal;
      viskores ::ErrorCode status =
        Locators.Get(partitionIndex).FindCell(point, cellIndex, parametricLocal);
      if (status != viskores ::ErrorCode ::Success)
      {
      }
      else
      {
        if (Ghosts.Get(partitionIndex).Get(cellIndex) == 0)
        {
          partitionId = partitionIndex;
          cellId = cellIndex;
          parametric = parametricLocal;
          found = true;
          break;
        }
      }
    }
    if (found)
    {
      return viskores::ErrorCode::Success;
    }
    else
    {
      return viskores::ErrorCode::CellNotFound;
    }
  }
};
} //namespace exec
} //namespace viskores

#endif //viskores_exec_CellLocatorPartitioned_h
