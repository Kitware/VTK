// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test that the AMReX grid reader correctly handles plotfiles with ghost cells.
// The test data is a 2-level AMReX plotfile (8x8x16 at L0, 16x16x16 at L1)
// written with nGrow=2 ghost cells.

#include "vtkAMReXGridReader.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

int TestAMReXGridReaderGhostCells(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMReX/GhostCells");
  vtkNew<vtkAMReXGridReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  reader->SetMaxLevel(1);
  reader->UpdateInformation();

  // Check that variables were found
  int numArrays = reader->GetNumberOfCellArrays();
  if (numArrays == 0)
  {
    vtkLogF(ERROR, "No cell arrays found in plotfile.");
    return EXIT_FAILURE;
  }

  // Enable density
  reader->SetCellArrayStatus("density", 1);
  reader->Update();

  auto* amr = reader->GetOutput();
  if (!amr)
  {
    vtkLogF(ERROR, "Output is null.");
    return EXIT_FAILURE;
  }

  // Check number of levels
  unsigned int numLevels = amr->GetNumberOfLevels();
  if (numLevels != 2)
  {
    vtkLogF(ERROR, "Expected 2 levels, got %u.", numLevels);
    return EXIT_FAILURE;
  }

  // Check Level 0: 2 blocks
  unsigned int numBlocksL0 = amr->GetNumberOfBlocks(0);
  if (numBlocksL0 != 2)
  {
    vtkLogF(ERROR, "Expected 2 blocks at level 0, got %u.", numBlocksL0);
    return EXIT_FAILURE;
  }

  // Check Level 1: 4 blocks
  unsigned int numBlocksL1 = amr->GetNumberOfBlocks(1);
  if (numBlocksL1 != 4)
  {
    vtkLogF(ERROR, "Expected 4 blocks at level 1, got %u.", numBlocksL1);
    return EXIT_FAILURE;
  }

  // Verify ghost cell handling on a Level 0 block
  // Each block has 8x8x8 valid cells + 2 ghost on each side = 12x12x12 = 1728 cells
  auto* ds = amr->GetDataSet(0, 0);
  if (!ds)
  {
    vtkLogF(ERROR, "Level 0 block 0 is null.");
    return EXIT_FAILURE;
  }

  // Check total cell count (12x12x12 = 1728)
  vtkIdType expectedCells = 12 * 12 * 12;
  if (ds->GetNumberOfCells() != expectedCells)
  {
    vtkLogF(
      ERROR, "Expected %lld cells at L0 block 0, got %lld.", expectedCells, ds->GetNumberOfCells());
    return EXIT_FAILURE;
  }

  // Check extent: should be (-2, 10, -2, 10, -2, 10)
  int extent[6];
  ds->GetExtent(extent);
  if (extent[0] != -2 || extent[1] != 10 || extent[2] != -2 || extent[3] != 10 || extent[4] != -2 ||
    extent[5] != 10)
  {
    vtkLogF(ERROR, "Unexpected extent at L0 block 0: (%d,%d,%d,%d,%d,%d)", extent[0], extent[1],
      extent[2], extent[3], extent[4], extent[5]);
    return EXIT_FAILURE;
  }

  // Check vtkGhostType array exists
  auto* ghostArray = vtkUnsignedCharArray::SafeDownCast(
    ds->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
  if (!ghostArray)
  {
    vtkLogF(ERROR, "No vtkGhostType array found at L0 block 0.");
    return EXIT_FAILURE;
  }

  // Count ghost vs valid cells
  int numGhost = 0;
  int numValid = 0;
  for (vtkIdType i = 0; i < ghostArray->GetNumberOfTuples(); ++i)
  {
    if (ghostArray->GetValue(i) & vtkDataSetAttributes::DUPLICATECELL)
    {
      numGhost++;
    }
    else
    {
      numValid++;
    }
  }

  // Valid cells should be 8x8x8 = 512
  int expectedValid = 8 * 8 * 8;
  int expectedGhost = expectedCells - expectedValid;
  if (numValid != expectedValid)
  {
    vtkLogF(ERROR, "Expected %d valid cells, got %d.", expectedValid, numValid);
    return EXIT_FAILURE;
  }
  if (numGhost != expectedGhost)
  {
    vtkLogF(ERROR, "Expected %d ghost cells, got %d.", expectedGhost, numGhost);
    return EXIT_FAILURE;
  }

  // Check that density data was loaded
  if (!ds->GetCellData()->GetArray("density"))
  {
    vtkLogF(ERROR, "Density array not found at L0 block 0.");
    return EXIT_FAILURE;
  }

  // Verify Level 1 block has correct structure too
  auto* dsL1 = amr->GetDataSet(1, 0);
  if (!dsL1)
  {
    vtkLogF(ERROR, "Level 1 block 0 is null.");
    return EXIT_FAILURE;
  }
  if (dsL1->GetNumberOfCells() != expectedCells)
  {
    vtkLogF(ERROR, "Expected %lld cells at L1 block 0, got %lld.", expectedCells,
      dsL1->GetNumberOfCells());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
