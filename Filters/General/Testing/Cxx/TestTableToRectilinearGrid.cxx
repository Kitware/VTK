// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCommand.h>
#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRectilinearGrid.h>
#include <vtkTable.h>
#include <vtkTableToRectilinearGrid.h>
#include <vtkTestErrorObserver.h>
#include <vtkTestUtilities.h>
#include <vtkVariantArray.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLTableReader.h>

#include <cstdlib>
#include <vector>

namespace
{
// when using previous arrays in a canonical way, removing "ghostpoint" from input leads to the
// following ghost in output.
constexpr vtkIdType ghostPointIdx = 8;
constexpr vtkIdType ghostCellIdx = 3;

/**
 * Fill shuffledTable with originalTable but with some inverted rows.
 */
void ShuffleTable(vtkTable* originalTable, vtkTable* shuffledTable)
{
  shuffledTable->DeepCopy(originalTable);
  const std::vector<std::pair<vtkIdType, vtkIdType>> permutations = { { 0, 12 }, { 5, 9 },
    { 3, 4 } };

  for (const auto& perm : permutations)
  {
    vtkNew<vtkVariantArray> row;
    originalTable->GetRow(perm.first, row);
    shuffledTable->SetRow(perm.second, row);
    originalTable->GetRow(perm.second, row);
    shuffledTable->SetRow(perm.first, row);
  }
}

// ----------------------------------------------------------------------------
void GetDefaultTable(int argc, char* argv[], vtkTable* table)
{
  std::string filePath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/rectilinear.vtt");
  vtkNew<vtkXMLTableReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->Update();
  table->ShallowCopy(reader->GetOutput());
}

// ----------------------------------------------------------------------------
void GetBaseline(int argc, char* argv[], vtkRectilinearGrid* baseline)
{
  std::string filePath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/table_to_rectilinear.vtr");
  vtkNew<vtkXMLRectilinearGridReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->Update();
  baseline->ShallowCopy(reader->GetOutput());
}

// ----------------------------------------------------------------------------
bool TestDefault(int argc, char* argv[])
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> table;
  ::GetDefaultTable(argc, argv, table);

  vtkNew<vtkTableToRectilinearGrid> tableToGrid;
  tableToGrid->SetInputData(table);
  tableToGrid->SetXYZColumns("X", "Y", "Z");
  tableToGrid->Update();
  auto output = tableToGrid->GetOutput();

  bool hasBlankPoints = output->HasAnyBlankPoints();
  vtkLogIf(ERROR, hasBlankPoints, "Grid should not have any blank points");

  vtkNew<vtkRectilinearGrid> baseline;
  ::GetBaseline(argc, argv, baseline);

  bool ret = vtkTestUtilities::CompareDataObjects(output, baseline);
  return ret && !hasBlankPoints;
}

// ----------------------------------------------------------------------------
bool TestHoledInput(int argc, char* argv[])
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> table;
  ::GetDefaultTable(argc, argv, table);

  // remove a corner point: it will ghost the cell it belongs to.
  table->RemoveRow(::ghostPointIdx);

  vtkNew<vtkTableToRectilinearGrid> tableToGrid;
  tableToGrid->SetInputData(table);
  tableToGrid->SetXYZColumns("X", "Y", "Z");
  tableToGrid->Update();

  auto output = tableToGrid->GetOutput();
  bool hasBlankPoints = output->HasAnyBlankPoints();
  vtkLogIf(ERROR, !hasBlankPoints, "Grid should have a blank point");
  bool hasBlankCells = output->HasAnyBlankCells();
  vtkLogIf(ERROR, !hasBlankCells, "Grid should have a blank cell");
  bool isBlankHidden = !output->IsCellVisible(::ghostCellIdx);
  vtkLogIf(ERROR, !isBlankHidden, "Blank cell should not be visible");

  vtkNew<vtkRectilinearGrid> baseline;
  ::GetBaseline(argc, argv, baseline);
  baseline->BlankCell(::ghostCellIdx);
  baseline->BlankPoint(::ghostPointIdx);
  bool ret = vtkTestUtilities::CompareDataObjects(output, baseline);

  return hasBlankPoints && hasBlankCells && isBlankHidden && ret;
}

// ----------------------------------------------------------------------------
bool TestOrdering(int argc, char* argv[])
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> originalTable;
  ::GetDefaultTable(argc, argv, originalTable);

  vtkNew<vtkTableToRectilinearGrid> tableToGrid;
  tableToGrid->SetInputData(originalTable);
  tableToGrid->SetXYZColumns("X", "Y", "Z");
  tableToGrid->Update();
  vtkNew<vtkRectilinearGrid> originalGrid;
  originalGrid->ShallowCopy(tableToGrid->GetOutput());

  vtkNew<vtkTable> shuffledTable;
  ::ShuffleTable(originalTable, shuffledTable);

  tableToGrid->SetInputData(shuffledTable);
  tableToGrid->Update();
  bool result = vtkTestUtilities::CompareDataObjects(originalGrid, tableToGrid->GetOutput());
  vtkLogIf(ERROR, !result, "Lines order should not change the resulting grid");

  // smoke test change the column order
  tableToGrid->SetXYZColumns("Z", "Y", "X");
  tableToGrid->Update();

  // duplicate a column
  tableToGrid->SetXYZColumns("X", "Y", "X");
  tableToGrid->Update();

  return result;
}

