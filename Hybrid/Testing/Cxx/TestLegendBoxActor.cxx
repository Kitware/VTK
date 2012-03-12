/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegendScaleActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the terrain annotation capabilities in VTK.
#include "vtkCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLegendBoxActor.h"
#include "vtkLineSource.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestLegendBoxActor( int argc, char * argv [] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  ren1->GetActiveCamera()->ParallelProjectionOn();

  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Colors.
  double textColor[5][3]     = {{1.0, 0.0, 0.0},
                                {0.0, 1.0, 0.0},
                                {0.0, 0.0, 1.0},
                                {1.0, 0.5, 0.5},
                                {0.5, 1.0, 0.5}};

  double backgroundColor[3]  = {0.8, 0.5, 0.0};

  const char *text[5] = {"Text1",
                         "Text2",
                         "Text3",
                         "Text4",
                         "Text5"};

  // Create the actor
  vtkSmartPointer<vtkLegendBoxActor> actor = vtkSmartPointer<vtkLegendBoxActor>::New();
  actor->SetNumberOfEntries(5);
  actor->SetUseBackground(1);
  actor->SetBackgroundColor(backgroundColor);
  actor->SetBackgroundOpacity(1.0);

  actor->GetPositionCoordinate()->SetCoordinateSystemToView();
  actor->GetPositionCoordinate()->SetValue(-0.7, -0.8);

  actor->GetPosition2Coordinate()->SetCoordinateSystemToView();
  actor->GetPosition2Coordinate()->SetValue(0.7, 0.8);

  // Create a test pipeline
  //
  for(int i=0; i < 5; ++i)
    {
    vtkSmartPointer<vtkLineSource> ls (vtkSmartPointer<vtkLineSource>::New());
    ls->Update();
    vtkSmartPointer<vtkPolyData> pd = ls->GetOutput();
    actor->SetEntry(i, pd, text[i], textColor[i]);
    }

  // Add the actors to the renderer, set the background and size
  ren1->AddViewProp(actor);
  ren1->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(350, 350);

  // render the image
  //
  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
