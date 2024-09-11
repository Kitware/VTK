// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReader.h"

#include "vtkCellTypes.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

int TestCGNSReaderPatchesAsElementNodes_checkblock(
  vtkMultiBlockDataSet* ds, const std::string name, int expected_ncells)
{
  // find block by name
  vtkSmartPointer<vtkCompositeDataIterator> iter =
    vtkSmartPointer<vtkCompositeDataIterator>::Take(ds->NewIterator());
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal())
  {
    if (name == iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME()))
    {
      break;
    }
    iter->GoToNextItem();
  }
  if (iter->IsDoneWithTraversal())
  {
    std::cerr << "Could not find node named " << name << std::endl;
    return EXIT_FAILURE;
  }

  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(iter->GetCurrentDataObject());

  // Validate number of cells
  if (ug->GetNumberOfCells() != expected_ncells)
  {
    std::cerr << "Wrong number of cells for block " << name << ". Expected " << expected_ncells
              << " but got " << ug->GetNumberOfCells() << "." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int TestCGNSReaderPatchesAsElementNodes(int argc, char* argv[])
{

  struct TestCase
  {
    std::string filename;
    unsigned int nblocks_patches;
    std::vector<std::string> block_names;
    std::vector<int> expected_ncells;

    TestCase(std::string filename_, unsigned int nblocks_patches_,
      std::vector<std::string> block_names_, std::vector<int> expected_ncells_)
      : filename(filename_)
      , nblocks_patches(nblocks_patches_)
      , block_names(block_names_)
      , expected_ncells(expected_ncells_)
    {
    }
  };

  std::vector<TestCase> cases = {
    // block_names and expected_ncells have been extracted using cgsnplot 3.2 that comes with CGNS
    // library 4.4.0
    TestCase("Data/MixedElementNodes.cgns", 2, { "SURFACE_TRIANGLES", "INLAID_MESH_2_FACES" },
      { 3836, 25800 }),
    TestCase("Data/channelBump_solution.cgns", 1, { "Elements_2D" }, { 79314 }),
    TestCase("Data/test_cylinder.cgns", 5,
      { "wall", "quad_inflow", "tri_inflow", "quad_outflow", "tri_outflow" },
      { 2200, 308, 62, 308, 62 }),
    TestCase("Data/EngineSector.cgns", 1, { "CELL_FACES" }, { 12646 }),
    TestCase("Data/Example_nface_n.cgns", 1, { "Elements_2D" }, { 34 }),
    TestCase("Data/Example_ngon_2d_base.cgns", 1, { "Elements_2D" }, { 16512 }),
    TestCase("Data/Example_ngon_pe.cgns", 1, { "Elements_2D" }, { 34 }),
  };

  for (TestCase& tcase : cases)
  {

    std::cout << "Testing " << tcase.filename << std::endl;
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, tcase.filename.c_str());
    vtkNew<vtkCGNSReader> reader;
    reader->SetFileName(fname);
    delete[] fname;

    // Read cell data
    reader->LoadSurfacePatchOn();
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

    // Check 2D boundaries
    vtkMultiBlockDataSet* patches = vtkMultiBlockDataSet::SafeDownCast(zone->GetBlock(1));

    if (patches->GetNumberOfBlocks() != tcase.nblocks_patches)
    {
      std::cerr << "Wrong number of patch blocks. Expected " << tcase.nblocks_patches << " but got "
                << patches->GetNumberOfBlocks() << "." << std::endl;
      return EXIT_FAILURE;
    }
    for (unsigned int i = 0; i < tcase.block_names.size(); i++)
    {
      if (TestCGNSReaderPatchesAsElementNodes_checkblock(
            patches, tcase.block_names[i], tcase.expected_ncells[i]) == EXIT_FAILURE)
      {
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}
