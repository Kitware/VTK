/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the vtkImageHistogram class
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
#include "vtkImageHistogram.h"

#include "vtkTestUtilities.h"

int ImageHistogram(int argc, char *argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyle> style;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin.GetPointer());
  iren->SetInteractorStyle(style.GetPointer());

  vtkNew<vtkPNGReader> reader;

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkImageHistogram> histogram;
  histogram->SetInputConnection(reader->GetOutputPort());
  histogram->GenerateHistogramImageOn();
  histogram->SetHistogramImageSize(256,256);
  histogram->SetHistogramImageScaleToSqrt();
  histogram->AutomaticBinningOn();
  histogram->Update();

  vtkIdType nbins = histogram->GetNumberOfBins();
  double range[2];
  range[0] = histogram->GetBinOrigin();
  range[1] = range[0] + (nbins - 1)*histogram->GetBinSpacing();

  for (int i = 0; i < 2; i++)
  {
    vtkNew<vtkRenderer> renderer;
    vtkCamera *camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.0,0.0,0.0);
    renderer->SetViewport(0.5*(i&1), 0.0,
                          0.5 + 0.5*(i&1), 1.0);
    renWin->AddRenderer(renderer.GetPointer());

    vtkNew<vtkImageSliceMapper> imageMapper;
    if ((i & 1) == 0)
    {
      imageMapper->SetInputConnection(reader->GetOutputPort());
    }
    else
    {
      imageMapper->SetInputConnection(histogram->GetOutputPort());
      imageMapper->BorderOn();
    }

    double *bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5*(bounds[0] + bounds[1]);
    point[1] = 0.5*(bounds[2] + bounds[3]);
    point[2] = 0.5*(bounds[4] + bounds[5]);

    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(128);

    vtkNew<vtkImageSlice> image;
    image->SetMapper(imageMapper.GetPointer());

    renderer->AddViewProp(image.GetPointer());

    if ((i & 1) == 0)
    {
      image->GetProperty()->SetColorWindow(range[1] - range[0]);
      image->GetProperty()->SetColorLevel(0.5*(range[0] + range[1]));
    }
    else
    {
      image->GetProperty()->SetInterpolationTypeToNearest();
      image->GetProperty()->SetColorWindow(255.0);
      image->GetProperty()->SetColorLevel(127.5);
    }
  }

  renWin->SetSize(512,256);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
