// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkLogger.h>

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGenerateFields.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridOrientedCursor.h"
#include "vtkHyperTreeGridSource.h"
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
    vtkLogF(ERROR, "Cell id %" VTK_ID_TYPE_PRId " expected validity is %.1f but got %.1f\n",
      currentId, expectedValidity, visibilityField->GetTuple1(currentId));
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
    vtkLogF(ERROR, "Cell id %" VTK_ID_TYPE_PRId " expected volume is %f but got %f instead.\n",
      currentId, ::expectedVolumes[depthField->GetTuple1(currentId)],
      volumeField->GetTuple1(currentId));
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
  vtkNew<vtkHyperTreeGridGenerateFields> generateFields;
  generateFields->SetCellSizeArrayName("Vol");
  generateFields->SetValidCellArrayName("Valid");
  generateFields->SetInputData(inputHTG);
  generateFields->Update();
  vtkHyperTreeGrid* leavesVolumeHTG = generateFields->GetHyperTreeGridOutput();

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
      vtkLogF(ERROR, "Node %" VTK_ID_TYPE_PRId " failed validation.\n", index);
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
  cursor->SetGlobalIndexStart(inputHTG->GetNumberOfCells() - 1);
  std::vector<int> levelIds(MAX_DEPTH, 0);
  for (int i = 0; i < MAX_DEPTH; i++)
  {
    cursor->SubdivideLeaf();
    cursor->ToChild(0);
    levelIds[i] = cursor->GetGlobalNodeIndex();
  }

  // Apply our filter
  vtkNew<vtkHyperTreeGridGenerateFields> generateFields;
  generateFields->SetInputData(inputHTG);
  generateFields->Update();
  vtkHyperTreeGrid* outputHTG = generateFields->GetHyperTreeGridOutput();

  // Check volume values
  vtkDataArray* volumeField =
    vtkDataArray::SafeDownCast(outputHTG->GetCellData()->GetArray("CellSize"));
  double expectedVolumeValue = 1000.0;

  std::array<double, 2> sizeRange;
  volumeField->GetRange(sizeRange.data());
  if (sizeRange[0] != 0.0 || sizeRange[1] != expectedVolumeValue)
  {
    vtkLogF(ERROR, "Range is [%f:%f] but expected [0.0:%f]\n", sizeRange[0], sizeRange[1],
      expectedVolumeValue);
    return false;
  }
  for (int i = 0; i < MAX_DEPTH; i++)
  {
    expectedVolumeValue /= 8.0;
    if (volumeField->GetTuple1(levelIds[i]) != expectedVolumeValue)
    {
      vtkLogF(ERROR, "Cell id %d expected volume is %f but got %f\n", levelIds[i],
        expectedVolumeValue, volumeField->GetTuple1(levelIds[i]));
      return false;
    }
  }

  return true;
}

bool TestTotalVolume()
{
  // Create a HTG
  vtkNew<vtkHyperTreeGridSource> source;
  source->SetDimensions(3, 4, 1);
  source->SetMaxDepth(2);
  source->SetDescriptor("RRRRR.|.... .... .... .... ....");
  source->Update();

  // Apply our filter
  vtkNew<vtkHyperTreeGridGenerateFields> generateFields;
  generateFields->SetInputConnection(source->GetOutputPort());
  generateFields->Update();
  vtkHyperTreeGrid* outputHTG = generateFields->GetHyperTreeGridOutput();

  double totalVisibleVolume =
    vtkDoubleArray::SafeDownCast(outputHTG->GetFieldData()->GetAbstractArray("TotalVisibleVolume"))
      ->GetTuple1(0);
  if (totalVisibleVolume != 6.0)
  {
    vtkLogF(ERROR, "Total visible volume is %f but expected 6.0\n", totalVisibleVolume);
    return false;
  }

  source->UseMaskOn();
  source->SetMask("111111|1110 1111 1111 1111 1111");
  generateFields->Update();

  totalVisibleVolume =
    vtkDoubleArray::SafeDownCast(outputHTG->GetFieldData()->GetAbstractArray("TotalVisibleVolume"))
      ->GetTuple1(0);
  if (totalVisibleVolume != 5.75)
  {
    vtkLogF(ERROR, "Total visible volume is %f but expected 5.75\n", totalVisibleVolume);
    return false;
  }

  return true;
}

bool TestCellCenter()
{
  // Create a HTG
  vtkNew<vtkHyperTreeGridSource> source;
  source->SetDimensions(3, 4, 1);
  source->SetMaxDepth(2);
  source->SetDescriptor("RRRRR.|.... .... .... .... ....");
  source->UseMaskOn();
  source->SetMask("111111|1110 1111 1111 1111 1111");
  source->Update();

  // Apply our filter
  vtkNew<vtkHyperTreeGridGenerateFields> generateFields;
  generateFields->SetInputConnection(source->GetOutputPort());
  generateFields->Update();
  vtkHyperTreeGrid* outputHTG = generateFields->GetHyperTreeGridOutput();

  auto cellCenterArray =
    vtkDoubleArray::SafeDownCast(outputHTG->GetCellData()->GetAbstractArray("CellCenter"));

  double* pt = cellCenterArray->GetTuple3(8);
  if (pt[0] != 0.25 || pt[1] != 0.75)
  {
    vtkLogF(ERROR, "CellCenter is %f %f but expected 0.25 0.75\n", pt[0], pt[1]);
    return false;
  }

  pt = cellCenterArray->GetTuple3(5);
  if (pt[0] != 1.5 || pt[1] != 2.5)
  {
    vtkLogF(ERROR, "CellCenter is %f %f but expected 1.5 2.5\n", pt[0], pt[1]);
    return false;
  }

  // CellCenter should be computed even for coarse cells
  pt = cellCenterArray->GetTuple3(0);
  if (pt[0] != 0.5 || pt[1] != 0.5)
  {
    vtkLogF(ERROR, "CellCenter is %f %f but expected 0.5 0.5\n", pt[0], pt[1]);
    return false;
  }

  // CellCenter should NOT be computed for masked cells
  pt = cellCenterArray->GetTuple3(9);
  if (pt[0] != 0.0 || pt[1] != 0.0)
  {
    vtkLogF(ERROR, "CellCenter should not be computed for masked cells\n");
    return false;
  }

  return true;
}

