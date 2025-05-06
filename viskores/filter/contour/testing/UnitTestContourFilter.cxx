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

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/contour/ContourFlyingEdges.h>
#include <viskores/filter/contour/ContourMarchingCells.h>
#include <viskores/filter/field_transform/GenerateIds.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/source/Tangle.h>

namespace
{

class TestContourFilter
{
public:
  template <typename ContourFilterType>
  void TestContourUniformGrid(viskores::IdComponent numPointsNoMergeDuplicate) const
  {
    std::cout << "Testing Contour filter on a uniform grid" << std::endl;

    viskores::source::Tangle tangle;
    tangle.SetCellDimensions({ 4, 4, 4 });
    viskores::filter::field_transform::GenerateIds genIds;
    genIds.SetUseFloat(true);
    genIds.SetGeneratePointIds(false);
    genIds.SetCellFieldName("cellvar");
    viskores::cont::DataSet dataSet = genIds.Execute(tangle.Execute());

    ContourFilterType filter;

    filter.SetGenerateNormals(true);
    filter.SetIsoValue(0, 0.5);
    filter.SetActiveField("tangle");
    filter.SetFieldsToPass(viskores::filter::FieldSelection::Mode::None);

    auto result = filter.Execute(dataSet);
    {
      VISKORES_TEST_ASSERT(result.GetNumberOfCoordinateSystems() == 1,
                           "Wrong number of coordinate systems in the output dataset");
      //since normals is on we have one field
      VISKORES_TEST_ASSERT(result.GetNumberOfFields() == 2,
                           "Wrong number of fields in the output dataset");
    }

    // let's execute with mapping fields.
    filter.SetFieldsToPass({ "tangle", "cellvar" });
    result = filter.Execute(dataSet);
    {
      const bool isMapped = result.HasField("tangle");
      VISKORES_TEST_ASSERT(isMapped, "mapping should pass");

      VISKORES_TEST_ASSERT(result.GetNumberOfFields() == 4,
                           "Wrong number of fields in the output dataset");

      //verify the cellvar result
      viskores::cont::ArrayHandle<viskores::FloatDefault> cellFieldArrayOut;
      result.GetField("cellvar").GetData().AsArrayHandle(cellFieldArrayOut);

      viskores::cont::Algorithm::Sort(cellFieldArrayOut);
      {
        std::vector<viskores::Id> correctcellIdStart = { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 5, 6, 6, 6 };
        std::vector<viskores::Id> correctcellIdEnd = { 57, 57, 58, 58, 58, 59, 59,
                                                       60, 61, 61, 62, 62, 63 };

        auto id_portal = cellFieldArrayOut.ReadPortal();
        for (std::size_t i = 0; i < correctcellIdStart.size(); ++i)
        {
          VISKORES_TEST_ASSERT(id_portal.Get(viskores::Id(i)) == correctcellIdStart[i]);
        }

        viskores::Id index =
          cellFieldArrayOut.GetNumberOfValues() - viskores::Id(correctcellIdEnd.size());
        for (std::size_t i = 0; i < correctcellIdEnd.size(); ++i, ++index)
        {
          VISKORES_TEST_ASSERT(id_portal.Get(index) == correctcellIdEnd[i]);
        }
      }

      viskores::cont::CoordinateSystem coords = result.GetCoordinateSystem();
      viskores::cont::UnknownCellSet cells = result.GetCellSet();

      //verify that the number of points is correct (72)
      //verify that the number of cells is correct (160)
      VISKORES_TEST_ASSERT(coords.GetNumberOfPoints() == 72,
                           "Should have less coordinates than the unmerged version");
      VISKORES_TEST_ASSERT(cells.GetNumberOfCells() == 160, "");
    }

    //Now try with vertex merging disabled.
    filter.SetMergeDuplicatePoints(false);
    filter.SetFieldsToPass(viskores::filter::FieldSelection::Mode::All);
    result = filter.Execute(dataSet);
    {
      viskores::cont::CoordinateSystem coords = result.GetCoordinateSystem();
      VISKORES_TEST_ASSERT(coords.GetNumberOfPoints() == numPointsNoMergeDuplicate,
                           "Shouldn't have less coordinates than the unmerged version");

      //verify that the number of cells is correct (160)
      viskores::cont::UnknownCellSet cells = result.GetCellSet();
      VISKORES_TEST_ASSERT(cells.GetNumberOfCells() == 160, "");
    }
  }

