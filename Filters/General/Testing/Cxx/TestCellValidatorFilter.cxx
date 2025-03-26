// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This tests vtkCellValidator as a filter

#include <vtkCellValidator.h>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkWeakPointer.h>

//------------------------------------------------------------------------------
bool TestArray(vtkDataArray* stateArray, const std::vector<vtkCellValidator::State>& expectedValues)
{
  if (!stateArray)
  {
    return false;
  }

  vtkIdType size = static_cast<vtkIdType>(expectedValues.size());
  if (stateArray->GetNumberOfTuples() != size)
  {
    return false;
  }

  for (vtkIdType cellId = 0; cellId < size; cellId++)
  {
    auto state = static_cast<vtkCellValidator::State>(stateArray->GetTuple1(cellId));
    if (state != vtkCellValidator::Valid && (state & expectedValues[cellId]) == 0)
    {
      std::cout << "ERROR: invalid cell state found at id: " << cellId << "\n";
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
int TestCellValidatorFilter(int, char*[])
{
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(0, 0.1, 0.1);
  polydata->SetPoints(points);

  std::vector<vtkCellValidator::State> cellsValidity;

  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell(2);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  cellsValidity.emplace_back(vtkCellValidator::Valid);

  lines->InsertNextCell(2);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  cellsValidity.emplace_back(vtkCellValidator::Valid);

  vtkNew<vtkCellArray> polys;
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  cellsValidity.emplace_back(vtkCellValidator::Valid);

  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(3);
  polys->InsertCellPoint(2);
  cellsValidity.emplace_back(vtkCellValidator::IntersectingEdges);

  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(5);
  polys->InsertCellPoint(3);
  cellsValidity.emplace_back(vtkCellValidator::Nonconvex);

  polys->InsertNextCell(2);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  // line is not a poly: wrong number of points
  cellsValidity.emplace_back(vtkCellValidator::WrongNumberOfPoints);

  polydata->SetLines(lines);
  polydata->SetPolys(polys);

  auto validator = vtkSmartPointer<vtkCellValidator>::New();
  validator->SetInputData(polydata);
  validator->Update();

  // weak ref to output, so we will be able to delete them
  vtkWeakPointer<vtkPolyData> output = validator->GetPolyDataOutput();
  vtkWeakPointer<vtkCellData> cellData = output->GetCellData();

  // "hard" ref to array, to test its persistence on dataset deletion.
  vtkSmartPointer<vtkDataArray> stateArray = cellData->GetArray("ValidityState");
  if (!TestArray(stateArray, cellsValidity))
  {
    return EXIT_FAILURE;
  }

  // test that ValidityState array is persistent when dataset is deleted.
  // This is done because the filters create an implicit array that called the dataset
  // at each request. On dataset DeleteEvent, the array should instanciate an explicit
  // copy of the values to stay valid.
  //
  // see vtkDataSetImplicitBackendInterface for more.
  validator = nullptr;

  if (!stateArray)
  {
    std::cout << "State array should exists\n";
    return EXIT_FAILURE;
  }

  if (output)
  {
    std::cout << "Output dataset should not exist anymore\n";
    return EXIT_FAILURE;
  }

  if (!TestArray(stateArray, cellsValidity))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
