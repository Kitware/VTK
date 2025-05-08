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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/density_estimate/ContinuousScatterPlot.h>
#include <viskores/filter/density_estimate/worklet/ContinuousScatterPlot.h>
#include <viskores/filter/geometry_refinement/Tetrahedralize.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{

VISKORES_CONT viskores::cont::DataSet ContinuousScatterPlot::DoExecute(
  const viskores::cont::DataSet& input)
{
  // This algorithm only operate on tetra cells, we need to apply the tetrahedralize filter first.
  auto tetrahedralizeFilter = viskores::filter::geometry_refinement::Tetrahedralize();
  auto tetraInput = tetrahedralizeFilter.Execute(input);
  viskores::cont::CellSetSingleType<> tetraCellSet;
  tetraInput.GetCellSet().AsCellSet(tetraCellSet);

  viskores::cont::Field scalarField1 = input.GetField(GetActiveFieldName(0));
  viskores::cont::Field scalarField2 = input.GetField(GetActiveFieldName(1));

  if (!(scalarField1.IsPointField() && scalarField2.IsPointField()))
  {
    throw viskores::cont::ErrorFilterExecution("Point fields expected.");
  }

  const auto& coords = input.GetCoordinateSystem().GetDataAsMultiplexer();
  viskores::cont::CoordinateSystem activeCoordSystem = input.GetCoordinateSystem();

  viskores::cont::DataSet scatterplotDataSet;
  viskores::worklet::ContinuousScatterPlot worklet;

  auto resolveFieldType = [&](const auto& resolvedScalar)
  {
    using FieldType = typename std::decay_t<decltype(resolvedScalar)>::ValueType;

    viskores::cont::CellSetSingleType<> scatterplotCellSet;
    viskores::cont::ArrayHandle<FieldType> density;
    viskores::cont::ArrayHandle<viskores::Vec<FieldType, 3>> newCoords;

    // Both fields need to resolve to the same type in order to perform calculations
    viskores::cont::ArrayHandle<FieldType> resolvedScalar2;
    viskores::cont::ArrayCopyShallowIfPossible(scalarField2.GetData(), resolvedScalar2);

    worklet.Run(tetraCellSet,
                coords,
                newCoords,
                density,
                resolvedScalar,
                resolvedScalar2,
                scatterplotCellSet);

    // Populate the new dataset representing the continuous scatterplot
    // Using the density field and coordinates calculated by the worklet
    activeCoordSystem = viskores::cont::CoordinateSystem(activeCoordSystem.GetName(), newCoords);
    scatterplotDataSet.AddCoordinateSystem(activeCoordSystem);
    scatterplotDataSet.SetCellSet(scatterplotCellSet);
    scatterplotDataSet.AddPointField(this->GetOutputFieldName(), density);
  };

  this->CastAndCallScalarField(scalarField1.GetData(), resolveFieldType);

  return scatterplotDataSet;
}

} // namespace density_estimate
} // namespace filter
} // namespace viskores
