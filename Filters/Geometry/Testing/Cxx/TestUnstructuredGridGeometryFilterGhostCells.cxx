/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestUnstructuredGridGeometryFilterGhostCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cstdlib>

#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridGeometryFilter.h"
#include "vtkXMLUnstructuredGridReader.h"

int TestUnstructuredGridGeometryFilterGhostCells(int argc, char* argv[])
{
  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ghost_cells.vtu");

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(cfname);
  delete[] cfname;

  // Test default parameters
  vtkNew<vtkUnstructuredGridGeometryFilter> ugridFilter;
  ugridFilter->SetInputConnection(reader->GetOutputPort());
  ugridFilter->Update();

  vtkUnstructuredGrid* ugrid =
    vtkUnstructuredGrid::SafeDownCast(ugridFilter->GetOutput());
  int withoutGhostCells = ugrid->GetNumberOfCells();
  if (withoutGhostCells != 4)
  {
    std::cerr << "Expected 4 cells with ghost cell clipping on, got "
              << withoutGhostCells << std::endl;
    return EXIT_FAILURE;
  }

  // Test passing through duplicate ghost cells
  ugridFilter->DuplicateGhostCellClippingOff();
  ugridFilter->Update();

  ugrid = vtkUnstructuredGrid::SafeDownCast(ugridFilter->GetOutput());
  int withGhostCells = ugrid->GetNumberOfCells();
  if (withGhostCells != 8)
  {
    std::cerr << "Expected 8 cells with ghost cell clipping off, got "
              << withGhostCells << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
