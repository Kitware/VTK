// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkBMPReader
// .SECTION Description
//

#include "vtkSmartPointer.h"

#include "vtkBMPReader.h"

#include "vtkImageData.h"
#include "vtkImageViewer2.h"
#include "vtkLookupTable.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <iostream>

int TestBMPReaderDoNotAllow8BitBMP(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <bmp file>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  vtkSmartPointer<vtkBMPReader> BMPReader = vtkSmartPointer<vtkBMPReader>::New();

  // Check the image can be read
  if (!BMPReader->CanReadFile(filename.c_str()))
  {
    std::cerr << "CanReadFile failed for " << filename << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  BMPReader->SetFileName(filename.c_str());
  BMPReader->Update();

  // Read and display the image properties
  int depth = BMPReader->GetDepth();
  std::cout << "depth: " << depth << std::endl;

  const char* fileExtensions = BMPReader->GetFileExtensions();
  std::cout << "fileExtensions: " << fileExtensions << std::endl;

  const char* descriptiveName = BMPReader->GetDescriptiveName();
  std::cout << "descriptiveName: " << *descriptiveName << std::endl;

  vtkSmartPointer<vtkLookupTable> lookupTable = BMPReader->GetLookupTable();
  lookupTable->Print(std::cout);

  const unsigned char* colors = BMPReader->GetColors();
  unsigned char const* first = reinterpret_cast<unsigned char*>(&colors);
  unsigned char const* last = reinterpret_cast<unsigned char*>(&colors + 1);
  std::cout << "colors: ";
  while (first != last)
  {
    std::cout << (int)*first << ' ';
    ++first;
  }
  std::cout << std::endl;

  int allow8BitBMP = 0;
  BMPReader->SetAllow8BitBMP(allow8BitBMP);
  std::cout << "allow8BitBMP: " << BMPReader->GetAllow8BitBMP() << std::endl;

  // Visualize
  vtkSmartPointer<vtkImageViewer2> imageViewer = vtkSmartPointer<vtkImageViewer2>::New();
  imageViewer->SetInputConnection(BMPReader->GetOutputPort());
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  imageViewer->SetupInteractor(renderWindowInteractor);
  imageViewer->Render();
  imageViewer->GetRenderer()->ResetCamera();
  renderWindowInteractor->Initialize();
  imageViewer->Render();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
