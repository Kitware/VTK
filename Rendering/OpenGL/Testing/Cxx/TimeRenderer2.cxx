/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TimeRenderer2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCullerCollection.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStripper.h"
#include "vtkTimerLog.h"
#include "vtkTriangleFilter.h"

int main( int argc, char *argv[] )
{
  // For timings
  int i;
  int RES = 200;
  if (argc > 1)
    {
    RES = atoi(argv[1]);
    }  
  // create a rendering window and both renderers
  vtkRenderer *ren1 = vtkRenderer::New();
  ren1->GetCullers()->InitTraversal();
  //ren1->RemoveCuller(ren1->GetCullers()->GetNextItem());
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  renWindow->AddRenderer(ren1);

  vtkPlaneSource *plane = vtkPlaneSource::New();
  plane->SetResolution(RES,RES);
  
  vtkPolyDataMapper *mapper;
  vtkActor *actor;
  vtkTriangleFilter *tfilter;
  vtkStripper *stripper;

  mapper = vtkPolyDataMapper::New();
  actor = vtkActor::New();
  tfilter = vtkTriangleFilter::New();
  stripper = vtkStripper::New();
  
  tfilter->SetInputConnection(plane->GetOutputPort());
  stripper->SetInputConnection(tfilter->GetOutputPort());
  mapper->SetInputConnection(stripper->GetOutputPort());
  actor->SetMapper(mapper);
  ren1->AddActor(actor);
  
  // set the size of our window
  renWindow->SetSize(500,500);
  
  // set the viewports and background of the renderers
  //  ren1->SetViewport(0,0,0.5,1);
  ren1->SetBackground(0.2,0.3,0.5);

  // draw the resulting scene
  renWindow->Render();
  ren1->GetActiveCamera()->Azimuth(3);
  renWindow->Render();
  // Set up times
  vtkTimerLog *tl = vtkTimerLog::New();
  
  tl->StartTimer();
  
  // do a azimuth of the cameras 3 degrees per iteration
  for (i = 0; i < 360; i += 3) 
    {
    ren1->GetActiveCamera()->Azimuth(3);
    renWindow->Render();
    }

  tl->StopTimer();
  
  cerr << "Wall Time = " << tl->GetElapsedTime() << "\n";
  cerr << "FrameRate = " << 120.0 / tl->GetElapsedTime() << "\n";
  cerr << "TriRate = " << RES*RES*2*120 / tl->GetElapsedTime() << "\n";

  // Clean up
  ren1->Delete();
  renWindow->Delete();
  tl->Delete();
  actor->Delete();
  mapper->Delete();
  stripper->Delete();
  tfilter->Delete();
  plane->Delete();
  return 1;
}
