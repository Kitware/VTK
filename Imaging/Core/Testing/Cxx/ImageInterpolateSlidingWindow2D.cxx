/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageInterpolateSlidingWindow2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the "SlidingWindow" option of the image interpolators
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkSmartPointer.h"

#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageReslice.h"
#include "vtkImageSincInterpolator.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkInteractorStyleImage.h"
#include "vtkPNGReader.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkTestUtilities.h"

int ImageInterpolateSlidingWindow2D(int argc, char* argv[])
{
  auto iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  auto style = vtkSmartPointer<vtkInteractorStyle>::New();
  auto renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  auto reader = vtkSmartPointer<vtkPNGReader>::New();

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  double range[2] = { 0, 4095 };

  for (int i = 0; i < 4; i++)
  {
    // compare results for SlidingWindowOn and SlidingWindowOff
    auto interpolator = vtkSmartPointer<vtkImageSincInterpolator>::New();
    interpolator->SlidingWindowOn();

    auto interpolatorOff = vtkSmartPointer<vtkImageSincInterpolator>::New();
    interpolatorOff->SlidingWindowOff();

    auto reslice = vtkSmartPointer<vtkImageReslice>::New();
    reslice->SetInputConnection(reader->GetOutputPort());
    reslice->SetInterpolator(interpolator);
    reslice->SetOutputScalarType(VTK_DOUBLE);

    auto resliceOff = vtkSmartPointer<vtkImageReslice>::New();
    resliceOff->SetInputConnection(reader->GetOutputPort());
    resliceOff->SetInterpolator(interpolatorOff);
    resliceOff->SetOutputScalarType(VTK_DOUBLE);

    auto imageMapper = vtkSmartPointer<vtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(reslice->GetOutputPort());
    imageMapper->BorderOn();

    // perform stretching and shrinking in x and y directions
    switch (i)
    {
      case 0:
        reslice->SetOutputSpacing(0.7, 0.8, 1.0);
        resliceOff->SetOutputSpacing(0.7, 0.8, 1.0);
        break;
      case 1:
        reslice->SetOutputSpacing(1.0, 0.8, 1.0);
        resliceOff->SetOutputSpacing(1.0, 0.8, 1.0);
        break;
      case 2:
        reslice->SetOutputSpacing(1.7, 1.8, 1.0);
        resliceOff->SetOutputSpacing(1.7, 1.8, 1.0);
        break;
      case 3:
        reslice->SetOutputSpacing(0.7, 1.0, 1.0);
        resliceOff->SetOutputSpacing(0.7, 1.0, 1.0);
        break;
    }

    reslice->Update();
    resliceOff->Update();

    // does "On" give the same results as "Off"?
    vtkDoubleArray* scalars =
      static_cast<vtkDoubleArray*>(reslice->GetOutput()->GetPointData()->GetScalars());
    vtkDoubleArray* scalarsOff =
      static_cast<vtkDoubleArray*>(resliceOff->GetOutput()->GetPointData()->GetScalars());
    double maxdiff = 0.0;
    for (vtkIdType j = 0; j < scalars->GetNumberOfValues(); j++)
    {
      double diff = scalars->GetValue(j) - scalarsOff->GetValue(j);
      maxdiff = (fabs(diff) > fabs(maxdiff) ? diff : maxdiff);
    }
    std::cerr << "Maximum Pixel Error: " << maxdiff << "\n";
    const double tol = 1e-10;
    if (fabs(maxdiff) > tol)
    {
      std::cerr << "Difference is larger than tolerance " << tol << "\n";
      return EXIT_FAILURE;
    }

    auto image = vtkSmartPointer<vtkImageSlice>::New();
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
    image->GetProperty()->SetInterpolationTypeToNearest();

    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddViewProp(image);
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);

    // use center point to set camera
    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);

    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(128);
  }

  renWin->SetSize(512, 512);

  iren->Initialize();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
