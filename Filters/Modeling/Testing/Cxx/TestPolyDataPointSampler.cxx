/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataPointSampler.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

// If WRITE_RESULT is defined, the result of the surface filter is saved.
//#define WRITE_RESULT

#include "vtkActor.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataPointSampler.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkStripper.h"
#include "vtkProperty.h"
#include "vtkCamera.h"

int TestPolyDataPointSampler(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create a generating polydata
  vtkSphereSource *ss = vtkSphereSource::New();
  ss->SetPhiResolution(25);
  ss->SetThetaResolution(38);
  ss->SetCenter(4.5, 5.5, 5.0);
  ss->SetRadius(2.5);

  // Create multiple samplers to test different parts of the algorithm
  vtkPolyDataPointSampler *sampler = vtkPolyDataPointSampler::New();
  sampler->SetInputConnection(ss->GetOutputPort());
  sampler->SetDistance(0.05);
  sampler->GenerateInteriorPointsOn();

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(sampler->GetOutputPort());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkStripper *stripper = vtkStripper::New();
  stripper->SetInputConnection(ss->GetOutputPort());

  vtkPolyDataPointSampler *sampler2 = vtkPolyDataPointSampler::New();
  sampler2->SetInputConnection(stripper->GetOutputPort());
  sampler2->SetDistance(0.05);
  sampler2->GenerateInteriorPointsOn();

  vtkPolyDataMapper *mapper2 = vtkPolyDataMapper::New();
  mapper2->SetInputConnection(sampler2->GetOutputPort());

  vtkActor *actor2 = vtkActor::New();
  actor2->SetMapper(mapper2);
  actor2->AddPosition(5.5,0,0);
  actor2->GetProperty()->SetColor(0,1,0);

  // Add actors
  renderer->AddActor(actor);
  renderer->AddActor(actor2);

  // Standard testing code.
  renWin->SetSize(500,250);
  renWin->Render();
  renderer->GetActiveCamera()->Zoom(2);
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();

  ss->Delete();
  sampler->Delete();
  stripper->Delete();
  sampler2->Delete();
  mapper->Delete();
  mapper2->Delete();
  actor->Delete();
  actor2->Delete();

  return !retVal;
}
