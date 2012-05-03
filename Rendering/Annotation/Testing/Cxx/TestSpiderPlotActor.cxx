/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSpiderPlotActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the spider plot capabilities in VTK.
#include "vtkSpiderPlotActor.h"
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
int TestSpiderPlotActor( int argc, char * argv [] )
{
  int numTuples = 12;

  vtkFloatArray *bitter = vtkFloatArray::New();
  bitter->SetNumberOfTuples(numTuples);

  vtkFloatArray *crispy = vtkFloatArray::New();
  crispy->SetNumberOfTuples(numTuples);

  vtkFloatArray *crunchy = vtkFloatArray::New();
  crunchy->SetNumberOfTuples(numTuples);

  vtkFloatArray *salty = vtkFloatArray::New();
  salty->SetNumberOfTuples(numTuples);

  vtkFloatArray *oily = vtkFloatArray::New();
  oily->SetNumberOfTuples(numTuples);

  for (int i=0; i<numTuples; i++)
    {
    bitter->SetTuple1(i, vtkMath::Random(1,10));
    crispy->SetTuple1(i, vtkMath::Random(-1,1));
    crunchy->SetTuple1(i, vtkMath::Random(1,100));
    salty->SetTuple1(i, vtkMath::Random(0,10));
    oily->SetTuple1(i, vtkMath::Random(5,25));
    }

  vtkDataObject *dobj = vtkDataObject::New();
  dobj->GetFieldData()->AddArray(bitter);
  dobj->GetFieldData()->AddArray(crispy);
  dobj->GetFieldData()->AddArray(crunchy);
  dobj->GetFieldData()->AddArray(salty);
  dobj->GetFieldData()->AddArray(oily);

  vtkSpiderPlotActor *actor = vtkSpiderPlotActor::New();
  actor->SetInputData(dobj);
  actor->SetTitle("Spider Plot");
  actor->SetIndependentVariablesToColumns();
  actor->GetPositionCoordinate()->SetValue(0.05,0.1,0.0);
  actor->GetPosition2Coordinate()->SetValue(0.95,0.85,0.0);
  actor->GetProperty()->SetColor(1,0,0);
  actor->SetAxisLabel(0,"Bitter");
  actor->SetAxisRange(0,1,10);
  actor->SetAxisLabel(1,"Crispy");
  actor->SetAxisRange(1,-1,1);
  actor->SetAxisLabel(2,"Crunchy");
  actor->SetAxisRange(2,1,100);
  actor->SetAxisLabel(3,"Salty");
  actor->SetAxisRange(3,0,10);
  actor->SetAxisLabel(4,"Oily");
  actor->SetAxisRange(4,5,25);
  actor->GetLegendActor()->SetNumberOfEntries(numTuples);
  for (int i=0; i<numTuples; i++)
    {
    double red=vtkMath::Random(0,1);
    double green=vtkMath::Random(0,1);
    double blue=vtkMath::Random(0,1);
    actor->SetPlotColor(i,red,green,blue);
    }
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
  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  bitter->Delete();
  crispy->Delete();
  crunchy->Delete();
  salty->Delete();
  oily->Delete();
  dobj->Delete();
  actor->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
