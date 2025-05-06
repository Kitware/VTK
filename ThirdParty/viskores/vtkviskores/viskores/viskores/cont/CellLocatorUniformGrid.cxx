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

#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellLocatorUniformGrid.h>
#include <viskores/cont/CellSetStructured.h>

namespace viskores
{
namespace cont
{

using UniformType = viskores::cont::ArrayHandleUniformPointCoordinates;
using Structured2DType = viskores::cont::CellSetStructured<2>;
using Structured3DType = viskores::cont::CellSetStructured<3>;

void CellLocatorUniformGrid::Build()
{
  viskores::cont::CoordinateSystem coords = this->GetCoordinates();
  viskores::cont::UnknownCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<UniformType>())
    throw viskores::cont::ErrorBadType("Coordinates are not uniform type.");

  if (cellSet.CanConvert<Structured2DType>())
  {
    this->Is3D = false;
    Structured2DType structuredCellSet = cellSet.AsCellSet<Structured2DType>();
    viskores::Id2 pointDims =
      structuredCellSet.GetSchedulingRange(viskores::TopologyElementTagPoint());
    this->PointDims = viskores::Id3(pointDims[0], pointDims[1], 1);
  }
  else if (cellSet.CanConvert<Structured3DType>())
  {
    this->Is3D = true;
    Structured3DType structuredCellSet = cellSet.AsCellSet<Structured3DType>();
    this->PointDims = structuredCellSet.GetSchedulingRange(viskores::TopologyElementTagPoint());
  }
  else
  {
    throw viskores::cont::ErrorBadType("Cells are not 2D or 3D structured type.");
  }

  UniformType uniformCoords = coords.GetData().AsArrayHandle<UniformType>();
  auto coordsPortal = uniformCoords.ReadPortal();
  this->Origin = coordsPortal.GetOrigin();

  viskores::Vec3f spacing = coordsPortal.GetSpacing();
  viskores::Vec3f unitLength;
  unitLength[0] = static_cast<viskores::FloatDefault>(this->PointDims[0] - 1);
  unitLength[1] = static_cast<viskores::FloatDefault>(this->PointDims[1] - 1);
  unitLength[2] = static_cast<viskores::FloatDefault>(this->PointDims[2] - 1);

  this->MaxPoint = this->Origin + spacing * unitLength;
  this->InvSpacing[0] = 1.f / spacing[0];
  this->InvSpacing[1] = 1.f / spacing[1];
  this->InvSpacing[2] = 1.f / spacing[2];

  this->CellDims[0] = this->PointDims[0] - 1;
  this->CellDims[1] = this->PointDims[1] - 1;
  this->CellDims[2] = this->PointDims[2] - 1;
}

viskores::exec::CellLocatorUniformGrid CellLocatorUniformGrid::PrepareForExecution(
  viskores::cont::DeviceAdapterId viskoresNotUsed(device),
  viskores::cont::Token& viskoresNotUsed(token)) const
{
  this->Update();
  return viskores::exec::CellLocatorUniformGrid(
    this->CellDims, this->Origin, this->InvSpacing, this->MaxPoint);
}

} //namespace cont
} //namespace viskores
