// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageData.h"
#include "vtkImageViewer.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTGAReader.h"

#include <iostream>

int TestTGAReader(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <tga file>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  vtkNew<vtkTGAReader> tgaReader;

  // Check the image can be read
  if (!tgaReader->CanReadFile(filename.c_str()))
  {
    std::cerr << "CanReadFile failed for " << filename << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  tgaReader->SetFileName(filename.c_str());
  tgaReader->Update();

  // Read and display the image properties
  const char* fileExtensions = tgaReader->GetFileExtensions();
  std::cout << "File extensions: " << fileExtensions << std::endl;

  const char* descriptiveName = tgaReader->GetDescriptiveName();
  std::cout << "Descriptive name: " << descriptiveName << std::endl;

  // Visualize
  vtkNew<vtkImageViewer> imageViewer;
  imageViewer->SetInputConnection(tgaReader->GetOutputPort());
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
