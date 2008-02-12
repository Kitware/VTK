/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestUncertaintyTubeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test creates some polylines with uncertainty values.

#include "vtkUncertaintyTubeFilter.h"
#include "vtkPolyData.h"
#include "vtkDelaunay2D.h"
#include "vtkCellArray.h"
#include "vtkShrinkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkPointData.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"

int TestUncertaintyTubeFilter( int argc, char* argv[] )
{
  vtkPoints *newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(10);
  newPts->SetPoint( 0, 10,10,0);
  newPts->SetPoint( 1, 10,10,2);
  newPts->SetPoint( 2, 10,10,4);
  newPts->SetPoint( 3, 10,10,8);
  newPts->SetPoint( 4, 10,10,12);
  newPts->SetPoint( 5, 1,1,2);
  newPts->SetPoint( 6, 1,2,3);
  newPts->SetPoint( 7, 1,4,3);
  newPts->SetPoint( 8, 1,8,4);
  newPts->SetPoint( 9, 1,16,5);

  vtkMath::RandomSeed(1177);
  vtkDoubleArray *s = vtkDoubleArray::New();
  s->SetNumberOfComponents(1);
  s->SetNumberOfTuples(10);
  vtkDoubleArray *v = vtkDoubleArray::New();
  v->SetNumberOfComponents(3);
  v->SetNumberOfTuples(10);
  for (int i=0; i<10; i++)
    {
    s->SetTuple1(i, vtkMath::Random(0,1));
    double x=vtkMath::Random(0.0,2);
    double y=vtkMath::Random(0.0,2);
    double z=vtkMath::Random(0.0,2);
    v->SetTuple3(i,x,y,z);
    }

  vtkCellArray *lines = vtkCellArray::New();
  lines->EstimateSize(2,5);
  lines->InsertNextCell(5);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertCellPoint(4);
  lines->InsertNextCell(5);
  lines->InsertCellPoint(5);
  lines->InsertCellPoint(6);
  lines->InsertCellPoint(7);
  lines->InsertCellPoint(8);
  lines->InsertCellPoint(9);
  
  vtkPolyData *pd = vtkPolyData::New();
  pd->SetPoints(newPts);
  pd->SetLines(lines);
  pd->GetPointData()->SetScalars(s);
  pd->GetPointData()->SetVectors(v);
  newPts->Delete();
  lines->Delete();
  s->Delete();
  v->Delete();

  vtkUncertaintyTubeFilter *utf = vtkUncertaintyTubeFilter::New();
  utf->SetInput(pd);
  utf->SetNumberOfSides(8);

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection( utf->GetOutputPort() );

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor(actor);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren->GetActiveCamera()->SetPosition(1,1,1);
  ren->GetActiveCamera()->SetFocalPoint(0,0,0);
  ren->ResetCamera();

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  pd->Delete();
  utf->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
