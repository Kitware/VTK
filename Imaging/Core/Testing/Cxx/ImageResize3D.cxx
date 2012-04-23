/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageResize3D.cxx

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
#include "vtkImageReader2.h"
#include "vtkImageResize.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int ImageResize3D(int argc, char *argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkInteractorStyleImage *style = vtkInteractorStyleImage::New();
  style->SetInteractionModeToImageSlicing();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkImageReader2 *reader = vtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0,63,0,63,1,93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);

  delete [] fname;

  vtkImageResize *resize = vtkImageResize::New();
  resize->SetInputConnection(reader->GetOutputPort());
  resize->SetResizeMethodToOutputSpacing();
  resize->SetOutputSpacing(0.80, 0.80, 1.5);
  resize->InterpolateOn();
  resize->Update();

  vtkImageResize *resize2 = vtkImageResize::New();
  resize2->SetInputConnection(reader->GetOutputPort());
  resize2->SetResizeMethodToMagnificationFactors();
  resize2->SetMagnificationFactors(4, 4, 1);
  resize2->InterpolateOff();

  double range[2] = { 0, 4095 };

  for (int i = 0; i < 4; i++)
    {
    vtkImageSliceMapper *imageMapper = vtkImageSliceMapper::New();
    if (i < 3)
      {
      imageMapper->SetInputConnection(resize->GetOutputPort());
      }
    else
      {
      imageMapper->SetInputConnection(resize2->GetOutputPort());
      }
    imageMapper->SetOrientation(i % 3);
    imageMapper->SliceAtFocalPointOn();

    vtkImageSlice *image = vtkImageSlice::New();
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5*(range[0] + range[1]));
    image->GetProperty()->SetInterpolationTypeToNearest();

    vtkRenderer *renderer = vtkRenderer::New();
    renderer->AddViewProp(image);
    renderer->SetBackground(0.0,0.0,0.0);
    renderer->SetViewport(0.5*(i&1), 0.25*(i&2),
                          0.5 + 0.5*(i&1), 0.5 + 0.25*(i&2));
    renWin->AddRenderer(renderer);

    // use center point to set camera
    double *bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5*(bounds[0] + bounds[1]);
    point[1] = 0.5*(bounds[2] + bounds[3]);
    point[2] = 0.5*(bounds[4] + bounds[5]);

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 500.0;
    camera->SetPosition(point);
    if (imageMapper->GetOrientation() == 2)
      {
      camera->SetViewUp(0.0, 1.0, 0.0);
      }
    else
      {
      camera->SetViewUp(0.0, 0.0, -1.0);
      }
    camera->ParallelProjectionOn();
    camera->SetParallelScale(0.8*128);

    imageMapper->Delete();
    renderer->Delete();
    image->Delete();
    }

  renWin->SetSize(512,512);

  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }
  iren->Delete();

  resize->Delete();
  resize2->Delete();
  reader->Delete();

  return !retVal;
}
