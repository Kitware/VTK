/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetSurfaceFilterWith1DGrids.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>

vtkSmartPointer<vtkDataSet> CreateRectilinearGrid()
{
  vtkSmartPointer<vtkRectilinearGrid> grid = vtkSmartPointer<vtkRectilinearGrid>::New();
  grid->SetDimensions(10, 1, 1);

  vtkSmartPointer<vtkDoubleArray> xArray = vtkSmartPointer<vtkDoubleArray>::New();
  for (int x = 0; x < 10; x++)
  {
    xArray->InsertNextValue(x);
  }

  vtkSmartPointer<vtkDoubleArray> yArray = vtkSmartPointer<vtkDoubleArray>::New();
  yArray->InsertNextValue(0.0);

  vtkSmartPointer<vtkDoubleArray> zArray = vtkSmartPointer<vtkDoubleArray>::New();
  zArray->InsertNextValue(0.0);

  grid->SetXCoordinates(xArray);
  grid->SetYCoordinates(yArray);
  grid->SetZCoordinates(zArray);

  return grid;
}

vtkSmartPointer<vtkDataSet> CreateStructuredGrid()
{
  vtkSmartPointer<vtkStructuredGrid> grid = vtkSmartPointer<vtkStructuredGrid>::New();

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  for (int x = 0; x < 10; x++)
  {
    points->InsertNextPoint(x, 0., 0.);
  }

  // Specify the dimensions of the grid
  grid->SetDimensions(10, 1, 1);
  grid->SetPoints(points);

  return grid;
}

int TestSurfaceFilter(vtkDataSet* grid)
{
  vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  surfaceFilter->SetInputData(grid);
  surfaceFilter->Update();

  vtkPolyData* surface = surfaceFilter->GetOutput();
  int numCells = surface->GetNumberOfCells();
  if (numCells != 9)
  {
    std::cerr << "Expected 9 cells, got: " << numCells << std::endl;
    return EXIT_FAILURE;
  }
  for (int i = 0; i < numCells; i++)
  {
    if (surface->GetCellType(i) != VTK_LINE)
    {
      std::cerr << "Cell " << i << " is not a VTK_LINE!" << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int TestDataSetSurfaceFilterWith1DGrids(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int ret = 0;

  ret |= TestSurfaceFilter(CreateRectilinearGrid());

  ret |= TestSurfaceFilter(CreateStructuredGrid());

  return ret;
}
