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
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/filter/contour/ContourFlyingEdges.h>
#include <viskores/filter/contour/worklet/ContourFlyingEdges.h>

namespace viskores
{
namespace filter
{

using SupportedTypes =
  viskores::List<viskores::UInt8, viskores::Int8, viskores::Float32, viskores::Float64>;

namespace contour
{
//-----------------------------------------------------------------------------
viskores::cont::DataSet ContourFlyingEdges::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  viskores::worklet::ContourFlyingEdges worklet;
  worklet.SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());

  if (!this->GetFieldFromDataSet(inDataSet).IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->IsoValues.empty())
  {
    throw viskores::cont::ErrorFilterExecution("No iso-values provided.");
  }

  viskores::cont::UnknownCellSet inCellSet = inDataSet.GetCellSet();
  const viskores::cont::CoordinateSystem& inCoords =
    inDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  if (!inCellSet.template IsType<viskores::cont::CellSetStructured<3>>())
  {
    throw viskores::cont::ErrorFilterExecution("This filter is only available for 3-Dimensional "
                                               "Structured Cell Sets");
  }

  // Get the CellSet's known dynamic type
  const viskores::cont::CellSetStructured<3>& inputCells =
    inDataSet.GetCellSet().AsCellSet<viskores::cont::CellSetStructured<3>>();

  using Vec3HandleType = viskores::cont::ArrayHandle<viskores::Vec3f>;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  viskores::cont::CellSetSingleType<> outputCells;

  auto resolveFieldType = [&](const auto& concrete)
  {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    using IVType = std::conditional_t<(sizeof(T) > 4), viskores::Float64, viskores::FloatDefault>;
    std::vector<IVType> ivalues(this->IsoValues.size());
    for (std::size_t i = 0; i < ivalues.size(); ++i)
    {
      ivalues[i] = static_cast<IVType>(this->IsoValues[i]);
    }

    if (this->GenerateNormals && !this->GetComputeFastNormals())
    {
      outputCells = worklet.Run(ivalues, inputCells, inCoords, concrete, vertices, normals);
    }
    else
    {
      outputCells = worklet.Run(ivalues, inputCells, inCoords, concrete, vertices);
    }
  };

  this->GetFieldFromDataSet(inDataSet)
    .GetData()
    .CastAndCallForTypesWithFloatFallback<SupportedTypes, VISKORES_DEFAULT_STORAGE_LIST>(
      resolveFieldType);

  auto mapper = [&](auto& result, const auto& f) { this->DoMapField(result, f, worklet); };
  viskores::cont::DataSet output = this->CreateResultCoordinateSystem(
    inDataSet, outputCells, inCoords.GetName(), vertices, mapper);

  this->ExecuteGenerateNormals(output, normals);
  this->ExecuteAddInterpolationEdgeIds(output, worklet);

  return output;
}
} // namespace contour
} // namespace filter
} // namespace viskores