// ----------------------------------------------------------------------------
bool TestMissingArray(int argc, char* argv[])
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> originalTable;
  ::GetDefaultTable(argc, argv, originalTable);

  vtkNew<vtkTest::ErrorObserver> observer;
  vtkNew<vtkTableToRectilinearGrid> tableToGrid;
  tableToGrid->AddObserver(vtkCommand::WarningEvent, observer);
  tableToGrid->SetInputData(originalTable);
  tableToGrid->SetXYZColumns("notfound", "Y", "Z");
  tableToGrid->Update();

  bool warn = observer->GetNumberOfWarnings() == 1;
  vtkLogIf(ERROR, !warn, "Missing named array is a warning.");
  bool ret = warn;

  int dim[3];
  tableToGrid->GetOutput()->GetDimensions(dim);

  bool xDim1 = dim[0] == 1;
  vtkLogIf(ERROR, !xDim1, "With missing input array, dimension should be 1");
  ret &= xDim1;

  return ret;
}

// ----------------------------------------------------------------------------
bool TestGridDimensions(int argc, char* argv[])
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> originalTable;
  ::GetDefaultTable(argc, argv, originalTable);

  vtkNew<vtkTableToRectilinearGrid> tableToGrid;
  tableToGrid->SetInputData(originalTable);

  vtkLog(INFO, "test 1D");
  tableToGrid->SetXColumn("X");
  tableToGrid->Update();

  vtkRectilinearGrid* output = tableToGrid->GetOutput();
  int dim[3];
  output->GetDimensions(dim);
  bool check1D = dim[0] > 1 && dim[1] == 1 && dim[2] == 1;
  vtkLogIf(ERROR, !check1D, "With single column set, grid should be 1D");
  bool ret = check1D;

  auto ycoord = output->GetYCoordinates();
  bool checkYCoord = ycoord != nullptr;
  checkYCoord = checkYCoord && (ycoord->GetNumberOfTuples() == 1);
  checkYCoord = checkYCoord && (ycoord->GetTuple1(0) == 0);

  vtkLogIf(ERROR, !checkYCoord, "Unset coordinates should be an array of 1 value set to 0.");
  ret &= checkYCoord;

  vtkLog(INFO, "test 2D");
  tableToGrid->SetYColumn("Y");
  tableToGrid->Update();
  output->GetDimensions(dim);
  bool check2D = dim[0] > 1 && dim[1] > 1 && dim[2] == 1;
  vtkLogIf(ERROR, !check2D, "With two columns set, grid should be 2D");
  ret &= check2D;

  return ret;
}

// ----------------------------------------------------------------------------
bool TestMultiComponentsArray(int argc, char* argv[])
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> originalTable;
  ::GetDefaultTable(argc, argv, originalTable);

  const std::string vectorArrayName = "vector";
  vtkNew<vtkDoubleArray> vector;
  vector->SetName(vectorArrayName.c_str());
  vector->SetNumberOfComponents(3);
  vector->SetNumberOfTuples(originalTable->GetNumberOfRows());
  vector->Fill(1);
  originalTable->AddColumn(vector);

  vtkNew<vtkTableToRectilinearGrid> tableToGrid;
  tableToGrid->SetInputData(originalTable);
  tableToGrid->SetXYZColumns("Y", vectorArrayName, "Z");
  tableToGrid->Update();

  return true;
}

// ----------------------------------------------------------------------------
bool TestInputArrayToProcess(int argc, char* argv[])
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> originalTable;
  ::GetDefaultTable(argc, argv, originalTable);

  vtkNew<vtkTableToRectilinearGrid> tableToGrid;
  tableToGrid->SetInputData(originalTable);

  bool checkName = (tableToGrid->GetXColumn().empty());
  vtkLogIf(ERROR, !checkName,
    "Default name retrieved for \"X\" column is not empty. Has " << tableToGrid->GetXColumn());
  bool ret = checkName;

  tableToGrid->SetInputArrayToProcess(0, 0, 0, vtkDataObject::ROW, "X");
  tableToGrid->SetInputArrayToProcess(1, 0, 0, vtkDataObject::ROW, "Y");
  tableToGrid->SetInputArrayToProcess(2, 0, 0, vtkDataObject::ROW, "Z");
  tableToGrid->Update();

  checkName = (tableToGrid->GetXColumn() == "X");
  vtkLogIf(
    ERROR, !checkName, "Wrong name retrieved for \"X\" column. Has " << tableToGrid->GetXColumn());
  ret &= checkName;
  checkName = (tableToGrid->GetYColumn() == "Y");
  vtkLogIf(
    ERROR, !checkName, "Wrong name retrieved for \"Y\" column. Has " << tableToGrid->GetYColumn());
  ret &= checkName;
  checkName = (tableToGrid->GetZColumn() == "Z");
  vtkLogIf(
    ERROR, !checkName, "Wrong name retrieved for \"Z\" column. Has " << tableToGrid->GetZColumn());
  ret &= checkName;

  vtkNew<vtkRectilinearGrid> baseline;
  ::GetBaseline(argc, argv, baseline);

  ret &= vtkTestUtilities::CompareDataObjects(tableToGrid->GetOutput(), baseline);
  return ret;
}
}

// ----------------------------------------------------------------------------
int TestTableToRectilinearGrid(int argc, char* argv[])
{
  bool ret = ::TestDefault(argc, argv);
  ret &= ::TestHoledInput(argc, argv);
  ret &= ::TestOrdering(argc, argv);
  ret &= ::TestMissingArray(argc, argv);
  ret &= ::TestGridDimensions(argc, argv);
  ret &= ::TestMultiComponentsArray(argc, argv);
  ret &= ::TestInputArrayToProcess(argc, argv);
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
