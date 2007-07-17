/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LabeledMesh.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This example was translated into C++ from its TCL counterpart 
// (VTK/Examples/Annotation/Tcl/labeledMesh.tcl) by Jake Nickel from 
// the University of Iowa.  It demonstrates the use of vtkLabeledDataMapper.
// This class is used for displaying numerical data from an underlying data 
// set.  In the case of this example, the underlying data are the point and 
// cell ids.

// First we include the necessary header files. 
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkIdFilter.h"
#include "vtkRenderer.h"
#include "vtkSelectVisiblePoints.h"
#include "vtkLabeledDataMapper.h"
#include "vtkCellCenters.h"
#include "vtkLabeledDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"

int xLength;
int yLength;
vtkSelectVisiblePoints * visPts;
vtkSelectVisiblePoints * visCells;
vtkPoints * pts;
vtkRenderWindow * renWin;

// Create a procedure to draw the selection window at each location it
// is moved to.
void PlaceWindow( int xmin, int ymin )
{
  int xmax = xmin + xLength;
  int ymax = ymin + yLength;

  visPts->SetSelection( xmin, xmax, ymin, ymax );
  visCells->SetSelection( xmin, xmax, ymin, ymax );

  pts->InsertPoint( 0, xmin, ymin, 0 );
  pts->InsertPoint( 1, xmin, ymin, 0 );
  pts->InsertPoint( 2, xmin, ymin, 0 );
  pts->InsertPoint( 3, xmin, ymin, 0 );
  
  // Call Modified because InsertPoints does not modify vtkPoints
  // (for performance reasons).
  pts->Modified();

  renWin->Render();
}

// Create a procedure to move the selection window across the data set.
void MoveWindow()
{
  for( int y = 100; y < 300; y += 25 )
  {
    for( int x = 100; x < 300; x += 25 )
    {
      PlaceWindow( x, y );
    }
  }
}

int main( int argc, char * argv[] )
{
  // Create a selection window.  We will display the point and cell ids that
  // lie within this window.
  int xmin = 200;
  xLength = 100;
  int xmax = xmin + xLength;
  int ymin = 200;
  yLength = 100;
  int ymax = ymin + yLength;

  pts = vtkPoints::New();
  pts->InsertPoint( 0, xmin, ymin, 0 );
  pts->InsertPoint( 1, xmax, ymin, 0 );
  pts->InsertPoint( 2, xmax, ymax, 0 );
  pts->InsertPoint( 3, xmin, ymax, 0 );

  vtkCellArray * rect = vtkCellArray::New();
  rect->InsertNextCell( 5 );
  rect->InsertCellPoint( 0 );
  rect->InsertCellPoint( 1 );
  rect->InsertCellPoint( 2 );
  rect->InsertCellPoint( 3 );
  rect->InsertCellPoint( 0 );

  vtkPolyData * selectRect = vtkPolyData::New();
  selectRect->SetPoints( pts );
  selectRect->SetLines( rect );

  vtkPolyDataMapper2D * rectMapper = vtkPolyDataMapper2D::New();
  rectMapper->SetInput( selectRect );

  vtkActor2D * rectActor = vtkActor2D::New();
  rectActor->SetMapper( rectMapper );

  // Create a sphere and its associated mapper and actor.
  vtkSphereSource * sphere = vtkSphereSource::New();
  vtkPolyDataMapper * sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  sphereMapper->GlobalImmediateModeRenderingOn();

  vtkActor *sphereActor = vtkActor::New();
  sphereActor->SetMapper( sphereMapper );

  // Generate data arrays containing point and cell ids
  vtkIdFilter * ids = vtkIdFilter::New();
  ids->SetInputConnection( sphere->GetOutputPort() );
  ids->PointIdsOn();
  ids->CellIdsOn();
  ids->FieldDataOn();

  // Create the renderer here because vtkSelectVisiblePoints needs it.
  vtkRenderer * ren1 = vtkRenderer::New();

  // Create labels for points
  visPts = vtkSelectVisiblePoints::New();
  visPts->SetInputConnection( ids->GetOutputPort() );
  visPts->SetRenderer( ren1 );
  visPts->SelectionWindowOn();
  visPts->SetSelection( xmin, xmin + xLength, ymin, ymin + yLength );

  // Create the mapper to display the point ids.  Specify the
  // format to use for the labels.  Also create the associated actor.
  vtkLabeledDataMapper * ldm = vtkLabeledDataMapper::New();
  ldm->SetInputConnection( visPts->GetOutputPort() );
  ldm->SetLabelModeToLabelFieldData();

  vtkActor2D * pointLabels = vtkActor2D::New();
  pointLabels->SetMapper( ldm );

  // Create labels for cells
  vtkCellCenters * cc = vtkCellCenters::New();
  cc->SetInputConnection( ids->GetOutputPort() );

  visCells = vtkSelectVisiblePoints::New();
  visCells->SetInputConnection( cc->GetOutputPort() );
  visCells->SetRenderer( ren1 );
  visCells->SelectionWindowOn();
  visCells->SetSelection( xmin, xmin + xLength, ymin, ymin + yLength );

  // Create the mapper to display the cell ids.  Specify the
  // format to use for the labels.  Also create the associated actor.
  vtkLabeledDataMapper * cellMapper = vtkLabeledDataMapper::New();
  cellMapper->SetInputConnection( visCells->GetOutputPort() );
  cellMapper->SetLabelModeToLabelFieldData();
  cellMapper->GetLabelTextProperty()->SetColor( 0, 1, 0 );

  vtkActor2D * cellLabels = vtkActor2D::New();
  cellLabels->SetMapper( cellMapper );


  // Create the RenderWindow and RenderWindowInteractor
  renWin = vtkRenderWindow::New();
  renWin->AddRenderer( ren1 );

  vtkRenderWindowInteractor * iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow( renWin );

  // Add the actors to the renderer; set the background and size; render
  ren1->AddActor( sphereActor );
  ren1->AddActor2D( rectActor );
  ren1->AddActor2D( pointLabels );
  ren1->AddActor2D( cellLabels );

  ren1->SetBackground( 1, 1, 1 );
  renWin->SetSize( 500, 500 );
  renWin->Render();

  // Move the selection window across the data set.
  MoveWindow();

  // Put the selection window in the center of the render window.
  // This works because the xmin = ymin = 200, xLength = yLength = 100, and
  // the render window size is 500 x 500.
  PlaceWindow( xmin, ymin );

  iren->Initialize();
  iren->Start();

  // Delete allocated memory
  pts->Delete();
  rect->Delete();
  selectRect->Delete();
  rectMapper->Delete();
  rectActor->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  ids->Delete();
  ren1->Delete();
  visPts->Delete();
  ldm->Delete();
  pointLabels->Delete();
  cc->Delete();
  visCells->Delete();
  cellMapper->Delete();
  cellLabels->Delete();
  renWin->Delete();
  iren->Delete();

  return 0;
}
