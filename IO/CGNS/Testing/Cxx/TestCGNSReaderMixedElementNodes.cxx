// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReader.h"

#include "vtkCellTypes.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

int TestCGNSReaderMixedElementNodes(int argc, char* argv[])
{
  // Dataset contains canonical HEXA_8 and TRI_3 cells as well as NGon and NFace
  // nodes. Therefore, the main mesh should have the HEXA_8 and NFace cells, while
  // boundaries are made of TRI_3 and NGon cells.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/MixedElementNodes.cgns");
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  // Read cell data
  reader->LoadBndPatchOn();
  reader->UpdateInformation();
  reader->Update();

  vtkMultiBlockDataSet* dataset = reader->GetOutput();

  if (!dataset)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  // Check main 3D mesh
  vtkMultiBlockDataSet* zone = vtkMultiBlockDataSet::SafeDownCast(
    vtkMultiBlockDataSet::SafeDownCast(dataset->GetBlock(0))->GetBlock(0));

  if (!zone)
  {
    std::cerr << "Could not find zone block under base block." << std::endl;
    return EXIT_FAILURE;
  }

  vtkUnstructuredGrid* internal = vtkUnstructuredGrid::SafeDownCast(zone->GetBlock(0));

  // Count number of cells and check cell types
  if (internal->GetNumberOfCells() != 8000)
  {
    std::cerr << "Wrong number of cells in main mesh. Expected 8000 but got "
              << internal->GetNumberOfCells() << "." << std::endl;
    return EXIT_FAILURE;
  }

  if (internal->GetCellType(0) != VTK_HEXAHEDRON)
  {
    std::cerr << "Wrong type of cell in main mesh. Expected VTK_HEXAHEDRON for cell 0 but got "
              << vtkCellTypes::GetClassNameFromTypeId(internal->GetCellType(0)) << "." << std::endl;
    return EXIT_FAILURE;
  }

  if (internal->GetCellType(4000) != VTK_POLYHEDRON)
  {
    std::cerr << "Wrong type of cell in main mesh. Expected VTK_POLYHEDRON for cell 4000 but got "
              << vtkCellTypes::GetClassNameFromTypeId(internal->GetCellType(4000)) << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check 2D boundaries
  vtkMultiBlockDataSet* patches = vtkMultiBlockDataSet::SafeDownCast(zone->GetBlock(1));

  if (patches->GetNumberOfBlocks() != 2)
  {
    std::cerr << "Wrong number of patch blocks. Expected 2 but got " << patches->GetNumberOfBlocks()
              << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Count number of cells for both boundaries
  vtkUnstructuredGrid* patch = vtkUnstructuredGrid::SafeDownCast(patches->GetBlock(0));

  if (patch->GetNumberOfCells() != 10)
  {
    std::cerr << "Wrong number of cells in first patch. Expected 10 but got "
              << patch->GetNumberOfCells() << "." << std::endl;
    return EXIT_FAILURE;
  }

  patch = vtkUnstructuredGrid::SafeDownCast(patches->GetBlock(1));

  if (patch->GetNumberOfCells() != 38)
  {
    std::cerr << "Wrong number of cells in second patch. Expected 38 but got "
              << patch->GetNumberOfCells() << "." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
