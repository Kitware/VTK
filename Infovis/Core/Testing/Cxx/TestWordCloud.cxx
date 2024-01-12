// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkSmartPointer.h"
#include "vtkWordCloud.h"

#include "vtkTestUtilities.h"
#include <vtkCamera.h>
#include <vtkImageViewer2.h>
#include <vtkNamedColors.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <iostream>

int TestWordCloud(int argc, char* argv[])
{
  const char* gettysburg = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Gettysburg.txt");
  const char* canterbury = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Canterbury.ttf");

  vtkWordCloud::OffsetDistributionContainer offset;
  offset[0] = 0;
  offset[1] = 0;
  auto wordCloud = vtkSmartPointer<vtkWordCloud>::New();
  wordCloud->SetFileName(gettysburg);
  wordCloud->SetOffsetDistribution(offset);
  wordCloud->SetFontFileName(canterbury);
  wordCloud->AddOrientation(0.0);
  wordCloud->AddOrientation(90.0);
  wordCloud->Update();
  std::cout << "File" << gettysburg << std::endl;
  std::cout << "Font" << canterbury << std::endl;
  std::cout << "Kept Words: " << wordCloud->GetKeptWords().size() << std::endl;
  std::cout << "Stopped Words: " << wordCloud->GetStoppedWords().size() << std::endl;
  std::cout << "Skipped Words: " << wordCloud->GetSkippedWords().size() << std::endl;

  // Display the final image
  auto colors = vtkSmartPointer<vtkNamedColors>::New();
  auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

  auto imageViewer = vtkSmartPointer<vtkImageViewer2>::New();
  imageViewer->SetInputData(wordCloud->GetOutput());
  imageViewer->SetupInteractor(interactor);
  imageViewer->GetRenderer()->SetBackground(colors->GetColor3d("Wheat").GetData());
  imageViewer->SetSize(wordCloud->GetSizes()[0], wordCloud->GetSizes()[1]);
  imageViewer->GetRenderer()->ResetCamera();

  // Zoom in a bit
  vtkCamera* camera = imageViewer->GetRenderer()->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetParallelScale(wordCloud->GetAdjustedSizes()[0] * .4);

  imageViewer->GetRenderWindow()->Render();
  interactor->Start();

  return EXIT_SUCCESS;
}
