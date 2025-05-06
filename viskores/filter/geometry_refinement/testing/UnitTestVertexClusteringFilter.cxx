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

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/geometry_refinement/VertexClustering.h>

using viskores::cont::testing::MakeTestDataSet;

namespace
{
}

void TestVertexClustering()
{
  viskores::cont::testing::MakeTestDataSet maker;
  viskores::cont::DataSet dataSet = maker.Make3DExplicitDataSetCowNose();

  viskores::filter::geometry_refinement::VertexClustering clustering;

  clustering.SetNumberOfDivisions(viskores::Id3(3, 3, 3));
  clustering.SetFieldsToPass({ "pointvar", "cellvar" });
  viskores::cont::DataSet output = clustering.Execute(dataSet);
  VISKORES_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                       "Number of output coordinate systems mismatch");

  using FieldArrayType = viskores::cont::ArrayHandle<viskores::Float32>;
  FieldArrayType pointvar =
    output.GetPointField("pointvar").GetData().AsArrayHandle<FieldArrayType>();
  FieldArrayType cellvar = output.GetCellField("cellvar").GetData().AsArrayHandle<FieldArrayType>();

  // test
  const viskores::Id output_points = 7;
  viskores::Float64 output_point[output_points][3] = {
    { 0.0174716, 0.0501928, 0.0930275 }, { 0.0307091, 0.1521420, 0.05392490 },
    { 0.0174172, 0.1371240, 0.1245530 }, { 0.0480879, 0.1518740, 0.10733400 },
    { 0.0180085, 0.2043600, 0.1453160 }, { -.000129414, 0.00247137, 0.17656100 },
    { 0.0108188, 0.1527740, 0.1679140 }
  };

  viskores::Float32 output_pointvar[output_points] = { 28.f, 19.f, 25.f, 15.f, 16.f, 21.f, 30.f };
  viskores::Float32 output_cellvar[] = { 145.f, 134.f, 138.f, 140.f, 149.f, 144.f };

  {
    using CellSetType = viskores::cont::CellSetSingleType<>;
    CellSetType cellSet;
    output.GetCellSet().AsCellSet(cellSet);
    auto cellArray = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                  viskores::TopologyElementTagPoint());
    std::cerr << "output_pointIds = " << cellArray.GetNumberOfValues() << "\n";
    std::cerr << "output_pointId[] = ";
    viskores::cont::printSummary_ArrayHandle(cellArray, std::cerr, true);
  }

  {
    auto pointArray = output.GetCoordinateSystem(0).GetDataAsMultiplexer();
    std::cerr << "output_points = " << pointArray.GetNumberOfValues() << "\n";
    std::cerr << "output_point[] = ";
    viskores::cont::printSummary_ArrayHandle(pointArray, std::cerr, true);
  }

  viskores::cont::printSummary_ArrayHandle(pointvar, std::cerr, true);
  viskores::cont::printSummary_ArrayHandle(cellvar, std::cerr, true);

  using PointType = viskores::Vec3f_64;
  auto pointArray = output.GetCoordinateSystem(0).GetDataAsMultiplexer();
  VISKORES_TEST_ASSERT(pointArray.GetNumberOfValues() == output_points,
                       "Number of output points mismatch");
  auto pointArrayPortal = pointArray.ReadPortal();
  for (viskores::Id i = 0; i < pointArray.GetNumberOfValues(); ++i)
  {
    const PointType& p1 = pointArrayPortal.Get(i);
    PointType p2 = viskores::make_Vec(output_point[i][0], output_point[i][1], output_point[i][2]);
    VISKORES_TEST_ASSERT(test_equal(p1, p2), "Point Array mismatch");
  }

  {
    auto portal = pointvar.ReadPortal();
    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == output_points, "Point field size mismatch.");
    for (viskores::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      VISKORES_TEST_ASSERT(test_equal(portal.Get(i), output_pointvar[i]), "Point field mismatch.");
    }
  }

  {
    auto portal = cellvar.ReadPortal();
    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == 6, "Cell field size mismatch.");
    for (viskores::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      VISKORES_TEST_ASSERT(test_equal(portal.Get(i), output_cellvar[i]), "Cell field mismatch.");
    }
  }
}

int UnitTestVertexClusteringFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestVertexClustering, argc, argv);
}
