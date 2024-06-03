// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridThreshold.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"

namespace
{
/**
 * Return true if both trees pointed by cursors represent the same structure,
 * and have matching data
 */
bool CheckTreeEqual(vtkHyperTreeGridNonOrientedGeometryCursor* cursor1,
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor2, vtkDataArray* depth1, vtkDataArray* depth2)
{
  vtkIdType currentId1 = cursor1->GetGlobalNodeIndex();
  vtkIdType currentId2 = cursor2->GetGlobalNodeIndex();

  if (cursor1->IsMasked() != cursor2->IsMasked())
  {
    std::cerr << "Mismatching leaves" << std::endl;
    return false;
  }

  if (cursor1->IsMasked())
  {
    return true;
  }

  if (depth1->GetTuple1(currentId1) != depth2->GetTuple1(currentId2))
  {
    std::cerr << "Depth array value mismatch" << std::endl;
    return false;
  }

  if (cursor1->IsLeaf() != cursor2->IsLeaf())
  {
    std::cerr << "Mismatching leaves" << std::endl;
    return false;
  }

  if (cursor1->IsLeaf())
  {
    return true;
  }

  if (cursor1->GetNumberOfChildren() != cursor2->GetNumberOfChildren())
  {
    std::cerr << "Mismatching number of children" << std::endl;
    return false;
  }

  // Recurse over children
  bool result = true;
  for (int child = 0; child < cursor1->GetNumberOfChildren(); ++child)
  {
    cursor1->ToChild(child);
    cursor2->ToChild(child);
    result &= ::CheckTreeEqual(cursor1, cursor2, depth1, depth2);
    cursor1->ToParent();
    cursor2->ToParent();
  }

  return result;
}
}

/**
 * Test that all 3 methods for thresholding give an equivalent analytic result
 */
int TestHyperTreeGridThresholdMethods(int, char*[])
{
  constexpr double THRESHOLD_MIN = 0.0;
  constexpr double THRESHOLD_MAX = 5.0;

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(3, 3, 3);
  source->SetMaxDepth(5);
  source->SetMaskedFraction(0.2);
  source->SetSeed(3);
  source->SetSplitFraction(0.8);
  source->Update();

  vtkHyperTreeGrid* inputHTG = source->GetHyperTreeGridOutput();
  inputHTG->GetCellData()->SetScalars(inputHTG->GetCellData()->GetArray("Depth"));

  // Threshold using masks
  std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
  vtkNew<vtkHyperTreeGridThreshold> thresholdMask;
  thresholdMask->SetInputConnection(source->GetOutputPort());
  thresholdMask->SetMemoryStrategy(vtkHyperTreeGridThreshold::MaskInput);
  thresholdMask->ThresholdBetween(THRESHOLD_MIN, THRESHOLD_MAX);
  thresholdMask->Update();
  vtkHyperTreeGrid* htgMask = thresholdMask->GetHyperTreeGridOutput();

  std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
  std::cout << "Mask threshold method took " << elapsed_seconds.count() << "s" << std::endl;

  // Threshold using deep copy of structure
  start = std::chrono::system_clock::now();
  vtkNew<vtkHyperTreeGridThreshold> thresholdCopy;
  thresholdCopy->SetInputConnection(source->GetOutputPort());
  thresholdCopy->SetMemoryStrategy(vtkHyperTreeGridThreshold::DeepThreshold);
  thresholdCopy->ThresholdBetween(THRESHOLD_MIN, THRESHOLD_MAX);
  thresholdCopy->Update();
  vtkHyperTreeGrid* htgCopy = thresholdCopy->GetHyperTreeGridOutput();

  elapsed_seconds = std::chrono::system_clock::now() - start;
  std::cout << "Deep copy threshold method took " << elapsed_seconds.count() << "s" << std::endl;

  // Threshold using indexed arrays
  start = std::chrono::system_clock::now();
  vtkNew<vtkHyperTreeGridThreshold> thresholdIndex;
  thresholdIndex->SetInputConnection(source->GetOutputPort());
  thresholdIndex->SetMemoryStrategy(vtkHyperTreeGridThreshold::CopyStructureAndIndexArrays);
  thresholdIndex->ThresholdBetween(THRESHOLD_MIN, THRESHOLD_MAX);
  thresholdIndex->Update();
  vtkHyperTreeGrid* htgIndex = thresholdIndex->GetHyperTreeGridOutput();

  elapsed_seconds = std::chrono::system_clock::now() - start;
  std::cout << "Indexed arrays threshold method took " << elapsed_seconds.count() << "s"
            << std::endl;

  // Check that all methods yield an identical HTG
  vtkDataArray* depthMask = vtkDataArray::SafeDownCast(htgMask->GetCellData()->GetArray("Depth"));
  vtkDataArray* depthCopy = vtkDataArray::SafeDownCast(htgCopy->GetCellData()->GetArray("Depth"));
  vtkDataArray* depthIndex = vtkDataArray::SafeDownCast(htgIndex->GetCellData()->GetArray("Depth"));

  vtkIdType indexMask = 0, indexCopy = 0, indexIndex = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator iteratorMask, iteratorCopy, iteratorIndex;
  htgMask->InitializeTreeIterator(iteratorMask);
  htgCopy->InitializeTreeIterator(iteratorCopy);
  htgIndex->InitializeTreeIterator(iteratorIndex);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursorMask, cursorCopy, cursorIndex;
  while (iteratorMask.GetNextTree(indexMask) && iteratorCopy.GetNextTree(indexCopy) &&
    iteratorIndex.GetNextTree(indexIndex))
  {
    htgMask->InitializeNonOrientedGeometryCursor(cursorMask, indexMask);
    htgCopy->InitializeNonOrientedGeometryCursor(cursorCopy, indexCopy);
    htgIndex->InitializeNonOrientedGeometryCursor(cursorIndex, indexIndex);

    if (!::CheckTreeEqual(cursorMask, cursorCopy, depthMask, depthCopy))
    {
      std::cerr << "Error: Threshold methods mask and copy do not have the same result."
                << std::endl;
      return EXIT_FAILURE;
    }

    if (!::CheckTreeEqual(cursorMask, cursorIndex, depthMask, depthIndex))
    {
      std::cerr << "Error: Threshold methods mask and index do not have the same result."
                << std::endl;
      return EXIT_FAILURE;
    }

    // Technically redundant
    if (!::CheckTreeEqual(cursorIndex, cursorCopy, depthIndex, depthCopy))
    {
      std::cerr << "Error: Threshold methods index and copy do not have the same result."
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
