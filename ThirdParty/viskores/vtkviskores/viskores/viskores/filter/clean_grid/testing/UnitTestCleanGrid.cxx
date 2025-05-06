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

#include <viskores/filter/clean_grid/CleanGrid.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/ContourMarchingCells.h>

namespace
{

void TestUniformGrid(viskores::filter::clean_grid::CleanGrid clean)
{
  std::cout << "Testing 'clean' uniform grid." << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;

  viskores::cont::DataSet inData = makeData.Make2DUniformDataSet0();

  clean.SetFieldsToPass({ "pointvar", "cellvar" });
  viskores::cont::DataSet outData = clean.Execute(inData);
  VISKORES_TEST_ASSERT(outData.HasField("pointvar"), "Failed to map point field");
  VISKORES_TEST_ASSERT(outData.HasField("cellvar"), "Failed to map cell field");

  viskores::cont::CellSetExplicit<> outCellSet;
  outData.GetCellSet().AsCellSet(outCellSet);
  VISKORES_TEST_ASSERT(outCellSet.GetNumberOfPoints() == 6,
                       "Wrong number of points: ",
                       outCellSet.GetNumberOfPoints());
  VISKORES_TEST_ASSERT(
    outCellSet.GetNumberOfCells() == 2, "Wrong number of cells: ", outCellSet.GetNumberOfCells());
  viskores::Id4 cellIds;
  outCellSet.GetIndices(0, cellIds);
  VISKORES_TEST_ASSERT((cellIds == viskores::Id4(0, 1, 4, 3)), "Bad cell ids: ", cellIds);
  outCellSet.GetIndices(1, cellIds);
  VISKORES_TEST_ASSERT((cellIds == viskores::Id4(1, 2, 5, 4)), "Bad cell ids: ", cellIds);

  viskores::cont::ArrayHandle<viskores::Float32> outPointField;
  outData.GetField("pointvar").GetData().AsArrayHandle(outPointField);
  VISKORES_TEST_ASSERT(outPointField.GetNumberOfValues() == 6,
                       "Wrong point field size: ",
                       outPointField.GetNumberOfValues());
  VISKORES_TEST_ASSERT(test_equal(outPointField.ReadPortal().Get(1), 20.1),
                       "Bad point field value: ",
                       outPointField.ReadPortal().Get(1));
  VISKORES_TEST_ASSERT(test_equal(outPointField.ReadPortal().Get(4), 50.1),
                       "Bad point field value: ",
                       outPointField.ReadPortal().Get(1));

  viskores::cont::ArrayHandle<viskores::Float32> outCellField;
  outData.GetField("cellvar").GetData().AsArrayHandle(outCellField);
  VISKORES_TEST_ASSERT(outCellField.GetNumberOfValues() == 2, "Wrong cell field size.");
  VISKORES_TEST_ASSERT(test_equal(outCellField.ReadPortal().Get(0), 100.1),
                       "Bad cell field value",
                       outCellField.ReadPortal().Get(0));
  VISKORES_TEST_ASSERT(test_equal(outCellField.ReadPortal().Get(1), 200.1),
                       "Bad cell field value",
                       outCellField.ReadPortal().Get(0));
}

void TestPointMerging()
{
  viskores::cont::testing::MakeTestDataSet makeDataSet;
  viskores::cont::DataSet baseData = makeDataSet.Make3DUniformDataSet3(viskores::Id3(4, 4, 4));

  viskores::filter::contour::ContourMarchingCells marchingCubes;
  marchingCubes.SetIsoValue(0.05);
  marchingCubes.SetMergeDuplicatePoints(false);
  marchingCubes.SetActiveField("pointvar");
  viskores::cont::DataSet inData = marchingCubes.Execute(baseData);
  constexpr viskores::Id originalNumPoints = 228;
  constexpr viskores::Id originalNumCells = 76;
  VISKORES_TEST_ASSERT(inData.GetCellSet().GetNumberOfPoints() == originalNumPoints);
  VISKORES_TEST_ASSERT(inData.GetNumberOfCells() == originalNumCells);

  viskores::filter::clean_grid::CleanGrid cleanGrid;

  std::cout << "Clean grid without any merging" << std::endl;
  cleanGrid.SetCompactPointFields(false);
  cleanGrid.SetMergePoints(false);
  cleanGrid.SetRemoveDegenerateCells(false);
  viskores::cont::DataSet noMerging = cleanGrid.Execute(inData);
  VISKORES_TEST_ASSERT(noMerging.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(noMerging.GetCellSet().GetNumberOfPoints() == originalNumPoints);
  VISKORES_TEST_ASSERT(noMerging.GetNumberOfPoints() == originalNumPoints);
  VISKORES_TEST_ASSERT(noMerging.GetField("pointvar").GetNumberOfValues() == originalNumPoints);
  VISKORES_TEST_ASSERT(noMerging.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid by merging very close points" << std::endl;
  cleanGrid.SetMergePoints(true);
  cleanGrid.SetFastMerge(false);
  viskores::cont::DataSet closeMerge = cleanGrid.Execute(inData);
  constexpr viskores::Id closeMergeNumPoints = 62;
  VISKORES_TEST_ASSERT(closeMerge.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(closeMerge.GetCellSet().GetNumberOfPoints() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeMerge.GetNumberOfPoints() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeMerge.GetField("pointvar").GetNumberOfValues() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid by merging very close points with fast merge" << std::endl;
  cleanGrid.SetFastMerge(true);
  viskores::cont::DataSet closeFastMerge = cleanGrid.Execute(inData);
  VISKORES_TEST_ASSERT(closeFastMerge.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(closeFastMerge.GetCellSet().GetNumberOfPoints() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeFastMerge.GetNumberOfPoints() == closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeFastMerge.GetField("pointvar").GetNumberOfValues() ==
                       closeMergeNumPoints);
  VISKORES_TEST_ASSERT(closeFastMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points" << std::endl;
  cleanGrid.SetFastMerge(false);
  cleanGrid.SetTolerance(0.1);
  viskores::cont::DataSet farMerge = cleanGrid.Execute(inData);
  constexpr viskores::Id farMergeNumPoints = 36;
  VISKORES_TEST_ASSERT(farMerge.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(farMerge.GetCellSet().GetNumberOfPoints() == farMergeNumPoints);
  VISKORES_TEST_ASSERT(farMerge.GetNumberOfPoints() == farMergeNumPoints);
  VISKORES_TEST_ASSERT(farMerge.GetField("pointvar").GetNumberOfValues() == farMergeNumPoints);
  VISKORES_TEST_ASSERT(farMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points quickly" << std::endl;
  cleanGrid.SetFastMerge(true);
  viskores::cont::DataSet farFastMerge = cleanGrid.Execute(inData);
  constexpr viskores::Id farFastMergeNumPoints = 19;
  VISKORES_TEST_ASSERT(farFastMerge.GetNumberOfCells() == originalNumCells);
  VISKORES_TEST_ASSERT(farFastMerge.GetCellSet().GetNumberOfPoints() == farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(farFastMerge.GetNumberOfPoints() == farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(farFastMerge.GetField("pointvar").GetNumberOfValues() ==
                       farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(farFastMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points quickly with degenerate cells"
            << std::endl;
  cleanGrid.SetRemoveDegenerateCells(true);
  viskores::cont::DataSet noDegenerateCells = cleanGrid.Execute(inData);
  constexpr viskores::Id numNonDegenerateCells = 18;
  VISKORES_TEST_ASSERT(noDegenerateCells.GetNumberOfCells() == numNonDegenerateCells);
  VISKORES_TEST_ASSERT(noDegenerateCells.GetCellSet().GetNumberOfPoints() == farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(noDegenerateCells.GetNumberOfPoints() == farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(noDegenerateCells.GetField("pointvar").GetNumberOfValues() ==
                       farFastMergeNumPoints);
  VISKORES_TEST_ASSERT(noDegenerateCells.GetField("cellvar").GetNumberOfValues() ==
                       numNonDegenerateCells);
}

void RunTest()
{
  viskores::filter::clean_grid::CleanGrid clean;

  std::cout << "*** Test with compact point fields on merge points off" << std::endl;
  clean.SetCompactPointFields(true);
  clean.SetMergePoints(false);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields off merge points off" << std::endl;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(false);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields on merge points on" << std::endl;
  clean.SetCompactPointFields(true);
  clean.SetMergePoints(true);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields off merge points on" << std::endl;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(true);
  TestUniformGrid(clean);

  std::cout << "*** Test point merging" << std::endl;
  TestPointMerging();
}

} // anonymous namespace

int UnitTestCleanGrid(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTest, argc, argv);
}
