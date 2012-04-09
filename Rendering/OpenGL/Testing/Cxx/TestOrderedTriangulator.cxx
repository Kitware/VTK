/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOrderedTriangulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkOrderedTriangulator
// .SECTION Description
// this program tests the class vtkOrderedTriangulator
// It shows the effect of the delaunay criteria compare to an iso parametric
// case where this criteria does not apply.

#include "vtkOrderedTriangulator.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkDataSetMapper.h"
#include "vtkShrinkFilter.h"
#include "vtkRegressionTestImage.h"

//------------------------------------------------------------------------------
double para_coord1[24] = {
  0.0, 0.0, 0.0, //0
  1.0, 0.0, 0.0, //1
  0.0, 1.0, 0.0, //2
  0.5, 0.0, 0.0, //3
  0.0, 0.5, 0.0, //4
  0.0, 0.0, 1.0, //5
};

double para_coord2[24] = {
  0.0, 1.0, 0.0, //0
  0.0, 0.0, 0.0, //1
  1.0, 0.0, 0.0, //2
  0.0, 0.5, 0.0, //3
  0.5, 0.5, 0.0, //4
  0.0, 0.0, 1.0, //5
  };

double points[33] = {
  0.0, 0.0, 0.0, //0
  0.0,-1.0, 0.0, //1
  1.0, 0.0, 0.0, //2
  0.0,-0.5, 0.0, //3
  0.5, 0.0, 0.0, //4
  0.0, 0.0, 1.0, //5
  };

//------------------------------------------------------------------------------
static void isomorphism(double in[3], double out[3])
{
  out[0] = in[0]+in[2];
  out[1] = in[0]+in[1];
  out[2] = in[1]+in[2];
}

