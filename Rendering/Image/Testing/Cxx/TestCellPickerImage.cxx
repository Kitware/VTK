/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellPickerImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests picking of images with vtkCellPicker
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkDataSetMapper.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageReader2.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkInteractorStyleImage.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

// A function to point an actor along a vector
void PointCone(vtkActor *actor, double nx, double ny, double nz)
{
  if (nx < 0.0)
  {
    actor->RotateWXYZ(180, 0, 1, 0);
    actor->RotateWXYZ(180, (nx - 1.0)*0.5, ny*0.5, nz*0.5);
  }
  else
  {
    actor->RotateWXYZ(180, (nx + 1.0)*0.5, ny*0.5, nz*0.5);
  }
}

int TestCellPickerImage(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyle *style = vtkInteractorStyleImage::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  vtkImageReader2 *reader = vtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  // use negative spacing to strengthen the testing
  reader->SetDataSpacing(3.2, 3.2, -1.5);
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/headsq/quarter");

  reader->SetFilePrefix(fname);
  reader->Update();
  delete[] fname;

  vtkRenderer *renderers[4];
  for (int i = 0; i < 4; i++)
  {
    vtkRenderer *renderer = vtkRenderer::New();
    renderers[i] = renderer;
    vtkCamera *camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5*(i&1), 0.25*(i&2),
                          0.5 + 0.5*(i&1), 0.5 + 0.25*(i&2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    vtkImageSliceMapper *imageMapper = vtkImageSliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());
    imageMapper->SliceAtFocalPointOn();

    const double *bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5*(bounds[0] + bounds[1]);
    point[1] = 0.5*(bounds[2] + bounds[3]);
    point[2] = 0.5*(bounds[4] + bounds[5]);

    if (i < 3)
    {
      imageMapper->SetOrientation(i);
    }

    point[imageMapper->GetOrientation()] += 30.0;
    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 470.0;
    camera->SetPosition(point);
    camera->SetClippingRange(250, 750);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(120.0);
    if (imageMapper->GetOrientation() != 2)
    {
      camera->SetViewUp(0.0, 0.0, 1.0);
    }

    if (i == 3)
    {
      camera->Azimuth(30);
      camera->Elevation(40);
    }

    vtkImageSlice *image = vtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    renderer->AddViewProp(image);

    image->GetProperty()->SetColorWindow(2000);
    image->GetProperty()->SetColorLevel(1000);

    image->Delete();
  }

  renWin->SetSize(400,400);
  renWin->Render();

  // a cone source that points along the z axis
  vtkConeSource *coneSource = vtkConeSource::New();
  coneSource->CappingOn();
  coneSource->SetHeight(24);
  coneSource->SetRadius(8);
  coneSource->SetResolution(31);
  coneSource->SetCenter(12, 0, 0);
  coneSource->SetDirection(-1, 0, 0);

  vtkCellPicker *picker = vtkCellPicker::New();
  picker->SetTolerance(1e-6);

  const static int pickpos[4][2] = {
    { 120, 90 },
    { 278, 99 },
    { 90, 310 },
    { 250, 260 }
  };

  bool pickSuccess = true;
  for (int i = 0; i < 4; i++)
  {
    vtkRenderer *renderer = renderers[i];

    // Pick the image
    picker->Pick(pickpos[i][0], pickpos[i][1], 0.0, renderer);
    double p[3], n[3];
    picker->GetPickPosition(p);
    picker->GetPickNormal(n);
    if (vtkImageSlice::SafeDownCast(picker->GetProp3D()) == 0)
    {
      cerr << "Pick did not get an image.\n";
      pickSuccess = false;
    }
    if (vtkImageSliceMapper::SafeDownCast(picker->GetMapper()) == 0)
    {
      cerr << "Pick did not get a mapper.\n";
      pickSuccess = false;
    }

    // Draw a cone where the pick occurred
    vtkActor *coneActor = vtkActor::New();
    coneActor->PickableOff();
    vtkDataSetMapper *coneMapper = vtkDataSetMapper::New();
    coneMapper->SetInputConnection(coneSource->GetOutputPort());
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(1, 0, 0);
    coneActor->SetPosition(p[0], p[1], p[2]);
    PointCone(coneActor, n[0], n[1], n[2]);
    renderer->AddViewProp(coneActor);
    coneMapper->Delete();
    coneActor->Delete();
  }

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    iren->Start();
  }
  iren->Delete();

  coneSource->Delete();
  picker->Delete();
  reader->Delete();

  return (!retVal || !pickSuccess);
}
