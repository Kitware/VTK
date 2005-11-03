/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCheckerboardWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkCheckerboardWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkCheckerboardWidget.h"
#include "vtkCheckerboardRepresentation.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkImageWrapPad.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkImageCheckerboard.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

int TestCheckerboardWidget( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create a checkerboard pipeline
  vtkImageCanvasSource2D *image1 = vtkImageCanvasSource2D::New();
  image1->SetNumberOfScalarComponents(3);
  image1->SetScalarTypeToUnsignedChar();
  image1->SetExtent(0,511,0,511,0,0);
  image1->SetDrawColor(255,255,0);
  image1->FillBox(0,511,0,511);

  vtkImageWrapPad *pad1 = vtkImageWrapPad::New();
  pad1->SetInput(image1->GetOutput());
  pad1->SetOutputWholeExtent(0,511,0,511,0,0);

  vtkImageCanvasSource2D *image2 = vtkImageCanvasSource2D::New();
  image2->SetNumberOfScalarComponents(3);
  image2->SetScalarTypeToUnsignedChar();
  image2->SetExtent(0,511,0,511,0,0);
  image2->SetDrawColor(0,255,255);
  image2->FillBox(0,511,0,511);

  vtkImageWrapPad *pad2 = vtkImageWrapPad::New();
  pad2->SetInput(image2->GetOutput());
  pad2->SetOutputWholeExtent(0,511,0,511,0,0);

  vtkImageCheckerboard *checkers = vtkImageCheckerboard::New();
  checkers->SetInput(0,pad1->GetOutput());
  checkers->SetInput(1,pad2->GetOutput());
  checkers->SetNumberOfDivisions(10,6,1);
 
  vtkImageActor *checkerboardActor = vtkImageActor::New();
  checkerboardActor->SetInput(checkers->GetOutput());

  // VTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene 
  // (i.e., matters pertaining to geometry).
  vtkCheckerboardRepresentation *rep = vtkCheckerboardRepresentation::New();
  rep->SetImageActor(checkerboardActor);
  rep->SetCheckerboard(checkers);

  vtkCheckerboardWidget *checkerboardWidget = vtkCheckerboardWidget::New();
  checkerboardWidget->SetInteractor(iren);
  checkerboardWidget->SetRepresentation(rep);

  // Add the actors to the renderer, set the background and size
  ren1->AddActor(checkerboardActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
  recorder->SetFileName("c:/record.log");
//  recorder->Record();
//  recorder->ReadFromInputStringOn();
//  recorder->SetInputString(eventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
//  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  image1->Delete();
  pad1->Delete();
  image2->Delete();
  pad2->Delete();
  checkers->Delete();
  checkerboardActor->Delete();
  checkerboardWidget->Off();
  checkerboardWidget->Delete();
  rep->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();
  
  return !retVal;

}


