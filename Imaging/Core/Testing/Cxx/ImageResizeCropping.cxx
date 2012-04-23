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
#include "vtkRegressionTestImage.h"

int ImageResizeCropping(int argc, char *argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyle *style = vtkInteractorStyleImage::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  vtkTIFFReader *reader = vtkTIFFReader::New();

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

  vtkOutlineSource *outline = vtkOutlineSource::New();
  outline->SetBounds(10, 149, 50, 199, -1, 1);

  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
  mapper->SetInputConnection(outline->GetOutputPort());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  for (int i = 0; i < 4; i++)
    {
    vtkImageResize *resize = vtkImageResize::New();
    resize->SetNumberOfThreads(1);
    resize->SetInputConnection(reader->GetOutputPort());
    resize->SetOutputDimensions(256, 256, 1);
    if ((i & 1) == 1)
      {
      resize->CroppingOn();
      resize->SetCroppingRegion(cropping[i]);
      }

    vtkImageSliceMapper *imageMapper = vtkImageSliceMapper::New();
    imageMapper->SetInputConnection(resize->GetOutputPort());

    if ((i & 2) == 2)
      {
      resize->BorderOn();
      imageMapper->BorderOn();
      }

    vtkImageSlice *image = vtkImageSlice::New();
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5*(range[0] + range[1]));

    vtkRenderer *renderer = vtkRenderer::New();
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

    imageMapper->Delete();
    renderer->Delete();
    image->Delete();
    resize->Delete();
    }

  renWin->SetSize(512,512);

  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }
  iren->Delete();

  reader->Delete();
  outline->Delete();
  mapper->Delete();
  actor->Delete();

  return !retVal;
}
