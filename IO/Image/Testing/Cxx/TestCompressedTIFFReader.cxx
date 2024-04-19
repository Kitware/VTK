// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkSmartPointer.h>

#include <vtkImageData.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTIFFReader.h>

int TestCompressedTIFFReader(int argc, char* argv[])
{
  // Verify input arguments
  if (argc < 3)
  {
    std::cout << "Usage: " << argv[0] << " Filename(.tif)" << std::endl;
    return EXIT_FAILURE;
  }

  // Read the image
  vtkSmartPointer<vtkTIFFReader> reader = vtkSmartPointer<vtkTIFFReader>::New();
  reader->SetFileName(argv[1]);
  reader->UpdateInformation();

  // Read the image in 4 chunks. This exercises the logic to read random scan
  // lines from files that do not support it.
  const int maxNumPieces = 4;
  for (int cc = 0; cc < maxNumPieces; cc++)
  {
    reader->UpdatePiece(cc, maxNumPieces, 0);
  }
  reader->UpdateWholeExtent();

  // Visualize
  vtkSmartPointer<vtkImageViewer2> imageViewer = vtkSmartPointer<vtkImageViewer2>::New();
  imageViewer->SetInputConnection(reader->GetOutputPort());
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
