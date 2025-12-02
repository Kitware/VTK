// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Check a vtkImageReslice bug that existed from VTK 6.0.0 to 9.5.2 inclusive.
// The following program will show a black image if the bug is present, or a
// white-gray image if the bug is not present.

#include <vtkDataArray.h>
#include <vtkImageActor.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageReslice.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

#include "vtkTestUtilities.h"

#include <iostream>

int ImageResliceGap(int, char*[])
{
  // create a solid input image, two slices thick
  int extent[6] = { 0, 4, 0, 3, 0, 1 };
  vtkNew<vtkImageData> image;
  image->SetDimensions(5, 4, 2);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  image->GetPointData()->GetScalars()->Fill(255);

  // create a transform that shifts slice by just under 0.5, a shift in the
  // range of 0.499993 to 0.499999 will trigger the bug if it is present
  double shift[] = { 1.0, 1.0, 0.499999 };
  vtkNew<vtkTransform> transform;
  transform->Translate(shift);

  // the bug occurs only if there is a filter upstream to vtkImageReslice,
  // we use vtkImageCast because it is one of the simplest filters
  vtkNew<vtkImageCast> upstream;
  upstream->SetInputData(image);
  upstream->SetOutputScalarTypeToShort();

  // create the reslice filter, stream data from the upstream filter
  vtkNew<vtkImageReslice> reslice;
  reslice->SetInputConnection(upstream->GetOutputPort());
  reslice->SetResliceTransform(transform);

  // display the result
  vtkNew<vtkImageActor> actor;
  actor->GetMapper()->SetInputConnection(reslice->GetOutputPort());

  // use a blue background
  vtkNew<vtkRenderer> renderer;
  renderer->AddViewProp(actor);
  renderer->SetBackground(0.2, 0.1, 1.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  // update the pipeline and print information
  // (if the bug is present, the rendered image will be black)
  renderWindow->Render();

  std::cout << "Transform Shift: ";
  std::cout << shift[0] << ", " << shift[1] << ", " << shift[2] << std::endl;

  reslice->GetOutput()->GetExtent(extent);
  std::cout << "Reslice Extent: ";
  std::cout << extent[0] << ", " << extent[1] << ", "       // X
            << extent[2] << ", " << extent[3] << ", "       // Y
            << extent[4] << ", " << extent[5] << std::endl; // Z

  upstream->GetOutput()->GetExtent(extent);
  std::cout << "Upstream Extent: ";
  std::cout << extent[0] << ", " << extent[1] << ", "       // X
            << extent[2] << ", " << extent[3] << ", "       // Y
            << extent[4] << ", " << extent[5] << std::endl; // Z

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
