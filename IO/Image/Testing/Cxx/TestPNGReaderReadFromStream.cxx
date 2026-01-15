// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFileResourceStream.h"
#include "vtkImageData.h"
#include "vtkImageViewer.h"
#include "vtkPNGReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <fstream>
#include <vector>

#include <iostream>

int TestPNGReaderReadFromStream(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <png file>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  // Open the file
  vtkNew<vtkFileResourceStream> stream;
  if (!stream->Open(filename.c_str()))
  {
    std::cerr << "Could not open file " << filename << std::endl;
  }

  // Initialize reader
  vtkNew<vtkPNGReader> pngReader;

  // Check the image can be read
  if (!pngReader->CanReadFile(stream))
  {
    std::cerr << "CanReadFile failed for stream\n";
    return EXIT_FAILURE;
  }

  pngReader->SetStream(stream);

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