//------------------------------------------------------------------------------
int TestOrderedTriangulator(int argc, char *argv[])
{
  int i;
  vtkOrderedTriangulator *triangulator1 = vtkOrderedTriangulator::New();
  triangulator1->InitTriangulation(-1.0,1.0, -1.0,1.0, -1.0,1.0, 200);
  triangulator1->PreSortedOff();

  vtkOrderedTriangulator *triangulator2 = vtkOrderedTriangulator::New();
  triangulator2->InitTriangulation(-1.0,1.0, -1.0,1.0, -1.0,1.0, 200);
  triangulator2->PreSortedOff();

  vtkOrderedTriangulator *isotriangulator1 = vtkOrderedTriangulator::New();
  isotriangulator1->InitTriangulation(-1.0,1.0, -1.0,1.0, -1.0,1.0, 200);
  isotriangulator1->PreSortedOff();

  vtkOrderedTriangulator *isotriangulator2 = vtkOrderedTriangulator::New();
  isotriangulator2->InitTriangulation(-1.0,1.0, -1.0,1.0, -1.0,1.0, 200);
  isotriangulator2->PreSortedOff();

  double *p = points;

  vtkPoints *Points = vtkPoints::New();
  Points->SetNumberOfPoints(6);
  for(i=0;i<6;i++,p+=3)
    {
    Points->SetPoint(i, p);
    }

  //first case
  for(i=0;i<6;i++)
  {
    double *temp = points + 3*i;
    double *temp2 = para_coord1 + 3*i;
    triangulator1->InsertPoint(i, temp, temp2, 0);
  }
  triangulator1->Triangulate();

  //second case:
  for(i=0;i<6;i++)
  {
    double *temp = points + 3*i;
    double *temp2 = para_coord2 + 3*i;
    triangulator2->InsertPoint(i, temp, temp2, 0);
  }
  triangulator2->Triangulate();

  //We now use isocoordinate and repeat case one and two
  //first case
  for(i=0;i<6;i++)
  {
    double *temp = points + 3*i;
    double *temp2 = para_coord1 + 3*i;
    double iso[3];
    isomorphism(temp2, iso);
    isotriangulator1->InsertPoint(i, temp, iso, 0);
  }
  isotriangulator1->Triangulate();

  //second case:
  for(i=0;i<6;i++)
  {
    double *temp = points + 3*i;
    double *temp2 = para_coord2 + 3*i;
    double iso[3];
    isomorphism(temp2, iso);
    isotriangulator2->InsertPoint(i, temp, iso, 0);
  }
  isotriangulator2->Triangulate();

  vtkUnstructuredGrid *aTetraGrid1 = vtkUnstructuredGrid::New();
  aTetraGrid1->Allocate (1, 1);
  triangulator1->AddTetras(0, aTetraGrid1);
  aTetraGrid1->SetPoints(Points);

  vtkUnstructuredGrid *aTetraGrid2 = vtkUnstructuredGrid::New();
  aTetraGrid2->Allocate (1, 1);
  triangulator2->AddTetras(0, aTetraGrid2);
  aTetraGrid2->SetPoints(Points);

  //iso cases:
  vtkUnstructuredGrid *isoTetraGrid1 = vtkUnstructuredGrid::New();
  isoTetraGrid1->Allocate (1, 1);
  isotriangulator1->AddTetras(0, isoTetraGrid1);
  isoTetraGrid1->SetPoints(Points);

  vtkUnstructuredGrid *isoTetraGrid2 = vtkUnstructuredGrid::New();
  isoTetraGrid2->Allocate (1, 1);
  isotriangulator2->AddTetras(0, isoTetraGrid2);
  isoTetraGrid2->SetPoints(Points);

  //First tets:
  vtkShrinkFilter *shrink1 = vtkShrinkFilter::New();
  shrink1->SetInputData( aTetraGrid1 );
  shrink1->SetShrinkFactor( 0.7 );

  vtkDataSetMapper *aTetraMapper1 = vtkDataSetMapper::New();
  aTetraMapper1->SetInputConnection(shrink1->GetOutputPort());

  vtkActor *aTetraActor1 = vtkActor::New();
  aTetraActor1->SetMapper (aTetraMapper1);

  //Second tets:
  vtkShrinkFilter *shrink2 = vtkShrinkFilter::New();
  shrink2->SetInputData( aTetraGrid2 );
  shrink2->SetShrinkFactor( 0.7 );

  vtkDataSetMapper *aTetraMapper2 = vtkDataSetMapper::New();
  aTetraMapper2->SetInputConnection(shrink2->GetOutputPort());

  vtkActor *aTetraActor2 = vtkActor::New();
  aTetraActor2->SetMapper (aTetraMapper2);

  //iso cases:
  //First tets:
  vtkShrinkFilter *isoshrink1 = vtkShrinkFilter::New();
  isoshrink1->SetInputData( isoTetraGrid1 );
  isoshrink1->SetShrinkFactor( 0.7 );

  vtkDataSetMapper *isoTetraMapper1 = vtkDataSetMapper::New();
  isoTetraMapper1->SetInputConnection(isoshrink1->GetOutputPort());

  vtkActor *isoTetraActor1 = vtkActor::New();
  isoTetraActor1->SetMapper (isoTetraMapper1);

  //Second tets:
  vtkShrinkFilter *isoshrink2 = vtkShrinkFilter::New();
  isoshrink2->SetInputData( isoTetraGrid2 );
  isoshrink2->SetShrinkFactor( 0.7 );

  vtkDataSetMapper *isoTetraMapper2 = vtkDataSetMapper::New();
  isoTetraMapper2->SetInputConnection(isoshrink2->GetOutputPort());

  vtkActor *isoTetraActor2 = vtkActor::New();
  isoTetraActor2->SetMapper (isoTetraMapper2);

  //Place everybody
  aTetraActor2->AddPosition   (1.2, 0.0, 0);
  isoTetraActor1->AddPosition (0.0, 1.2, 0);
  isoTetraActor2->AddPosition (1.2, 1.2, 0);

  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();

  renderer->SetBackground(1,1,1);
  renderer->AddActor( aTetraActor1 );
  renderer->AddActor( aTetraActor2 );
  renderer->AddActor( isoTetraActor1 );
  renderer->AddActor( isoTetraActor2 );

  renWin->AddRenderer( renderer );
  renWin->SetSize( 300, 300 );

  iren->SetRenderWindow( renWin );
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  Points->Delete();
  triangulator1->Delete();
  aTetraGrid1->Delete();
  shrink1->Delete();
  aTetraActor1->Delete();
  aTetraMapper1->Delete();

  triangulator2->Delete();
  aTetraGrid2->Delete();
  shrink2->Delete();
  aTetraActor2->Delete();
  aTetraMapper2->Delete();

  isotriangulator1->Delete();
  isoTetraGrid1->Delete();
  isoshrink1->Delete();
  isoTetraActor1->Delete();
  isoTetraMapper1->Delete();

  isotriangulator2->Delete();
  isoTetraGrid2->Delete();
  isoshrink2->Delete();
  isoTetraActor2->Delete();
  isoTetraMapper2->Delete();

  renderer->Delete();
  renWin->Delete();
  iren->Delete();

  return 0;
}
