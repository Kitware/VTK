// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridOrientedCursor.h"
#include "vtkHyperTreeGridVisibleLeavesSize.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"
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

/**
 * Test the filter with ghost and masked cells.
 * Verify cell validity and expected volumes on a uniform HTG
 */
bool TestMaskGhostSizes(int argc, char* argv[])
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
  vtkNew<vtkHyperTreeGridVisibleLeavesSize> leavesFilter;
  leavesFilter->SetCellSizeArrayName("Vol");
  leavesFilter->SetValidCellArrayName("Valid");
  leavesFilter->SetInputData(inputHTG);
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
      return false;
    }
  }

  return true;
}

/**
 * Create a HTG with more than 256 levels. When the HTG has more than 256 different cell volumes
 * (which is the case for >256 levels or when manually editing HT scales) the internal cell size
 * structure changes, not using an implicit indexed array anymore. This test covers this case.
 */
bool TestDifferentVolumes()
{
  constexpr int MAX_DEPTH = 280;

  // Create a pseudo-random HTG
  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(3, 3, 3);
  source->SetOutputBounds(-10, 10, -10, 10, -10, 10);
  source->SetSplitFraction(0.1);
  source->SetMaskedFraction(0.3);
  source->SetSeed(0);
  source->Update();

  // Refine the first cell until we have at least 256 levels
  vtkHyperTreeGrid* inputHTG = source->GetHyperTreeGridOutput();
  inputHTG->SetMask(nullptr);
  vtkNew<vtkHyperTreeGridOrientedCursor> cursor;
  inputHTG->SetDepthLimiter(MAX_DEPTH);
  inputHTG->InitializeOrientedCursor(cursor, 0);
  cursor->SetGlobalIndexStart(inputHTG->GetNumberOfCells());
  std::vector<int> levelIds(MAX_DEPTH, 0);
  for (int i = 0; i < MAX_DEPTH; i++)
  {
    cursor->SubdivideLeaf();
    cursor->ToChild(0);
    levelIds[i] = cursor->GetGlobalNodeIndex();
  }

  // Apply our filter
  vtkNew<vtkHyperTreeGridVisibleLeavesSize> leavesFilter;
  leavesFilter->SetInputData(inputHTG);
  leavesFilter->Update();
  vtkHyperTreeGrid* outputHTG = leavesFilter->GetHyperTreeGridOutput();

  // Check volume values
  vtkDataArray* volumeField =
    vtkDataArray::SafeDownCast(outputHTG->GetCellData()->GetArray("CellSize"));
  double expectedVolumeValue = 1000.0;

  std::array<double, 2> sizeRange;
  volumeField->GetRange(sizeRange.data());
  if (sizeRange[0] != 0.0 || sizeRange[1] != expectedVolumeValue)
  {
    std::cerr << "Range is [" << sizeRange[0] << ":" << sizeRange[1]
              << "] but expected [0.0:" << expectedVolumeValue << "]" << std::endl;
    return false;
  }
  for (int i = 0; i < MAX_DEPTH; i++)
  {
    expectedVolumeValue /= 8.0;
    if (volumeField->GetTuple1(levelIds[i]) != expectedVolumeValue)
    {
      std::cerr << "Cell id " << levelIds[i] << " expected volume is " << std::setprecision(15)
                << expectedVolumeValue << " but got " << volumeField->GetTuple1(levelIds[i])
                << std::endl;
      return false;
    }
  }

  return true;
}

}

int TestHyperTreeGridVisibleLeavesSize(int argc, char* argv[])
{
  bool result = true;
  result &= ::TestMaskGhostSizes(argc, argv);
  result &= ::TestDifferentVolumes();

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
