// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkBMPReader
// .SECTION Description
//

#include "vtkBMPReader.h"
#include "vtkFileResourceStream.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkImageViewer.h"
#include "vtkLookupTable.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

#include <iostream>

int TestBMPReaderStream(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <bmp file>" << std::endl;
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
  vtkNew<vtkBMPReader> bmpReader;
  bmpReader->SetStream(stream);
  bmpReader->Allow8BitBMPOn();
  bmpReader->Update();

  // Visualize
  vtkSmartPointer<vtkImageMapToColors> map = vtkSmartPointer<vtkImageMapToColors>::New();
  map->SetInputConnection(bmpReader->GetOutputPort());
  map->SetLookupTable(bmpReader->GetLookupTable());
  map->SetOutputFormatToRGB();

  vtkSmartPointer<vtkImageViewer> imageViewer = vtkSmartPointer<vtkImageViewer>::New();
  imageViewer->SetInputConnection(map->GetOutputPort());
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
