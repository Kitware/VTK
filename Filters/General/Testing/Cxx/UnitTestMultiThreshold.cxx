/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestMultiThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkMultiThreshold.h"

#include "vtkImageData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkImageDataToPointSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkDataObject.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkExecutive.h"
#include "vtkTestErrorObserver.h"

static void TestPrint();
static int TestErrorsAndWarnings();
static int TestFilter(int, int);
static void CreateStructuredGrid(
  vtkSmartPointer<vtkStructuredGrid>&, int, int);
static int GetBlockCellCount(vtkMultiBlockDataSet *mbds, int block);

int UnitTestMultiThreshold(int, char *[])
{
  int status = 0;

  TestPrint();
  status += TestFilter(50, 40);
  status += TestErrorsAndWarnings();

  return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

void TestPrint()
{
  vtkSmartPointer<vtkMultiThreshold> threshold =
    vtkSmartPointer<vtkMultiThreshold>::New();

  // Print right after constructed
  threshold->Print(std::cout);

  vtkSmartPointer<vtkStructuredGrid> sg =
    vtkSmartPointer<vtkStructuredGrid>::New();

  CreateStructuredGrid(sg, 3, 3);
  threshold->SetInputData(0, sg);
  threshold->Update();

  // Print after update
  threshold->Print(std::cout);
}

void CreateStructuredGrid(vtkSmartPointer<vtkStructuredGrid> &sg,
                 int numCols,
                 int numRows)
{
  vtkSmartPointer<vtkImageData> image =
    vtkSmartPointer<vtkImageData>::New();

  // Specify the size of the image data
  image->SetDimensions(numCols + 1, numRows + 1, 1);

  image->AllocateScalars(VTK_INT,1);

  // Populate the point data
  vtkSmartPointer<vtkFloatArray> vectors =
    vtkSmartPointer<vtkFloatArray>::New();
  vectors->SetName("PointVectors");
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples((numRows + 1) * (numCols + 1));
  int pointNo = 0;
  for (int j = 0; j < numRows + 1; ++j)
  {
    for (int i = 0; i < numCols + 1; ++i)
    {
      float vec[3];
      int* pixel = static_cast<int*>(image->GetScalarPointer(i, j, 0));
      vec[0] = 0.0;
      vec[1] = 0.0;
      vec[2] = vtkMath::Random(-10.0, 10.0);
      vectors->SetTuple(pointNo, vec);
      *pixel = pointNo++;
    }
  }
  image->GetPointData()->AddArray(vectors);

  // Populate the cell data
  vtkSmartPointer<vtkIntArray> columns =
    vtkSmartPointer<vtkIntArray>::New();
  columns->SetNumberOfTuples(numCols * numRows);
  columns->SetName("Columns");

  vtkSmartPointer<vtkIntArray> rows =
    vtkSmartPointer<vtkIntArray>::New();
  rows->SetNumberOfTuples(numCols * numRows);
  rows->SetName("Rows");

  vtkSmartPointer<vtkIntArray> cells =
    vtkSmartPointer<vtkIntArray>::New();
  cells->SetNumberOfTuples(numCols * numRows);
  cells->SetName("Cells");

  int cell = 0;
  for (int row = 0; row < numRows; ++row)
  {
    for (int col = 0; col < numCols; ++col)
    {
      columns->SetTuple1(cell, col);
      rows->SetTuple1(cell, row);
      cells->SetTuple1(cell, cell);
      ++cell;
    }
  }
  image->GetCellData()->AddArray(columns);
  image->GetCellData()->AddArray(rows);
  image->GetCellData()->AddArray(cells);

  // Convert the image data to a point set
  vtkSmartPointer<vtkImageDataToPointSet> imToPs =
    vtkSmartPointer<vtkImageDataToPointSet>::New();
  imToPs->SetInputData(image);
  imToPs->Update();
  sg = imToPs->GetOutput();
}

int TestFilter(int columns, int rows)
{
  int status = 0;
  int cells = rows * columns;
  int points = (rows + 1) * (columns + 1);

  vtkSmartPointer<vtkStructuredGrid> sg =
    vtkSmartPointer<vtkStructuredGrid>::New();

  CreateStructuredGrid(sg, columns, rows);

  vtkSmartPointer<vtkMultiThreshold> threshold =
    vtkSmartPointer<vtkMultiThreshold>::New();

  threshold->SetInputData(0, sg);
  std::vector<int> intervalSets;
  std::vector<int> expectedCellCounts;

  // 0: Row rows/2 expect columns cells
  intervalSets.push_back(threshold->AddIntervalSet(
    rows/2, rows/2,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Rows", 0, 1 ));
  expectedCellCounts.push_back(columns);

  // 1: Column columns/2 expect rows cells
  intervalSets.push_back(threshold->AddIntervalSet(
    columns/2, columns/2,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Columns", 0, 1 ));
  expectedCellCounts.push_back(rows);

  // 2: Cells expect cells / 2
  intervalSets.push_back(threshold->AddIntervalSet(
                           cells / 2, cells,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::OPEN,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Cells", 0, 1 ));
  expectedCellCounts.push_back((rows * columns) / 2);

  // 3: Points (0, points/2)
  intervalSets.push_back(threshold->AddIntervalSet(
    0, points / 2,
    vtkMultiThreshold::OPEN, vtkMultiThreshold::OPEN,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, 0, 0, 1 ));
  expectedCellCounts.push_back(-1);

  // 4: Row x and Column y expect 1 cell
  int intersection[2];
  intersection[0] = intervalSets[0];
  intersection[1] = intervalSets[1];
  intervalSets.push_back(threshold->AddBooleanSet(vtkMultiThreshold::AND, 2, intersection));
  expectedCellCounts.push_back(1);

  // 5: Row x or Column y expect rows + columns - 1
  intervalSets.push_back(threshold->AddBooleanSet(vtkMultiThreshold::OR, 2, intersection));
  expectedCellCounts.push_back(rows + columns - 1);

  // 6: Row x or Column y but not both  expect rows + columns - 2
  intervalSets.push_back(threshold->AddBooleanSet(vtkMultiThreshold::XOR, 2, intersection));
  expectedCellCounts.push_back(rows + columns - 2);

  // 7: expect rows + columns - 2
  intervalSets.push_back(threshold->AddBooleanSet(vtkMultiThreshold::WOR, 2, intersection));
  expectedCellCounts.push_back(rows + columns - 2);

  // 8: Not Row 1 or Column 2 expect rows * columns - 1
  intervalSets.push_back(threshold->AddBooleanSet(vtkMultiThreshold::NAND, 2, intersection));
  expectedCellCounts.push_back(rows * columns - 1);

  // 9-12: Convenience members
 intervalSets.push_back(threshold->AddLowpassIntervalSet(
    1,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Rows", 0, 1 ));
  expectedCellCounts.push_back(2 * columns);

  intervalSets.push_back(threshold->AddHighpassIntervalSet(
    rows - 1,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Rows", 0, 1 ));
  expectedCellCounts.push_back(columns);

  intervalSets.push_back(threshold->AddBandpassIntervalSet(
    1, 2,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Columns", 0, 1 ));
  expectedCellCounts.push_back(2 * rows);

  intervalSets.push_back(threshold->AddNotchIntervalSet(
    1, 1,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Rows", 0, 1 ));
  expectedCellCounts.push_back((rows - 1) * columns);

  // 13-16: PointVectors
  intervalSets.push_back(threshold->AddIntervalSet(
    1, 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "PointVectors", 2, 1 ));
  expectedCellCounts.push_back(-1);

  intervalSets.push_back(threshold->AddIntervalSet(
    1, 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "PointVectors", -1, 1 ));
  expectedCellCounts.push_back(-1);

  intervalSets.push_back(threshold->AddIntervalSet(
    1, 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "PointVectors", -2, 1 ));
  expectedCellCounts.push_back(-1);

  intervalSets.push_back(threshold->AddIntervalSet(
    1, 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "PointVectors", -3, 0 ));
  expectedCellCounts.push_back(-1);

  for (size_t n = 0; n < intervalSets.size(); ++n)
  {
    std::cout << "OutputSet: "
              << threshold->OutputSet(intervalSets[n]) << std::endl;
  }

  // Add the first set again, should do nothing
  std::cout << "OutputSet: "
            << threshold->OutputSet(intervalSets[0]) << std::endl;
  threshold->Update();

  int blocksBefore = threshold->GetOutput()->GetNumberOfBlocks();
  for (int b = 0; b < blocksBefore; ++b)
  {
    std::cout << "Block " << b << " has "
              << GetBlockCellCount(threshold->GetOutput(), b) << " cells";
    if (expectedCellCounts[b] != -1 &&
        expectedCellCounts[b] != GetBlockCellCount(threshold->GetOutput(), b))
    {
      std::cout << " but expected " << expectedCellCounts[b];
      ++status;
    }
    std::cout << std::endl;
  }

  // Add the first set again, should do nothing
  std::cout << "OutputSet: "
            << threshold->OutputSet(intervalSets[0]) << std::endl;
  threshold->Update();
  int blocksAfter = threshold->GetOutput()->GetNumberOfBlocks();
  if (blocksBefore != blocksAfter)
  {
    std::cout
      << "ERROR: A duplicate OutputSet() should not produce extra output"
      << std::endl;
    ++status;
  }
  threshold->Print(std::cout);
  return status;
}

int TestErrorsAndWarnings()
{
  int status = 0;
  vtkSmartPointer<vtkTest::ErrorObserver>  filterObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkStructuredGrid> sg =
    vtkSmartPointer<vtkStructuredGrid>::New();

  CreateStructuredGrid(sg, 4, 3);

  vtkSmartPointer<vtkMultiThreshold> threshold =
    vtkSmartPointer<vtkMultiThreshold>::New();
  threshold->SetInputData(sg);
  threshold->AddObserver(vtkCommand::ErrorEvent, filterObserver);
  threshold->AddObserver(vtkCommand::WarningEvent, filterObserver);

  std::vector<int> intervalSets;
  intervalSets.push_back(threshold->AddIntervalSet(
    1, 1,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Rows", 0, 1 ));
  intervalSets.push_back(threshold->AddIntervalSet(
    1, 1,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "aColumns", 0, 1 ));
  intervalSets.push_back(threshold->AddIntervalSet(
    2, 3,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "Cells", 0, 1 ));

  // WARNING: You passed a null array name
  intervalSets.push_back(threshold->AddIntervalSet(
    0, 2,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, (char *) NULL, 0, 1 ));
  if (filterObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << filterObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'You passed a null array name' warning" << std::endl;
    ++status;
  }
  filterObserver->Clear();


  // WARNING: You passed an invalid attribute type (100)
  intervalSets.push_back(threshold->AddIntervalSet(
    0, 2,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, 100, 0, 1 ));
  if (filterObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << filterObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'You passed an invalid attribute type (100)' warning" << std::endl;
    ++status;
  }
  filterObserver->Clear();

  int intersection[2];
  intersection[0] = intervalSets[0];

  // ERROR: Operators require at least one operand. You passed 0.
  intervalSets.push_back(threshold->AddBooleanSet(vtkMultiThreshold::AND, 0, intersection));
  if (filterObserver->GetError())
  {
    std::cout << "Caught expected error: "
              << filterObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Operators require at least one operand. You passed 0.' error" << std::endl;
    ++status;
  }
  filterObserver->Clear();

  // ERROR: Invalid operation (10)
  intervalSets.push_back(threshold->AddBooleanSet(10, 1, intersection));
  if (filterObserver->GetError())
  {
    std::cout << "Caught expected error: "
              << filterObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Invalid operation (10)' error" << std::endl;
    ++status;
  }
  filterObserver->Clear();

  // ERROR: Input 1 is invalid(100)
  intersection[1] = 100;
  intervalSets.push_back(threshold->AddBooleanSet(vtkMultiThreshold::XOR, 2, intersection));
  if (filterObserver->GetError())
  {
    std::cout << "Caught expected error: "
              << filterObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Input 1 is invalid(100)' error" << std::endl;
    ++status;
  }
  filterObserver->Clear();

  intersection[1] = intervalSets[1];
  intervalSets.push_back(threshold->AddBooleanSet(vtkMultiThreshold::XOR, 2, intersection));

  // 9: PointVectors
  intervalSets.push_back(threshold->AddIntervalSet(
    1, 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "PointXXXVectors", 0, 1 ));
  intervalSets.push_back(threshold->AddIntervalSet(
    1, 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, 0, -1, 1 ));

  // WARNING: You passed an invalid atribute type (100)
  intervalSets.push_back(threshold->AddIntervalSet(
    1, 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    100, "PointVectors", -2, 1 ));
  if (filterObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << filterObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'You passed an invalid atribute type (100)' warning" << std::endl;
    ++status;
  }
  filterObserver->Clear();


  // WARNING: Intervals must be specified with ascending values (xmin
  // <= xmax)
  intervalSets.push_back(threshold->AddIntervalSet(
    11, 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "PointVectors", -3, 1 ));
  if (filterObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << filterObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'One of the interval endpoints is not a number.' warning" << std::endl;
    ++status;
  }
  filterObserver->Clear();

#ifndef _WIN32
  // WARNING: One of the interval endpoints is not a number.
  intervalSets.push_back(threshold->AddIntervalSet(
    vtkMath::Nan(), 10,
    vtkMultiThreshold::CLOSED, vtkMultiThreshold::CLOSED,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "PointVectors", -3, 1 ));
  if (filterObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << filterObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'One of the interval endpoints is not a number.' warning" << std::endl;
    ++status;
  }
  filterObserver->Clear();
#endif

  // WARNING: An open interval with equal endpoints will always be
  // empty. I won't help you waste my time.
  intervalSets.push_back(threshold->AddIntervalSet(
    10, 10,
    vtkMultiThreshold::OPEN, vtkMultiThreshold::OPEN,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "PointVectors", -3, 1 ));
  if (filterObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << filterObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'An open interval with equal endpoints will always be...' warning" << std::endl;
    ++status;
  }
  filterObserver->Clear();


  // WARNING: Cannot output 1000 because there is not set with that
  // label
  threshold->OutputSet(1000);
  if (filterObserver->GetWarning())
  {
    std::cout << "Caught expected warning: "
              << filterObserver->GetWarningMessage();
  }
  else
  {
    std::cout << "Failed to catch expected 'Cannot output 1000 because there is not set with that label' warning" << std::endl;
    ++status;
  }
  filterObserver->Clear();

  vtkSmartPointer<vtkTest::ErrorObserver>  executiveObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  threshold->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, executiveObserver);
  threshold->Update();

  if (executiveObserver->GetError())
  {
    std::cout << "Caught expected error: "
              << executiveObserver->GetErrorMessage();
  }
  else
  {
    std::cout << "Failed to catch expected pipeline error" << std::endl;
    ++status;
  }
  filterObserver->Clear();

  return status;
}

int GetBlockCellCount(vtkMultiBlockDataSet *mbds, int block)
{
  vtkMultiBlockDataSet *mds =
    vtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(block));
  vtkUnstructuredGrid *ug =
    vtkUnstructuredGrid::SafeDownCast(mds->GetBlock(0));
  return ug->GetNumberOfCells();
}
