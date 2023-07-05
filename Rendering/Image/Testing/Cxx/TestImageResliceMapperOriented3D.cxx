// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests oriented images (images with non-identity for Direction)
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageReader2.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageSlice.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

int TestImageResliceMapperOriented3D(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyle> style;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  vtkNew<vtkImageReader2> reader;
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  // compute a direction matrix for testing
  vtkNew<vtkTransform> trans;
  trans->RotateWXYZ(20, 0.7071067811865476, 0.0, 0.7071067811865476);
  double direction[9];
  for (int i = 0; i < 3; i++)
  {
    double column[3] = { 0.0, 0.0, 0.0 };
    column[i] = 1.0;
    trans->TransformVector(column, column);
    direction[i] = column[0];
    direction[3 + i] = column[1];
    direction[6 + i] = column[2];
  }
  reader->SetDataDirection(direction);
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  reader->SetFilePrefix(fname);
  reader->Update();
  delete[] fname;

  for (int i = 0; i < 4; i++)
  {
    vtkRenderer* renderer = vtkRenderer::New();
    vtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    vtkNew<vtkImageResliceMapper> imageMapper;
    imageMapper->SetInputConnection(reader->GetOutputPort());
    imageMapper->SliceAtFocalPointOn();
    imageMapper->SliceFacesCameraOn();

    if (i == 0 || i == 1)
    {
      // test the code path that uses textures for 2D interpolation
      imageMapper->ResampleToScreenPixelsOff();
    }
    else
    {
      // test the code path that uses vtkImageReslice for all interpolation
      imageMapper->ResampleToScreenPixelsOn();
    }

    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);

    camera->SetFocalPoint(point);
    point[i % 3] += 500.0;
    camera->SetPosition(point);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(120.0);

    if (i != 2)
    {
      camera->SetViewUp(0.0, 0.0, -1.0);
    }

    if (i == 3)
    {
      camera->Azimuth(30);
      camera->Elevation(40);
    }

    vtkNew<vtkImageSlice> image;
    image->SetMapper(imageMapper);
    renderer->AddViewProp(image);

    image->GetProperty()->SetColorWindow(2000);
    image->GetProperty()->SetColorLevel(1000);
  }

  renWin->SetSize(400, 400);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
