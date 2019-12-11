/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPNGReaderReadFromMemory.cxx

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
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <fstream>
#include <vector>

int TestPNGReaderReadFromMemory(int argc, char* argv[])
{

  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <png file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  // Open the file
  vtksys::ifstream stream(filename.c_str(), std::ios::in | std::ios::binary);
  if (!stream.is_open())
  {
    std::cerr << "Could not open file " << filename.c_str() << std::endl;
  }
  // Get file size
  unsigned long len = vtksys::SystemTools::FileLength(filename);

  // Load the file into a buffer
  std::vector<char> buffer = std::vector<char>(std::istreambuf_iterator<char>(stream), {});

  // Initialize reader
  vtkNew<vtkPNGReader> pngReader;
  pngReader->SetMemoryBuffer(buffer.data());
  pngReader->SetMemoryBufferLength(len);

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
