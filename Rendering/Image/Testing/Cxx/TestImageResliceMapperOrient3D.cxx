/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageResliceMapperOrient3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests 3D images that are not in the XY plane.
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkImageReader2.h"

int TestImageResliceMapperOrient3D(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyle *style = vtkInteractorStyleImage::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  vtkImageReader2 *reader = vtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/headsq/quarter");

  reader->SetFilePrefix(fname);
  reader->Update();
  delete[] fname;

  for (int i = 0; i < 4; i++)
  {
    vtkRenderer *renderer = vtkRenderer::New();
    vtkCamera *camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5*(i&1), 0.25*(i&2),
                          0.5 + 0.5*(i&1), 0.5 + 0.25*(i&2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    vtkImageResliceMapper *imageMapper = vtkImageResliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());
    imageMapper->SliceAtFocalPointOn();
    imageMapper->SliceFacesCameraOn();
    // exercise the Streaming options, for better coverage
    imageMapper->StreamingOn();

    const double *bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5*(bounds[0] + bounds[1]);
    point[1] = 0.5*(bounds[2] + bounds[3]);
    point[2] = 0.5*(bounds[4] + bounds[5]);

    camera->SetFocalPoint(point);
    point[i%3] += 500.0;
    camera->SetPosition(point);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(120.0);
    if (i != 2)
    {
      camera->SetViewUp(0.0, 0.0,-1.0);
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
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    iren->Start();
  }
  iren->Delete();

  reader->Delete();

  return !retVal;
}
