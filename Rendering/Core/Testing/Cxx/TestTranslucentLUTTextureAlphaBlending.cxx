/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTranslucentLUTTextureAlphaBlending.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers rendering of an actor with a translucent LUT and alpha
// blending. The mapper uses texture coordinate interpolation and
// 1D texture mapping (better quality than color interpolation).
//
// The result looks wrong (AS EXPECTED) compare to its counterpart using
// depth peeling.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"

#include "vtkImageSinusoidSource.h"
#include "vtkImageData.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkCamera.h"

int TestTranslucentLUTTextureAlphaBlending(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  vtkImageSinusoidSource *imageSource=vtkImageSinusoidSource::New();
  imageSource->SetWholeExtent(0,9,0,9,0,9);
  imageSource->SetPeriod(5);
  imageSource->Update();

  vtkImageData *image=imageSource->GetOutput();
  double range[2];
  image->GetScalarRange(range);

  vtkDataSetSurfaceFilter *surface=vtkDataSetSurfaceFilter::New();

  surface->SetInputConnection(imageSource->GetOutputPort());
  imageSource->Delete();

  vtkPolyDataMapper *mapper=vtkPolyDataMapper::New();
  mapper->SetInputConnection(surface->GetOutputPort());
  surface->Delete();

  vtkLookupTable *lut=vtkLookupTable::New();
  lut->SetTableRange(range);
  lut->SetAlphaRange(0.5,0.5);
  lut->SetHueRange(0.2,0.7);
  lut->SetNumberOfTableValues(256);
  lut->Build();

  mapper->SetScalarVisibility(1);
  mapper->SetLookupTable(lut);
  mapper->UseLookupTableScalarRangeOn();
  mapper->InterpolateScalarsBeforeMappingOn();
  lut->Delete();

  vtkActor *actor=vtkActor::New();
  renderer->AddActor(actor);
  actor->Delete();
  actor->SetMapper(mapper);
  mapper->Delete();

  renderer->SetBackground(0.1,0.3,0.0);
  renWin->SetSize(400,400);
  renWin->Render();
  vtkCamera *camera=renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