  template <typename ContourFilterType>
  void Test3DUniformDataSet0() const
  {
    viskores::cont::testing::MakeTestDataSet maker;
    viskores::cont::DataSet inputData = maker.Make3DUniformDataSet0();
    std::string fieldName = "pointvar";

    // Defend the test against changes to Make3DUniformDataSet0():
    VISKORES_TEST_ASSERT(inputData.HasField(fieldName));
    viskores::cont::Field pointField = inputData.GetField(fieldName);

    viskores::Range range;
    pointField.GetRange(&range);
    viskores::FloatDefault isovalue = 100.0;
    // Range = [10.1, 180.5]
    VISKORES_TEST_ASSERT(range.Contains(isovalue));

    ContourFilterType filter;
    filter.SetGenerateNormals(false);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(isovalue);
    filter.SetActiveField(fieldName);
    viskores::cont::DataSet outputData = filter.Execute(inputData);
    VISKORES_TEST_ASSERT(outputData.GetNumberOfCells() == 8);
    VISKORES_TEST_ASSERT(outputData.GetNumberOfPoints() == 9);
  }

  template <typename ContourFilterType>
  void TestContourWedges() const
  {
    std::cout << "Testing Contour filter on wedge cells" << std::endl;

    auto pathname = viskores::cont::testing::Testing::DataPath("unstructured/wedge_cells.vtk");
    viskores::io::VTKDataSetReader reader(pathname);

    viskores::cont::DataSet dataSet = reader.ReadDataSet();

    viskores::cont::ArrayHandle<viskores::Float32> fieldArray;
    dataSet.GetPointField("gyroid").GetData().AsArrayHandle(fieldArray);

    ContourFilterType isosurfaceFilter;
    isosurfaceFilter.SetActiveField("gyroid");
    isosurfaceFilter.SetMergeDuplicatePoints(false);
    isosurfaceFilter.SetIsoValue(0.0);

    auto result = isosurfaceFilter.Execute(dataSet);
    VISKORES_TEST_ASSERT(result.GetNumberOfCells() == 52);
  }

  void TestUnsupportedFlyingEdges() const
  {
    viskores::cont::testing::MakeTestDataSet maker;
    viskores::cont::DataSet explicitDataSet = maker.Make3DExplicitDataSet0();

    viskores::filter::contour::ContourFlyingEdges filter;
    filter.SetIsoValue(2.0);
    filter.SetActiveField("pointvar");

    try
    {
      filter.Execute(explicitDataSet);
      VISKORES_TEST_FAIL("Flying Edges filter should not run on explicit datasets");
    }
    catch (viskores::cont::ErrorFilterExecution&)
    {
      std::cout << "Execution successfully aborted" << std::endl;
    }
  }

  template <typename ContourFilterType>
  void TestNonUniformStructured() const
  {
    auto pathname =
      viskores::cont::testing::Testing::DataPath("rectilinear/simple_rectilinear1_ascii.vtk");
    viskores::io::VTKDataSetReader reader(pathname);
    viskores::cont::DataSet rectilinearDataset = reader.ReadDataSet();

    // Single-cell contour
    ContourFilterType filter;
    filter.SetActiveField("var");
    filter.SetIsoValue(2.0);
    viskores::cont::DataSet outputSingleCell = filter.Execute(rectilinearDataset);
    auto coordinates = outputSingleCell.GetCoordinateSystem()
                         .GetData()
                         .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>();

    VISKORES_TEST_ASSERT(outputSingleCell.GetNumberOfPoints() == 3,
                         "Wrong number of points in rectilinear contour");
    VISKORES_TEST_ASSERT(outputSingleCell.GetNumberOfCells() == 1,
                         "Wrong number of cells in rectilinear contour");
    VISKORES_TEST_ASSERT(outputSingleCell.GetCellSet().GetCellShape(0) ==
                           viskores::CELL_SHAPE_TRIANGLE,
                         "Wrong contour cell shape");

    auto expectedCoordinates = viskores::cont::make_ArrayHandle<viskores::Vec3f>(
      { viskores::Vec3f{ 10.0f, -10.0f, 9.66341f },
        viskores::Vec3f{ 9.30578f, -10.0f, 10.0f },
        viskores::Vec3f{ 10.0f, -9.78842f, 10.0f } });
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(coordinates, expectedCoordinates),
                         "Wrong contour coordinates");

