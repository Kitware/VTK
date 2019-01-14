/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageSliceMapperOrient2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests 2D images that are not in the XY plane.
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
#include "vtkImageSliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkImagePermute.h"
#include "vtkPNGReader.h"

#include "vtkImageMapper.h"
#include "vtkActor2D.h"

int TestImageSliceMapperOrient2D(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyle *style = vtkInteractorStyleImage::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  vtkPNGReader *reader = vtkPNGReader::New();
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);
  reader->SetDataSpacing(0.9, 0.9, 1.0);

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/vtk.png");

  reader->SetFileName(fname);
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

    vtkImageSliceMapper *imageMapper = vtkImageSliceMapper::New();

    if (i == 0 || i == 1)
    {
      vtkImagePermute *permute = vtkImagePermute::New();
      permute->SetInputConnection(reader->GetOutputPort());
      permute->SetFilteredAxes((2-i)%3, (3-i)%3, (4-i)%3);
      imageMapper->SetInputConnection(permute->GetOutputPort());
      permute->Delete();
      imageMapper->SetOrientation(i);
    }
    else
    {
      imageMapper->SetInputConnection(reader->GetOutputPort());
    }

    const double *bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5*(bounds[0] + bounds[1]);
    point[1] = 0.5*(bounds[2] + bounds[3]);
    point[2] = 0.5*(bounds[4] + bounds[5]);

    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 1.0;
    camera->SetPosition(point);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(120.0);
    if (imageMapper->GetOrientation() == 1)
    {
      camera->SetViewUp(1.0, 0.0, 0.0);
    }
    else if (imageMapper->GetOrientation() == 0)
    {
      camera->SetViewUp(0.0, 0.0, 1.0);
    }

    vtkImageSlice *image = vtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    renderer->AddViewProp(image);

    if (i == 3)
    {
      image->GetProperty()->SetColorWindow(127.5);
    }

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
