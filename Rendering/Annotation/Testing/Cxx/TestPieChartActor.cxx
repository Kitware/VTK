/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPieChartActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the spider plot capabilities in VTK.
#include "vtkPieChartActor.h"
#include "vtkFloatArray.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkMath.h"
#include "vtkTextProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkProperty2D.h"
#include "vtkLegendBoxActor.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestPieChartActor( int argc, char * argv [] )
{
  int numTuples = 6;

  vtkFloatArray *bitter = vtkFloatArray::New();
  bitter->SetNumberOfTuples(numTuples);

  for (int i=0; i<numTuples; i++)
  {
    bitter->SetTuple1(i, vtkMath::Random(1,100));
  }

  vtkDataObject *dobj = vtkDataObject::New();
  dobj->GetFieldData()->AddArray(bitter);

  vtkPieChartActor *actor = vtkPieChartActor::New();
  actor->SetInputData(dobj);
  actor->SetTitle("Pie Chart");
  actor->GetPositionCoordinate()->SetValue(0.05,0.1,0.0);
  actor->GetPosition2Coordinate()->SetValue(0.95,0.85,0.0);
  actor->GetProperty()->SetColor(0.1,0.1,0.1);
  actor->GetLegendActor()->SetNumberOfEntries(numTuples);
  for (int i=0; i<numTuples; i++)
  {
    double red=vtkMath::Random(0,1);
    double green=vtkMath::Random(0,1);
    double blue=vtkMath::Random(0,1);
    actor->SetPieceColor(i,red,green,blue);
  }
  actor->SetPieceLabel(0,"oil");
  actor->SetPieceLabel(1,"gas");
  actor->SetPieceLabel(2,"water");
  actor->SetPieceLabel(3,"snake oil");
  actor->SetPieceLabel(4,"tequila");
  actor->SetPieceLabel(5,"beer");
  actor->LegendVisibilityOn();

  // Set text colors (same as actor for backward compat with test)
  actor->GetTitleTextProperty()->SetColor(1,1,0);
  actor->GetLabelTextProperty()->SetColor(1,0,0);

  // Create the RenderWindow, Renderer and both Actors
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren1->AddActor(actor);
  ren1->SetBackground(0,0,0);
  renWin->SetSize(500,200);

  // render the image
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  bitter->Delete();
  dobj->Delete();
  actor->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
