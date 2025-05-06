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
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <cstdio>
#include <string>
#include <vector>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/ErrorExecution.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/mesh_info/MeshQuality.h>

namespace
{

//TODO: This should be a general facility.
const char* GetCellShapeName(viskores::UInt8 shape)
{
  switch (shape)
  {
    viskoresGenericCellShapeMacro(return viskores::GetCellShapeName(CellShapeTag{}));
    default:
      return "Unknown";
  }
}

//Adapted from viskores/cont/testing/MakeTestDataSet.h
//Modified the content of the MakeExplicitDataSetZoo() function
inline viskores::cont::DataSet MakeExplicitDataSet()
{
  viskores::cont::DataSet dataSet;

  using CoordType = viskores::Vec3f_64;

  std::vector<CoordType> coords = {
    { 0, 0, 0 },  { 3, 0, 0 },  { 2, 2, 0 },  { 4, 0, 0 },  { 7, 0, 0 },  { 7, 2, 0 },
    { 6, 2, 0 },  { 8, 0, 0 },  { 11, 0, 0 }, { 9, 2, 0 },  { 9, 1, 1 },  { 9, 3, 0 },
    { 11, 3, 0 }, { 11, 5, 0 }, { 9, 5, 0 },  { 10, 4, 1 }, { 12, 0, 0 }, { 12, 3, 0 },
    { 12, 2, 1 }, { 15, 0, 0 }, { 15, 3, 0 }, { 15, 1, 1 }, { 16, 0, 0 }, { 18, 0, 0 },
    { 18, 2, 0 }, { 16, 2, 0 }, { 17, 1, 1 }, { 19, 1, 1 }, { 19, 3, 1 }, { 17, 3, 1 }
  };

  std::vector<viskores::UInt8> shapes;
  std::vector<viskores::IdComponent> numindices;
  std::vector<viskores::Id> conn;

  //Construct the shapes/cells of the dataset
  //This is a zoo of points, lines, polygons, and polyhedra
  shapes.push_back(viskores::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);

  shapes.push_back(viskores::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(3);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);

  shapes.push_back(viskores::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(7);
  conn.push_back(8);
  conn.push_back(9);
  conn.push_back(10);

  shapes.push_back(viskores::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(11);
  conn.push_back(12);
  conn.push_back(13);
  conn.push_back(14);
  conn.push_back(15);

  shapes.push_back(viskores::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(16);
  conn.push_back(17);
  conn.push_back(18);
  conn.push_back(19);
  conn.push_back(20);
  conn.push_back(21);

  shapes.push_back(viskores::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(22);
  conn.push_back(23);
  conn.push_back(24);
  conn.push_back(25);
  conn.push_back(26);
  conn.push_back(27);
  conn.push_back(28);
  conn.push_back(29);

  dataSet =
    viskores::cont::DataSetBuilderExplicit::Create(coords, shapes, numindices, conn, "coordinates");

  return dataSet;
}

inline viskores::cont::DataSet MakeSingleTypeDataSet()
{
  using CoordType = viskores::Vec3f_64;

  viskores::cont::ArrayHandle<CoordType> coords =
    viskores::cont::make_ArrayHandle<viskores::Vec3f_64>(
      { { 0, 0, 0 }, { 3, 0, 0 }, { 2, 2, 0 }, { 4, 0, 0 } });

  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.PrepareToAddCells(2, 3 * 2);
  cellSet.AddCell(viskores::CELL_SHAPE_TRIANGLE, 3, viskores::Id3(0, 1, 2));
  cellSet.AddCell(viskores::CELL_SHAPE_TRIANGLE, 3, viskores::Id3(2, 1, 3));
  cellSet.CompleteAddingCells(coords.GetNumberOfValues());

  viskores::cont::DataSet dataset;
  dataset.SetCellSet(cellSet);
  dataset.AddCoordinateSystem(viskores::cont::CoordinateSystem("coords", coords));
  return dataset;
}

bool TestMeshQualityFilter(const viskores::cont::DataSet& input,
                           const std::vector<viskores::FloatDefault>& expectedVals,
                           const std::string& outputname,
                           viskores::filter::mesh_info::MeshQuality& filter)
{
  viskores::cont::DataSet output;
  try
  {
    output = filter.Execute(input);
  }
  catch (viskores::cont::ErrorExecution& error)
  {
    std::cout << "Metric '" << outputname << "' threw execution exception " << error.GetMessage()
              << std::endl;
    return true;
  }

  //Test the computed metric values (for all cells) and expected metric
  //values for equality.
  viskores::cont::ArrayHandle<viskores::Float64> values;
  output.GetField(outputname).GetData().AsArrayHandle(values);
  auto portal1 = values.ReadPortal();
  if (portal1.GetNumberOfValues() != (viskores::Id)expectedVals.size())
  {
    std::cout << "Number of expected values for " << outputname << " does not match.\n";
    return true;
  }

  bool anyFailures = false;
  for (size_t i = 0; i < expectedVals.size(); i++)
  {
    viskores::Id id = (viskores::Id)i;
    if (!test_equal(portal1.Get(id), expectedVals[i]))
    {
      std::cout << "Metric `" << outputname << "` for cell " << i << " (type `"
                << GetCellShapeName(input.GetCellSet().GetCellSetBase()->GetCellShape(id))
                << "` does not match. Expected " << expectedVals[i] << " and got "
                << portal1.Get(id) << "\n";
      anyFailures = true;
    }
  }
  return anyFailures;
}

int TestMeshQuality()
{
  using FloatVec = std::vector<viskores::FloatDefault>;

  //Test variables
  viskores::cont::DataSet explicitInput = MakeExplicitDataSet();
  viskores::cont::DataSet singleTypeInput = MakeSingleTypeDataSet();

  int numFailures = 0;
  bool testFailed = false;

  std::vector<FloatVec> expectedValues;
  std::vector<viskores::filter::mesh_info::CellMetric> metrics;
  std::vector<std::string> metricName;
  std::vector<viskores::cont::DataSet> inputs;

  expectedValues.push_back(FloatVec{ 0, 0, 1, 1.333333333f, 4, 4 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Volume);
  metricName.emplace_back("volume");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 3, 4, 0, 0, 0, 0 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Area);
  metricName.emplace_back("area");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 3, 1 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Area);
  metricName.emplace_back("area");
  inputs.push_back(singleTypeInput);

  expectedValues.push_back(FloatVec{ 1.164010f, 1.118034f, 1.648938f, 0, 0, 1.1547f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::AspectRatio);
  metricName.emplace_back("aspectRatio");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 1.164010f, 2.47582f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::AspectRatio);
  metricName.emplace_back("aspectRatio");
  inputs.push_back(singleTypeInput);

  expectedValues.push_back(FloatVec{ 0, 0, 1.52012f, 0, 0, 0 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::AspectGamma);
  metricName.emplace_back("aspectGamma");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 1.058475f, 2.25f, 1.354007f, 0, 0, 1.563472f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Condition);
  metricName.emplace_back("condition");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 1.058475f, 2.02073f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Condition);
  metricName.emplace_back("condition");
  inputs.push_back(singleTypeInput);

  expectedValues.push_back(FloatVec{ 45, 45, -1, -1, -1, -1 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::MinAngle);
  metricName.emplace_back("minAngle");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 45, 18.4348f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::MinAngle);
  metricName.emplace_back("minAngle");
  inputs.push_back(singleTypeInput);

  expectedValues.push_back(FloatVec{ 71.56505f, 135, -1, -1, -1, -1 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::MaxAngle);
  metricName.emplace_back("maxAngle");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 71.56505f, 116.565f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::MaxAngle);
  metricName.emplace_back("maxAngle");
  inputs.push_back(singleTypeInput);

  expectedValues.push_back(FloatVec{ -1, -1, -1, -1, -1, 1.73205f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::MinDiagonal);
  metricName.emplace_back("minDiagonal");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ -1, -1, -1, -1, -1, 4.3589f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::MaxDiagonal);
  metricName.emplace_back("maxDiagonal");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 0, 2, 6, 0, 0, 4 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Jacobian);
  metricName.emplace_back("jacobian");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 0.816497f, 0.707107f, 0.408248f, -2, -2, 0.57735f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::ScaledJacobian);
  metricName.emplace_back("scaledJacobian");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 0.816497f, 0.365148f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::ScaledJacobian);
  metricName.emplace_back("scaledJacobian");
  inputs.push_back(singleTypeInput);