bool TestValidCell()
{
  // Create a HTG
  vtkNew<vtkHyperTreeGridSource> source;
  source->SetDimensions(3, 4, 1);
  source->SetMaxDepth(2);
  source->SetDescriptor("RRRRR.|.... .... .... .... ....");
  source->UseMaskOn();
  source->SetMask("111111|1110 1111 1111 1111 1111");
  source->Update();

  // Apply our filter
  vtkNew<vtkHyperTreeGridGenerateFields> generateFields;
  generateFields->SetInputConnection(source->GetOutputPort());
  generateFields->Update();
  vtkHyperTreeGrid* outputHTG = generateFields->GetHyperTreeGridOutput();

  auto validCellArray =
    vtkBitArray::SafeDownCast(outputHTG->GetCellData()->GetAbstractArray("ValidCell"));

  if (validCellArray->GetTuple1(8) == 0)
  {
    vtkLogF(ERROR, "Unmasked leaf should be valid");
    return false;
  }

  if (validCellArray->GetTuple1(9) == 1)
  {
    vtkLogF(ERROR, "Masked cell should be invalid");
    return false;
  }

  return true;
}

// Check if array `arrayName` existence matches `shouldExist` in htg cell or field data depending on
// `isFieldData`.
bool CheckArray(std::string arrayName, vtkHyperTreeGrid* htg, bool shouldExist, bool isFieldData)
{
  if ((isFieldData &&
        htg->GetFieldData()->HasArray(arrayName.c_str()) !=
          static_cast<vtkTypeBool>(shouldExist)) ||
    (!isFieldData &&
      htg->GetCellData()->HasArray(arrayName.c_str()) != static_cast<vtkTypeBool>(shouldExist)))
  {
    if (shouldExist)
    {
      vtkLogF(ERROR, "Missing array %s in htg %s\n", arrayName.c_str(),
        isFieldData ? "field data" : "cell data");
    }
    else
    {
      vtkLogF(ERROR, "Array %s should exist in %s\n", arrayName.c_str(),
        isFieldData ? "field data" : "cell data");
    }
    return false;
  }
  return true;
}

bool TestArrayDisabling()
{
  // Create a pseudo-random HTG
  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(3, 3, 3);
  source->SetOutputBounds(-10, 10, -10, 10, -10, 10);
  source->SetSplitFraction(0.5);
  source->SetMaskedFraction(0);
  source->SetSeed(0);
  source->Update();

  // Apply our filter
  vtkNew<vtkHyperTreeGridGenerateFields> generateFields;
  generateFields->SetInputConnection(source->GetOutputPort());
  generateFields->Update();
  vtkHyperTreeGrid* outputHTG = generateFields->GetHyperTreeGridOutput();

  bool valid = true;
  valid &= CheckArray("ValidCell", outputHTG, true, false);
  valid &= CheckArray("CellSize", outputHTG, true, false);
  valid &= CheckArray("CellCenter", outputHTG, true, false);
  valid &= CheckArray("TotalVisibleVolume", outputHTG, true, true);

  if (!valid)
  {
    return false;
  }

  generateFields->ComputeCellCenterArrayOff();
  generateFields->Update();

  valid = true;
  valid &= CheckArray("ValidCell", outputHTG, true, false);
  valid &= CheckArray("CellSize", outputHTG, true, false);
  valid &= CheckArray("CellCenter", outputHTG, false, false);
  valid &= CheckArray("TotalVisibleVolume", outputHTG, true, true);

  if (!valid)
  {
    return false;
  }

  generateFields->ComputeValidCellArrayOff();
  generateFields->Update();

  valid = true;
  valid &= CheckArray("ValidCell", outputHTG, false, false);
  valid &= CheckArray("CellSize", outputHTG, true, false);
  valid &= CheckArray("CellCenter", outputHTG, false, false);
  valid &= CheckArray("TotalVisibleVolume", outputHTG, false, true);

  if (!valid)
  {
    return false;
  }

  return true;
}

}

int TestHyperTreeGridGenerateFields(int argc, char* argv[])
{
  bool result = true;
  result &= ::TestMaskGhostSizes(argc, argv);
  result &= ::TestDifferentVolumes();
  result &= ::TestTotalVolume();
  result &= ::TestCellCenter();
  result &= ::TestArrayDisabling();
  result &= ::TestValidCell();

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
