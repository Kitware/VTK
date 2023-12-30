// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellIterator.h"

#include "vtkGenericCell.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include <sstream>
#include <string>

// Enable/disable code that helps/hinders profiling.
#undef PROFILE
// #define PROFILE

//------------------------------------------------------------------------------
// Compare the cell type, point ids, and points in 'grid' with those returned
// in 'iter'.
bool testCellIterator(vtkCellIterator* iter, vtkUnstructuredGrid* grid)
{
  vtkIdType cellId = 0;
  vtkNew<vtkGenericCell> cell;
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal())
  {
    grid->GetCell(cellId, cell);

    if (iter->GetCellType() != cell->GetCellType())
    {
      cerr << "Type mismatch for cell " << cellId << endl;
      return false;
    }

    vtkIdType numPoints = iter->GetNumberOfPoints();
    if (numPoints != cell->GetNumberOfPoints())
    {
      cerr << "Number of points mismatch for cell " << cellId << endl;
      return false;
    }

    for (vtkIdType pointInd = 0; pointInd < numPoints; ++pointInd)
    {
      if (iter->GetPointIds()->GetId(pointInd) != cell->PointIds->GetId(pointInd))
      {
        cerr << "Point id mismatch in cell " << cellId << endl;
        return false;
      }

      double iterPoint[3];
      double cellPoint[3];
      iter->GetPoints()->GetPoint(pointInd, iterPoint);
      cell->Points->GetPoint(pointInd, cellPoint);
      if (iterPoint[0] != cellPoint[0] || iterPoint[1] != cellPoint[1] ||
        iterPoint[2] != cellPoint[2])
      {
        cerr << "Point mismatch in cell " << cellId << endl;
        return false;
      }
    }

    iter->GoToNextCell();
    ++cellId;
  }

  // ensure that we checked all of the cells
  if (cellId != grid->GetNumberOfCells())
  {
    cerr << "Iterator did not cover all cells in the dataset!" << endl;
    return false;
  }

  //  cout << "Verified " << cellId << " cells with a " << iter->GetClassName()
  //       << "." << endl;
  return true;
}

#define TEST_ITERATOR(iter_, className_)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (std::string(#className_) != std::string(iter->GetClassName()))                             \
    {                                                                                              \
      cerr << "Unexpected iterator type (expected " #className_ ", got "                           \
           << (iter_)->GetClassName() << ")" << endl;                                              \
      return false;                                                                                \
    }                                                                                              \
                                                                                                   \
    if (!testCellIterator(iter_, grid))                                                            \
    {                                                                                              \
      cerr << #className_ << " test failed." << endl;                                              \
      return false;                                                                                \
    }                                                                                              \
                                                                                                   \
    if (!testCellIterator(iter_, grid))                                                            \
    {                                                                                              \
      cerr << #className_ << " test failed after rewind." << endl;                                 \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

bool runValidation(vtkUnstructuredGrid* grid)
{
  // vtkDataSetCellIterator:
  vtkCellIterator* iter = grid->vtkDataSet::NewCellIterator();
  TEST_ITERATOR(iter, vtkDataSetCellIterator);
  iter->Delete();

  // vtkPointSetCellIterator:
  iter = grid->vtkPointSet::NewCellIterator();
  TEST_ITERATOR(iter, vtkPointSetCellIterator);
  iter->Delete();

  // vtkUnstructuredGridCellIterator:
  iter = grid->vtkUnstructuredGrid::NewCellIterator();
  TEST_ITERATOR(iter, vtkUnstructuredGridCellIterator);
  iter->Delete();

  return true;
}

int TestCellIterators(int argc, char* argv[])
{
  // Load an unstructured grid dataset
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/blowGeom.vtk");
  std::string fileName(fileNameC);
  delete[] fileNameC;

  vtkNew<vtkUnstructuredGridReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkUnstructuredGrid* grid(reader->GetOutput());
  if (!grid)
  {
    cerr << "Error reading file: " << fileName << endl;
    return EXIT_FAILURE;
  }

#ifndef PROFILE
  if (!runValidation(grid))
  {
    return EXIT_FAILURE;
  }
#endif // not PROFILE

  return EXIT_SUCCESS;
}
