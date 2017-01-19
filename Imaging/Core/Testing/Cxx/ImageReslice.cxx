/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageReslice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the vtkImageReslice class
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkNew.h"

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
#include "vtkTransform.h"
#include "vtkImageReslice.h"

#include "vtkTestUtilities.h"

int ImageReslice(int argc, char *argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyle> style;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin.Get());
  iren->SetInteractorStyle(style.Get());

  vtkNew<vtkPNGReader> reader;

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  double range[2] = { 0.0, 4095.0 };

  vtkNew<vtkTransform> transform;
  transform->RotateZ(25.0);
  transform->Scale(0.9, 0.9, 1.0);

  for (int i = 0; i < 4; i++)
  {
    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputConnection(reader->GetOutputPort());
    reslice->SetOutputSpacing(1.0, 1.0, 1.0);

    if ((i & 1) == 0)
    {
      // Images on the left
      reslice->TransformInputSamplingOff();
    }
    else
    {
      // Images on the right
      reslice->TransformInputSamplingOn();
    }

    if ((i & 2) == 0)
    {
      // Images on the bottom
      reslice->SetResliceAxes(transform->GetMatrix());
    }
    else
    {
      // Images on the top, note that (by design) the ResliceTransform
      // is ignored by TransformInputSampling, unlike the ResliceAxes
      reslice->SetResliceTransform(transform.Get());
    }

    vtkNew<vtkImageSliceMapper> imageMapper;
    imageMapper->SetInputConnection(reslice->GetOutputPort());
    imageMapper->BorderOn();

    vtkNew<vtkImageSlice> image;
    image->SetMapper(imageMapper.Get());

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5*(range[0] + range[1]));
    image->GetProperty()->SetInterpolationTypeToNearest();

    vtkNew<vtkRenderer> renderer;
    renderer->AddViewProp(image.Get());
    renderer->SetBackground(0.0,0.0,0.0);
    renderer->SetViewport(0.5*(i&1), 0.25*(i&2),
                          0.5 + 0.5*(i&1), 0.5 + 0.25*(i&2));
    renWin->AddRenderer(renderer.Get());

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
