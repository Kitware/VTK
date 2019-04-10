/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPNGReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageData.h"
#include "vtkImageViewer.h"
#include "vtkPNGReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestPNGReader(int argc, char* argv[])
{

  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <png file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  vtkNew<vtkPNGReader> pngReader;

  // Check the image can be read
  if (!pngReader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename.c_str() << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  pngReader->SetFileName(filename.c_str());
  pngReader->Update();

  // Read and display the image properties
  const char* fileExtensions = pngReader->GetFileExtensions();
  cout << "File extensions: " << fileExtensions << endl;

  const char* descriptiveName = pngReader->GetDescriptiveName();
  cout << "Descriptive name: " << descriptiveName << endl;

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
