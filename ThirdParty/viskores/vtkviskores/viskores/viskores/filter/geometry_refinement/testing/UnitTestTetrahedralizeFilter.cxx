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

#include <viskores/filter/geometry_refinement/Tetrahedralize.h>

using viskores::cont::testing::MakeTestDataSet;

namespace
{

class TestingTetrahedralize
{
public:
  void TestStructured() const
  {
    std::cout << "Testing tetrahedralize structured" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();

    viskores::filter::geometry_refinement::Tetrahedralize tetrahedralize;
    tetrahedralize.SetFieldsToPass({ "pointvar", "cellvar" });

    viskores::cont::DataSet output = tetrahedralize.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 20),
                         "Wrong result for Tetrahedralize");
    VISKORES_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 18),
                         "Wrong number of points for Tetrahedralize");

    viskores::cont::ArrayHandle<viskores::Float32> outData =
      output.GetField("cellvar")
        .GetData()
        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();

    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(5) == 100.2f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(6) == 100.2f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(7) == 100.2f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(8) == 100.2f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(9) == 100.2f, "Wrong cell field data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing tetrahedralize explicit" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    viskores::filter::geometry_refinement::Tetrahedralize tetrahedralize;
    tetrahedralize.SetFieldsToPass({ "pointvar", "cellvar" });

    viskores::cont::DataSet output = tetrahedralize.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 11),
                         "Wrong result for Tetrahedralize");
    VISKORES_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 11),
                         "Wrong number of points for Tetrahedralize");

    viskores::cont::ArrayHandle<viskores::Float32> outData =
      output.GetField("cellvar")
        .GetData()
        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();

    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(5) == 110.f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(6) == 110.f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(8) == 130.5f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(9) == 130.5f, "Wrong cell field data");
    VISKORES_TEST_ASSERT(outData.ReadPortal().Get(10) == 130.5f, "Wrong cell field data");
  }

  void TestCellSetSingleTypeTetra() const
  {
    viskores::cont::DataSet dataset;
    viskores::cont::CellSetSingleType<> cellSet;

    auto connectivity = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 3, 3, 2, 1, 4 });
    cellSet.Fill(5, viskores::CELL_SHAPE_TETRA, 4, connectivity);

    dataset.SetCellSet(cellSet);

    viskores::filter::geometry_refinement::Tetrahedralize tetrahedralize;
    viskores::cont::DataSet output = tetrahedralize.Execute(dataset);

    VISKORES_TEST_ASSERT(dataset.GetCellSet().GetCellSetBase() ==
                           output.GetCellSet().GetCellSetBase(),
                         "Pointer to the CellSetSingleType has changed.");
  }

  void TestCellSetExplicitTetra() const
  {
    std::vector<viskores::Vec3f_32> coords{
      viskores::Vec3f_32(0.0f, 0.0f, 0.0f), viskores::Vec3f_32(2.0f, 0.0f, 0.0f),
      viskores::Vec3f_32(2.0f, 4.0f, 0.0f), viskores::Vec3f_32(0.0f, 4.0f, 0.0f),
      viskores::Vec3f_32(1.0f, 0.0f, 3.0f),
    };
    std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TETRA, viskores::CELL_SHAPE_TETRA };
    std::vector<viskores::IdComponent> indices{ 4, 4 };
    std::vector<viskores::Id> connectivity{ 0, 1, 2, 3, 1, 2, 3, 4 };

    viskores::cont::DataSetBuilderExplicit dsb;
    viskores::cont::DataSet dataset = dsb.Create(coords, shapes, indices, connectivity);

    viskores::filter::geometry_refinement::Tetrahedralize tetrahedralize;
    viskores::cont::DataSet output = tetrahedralize.Execute(dataset);
    viskores::cont::UnknownCellSet outputCellSet = output.GetCellSet();

    VISKORES_TEST_ASSERT(outputCellSet.IsType<viskores::cont::CellSetSingleType<>>(),
                         "Output CellSet is not CellSetSingleType");
    VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 2, "Wrong number of cells");
    VISKORES_TEST_ASSERT(outputCellSet.GetCellShape(0) == viskores::CellShapeTagTetra::Id,
                         "Cell is not tetra");
    VISKORES_TEST_ASSERT(outputCellSet.GetCellShape(1) == viskores::CellShapeTagTetra::Id,
                         "Cell is not tetra");
  }

  void operator()() const
  {
    this->TestStructured();
    this->TestExplicit();
    this->TestCellSetSingleTypeTetra();
    this->TestCellSetExplicitTetra();
  }
};
}

int UnitTestTetrahedralizeFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestingTetrahedralize(), argc, argv);
}
