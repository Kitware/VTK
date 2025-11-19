// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageData.h"
#include "vtkImageViewer.h"
#include "vtkPNGReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <iostream>

int TestPNGReader(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <png file>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  vtkNew<vtkPNGReader> pngReader;

  // Check the image can be read
  if (!pngReader->CanReadFile(filename.c_str()))
  {
    std::cerr << "CanReadFile failed for " << filename << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  pngReader->SetFileName(filename.c_str());
  pngReader->Update();

  // Read and display the image properties
  const char* fileExtensions = pngReader->GetFileExtensions();
  std::cout << "File extensions: " << fileExtensions << std::endl;

  const char* descriptiveName = pngReader->GetDescriptiveName();
  std::cout << "Descriptive name: " << descriptiveName << std::endl;

  // Visualize
  vtkNew<vtkImageViewer> imageViewer;
  imageViewer->SetInputConnection(pngReader->GetOutputPort());
  imageViewer->SetColorWindow(256);
  imageViewer->SetColorLevel(127.5);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  imageViewer->SetupInteractor(renderWindowInteractor);
  imageViewer->Render();

  vtkRenderWindow* renWin = imageViewer->GetRenderWindow();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
