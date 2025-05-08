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
#ifndef viskores_cont_CellLocatorUniformGrid_h
#define viskores_cont_CellLocatorUniformGrid_h

#include <viskores/cont/CellLocatorBase.h>

#include <viskores/exec/CellLocatorUniformGrid.h>

namespace viskores
{
namespace cont
{

/// @brief A cell locator optimized for finding cells in a uniform grid.
///
/// This locator is optimized for structured data that has uniform axis-aligned spacing.
/// For this cell locator to work, it has to be given a cell set of type
/// `viskores::cont::CellSetStructured` and a coordinate system using a
/// `viskores::cont::ArrayHandleUniformPointCoordinates` for its coordinate system.
/// If the data set matches this structure, then this locator will be faster than
/// any others.
class VISKORES_CONT_EXPORT CellLocatorUniformGrid : public viskores::cont::CellLocatorBase
{
public:
  using LastCell = viskores::exec::CellLocatorUniformGrid::LastCell;

  VISKORES_CONT viskores::exec::CellLocatorUniformGrid PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const;

private:
  viskores::Id3 CellDims;
  viskores::Id3 PointDims;
  viskores::Vec3f Origin;
  viskores::Vec3f InvSpacing;
  viskores::Vec3f MaxPoint;
  bool Is3D = true;

  VISKORES_CONT void Build() override;
};
}
} // viskores::cont

#endif //viskores_cont_CellLocatorUniformGrid_h
