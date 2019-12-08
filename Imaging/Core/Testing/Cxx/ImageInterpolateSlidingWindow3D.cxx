/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageInterpolateSlidingWindow3D.cxx

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
#include "vtkImageInterpolator.h"
#include "vtkImageProperty.h"
#include "vtkImageReader2.h"
#include "vtkImageReslice.h"
#include "vtkImageSincInterpolator.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkInteractorStyleImage.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkTestUtilities.h"

int ImageInterpolateSlidingWindow3D(int argc, char* argv[])
{
  auto iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  auto style = vtkSmartPointer<vtkInteractorStyleImage>::New();
  style->SetInteractionModeToImageSlicing();
  auto renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkSmartPointer<vtkImageReader2> reader = vtkSmartPointer<vtkImageReader2>::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);

  delete[] fname;

  auto interpolator = vtkSmartPointer<vtkImageSincInterpolator>::New();
  interpolator->SlidingWindowOn();

  auto reslice = vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInputConnection(reader->GetOutputPort());
  // slightly modify Z spacing to force slice interpolation
  reslice->SetOutputSpacing(0.80, 0.80, 1.5001);
  reslice->SetInterpolator(interpolator);
  reslice->SetOutputScalarType(VTK_DOUBLE);
  reslice->Update();

  // repeat with SlidingWindowOff for comparison
  auto interpolatorOff = vtkSmartPointer<vtkImageSincInterpolator>::New();
  interpolatorOff->SlidingWindowOff();

  auto resliceOff = vtkSmartPointer<vtkImageReslice>::New();
  resliceOff->SetInputConnection(reader->GetOutputPort());
  resliceOff->SetOutputSpacing(reslice->GetOutputSpacing());
  resliceOff->SetInterpolator(interpolatorOff);
  resliceOff->SetOutputScalarType(VTK_DOUBLE);
  resliceOff->Update();

  // compare SlidingWindowOn against SlidingWindowOff
  vtkDoubleArray* scalars =
    static_cast<vtkDoubleArray*>(reslice->GetOutput()->GetPointData()->GetScalars());
  vtkDoubleArray* scalarsOff =
    static_cast<vtkDoubleArray*>(resliceOff->GetOutput()->GetPointData()->GetScalars());
  double maxdiff = 0.0;
  for (vtkIdType j = 0; j < scalars->GetNumberOfTuples(); j++)
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

  // also check that "no interpolation" works
  auto nearest = vtkSmartPointer<vtkImageInterpolator>::New();
  nearest->SetInterpolationModeToNearest();
  nearest->SlidingWindowOn();

  auto reslice2 = vtkSmartPointer<vtkImageReslice>::New();
  reslice2->SetInputConnection(reader->GetOutputPort());
  reslice2->SetOutputSpacing(0.80, 0.80, 1.5);
  // force type conversion to avoid vtkImageReslice fast path,
  // which would 'optimize away' the interpolator
  reslice2->SetOutputScalarType(VTK_FLOAT);
  reslice2->SetInterpolator(nearest);
  reslice2->Update();

  double range[2] = { 0, 4095 };

  for (int i = 0; i < 4; i++)
  {
    auto imageMapper = vtkSmartPointer<vtkImageSliceMapper>::New();
    if (i < 3)
    {
      imageMapper->SetInputConnection(reslice->GetOutputPort());
    }
    else
    {
      imageMapper->SetInputConnection(reslice2->GetOutputPort());
    }
    imageMapper->SetOrientation(i % 3);
    imageMapper->SliceAtFocalPointOn();

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
    if (imageMapper->GetOrientation() == 2)
    {
      camera->SetViewUp(0.0, 1.0, 0.0);
    }
    else
    {
      camera->SetViewUp(0.0, 0.0, -1.0);
    }
    camera->ParallelProjectionOn();
    camera->SetParallelScale(0.8 * 128);
  }

  renWin->SetSize(512, 512);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
