// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridThreshold.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"

int TestHyperTreeGridThresholdMethods(int argc, char* argv[])
{
  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(3, 3, 3);
  source->SetMaxDepth(8);
  // source->SetMaskedFraction(0.2);
  source->SetSeed(3);
  source->SetSplitFraction(0.8);
  source->Update();

  vtkHyperTreeGrid* inputHTG = source->GetHyperTreeGridOutput();
  inputHTG->GetCellData()->SetScalars(inputHTG->GetCellData()->GetArray("Depth"));

  // Threshold using masks
  std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
  vtkNew<vtkHyperTreeGridThreshold> threshold;
  threshold->SetInputConnection(source->GetOutputPort());
  threshold->SetMemoryStrategy(vtkHyperTreeGridThreshold::MaskInput);
  threshold->SetLowerThreshold(1.);
  threshold->SetUpperThreshold(3.);
  threshold->Update();
  vtkHyperTreeGrid* outputHTG = threshold->GetHyperTreeGridOutput();

  std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
  std::cout << "Mask threshold method took " << elapsed_seconds.count() << "s" << std::endl;

  // Threshold using deep copy of structure
  start = std::chrono::system_clock::now();
  vtkNew<vtkHyperTreeGridThreshold> threshold2;
  threshold2->SetInputConnection(source->GetOutputPort());
  threshold2->SetMemoryStrategy(vtkHyperTreeGridThreshold::DeepThreshold);
  threshold2->SetLowerThreshold(1.);
  threshold2->SetUpperThreshold(3.);
  threshold2->Update();
  outputHTG = threshold->GetHyperTreeGridOutput();

  elapsed_seconds = std::chrono::system_clock::now() - start;
  std::cout << "Deep copy threshold method took " << elapsed_seconds.count() << "s" << std::endl;

  // Threshold using indexed arrays
  start = std::chrono::system_clock::now();
  vtkNew<vtkHyperTreeGridThreshold> threshold3;
  threshold3->SetInputConnection(source->GetOutputPort());
  threshold3->SetMemoryStrategy(vtkHyperTreeGridThreshold::CopyStructureAndIndexArrays);
  threshold3->SetLowerThreshold(1.);
  threshold3->SetUpperThreshold(3.);
  threshold3->Update();
  outputHTG = threshold3->GetHyperTreeGridOutput();

  elapsed_seconds = std::chrono::system_clock::now() - start;
  std::cout << "Indexed arrays threshold method took " << elapsed_seconds.count() << "s"
            << std::endl;

  return EXIT_SUCCESS;
}
