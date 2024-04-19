// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkThreshold.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

int TestThresholdComponents(int, char*[])
{
  vtkNew<vtkSphereSource> source;
  source->GenerateNormalsOn();

  vtkNew<vtkThreshold> filter;
  filter->SetInputConnection(source->GetOutputPort());
  filter->SetInputArrayToProcess(0, 0, 0, 0, "Normals");

  filter->SetThresholdFunction(vtkThreshold::THRESHOLD_UPPER);
  filter->SetUpperThreshold(0.0);
  filter->AllScalarsOff();

  filter->SetComponentModeToUseSelected();
  filter->SetSelectedComponent(0);
  filter->Update();

  if (filter->GetOutput()->GetNumberOfCells() != 66)
  {
    std::cerr << "Unexpected cell count after thresholding component X. Got: "
              << filter->GetOutput()->GetNumberOfCells() << ", expected: 66." << std::endl;
    return EXIT_FAILURE;
  }

  filter->SetSelectedComponent(1);
  filter->Update();

  if (filter->GetOutput()->GetNumberOfCells() != 76)
  {
    std::cerr << "Unexpected cell count after thresholding component Y. Got: "
              << filter->GetOutput()->GetNumberOfCells() << ", expected: 76." << std::endl;
    return EXIT_FAILURE;
  }

  filter->SetSelectedComponent(2);
  filter->Update();

  if (filter->GetOutput()->GetNumberOfCells() != 56)
  {
    std::cerr << "Unexpected cell count after thresholding component Z. Got: "
              << filter->GetOutput()->GetNumberOfCells() << ", expected: 56." << std::endl;
    return EXIT_FAILURE;
  }

  filter->SetSelectedComponent(3);
  filter->Update();

  if (filter->GetOutput()->GetNumberOfCells() != 96)
  {
    std::cerr << "Unexpected cell count after thresholding magnitude. Got: "
              << filter->GetOutput()->GetNumberOfCells() << ", expected: 96." << std::endl;
    return EXIT_FAILURE;
  }

  filter->SetComponentModeToUseAll();
  filter->Update();

  if (filter->GetOutput()->GetNumberOfCells() != 31)
  {
    std::cerr << "Unexpected cell count after thresholding all components. Got: "
              << filter->GetOutput()->GetNumberOfCells() << ", expected: 31." << std::endl;
    return EXIT_FAILURE;
  }

  filter->SetComponentModeToUseAny();
  filter->Update();

  if (filter->GetOutput()->GetNumberOfCells() != 92)
  {
    std::cerr << "Unexpected cell count after thresholding any component. Got: "
              << filter->GetOutput()->GetNumberOfCells() << ", expected: 92." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
