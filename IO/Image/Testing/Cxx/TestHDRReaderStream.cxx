// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFileResourceStream.h"
#include "vtkHDRReader.h"
#include "vtkImageData.h"
#include "vtkImageViewer.h"
#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <iostream>

int TestHDRReaderStream(int argc, char* argv[])
{
  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <hdr file>" << std::endl;
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
  vtkNew<vtkHDRReader> reader;
  reader->SetStream(stream);
  reader->UpdateInformation();

  // Whole extent
  const int* we = reader->GetDataExtent();
  // Crop the image
  const int extents[6] = { we[0] + we[1] / 5, we[1] - we[1] / 5, we[2] + we[3] / 6,
    we[3] - we[3] / 6, 0, 0 };
  reader->UpdateExtent(extents);
  // Visualize
  vtkNew<vtkImageViewer> imageViewer;
  imageViewer->SetInputData(reader->GetOutput());

  imageViewer->SetColorWindow(1);
  imageViewer->SetColorLevel(1);
  imageViewer->SetPosition(0, 100);

  vtkNew<vtkRenderWindowInteractor> iren;
  imageViewer->SetupInteractor(iren);

  imageViewer->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
