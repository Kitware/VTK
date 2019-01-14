/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageResize.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the vtkImageResize class
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkSmartPointer.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkPNGReader.h"
#include "vtkImageResize.h"

#include "vtkTestUtilities.h"

int ImageResize(int argc, char *argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkInteractorStyle> style =
    vtkSmartPointer<vtkInteractorStyle>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  vtkSmartPointer<vtkPNGReader> reader =
    vtkSmartPointer<vtkPNGReader>::New();

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  double range[2] = { 0, 4095 };

  for (int i = 0; i < 4; i++)
  {
    vtkSmartPointer<vtkImageResize> resize =
      vtkSmartPointer<vtkImageResize>::New();
    resize->SetInputConnection(reader->GetOutputPort());
    resize->SetOutputDimensions(64, 64, 1);

    vtkSmartPointer<vtkImageSliceMapper> imageMapper =
      vtkSmartPointer<vtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(resize->GetOutputPort());
    imageMapper->BorderOn();

    if ((i & 1) == 0)
    {
      resize->BorderOff();
    }
    else
    {
      resize->BorderOn();
    }

    if ((i & 2) == 0)
    {
      resize->InterpolateOff();
    }
    else
    {
      resize->InterpolateOn();
    }

    vtkSmartPointer<vtkImageSlice> image =
      vtkSmartPointer<vtkImageSlice>::New();
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5*(range[0] + range[1]));
    image->GetProperty()->SetInterpolationTypeToNearest();

    vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
    renderer->AddViewProp(image);
    renderer->SetBackground(0.0,0.0,0.0);
    renderer->SetViewport(0.5*(i&1), 0.25*(i&2),
                          0.5 + 0.5*(i&1), 0.5 + 0.25*(i&2));
    renWin->AddRenderer(renderer);

    // use center point to set camera
    const double *bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5*(bounds[0] + bounds[1]);
    point[1] = 0.5*(bounds[2] + bounds[3]);
    point[2] = 0.5*(bounds[4] + bounds[5]);

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(128);

  }

  renWin->SetSize(512,512);

  iren->Initialize();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
