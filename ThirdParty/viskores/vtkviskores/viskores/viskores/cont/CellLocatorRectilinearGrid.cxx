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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/CellLocatorRectilinearGrid.h>
#include <viskores/cont/CellSetStructured.h>

#include <viskores/exec/CellLocatorRectilinearGrid.h>

namespace viskores
{
namespace cont
{

void CellLocatorRectilinearGrid::Build()
{
  viskores::cont::CoordinateSystem coords = this->GetCoordinates();
  viskores::cont::UnknownCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<RectilinearType>())
    throw viskores::cont::ErrorBadType("Coordinates are not rectilinear type.");

  if (cellSet.CanConvert<Structured2DType>())
  {
    this->Is3D = false;
    viskores::Vec<viskores::Id, 2> celldims =
      cellSet.AsCellSet<Structured2DType>().GetSchedulingRange(viskores::TopologyElementTagCell());
    this->PlaneSize = celldims[0] * celldims[1];
    this->RowSize = celldims[0];
  }
  else if (cellSet.CanConvert<Structured3DType>())
  {
    this->Is3D = true;
    viskores::Vec<viskores::Id, 3> celldims =
      cellSet.AsCellSet<Structured3DType>().GetSchedulingRange(viskores::TopologyElementTagCell());
    this->PlaneSize = celldims[0] * celldims[1];
    this->RowSize = celldims[0];
  }
  else
  {
    throw viskores::cont::ErrorBadType("Cells are not 2D or 3D structured type.");
  }
}

viskores::exec::CellLocatorRectilinearGrid CellLocatorRectilinearGrid::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  this->Update();

  using ExecObjType = viskores::exec::CellLocatorRectilinearGrid;

  if (this->Is3D)
  {
    return ExecObjType(this->PlaneSize,
                       this->RowSize,
                       this->GetCellSet().AsCellSet<Structured3DType>(),
                       this->GetCoordinates().GetData().template AsArrayHandle<RectilinearType>(),
                       device,
                       token);
  }
  else
  {
    return ExecObjType(this->PlaneSize,
                       this->RowSize,
                       this->GetCellSet().AsCellSet<Structured2DType>(),
                       this->GetCoordinates().GetData().template AsArrayHandle<RectilinearType>(),
                       device,
                       token);
  }
}

} //namespace cont
} //namespace viskores
