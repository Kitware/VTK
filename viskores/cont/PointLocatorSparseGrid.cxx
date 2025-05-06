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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <viskores/cont/PointLocatorSparseGrid.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace cont
{

namespace internal
{

class BinPointsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coord, FieldOut label);

  using ExecutionSignature = void(_1, _2);

  VISKORES_CONT
  BinPointsWorklet(viskores::Vec3f min, viskores::Vec3f max, viskores::Id3 dims)
    : Min(min)
    , Dims(dims)
    , Dxdydz((max - Min) / Dims)
  {
  }

  template <typename CoordVecType, typename IdType>
  VISKORES_EXEC void operator()(const CoordVecType& coord, IdType& label) const
  {
    viskores::Id3 ijk = (coord - Min) / Dxdydz;
    ijk = viskores::Max(ijk, viskores::Id3(0));
    ijk = viskores::Min(ijk, this->Dims - viskores::Id3(1));
    label = ijk[0] + ijk[1] * Dims[0] + ijk[2] * Dims[0] * Dims[1];
  }

private:
  viskores::Vec3f Min;
  viskores::Id3 Dims;
  viskores::Vec3f Dxdydz;
};

} // viskores::cont::internal

void PointLocatorSparseGrid::Build()
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "PointLocatorSparseGrid::Build");

  if (this->IsRangeInvalid())
  {
    this->Range = this->GetCoordinates().GetRange();
  }

  auto rmin = viskores::make_Vec(static_cast<viskores::FloatDefault>(this->Range[0].Min),
                                 static_cast<viskores::FloatDefault>(this->Range[1].Min),
                                 static_cast<viskores::FloatDefault>(this->Range[2].Min));
  auto rmax = viskores::make_Vec(static_cast<viskores::FloatDefault>(this->Range[0].Max),
                                 static_cast<viskores::FloatDefault>(this->Range[1].Max),
                                 static_cast<viskores::FloatDefault>(this->Range[2].Max));

  // generate unique id for each input point
  viskores::cont::ArrayHandleIndex pointIndex(this->GetCoordinates().GetNumberOfValues());
  viskores::cont::ArrayCopy(pointIndex, this->PointIds);

  using internal::BinPointsWorklet;

  // bin points into cells and give each of them the cell id.
  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  BinPointsWorklet cellIdWorklet(rmin, rmax, this->Dims);
  viskores::cont::Invoker invoke;
  invoke(cellIdWorklet, this->GetCoordinates(), cellIds);

  // Group points of the same cell together by sorting them according to the cell ids
  viskores::cont::Algorithm::SortByKey(cellIds, this->PointIds);

  // for each cell, find the lower and upper bound of indices to the sorted point ids.
  viskores::cont::ArrayHandleCounting<viskores::Id> cell_ids_counting(
    0, 1, this->Dims[0] * this->Dims[1] * this->Dims[2]);
  viskores::cont::Algorithm::UpperBounds(cellIds, cell_ids_counting, this->CellUpper);
  viskores::cont::Algorithm::LowerBounds(cellIds, cell_ids_counting, this->CellLower);
}

viskores::exec::PointLocatorSparseGrid PointLocatorSparseGrid::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  auto rmin = viskores::make_Vec(static_cast<viskores::FloatDefault>(this->Range[0].Min),
                                 static_cast<viskores::FloatDefault>(this->Range[1].Min),
                                 static_cast<viskores::FloatDefault>(this->Range[2].Min));
  auto rmax = viskores::make_Vec(static_cast<viskores::FloatDefault>(this->Range[0].Max),
                                 static_cast<viskores::FloatDefault>(this->Range[1].Max),
                                 static_cast<viskores::FloatDefault>(this->Range[2].Max));
  return viskores::exec::PointLocatorSparseGrid(
    rmin,
    rmax,
    this->Dims,
    this->GetCoordinates().GetDataAsMultiplexer().PrepareForInput(device, token),
    this->PointIds.PrepareForInput(device, token),
    this->CellLower.PrepareForInput(device, token),
    this->CellUpper.PrepareForInput(device, token));
}

} // viskores::cont
} // viskores