  expectedValues.push_back(FloatVec{ -1, 8.125f, -1, -1, -1, 2.62484f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Oddy);
  metricName.emplace_back("oddy");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ -1, 0.620174f, -1, -1, -1, 0.397360f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::DiagonalRatio);
  metricName.emplace_back("diagonalRatio");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 0.944755f, 0.444444f, 0.756394f, -1, -1, 0.68723f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Shape);
  metricName.emplace_back("shape");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 0.944755f, 0.494872f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Shape);
  metricName.emplace_back("shape");
  inputs.push_back(singleTypeInput);

  expectedValues.push_back(FloatVec{ -1, 0.707107f, -1, -1, -1, 0.57735f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Shear);
  metricName.emplace_back("shear");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ -1, 0.447214f, -1, -1, -1, 0.57735f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Skew);
  metricName.emplace_back("skew");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ -1, (float)0.392232, -1, -1, -1, (float)0.688247 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Stretch);
  metricName.emplace_back("stretch");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ -1, 0.5, -1, -1, -1, 0 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Taper);
  metricName.emplace_back("taper");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ -1, 1, -1, -1, -1, -1 });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Warpage);
  metricName.emplace_back("warpage");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ -1, -1, -1, -1, -1, 0.707107f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::Dimension);
  metricName.emplace_back("dimension");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 0.151235f, 0.085069f, 0.337149f, -1, -1, 0.185378f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::RelativeSizeSquared);
  metricName.emplace_back("relativeSizeSquared");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 0.444444f, 0.25f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::RelativeSizeSquared);
  metricName.emplace_back("relativeSizeSquared");
  inputs.push_back(singleTypeInput);

  expectedValues.push_back(FloatVec{ 0.142880f, 0.037809f, 0.255017f, -1, -1, 0.127397f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::ShapeAndSize);
  metricName.emplace_back("shapeAndSize");
  inputs.push_back(explicitInput);

  expectedValues.push_back(FloatVec{ 0.419891f, 0.123718f });
  metrics.push_back(viskores::filter::mesh_info::CellMetric::ShapeAndSize);
  metricName.emplace_back("shapeAndSize");
  inputs.push_back(singleTypeInput);

  auto numTests = metrics.size();
  for (size_t i = 0; i < numTests; i++)
  {
    printf("Testing metric %s\n", metricName[i].c_str());
    viskores::filter::mesh_info::MeshQuality filter;
    filter.SetMetric(metrics[i]);
    testFailed = TestMeshQualityFilter(inputs[i], expectedValues[i], metricName[i], filter);
    if (testFailed)
    {
      numFailures++;
      printf("\ttest \"%s\" failed\n", metricName[i].c_str());
    }
    else
      printf("\t... passed\n");
  }

  if (numFailures > 0)
  {
    printf("Number of failed metrics is %d\n", numFailures);
    bool see_previous_messages = false; // this variable name plays well with macro
    VISKORES_TEST_ASSERT(see_previous_messages, "Failure occurred during test");
  }
  return 0;
}

} // anonymous namespace

int UnitTestMeshQualityFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestMeshQuality, argc, argv);
}
