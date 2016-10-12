/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageStack.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the vtkImageStack class for image layering.
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkImageStack.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkPlane.h"
#include "vtkImageReader2.h"
#include "vtkOutlineFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"

int TestImageStack(int argc, char* argv[])
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

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/headsq/quarter");

  vtkLookupTable *table = vtkLookupTable::New();
  table->SetValueRange(0.0, 1.0);
  table->SetSaturationRange(1.0, 1.0);
  table->SetHueRange(0.0, 0.0);
  table->SetAlphaRange(0.0, 1.0);
  table->SetRampToLinear();
  table->Build();

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
    property->SetLayerNumber(0);

    vtkImageProperty *property2 = vtkImageProperty::New();
    property2->SetColorWindow(2000);
    property2->SetColorLevel(1000);
    property2->SetAmbient(0.0);
    property2->SetDiffuse(1.0);
    property2->SetLookupTable(table);
    property2->SetInterpolationTypeToLinear();
    property2->SetLayerNumber(1);
    property2->BackingOn();

    if (i < 2)
    {
      property2->CheckerboardOn();
      property2->SetCheckerboardSpacing(25.0, 25.0);
    }


    for (int j = 0; j < 3; j++)
    {
      double normal[3];
      normal[0] = 0;
      normal[1] = 0;
      normal[2] = 0;
      normal[j] = 1;

      vtkImageMapper3D *imageMapper;
      vtkImageMapper3D *imageMapper2;

      if (i % 2)
      {
        vtkImageResliceMapper *imageRMapper = vtkImageResliceMapper::New();
        vtkImageResliceMapper *imageRMapper2 = vtkImageResliceMapper::New();
        imageRMapper->GetSlicePlane()->SetNormal(normal);
        imageRMapper2->GetSlicePlane()->SetNormal(normal);
        imageMapper = imageRMapper;
        imageMapper2 = imageRMapper2;
      }
      else
      {
        vtkImageSliceMapper *imageSMapper = vtkImageSliceMapper::New();
        vtkImageSliceMapper *imageSMapper2 = vtkImageSliceMapper::New();
        imageSMapper->SetOrientation(j);
        imageSMapper2->SetOrientation(j);
        imageMapper = imageSMapper;
        imageMapper2 = imageSMapper2;
      }

      imageMapper->SetInputConnection(reader->GetOutputPort());
      imageMapper->SliceAtFocalPointOn();
      imageMapper->BorderOn();

      imageMapper2->SetInputConnection(reader->GetOutputPort());
      imageMapper2->SliceAtFocalPointOn();
      imageMapper2->BorderOn();

      vtkImageSlice *image = vtkImageSlice::New();
      image->SetProperty(property);
      image->SetMapper(imageMapper);
      imageMapper->Delete();

      vtkImageSlice *image2 = vtkImageSlice::New();
      image2->SetProperty(property2);
      image2->SetMapper(imageMapper2);
      imageMapper2->Delete();

      vtkImageStack *imageStack = vtkImageStack::New();
      imageStack->AddImage(image2);
      imageStack->AddImage(image);
      imageStack->SetActiveLayer(1);
      image->Delete();
      image2->Delete();

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
      if (i < 2)
      {
        imageStack->RotateY(-5);
        imageStack->RotateX(-10);
        actor->RotateY(-5);
        actor->RotateX(-10);
      }

      renderer->AddViewProp(imageStack);
      renderer->AddViewProp(actor);
      imageStack->Delete();
      actor->Delete();
    }

    property->Delete();
    property2->Delete();

    camera->ParallelProjectionOn();
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

  table->Delete();
  reader->Delete();

  return !retVal;
}
