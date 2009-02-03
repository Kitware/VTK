/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContourWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test functionality to initialize a contour widget from user supplied
// polydata. Here we will create closed circle and initialize it from that.
#include "vtkContourWidget.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkTestUtilities.h"
#include "vtkCamera.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkMath.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

int TestContourWidget2( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 600);

  vtkOrientedGlyphContourRepresentation *contourRep = vtkOrientedGlyphContourRepresentation::New();
  vtkContourWidget *contourWidget = vtkContourWidget::New();
  contourWidget->SetInteractor(iren);
  contourWidget->SetRepresentation(contourRep);
  contourWidget->On();

  for (int i = 0; i < argc; i++)
    {
    if (strcmp("-Shift", argv[i]) == 0)
      {
      contourWidget->GetEventTranslator()->RemoveTranslation( 
                          vtkCommand::LeftButtonPressEvent );
      contourWidget->GetEventTranslator()->SetTranslation( 
                        vtkCommand::LeftButtonPressEvent,
                        vtkWidgetEvent::Translate );
      }
    else if (strcmp("-Scale", argv[i]) == 0)
      {
      contourWidget->GetEventTranslator()->RemoveTranslation( 
                          vtkCommand::LeftButtonPressEvent );
      contourWidget->GetEventTranslator()->SetTranslation( 
                        vtkCommand::LeftButtonPressEvent,
                        vtkWidgetEvent::Scale );
      }
    }


  vtkPolyData * pd = vtkPolyData::New();

  vtkPoints    *points      = vtkPoints::New();
  vtkCellArray *lines       = vtkCellArray::New();
  vtkIdType    *lineIndices = new vtkIdType[21];
  for (int i = 0; i< 20; i++)
    {
    const double angle = 2.0*vtkMath::Pi()*i/20.0;
    points->InsertPoint(static_cast<vtkIdType>(i), 0.1*cos(angle),
                        0.1*sin(angle), 0.0 );
    lineIndices[i] = static_cast<vtkIdType>(i);
    }

  lineIndices[20] = 0;
  lines->InsertNextCell(21,lineIndices);
  delete [] lineIndices;
  pd->SetPoints(points);
  pd->SetLines(lines);
  points->Delete();
  lines->Delete();

  contourWidget->Initialize(pd);
  contourWidget->Render();
  ren1->ResetCamera();
  renWin->Render();
    
  iren->Initialize();
  
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  pd->Delete();
  contourWidget->Off();
  contourWidget->Delete();
  contourRep->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  
  return !retVal;
}


