/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetSurfaceFilterQuadraticTetsGhostCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cstdlib>

#include "vtkNew.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

int TestDataSetSurfaceFilterQuadraticTetsGhostCells(int argc, char* argv[])
{
  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadratic_tets_with_ghost_cells_0.vtu");

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(cfname);
  delete[] cfname;

  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputConnection(reader->GetOutputPort());
  surfaceFilter->Update();

  vtkPolyData* surface = surfaceFilter->GetOutput();
  int numCells = surface->GetNumberOfCells();
  if (numCells != 556)
  {
    std::cerr << "Expected 548 cells, got: " << numCells << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
