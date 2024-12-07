// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkImageBlend.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkInteractorStyle.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

int ImageBlend(int, char*[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyle> style;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  int dims[3] = { 256, 256, 4 };
  double spacing[3] = { 1, 1, 1 };
  double origin[3] = { 0, 0, 0 };
  vtkNew<vtkImageData> imageData1;
  imageData1->SetDimensions(dims);
  imageData1->SetSpacing(spacing);
  imageData1->SetOrigin(origin);
  imageData1->AllocateScalars(VTK_UNSIGNED_SHORT, 4);

  for (int x = 0; x < dims[0]; ++x)
  {
    for (int y = 0; y < dims[1]; ++y)
    {
      if (x < dims[0] / 2)
      {
        double val = abs((dims[0] - x - 100) * (dims[1] - y - 100));
        imageData1->SetScalarComponentFromFloat(x, y, 0, 0, val);
        imageData1->SetScalarComponentFromFloat(x, y, 0, 1, val);
        imageData1->SetScalarComponentFromFloat(x, y, 0, 2, val);
        imageData1->SetScalarComponentFromFloat(x, y, 0, 3, val);
      }
      else
      {
        imageData1->SetScalarComponentFromFloat(x, y, 0, 0, 0);
      }
    }
  }

  vtkNew<vtkImageData> imageData2;
  imageData2->SetDimensions(dims);
  imageData2->SetSpacing(spacing);
  imageData2->SetOrigin(origin);
  imageData2->AllocateScalars(VTK_UNSIGNED_SHORT, 4);

  for (int x = 0; x < dims[0]; ++x)
  {
    for (int y = 0; y < dims[1]; ++y)
    {
      if (x > (dims[0] / 2) - 50 && x < (dims[0] / 2) + 50 && y > (dims[1] / 2) - 50 &&
        y < (dims[1] / 2) + 50)
      {
        double val = x * y;
        imageData2->SetScalarComponentFromFloat(x, y, 0, 0, val);
        imageData2->SetScalarComponentFromFloat(x, y, 0, 1, val);
        imageData2->SetScalarComponentFromFloat(x, y, 0, 2, val);
        imageData2->SetScalarComponentFromFloat(x, y, 0, 3, val);
      }
      else
      {
        imageData2->SetScalarComponentFromFloat(x, y, 0, 0, 0);
      }
    }
  }

  vtkNew<vtkImageBlend> blend;
  blend->AddInputData(imageData1);
  blend->AddInputData(imageData2);
  blend->SetOpacity(0, 0.3);
  blend->SetOpacity(1, 0.7);
  blend->SetBlendModeToNormal();
  blend->BlendAlphaOn();

  vtkNew<vtkImageSliceMapper> imageMapper;
  imageMapper->SetInputConnection(blend->GetOutputPort());
  imageMapper->BorderOn();

  vtkNew<vtkImageSlice> imageSlice;
  imageSlice->SetMapper(imageMapper);

  double range[2] = { 0, 4095 };
  imageSlice->GetProperty()->SetColorWindow(range[1] - range[0]);
  imageSlice->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
  imageSlice->GetProperty()->SetInterpolationTypeToNearest();

  vtkNew<vtkRenderer> renderer;
  renderer->AddViewProp(imageSlice);
  renderer->SetBackground(0.0, 0.0, 0.0);
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

  renWin->SetSize(512, 512);

  iren->Initialize();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