    // Generating normals triggers a different worklet for Flying Edges pass 4,
    // But it should not change anything on the contour itself.
    filter.SetGenerateNormals(true);
    viskores::cont::DataSet outputNormals = filter.Execute(rectilinearDataset);
    coordinates = outputNormals.GetCoordinateSystem()
                    .GetData()
                    .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>();
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(coordinates, expectedCoordinates),
                         "Wrong contour coordinates");

    // Full contour
    filter.SetIsoValue(3.0);
    filter.SetGenerateNormals(false);
    viskores::cont::DataSet output = filter.Execute(rectilinearDataset);

    VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 93,
                         "Wrong number of points in rectilinear contour");
    VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 144,
                         "Wrong number of cells in rectilinear contour");
  }

  void TestMixedShapes() const
  {
    auto pathname =
      viskores::cont::testing::Testing::DataPath("unstructured/mixed-cell-shapes.vtk");
    viskores::io::VTKDataSetReader reader(pathname);
    viskores::cont::DataSet input = reader.ReadDataSet();

    // Single-cell contour
    viskores::filter::contour::Contour filter;
    filter.SetActiveField("scalars");
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValues({ 5.5, 9.5, 11.5, 14.5, 17.5, 20.5, 25.5 });

    {
      filter.SetInputCellDimensionToAuto();
      viskores::cont::DataSet output = filter.Execute(input);
      VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 18);
      VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 10);
      VISKORES_TEST_ASSERT(output.GetCellSet().GetCellShape(0) == viskores::CELL_SHAPE_TRIANGLE);
    }

    {
      filter.SetInputCellDimensionToPolyhedra();
      viskores::cont::DataSet output = filter.Execute(input);
      VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 18);
      VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 10);
      VISKORES_TEST_ASSERT(output.GetCellSet().GetCellShape(0) == viskores::CELL_SHAPE_TRIANGLE);
    }

    {
      filter.SetInputCellDimensionToPolygons();
      viskores::cont::DataSet output = filter.Execute(input);
      VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 16);
      VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 8);
      VISKORES_TEST_ASSERT(output.GetCellSet().GetCellShape(0) == viskores::CELL_SHAPE_LINE);
    }

    {
      filter.SetInputCellDimensionToLines();
      viskores::cont::DataSet output = filter.Execute(input);
      VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 2);
      VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 2);
      VISKORES_TEST_ASSERT(output.GetCellSet().GetCellShape(0) == viskores::CELL_SHAPE_VERTEX);
    }

    {
      filter.SetInputCellDimensionToAll();
      viskores::cont::DataSet output = filter.Execute(input);
      VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == 36);
      VISKORES_TEST_ASSERT(output.GetNumberOfCells() == 20);
    }
  }

  void operator()() const
  {
    this->TestContourUniformGrid<viskores::filter::contour::Contour>(72);
    this->TestContourUniformGrid<viskores::filter::contour::ContourFlyingEdges>(72);
    // Unlike flying edges, marching cells does not have point merging for free,
    // So the number of points should increase when disabling duplicate point merging.
    this->TestContourUniformGrid<viskores::filter::contour::ContourMarchingCells>(480);

    this->Test3DUniformDataSet0<viskores::filter::contour::Contour>();
    this->Test3DUniformDataSet0<viskores::filter::contour::ContourMarchingCells>();
    this->Test3DUniformDataSet0<viskores::filter::contour::ContourFlyingEdges>();

    this->TestContourWedges<viskores::filter::contour::Contour>();
    this->TestContourWedges<viskores::filter::contour::ContourMarchingCells>();

    this->TestNonUniformStructured<viskores::filter::contour::Contour>();
    this->TestNonUniformStructured<viskores::filter::contour::ContourFlyingEdges>();
    this->TestNonUniformStructured<viskores::filter::contour::ContourMarchingCells>();

    this->TestUnsupportedFlyingEdges();

    this->TestMixedShapes();
  }

}; // class TestContourFilter
} // namespace

int UnitTestContourFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContourFilter{}, argc, argv);
}
