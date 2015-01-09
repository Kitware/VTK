/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageResliceMapperOffAxis.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests off-axis views of 3D images.
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
#include "vtkPlane.h"
#include "vtkImageReader2.h"
#include "vtkOutlineFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"

int TestImageResliceMapperOffAxis(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  renWin->SetMultiSamples(0);
  renWin->Delete();

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

    vtkImageProperty *property = vtkImageProperty::New();
    property->SetColorWindow(2000);
    property->SetColorLevel(1000);
    property->SetAmbient(0.0);
    property->SetDiffuse(1.0);
    property->SetInterpolationTypeToLinear();

    for (int j = 0; j < 3; j++)
      {
      double normal[3];
      normal[0] = 0;
      normal[1] = 0;
      normal[2] = 0;
      normal[j] = 1;

      vtkImageResliceMapper *imageMapper = vtkImageResliceMapper::New();
      imageMapper->SetInputConnection(reader->GetOutputPort());
      imageMapper->GetSlicePlane()->SetNormal(normal);
      imageMapper->SliceAtFocalPointOn();
      imageMapper->BorderOn();
      imageMapper->SetResampleToScreenPixels((i >= 2));

      vtkImageSlice *image = vtkImageSlice::New();
      image->SetProperty(property);
      image->SetMapper(imageMapper);
      imageMapper->Delete();

      vtkOutlineFilter *outline = vtkOutlineFilter::New();
      outline->SetInputConnection(reader->GetOutputPort());

      vtkDataSetMapper *mapper = vtkDataSetMapper::New();
      mapper->SetInputConnection(outline->GetOutputPort());
      outline->Delete();

      vtkActor *actor = vtkActor::New();
      actor->SetMapper(mapper);
      mapper->Delete();

      if (i % 2)
        {
        image->RotateX(10);
        image->RotateY(5);
        actor->RotateX(10);
        actor->RotateY(5);
        }

      renderer->AddViewProp(image);
      renderer->AddViewProp(actor);
      image->Delete();
      actor->Delete();
      }

    property->Delete();

    if (i < 2)
      {
      camera->ParallelProjectionOn();
      }

    camera->Azimuth(10);
    camera->Elevation(-120);
    renderer->ResetCamera();
    camera->Dolly(1.2);
    camera->SetParallelScale(125);
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
