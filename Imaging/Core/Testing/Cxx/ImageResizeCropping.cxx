/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageResizeCropping.cxx

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
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkDataSetMapper.h"
#include "vtkOutlineSource.h"
#include "vtkTIFFReader.h"
#include "vtkImageResize.h"

#include "vtkTestUtilities.h"

int ImageResizeCropping(int argc, char *argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkInteractorStyle> style =
    vtkSmartPointer<vtkInteractorStyle>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  vtkSmartPointer<vtkTIFFReader> reader =
    vtkSmartPointer<vtkTIFFReader>::New();

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/beach.tif");

  reader->SetFileName(fname);
  reader->SetOrientationType(4);
  delete[] fname;

  double range[2] = { 0, 255 };
  double cropping[4][6] = {
    { 0, 199, 0, 199, 0, 0 },
    { 10, 149, 50, 199, 0, 0 },
    { -0.5, 199.5, -0.5, 199.5, 0, 0 },
    { 9.5, 149.5, 199.5, 49.5, 0, 0 },
  };

  vtkSmartPointer<vtkOutlineSource> outline =
    vtkSmartPointer<vtkOutlineSource>::New();
  outline->SetBounds(10, 149, 50, 199, -1, 1);

  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  for (int i = 0; i < 4; i++)
    {
    vtkSmartPointer<vtkImageResize> resize =
      vtkSmartPointer<vtkImageResize>::New();
    resize->SetNumberOfThreads(1);
    resize->SetInputConnection(reader->GetOutputPort());
    resize->SetOutputDimensions(256, 256, 1);
    if ((i & 1) == 1)
      {
      resize->CroppingOn();
      resize->SetCroppingRegion(cropping[i]);
      }

    vtkSmartPointer<vtkImageSliceMapper> imageMapper =
      vtkSmartPointer<vtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(resize->GetOutputPort());

    if ((i & 2) == 2)
      {
      resize->BorderOn();
      imageMapper->BorderOn();
      }

    vtkSmartPointer<vtkImageSlice> image =
      vtkSmartPointer<vtkImageSlice>::New();
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5*(range[0] + range[1]));

    vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
    renderer->AddViewProp(image);
    if (i == 0)
      {
      renderer->AddViewProp(actor);
      }
    renderer->SetBackground(0.0,0.0,0.0);
    renderer->SetViewport(0.5*(i&1), 0.25*(i&2),
                          0.5 + 0.5*(i&1), 0.5 + 0.25*(i&2));
    renWin->AddRenderer(renderer);

    double point[3] = { 99.5, 99.5, 0.0 };

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->SetFocalPoint(point);
    point[2] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(100);

    }

  renWin->SetSize(512,512);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
