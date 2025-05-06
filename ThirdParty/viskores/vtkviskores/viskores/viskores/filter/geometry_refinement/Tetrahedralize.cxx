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

#include <viskores/cont/Algorithm.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/geometry_refinement/Tetrahedralize.h>
#include <viskores/filter/geometry_refinement/worklet/Tetrahedralize.h>

namespace
{
VISKORES_CONT bool DoMapField(viskores::cont::DataSet& result,
                              const viskores::cont::Field& field,
                              const viskores::worklet::Tetrahedralize& worklet)
{
  if (field.IsPointField())
  {
    // point data is copied as is because it was not collapsed
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    // cell data must be scattered to the cells created per input cell
    viskores::cont::ArrayHandle<viskores::Id> permutation =
      worklet.GetOutCellScatter().GetOutputToInputMap();
    return viskores::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}

struct IsShapeTetra
{
  VISKORES_EXEC_CONT
  bool operator()(viskores::UInt8 shape) const { return shape == viskores::CELL_SHAPE_TETRA; }
};

struct BinaryAnd
{
  VISKORES_EXEC_CONT
  bool operator()(bool u, bool v) const { return u && v; }
};
} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{
VISKORES_CONT viskores::cont::DataSet Tetrahedralize::DoExecute(
  const viskores::cont::DataSet& input)
{
  const viskores::cont::UnknownCellSet& inCellSet = input.GetCellSet();

  // In case we already have a CellSetSingleType of tetras,
  // don't call the worklet and return the input DataSet directly
  if (inCellSet.CanConvert<viskores::cont::CellSetSingleType<>>() &&
      inCellSet.AsCellSet<viskores::cont::CellSetSingleType<>>().GetCellShapeAsId() ==
        viskores::CellShapeTagTetra::Id)
  {
    return input;
  }

  viskores::cont::CellSetSingleType<> outCellSet;
  viskores::cont::DataSet output;

  // Optimization in case we only have tetras in the CellSet
  bool allTetras = false;
  if (inCellSet.CanConvert<viskores::cont::CellSetExplicit<>>())
  {
    viskores::cont::CellSetExplicit<> inCellSetExplicit =
      inCellSet.AsCellSet<viskores::cont::CellSetExplicit<>>();

    auto shapeArray = inCellSetExplicit.GetShapesArray(viskores::TopologyElementTagCell(),
                                                       viskores::TopologyElementTagPoint());
    auto isCellTetraArray = viskores::cont::make_ArrayHandleTransform(shapeArray, IsShapeTetra{});

    allTetras = viskores::cont::Algorithm::Reduce(isCellTetraArray, true, BinaryAnd{});

    if (allTetras)
    {
      // Reuse the input's connectivity array
      outCellSet.Fill(inCellSet.GetNumberOfPoints(),
                      viskores::CellShapeTagTetra::Id,
                      4,
                      inCellSetExplicit.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                             viskores::TopologyElementTagPoint()));

      // Copy all fields from the input
      output = this->CreateResult(input,
                                  outCellSet,
                                  [&](auto& result, const auto& f)
                                  {
                                    result.AddField(f);
                                    return true;
                                  });
    }
  }

  if (!allTetras)
  {
    viskores::worklet::Tetrahedralize worklet;
    viskores::cont::CastAndCall(inCellSet,
                                [&](const auto& concrete) { outCellSet = worklet.Run(concrete); });

    auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
    // create the output dataset (without a CoordinateSystem).
    output = this->CreateResult(input, outCellSet, mapper);
  }

  // We did not change the geometry of the input dataset at all. Just attach coordinate system
  // of input dataset to output dataset.
  for (viskores::IdComponent coordSystemId = 0;
       coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    output.AddCoordinateSystem(input.GetCoordinateSystem(coordSystemId));
  }
  return output;
}
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores
