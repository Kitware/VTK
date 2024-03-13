// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridVisibleLeavesVolume.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLHyperTreeGridReader.h"

namespace
{
// Volume value only depends on its depth in the tree.
// Going down a level divides the volume of the cell by 8 in 3 dimensions.
std::array<double, 4> expectedVolumes{ 1000.0, 125.0, 15.625, 1.953125 };

/**
 * Return true if the expected validity value for currentId corresponds to the actual cell
 * value.
 */
bool CheckCellValidity(double expectedValidity, vtkIdType currentId, vtkHyperTreeGrid* outputHTG)
{
  vtkDataArray* visibilityField =
    vtkDataArray::SafeDownCast(outputHTG->GetCellData()->GetArray("Valid"));
  if (expectedValidity != visibilityField->GetTuple1(currentId))
  {
    std::cerr << "Cell id " << currentId << " expected validity is " << expectedValidity
              << " but got " << visibilityField->GetTuple1(currentId) << std::endl;
    return false;
  }
  return true;
}

/**
 * Return true if the expected volume value for currentId corresponds to the actual cell
 * value.
 */
bool CheckVolume(vtkIdType currentId, vtkHyperTreeGrid* outputHTG)
{
  vtkDataArray* volumeField = vtkDataArray::SafeDownCast(outputHTG->GetCellData()->GetArray("Vol"));
  vtkDataArray* depthField =
    vtkDataArray::SafeDownCast(outputHTG->GetCellData()->GetArray("Depth"));
  if (::expectedVolumes[depthField->GetTuple1(currentId)] != volumeField->GetTuple1(currentId))
  {
    std::cerr << "Cell id " << currentId << " expected volume is " << std::setprecision(15)
              << ::expectedVolumes[depthField->GetTuple1(currentId)] << " but got "
              << volumeField->GetTuple1(currentId) << " instead." << std::endl;
    return false;
  }
  return true;
}

/**
 * Return true if the cell validity and volume fields correspond to expected values for the root
 * tree pointed by the cursor.
 */
bool CheckTree(vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkHyperTreeGrid* outputHTG)
{
  vtkIdType currentId = cursor->GetGlobalNodeIndex();

  double expectedValidity = 0.0;
  if (cursor->IsLeaf() && !cursor->IsMasked() && !outputHTG->GetGhostCells()->GetTuple1(currentId))
  {
    // Cell is only tagged as valid if it is a non-masked, non-ghost leaf cell
    expectedValidity = 1.0;
  }

  // Verify cell field values
  if (!::CheckCellValidity(expectedValidity, currentId, outputHTG) ||
    !::CheckVolume(currentId, outputHTG))
  {
    return false;
  }

  // Recurse over children
  bool result = true;
  if (!cursor->IsLeaf() && !cursor->IsMasked())
  {
    for (int child = 0; child < cursor->GetNumberOfChildren(); ++child)
    {
      cursor->ToChild(child);
      result &= ::CheckTree(cursor, outputHTG);
      cursor->ToParent();
    }
  }

  return result;
}
}

int TestHyperTreeGridVisibleLeavesVolume(int argc, char* argv[])
{
  // Read HTG file containing ghost cells
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  std::string ghostFile{ vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/ghost.htg") };
  reader->SetFileName(ghostFile.c_str());

  // Append a mask
  reader->Update();
  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());
  vtkNew<vtkBitArray> maskArray;
  maskArray->SetNumberOfTuples(inputHTG->GetNumberOfCells());
  maskArray->SetTuple1(371, 1); // Mask leaf cell (depth = 2)
  maskArray->SetTuple1(372, 1); // Mask refined cell (depth = 2)
  inputHTG->SetMask(maskArray);

  // Compute visible leaves volume
  vtkNew<vtkHyperTreeGridVisibleLeavesVolume> leavesFilter;
  leavesFilter->SetCellVolumeArrayName("Vol");
  leavesFilter->SetValidCellArrayName("Valid");
  leavesFilter->SetInputConnection(reader->GetOutputPort());
  leavesFilter->Update();
  vtkHyperTreeGrid* leavesVolumeHTG = leavesFilter->GetHyperTreeGridOutput();

  // Iterate over the input tree, and check the output fields
  vtkIdType index = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator iterator;
  leavesVolumeHTG->InitializeTreeIterator(iterator);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> outCursor;
  while (iterator.GetNextTree(index))
  {
    leavesVolumeHTG->InitializeNonOrientedGeometryCursor(outCursor, index);
    if (!::CheckTree(outCursor, leavesVolumeHTG))
    {
      std::cerr << "Node " << index << " failed validation." << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
