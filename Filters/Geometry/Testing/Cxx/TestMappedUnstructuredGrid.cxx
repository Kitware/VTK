/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMappedUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataSetSurfaceFilter.h"
#include "vtkMappedUnstructuredGridGenerator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridBase.h"

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;             \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestMappedUnstructuredGrid(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int rc = 0;

  vtkUnstructuredGridBase* mg;
  vtkMappedUnstructuredGridGenerator::GenerateMappedUnstructuredGrid(&mg);

  vtkUnstructuredGrid* ug;
  vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(&ug);

  vtkNew<vtkDataSetSurfaceFilter> filter;
  filter->SetInputData(mg);
  filter->Update();

  vtkIdType numberOfFaces = filter->GetOutput()->GetNumberOfCells();
  mg->Delete();

  filter->SetInputData(ug);
  filter->Update();
  vtkIdType expectedNumberOfFaces = filter->GetOutput()->GetNumberOfCells();
  ug->Delete();

  vtk_assert(expectedNumberOfFaces == numberOfFaces);

  return (rc);
}
