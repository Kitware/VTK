/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHDRReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHDRReader.h"
#include "vtkImageData.h"
#include "vtkImageViewer.h"
#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHDRReader(int argc, char* argv[])
{
  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <hdr file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  vtkNew<vtkHDRReader> reader;

  // Check the image can be read
  if (!reader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename.c_str() << "\n";
    return EXIT_FAILURE;
  }

  reader->SetFileName(filename.c_str());
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
