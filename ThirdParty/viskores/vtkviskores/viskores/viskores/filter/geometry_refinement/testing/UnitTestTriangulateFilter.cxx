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

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/geometry_refinement/Triangulate.h>

using viskores::cont::testing::MakeTestDataSet;

namespace
{

class TestingTriangulate
{
public:
  void TestStructured() const
  {
    std::cout << "Testing triangulate structured" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    viskores::filter::geometry_refinement::Triangulate triangulate;
    triangulate.SetFieldsToPass({ "pointvar", "cellvar" });
    viskores::cont::DataSet output = triangulate.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 32), "Wrong result for Triangulate");
    VISKORES_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 25),
                         "Wrong number of points for Triangulate");

    viskores::cont::ArrayHandle<viskores::Float32> outData =
      output.GetField("cellvar")
        .GetData()
        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();

    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(2) == 1.f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(3) == 1.f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(30) == 15.f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(31) == 15.f, "Wrong cell field data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing triangulate explicit" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make2DExplicitDataSet0();
    viskores::filter::geometry_refinement::Triangulate triangulate;
    triangulate.SetFieldsToPass({ "pointvar", "cellvar" });
    viskores::cont::DataSet output = triangulate.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 14), "Wrong result for Triangulate");
    VISKORES_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 16),
                         "Wrong number of points for Triangulate");

    viskores::cont::ArrayHandle<viskores::Float32> outData =
      output.GetField("cellvar")
        .GetData()
        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();

    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(1) == 1.f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(2) == 1.f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(5) == 3.f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(6) == 3.f, "Wrong cell field data");
  }

  void TestCellSetSingleTypeTriangle() const
  {
    viskores::cont::DataSet dataset;
    viskores::cont::CellSetSingleType<> cellSet;

    auto connectivity = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 1, 2, 3 });
    cellSet.Fill(4, viskores::CELL_SHAPE_TRIANGLE, 3, connectivity);

    dataset.SetCellSet(cellSet);

    viskores::filter::geometry_refinement::Triangulate triangulate;
    viskores::cont::DataSet output = triangulate.Execute(dataset);

    VISKORES_TEST_ASSERT(dataset.GetCellSet().GetCellSetBase() ==
                           output.GetCellSet().GetCellSetBase(),
                         "Pointer to the CellSetSingleType has changed.");
  }

  void TestCellSetExplicitTriangle() const
  {
    std::vector<viskores::Vec3f_32> coords{ viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                            viskores::Vec3f_32(2.0f, 0.0f, 0.0f),
                                            viskores::Vec3f_32(2.0f, 4.0f, 0.0f),
                                            viskores::Vec3f_32(0.0f, 4.0f, 0.0f) };
    std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TRIANGLE,
                                         viskores::CELL_SHAPE_TRIANGLE };
    std::vector<viskores::IdComponent> indices{ 3, 3 };
    std::vector<viskores::Id> connectivity{ 0, 1, 2, 1, 2, 3 };

    viskores::cont::DataSetBuilderExplicit dsb;
    viskores::cont::DataSet dataset = dsb.Create(coords, shapes, indices, connectivity);

    viskores::filter::geometry_refinement::Triangulate triangulate;
    viskores::cont::DataSet output = triangulate.Execute(dataset);
    viskores::cont::UnknownCellSet outputCellSet = output.GetCellSet();

    VISKORES_TEST_ASSERT(outputCellSet.IsType<viskores::cont::CellSetSingleType<>>(),
                         "Output CellSet is not CellSetSingleType");
    VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 2, "Wrong number of cells");
    VISKORES_TEST_ASSERT(outputCellSet.GetCellShape(0) == viskores::CellShapeTagTriangle::Id,
                         "Cell is not triangular");
    VISKORES_TEST_ASSERT(outputCellSet.GetCellShape(1) == viskores::CellShapeTagTriangle::Id,
                         "Cell is not triangular");
  }

  void operator()() const
  {
    this->TestStructured();
    this->TestExplicit();
    this->TestCellSetSingleTypeTriangle();
    this->TestCellSetExplicitTriangle();
  }
};
}

int UnitTestTriangulateFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestingTriangulate(), argc, argv);
}
