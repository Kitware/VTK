// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridThreshold.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkTestUtilities.h"

#include <chrono>

/**
 * Test that all 3 methods for HTG thresholding give an equivalent analytic result
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

  if (!vtkTestUtilities::CompareDataObjects(htgMask, htgCopy))
  {
    std::cerr << "Mask and copy method do not give the same result" << std::endl;
    return EXIT_FAILURE;
  }
  else if (!vtkTestUtilities::CompareDataObjects(htgMask, htgIndex))
  {
    std::cerr << "Mask and copy index array do not give the same result" << std::endl;
    return EXIT_FAILURE;
  }
  else if (!vtkTestUtilities::CompareDataObjects(htgIndex, htgCopy))
  {
    // Technically redundant
    std::cerr << "Index array and copy method do not give the same result" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
