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

#include <viskores/CellClassification.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/mesh_info/GhostCellClassify.h>
#include <viskores/worklet/WorkletPointNeighborhood.h>

namespace viskores
{
namespace filter
{
namespace detail
{

class SetStructuredGhostCells1D : public viskores::worklet::WorkletPointNeighborhood
{
public:
  explicit SetStructuredGhostCells1D(viskores::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VISKORES_EXEC void operator()(const viskores::exec::BoundaryState& boundary,
                                viskores::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInXBoundary(this->NumLayers);
    value =
      (notOnBoundary) ? viskores::CellClassification::Normal : viskores::CellClassification::Ghost;
  }

private:
  viskores::IdComponent NumLayers;
};

class SetStructuredGhostCells2D : public viskores::worklet::WorkletPointNeighborhood
{
public:
  explicit SetStructuredGhostCells2D(viskores::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VISKORES_EXEC void operator()(const viskores::exec::BoundaryState& boundary,
                                viskores::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInXBoundary(this->NumLayers) &&
      boundary.IsRadiusInYBoundary(this->NumLayers);
    value =
      (notOnBoundary) ? viskores::CellClassification::Normal : viskores::CellClassification::Ghost;
  }

private:
  viskores::IdComponent NumLayers;
};

class SetStructuredGhostCells3D : public viskores::worklet::WorkletPointNeighborhood
{
public:
  explicit SetStructuredGhostCells3D(viskores::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  VISKORES_EXEC void operator()(const viskores::exec::BoundaryState& boundary,
                                viskores::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInBoundary(this->NumLayers);
    value =
      (notOnBoundary) ? viskores::CellClassification::Normal : viskores::CellClassification::Ghost;
  }

private:
  viskores::IdComponent NumLayers;
};
} // namespace detail

namespace mesh_info
{

VISKORES_CONT viskores::cont::DataSet GhostCellClassify::DoExecute(
  const viskores::cont::DataSet& input)
{
  const viskores::cont::UnknownCellSet& cellset = input.GetCellSet();
  viskores::cont::ArrayHandle<viskores::UInt8> ghosts;
  const viskores::Id numCells = cellset.GetNumberOfCells();

  //Structured cases are easy...
  if (cellset.template IsType<viskores::cont::CellSetStructured<1>>())
  {
    if (numCells <= 2)
      throw viskores::cont::ErrorFilterExecution(
        "insufficient number of cells for GhostCellClassify.");

    viskores::cont::CellSetStructured<1> cellset1d =
      cellset.AsCellSet<viskores::cont::CellSetStructured<1>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    viskores::cont::CellSetStructured<3> dual;
    const auto dim = cellset1d.GetCellDimensions();
    dual.SetPointDimensions(viskores::Id3{ dim, 1, 1 });
    this->Invoke(viskores::filter::detail::SetStructuredGhostCells1D{}, dual, ghosts);
  }
  else if (cellset.template IsType<viskores::cont::CellSetStructured<2>>())
  {
    if (numCells <= 4)
      throw viskores::cont::ErrorFilterExecution(
        "insufficient number of cells for GhostCellClassify.");

    viskores::cont::CellSetStructured<2> cellset2d =
      cellset.AsCellSet<viskores::cont::CellSetStructured<2>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    viskores::cont::CellSetStructured<3> dual;
    const auto dims = cellset2d.GetCellDimensions();
    dual.SetPointDimensions(viskores::Id3{ dims[0], dims[1], 1 });
    this->Invoke(viskores::filter::detail::SetStructuredGhostCells2D{}, dual, ghosts);
  }
  else if (cellset.template IsType<viskores::cont::CellSetStructured<3>>())
  {
    if (numCells <= 8)
      throw viskores::cont::ErrorFilterExecution(
        "insufficient number of cells for GhostCellClassify.");

    viskores::cont::CellSetStructured<3> cellset3d =
      cellset.AsCellSet<viskores::cont::CellSetStructured<3>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    viskores::cont::CellSetStructured<3> dual;
    dual.SetPointDimensions(cellset3d.GetCellDimensions());
    this->Invoke(viskores::filter::detail::SetStructuredGhostCells3D{}, dual, ghosts);
  }
  else
  {
    throw viskores::cont::ErrorFilterExecution("Unsupported cellset type for GhostCellClassify.");
  }

  auto output = this->CreateResult(input);
  output.SetGhostCellField(this->GhostCellName, ghosts);
  return output;
}
}
}
}
