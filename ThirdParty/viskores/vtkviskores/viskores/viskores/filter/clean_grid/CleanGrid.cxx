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
#include <viskores/filter/MapFieldMergeAverage.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/clean_grid/worklet/PointMerge.h>
#include <viskores/filter/clean_grid/worklet/RemoveDegenerateCells.h>
#include <viskores/filter/clean_grid/worklet/RemoveUnusedPoints.h>

namespace viskores
{
namespace filter
{
namespace clean_grid
{
struct SharedStates
{
  viskores::worklet::RemoveUnusedPoints PointCompactor;
  viskores::worklet::RemoveDegenerateCells CellCompactor;
  viskores::worklet::PointMerge PointMerger;
};
}
}
}

// New Filter Design: DoMapField is now a free function in an anonymous namespace. It should be
// considered as a convenience/extension to the lambda passed to CreateResult.
// Being a free function discourages the developer to "pass" mutable states from DoExecute phase
// to DoMapField phase via data member. However, there is nothing to prevent developer doing
// stupid thing to circumvent the protection. One example here is that the developer could
// always pass a mutable reference/pointer to the filter instance and thus pass mutable state
// across the DoExecute and DoMapField boundary. We need to explicitly discourage developer
// trying to do such a thing in the manual.
namespace
{
bool DoMapField(viskores::cont::DataSet& result,
                const viskores::cont::Field& field,
                const viskores::filter::clean_grid::CleanGrid& self,
                viskores::filter::clean_grid::SharedStates& worklets)
{
  if (field.IsPointField() && (self.GetCompactPointFields() || self.GetMergePoints()))
  {
    viskores::cont::Field compactedField;
    if (self.GetCompactPointFields())
    {
      bool success = viskores::filter::MapFieldPermutation(
        field, worklets.PointCompactor.GetPermutationArray(), compactedField);
      if (!success)
      {
        return false;
      }
    }
    else
    {
      compactedField = field;
    }
    if (self.GetMergePoints())
    {
      return viskores::filter::MapFieldMergeAverage(
        compactedField, worklets.PointMerger.GetMergeKeys(), result);
    }
    else
    {
      result.AddField(compactedField);
      return true;
    }
  }
  else if (field.IsCellField() && self.GetRemoveDegenerateCells())
  {
    return viskores::filter::MapFieldPermutation(
      field, worklets.CellCompactor.GetValidCellIds(), result);
  }
  else
  {
    result.AddField(field);
    return true;
  }
}
} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace clean_grid
{
//-----------------------------------------------------------------------------
viskores::cont::DataSet CleanGrid::GenerateOutput(const viskores::cont::DataSet& inData,
                                                  viskores::cont::CellSetExplicit<>& outputCellSet,
                                                  clean_grid::SharedStates& worklets)
{
  using VecId = std::size_t;
  const auto activeCoordIndex = static_cast<VecId>(this->GetActiveCoordinateSystemIndex());

  // Start with a shallow copy of the coordinate systems
  viskores::cont::CoordinateSystem activeCoordSystem = inData.GetCoordinateSystem(activeCoordIndex);

  // Optionally adjust the cell set indices to remove all unused points
  if (this->GetCompactPointFields())
  {
    worklets.PointCompactor.FindPointsStart();
    worklets.PointCompactor.FindPoints(outputCellSet);
    worklets.PointCompactor.FindPointsEnd();

    outputCellSet = worklets.PointCompactor.MapCellSet(outputCellSet);

    viskores::filter::MapFieldPermutation(
      activeCoordSystem, worklets.PointCompactor.GetPermutationArray(), activeCoordSystem);
  }

  // Optionally find and merge coincident points
  if (this->GetMergePoints())
  {
    viskores::Bounds bounds = activeCoordSystem.GetBounds();

    viskores::Float64 delta = this->GetTolerance();
    if (!this->GetToleranceIsAbsolute())
    {
      delta *= viskores::Magnitude(
        viskores::make_Vec(bounds.X.Length(), bounds.Y.Length(), bounds.Z.Length()));
    }

    auto coordArray = activeCoordSystem.GetData();
    worklets.PointMerger.Run(delta, this->GetFastMerge(), bounds, coordArray);
    activeCoordSystem = viskores::cont::CoordinateSystem(activeCoordSystem.GetName(), coordArray);

    outputCellSet = worklets.PointMerger.MapCellSet(outputCellSet);
  }

  // Optionally remove degenerate cells
  if (this->GetRemoveDegenerateCells())
  {
    outputCellSet = worklets.CellCompactor.Run(outputCellSet);
  }

  // New Filter Design: We pass the actions needed to be done as a lambda to the generic
  // CreateResult method. CreateResult now acts as thrust::transform_if on the
  // Fields. Shared mutable state is captured by the lambda. We could also put all the logic
  // of field mapping in the lambda. However, it is cleaner to put it in the filter specific
  // implementation of DoMapField which takes mutable state as an extra parameter.
  //
  // For filters that do not need to do interpolation for mapping fields, we provide an overload
  // that does not take the extra arguments and just AddField.
  auto mapper = [&](auto& outDataSet, const auto& f)
  { DoMapField(outDataSet, f, *this, worklets); };
  return this->CreateResultCoordinateSystem(inData, outputCellSet, activeCoordSystem, mapper);
}

viskores::cont::DataSet CleanGrid::DoExecute(const viskores::cont::DataSet& inData)
{
  // New Filter Design: mutable states that was a data member of the filter is now a local
  // variable. Each invocation of DoExecute() in the std::async will have a copy of worklets
  // thus making it thread-safe.
  clean_grid::SharedStates worklets;

  using CellSetType = viskores::cont::CellSetExplicit<>;

  CellSetType outputCellSet;
  viskores::cont::UnknownCellSet inCellSet = inData.GetCellSet();
  if (inCellSet.IsType<CellSetType>())
  {
    // Is expected type, do a shallow copy
    outputCellSet = inCellSet.AsCellSet<CellSetType>();
  }
  else
  { // Clean the grid
    viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;

    this->Invoke(worklet::CellDeepCopy::CountCellPoints{}, inCellSet, numIndices);

    viskores::cont::ArrayHandle<viskores::UInt8> shapes;
    viskores::cont::ArrayHandle<viskores::Id> offsets;
    viskores::Id connectivitySize;
    viskores::cont::ConvertNumComponentsToOffsets(numIndices, offsets, connectivitySize);
    numIndices.ReleaseResourcesExecution();

    viskores::cont::ArrayHandle<viskores::Id> connectivity;
    connectivity.Allocate(connectivitySize);

    this->Invoke(worklet::CellDeepCopy::PassCellStructure{},
                 inCellSet,
                 shapes,
                 viskores::cont::make_ArrayHandleGroupVecVariable(connectivity, offsets));

    outputCellSet.Fill(inCellSet.GetNumberOfPoints(), shapes, connectivity, offsets);
  }

  // New Filter Design: The share, mutable state is pass to other methods via parameter, not as
  // a data member.
  return this->GenerateOutput(inData, outputCellSet, worklets);
}
} //namespace clean_grid
} //namespace filter
} //namespace viskores
