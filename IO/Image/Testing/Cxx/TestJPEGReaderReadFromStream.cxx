// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkJPEGReader
// .SECTION Description
//

#include "vtkFileResourceStream.h"
#include "vtkImageData.h"
#include "vtkImageViewer.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtksys/SystemTools.hxx"

#include <vector>

int TestJPEGReaderReadFromStream(int argc, char* argv[])
{

  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <jpeg file>" << endl;
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
  vtkNew<vtkJPEGReader> jpegReader;
  jpegReader->SetStream(stream);

  // Check the image can be read
  if (!jpegReader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  jpegReader->Update();

  // Visualize
  vtkNew<vtkImageViewer> imageViewer;
  imageViewer->SetInputConnection(jpegReader->GetOutputPort());
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
