/*=========================================================================

  Copyright (c) Kitware Inc.
  All rights reserved.

=========================================================================*/
// .SECTION Thanks
// This test was written by Charles Law and Philippe Pebay, Kitware 2012

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeFractalSource.h"

#include "vtkCamera.h"
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataSetMapper.h"
#include "vtkGeometryFilter.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkShrinkFilter.h"
#include "vtkPolyDataWriter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnstructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetWriter.h"

int TestHyperTreeGrid( int argc, char** argv )
{
  vtkNew<vtkHyperTreeFractalSource> fractal;
  fractal->SetMaximumLevel( 3 );
  fractal->DualOn();
  fractal->SetGridSize( 3, 2, 2 );
  fractal->SetDimension( 3 );
  fractal->SetAxisBranchFactor( 3 );
  vtkHyperTreeGrid* htGrid = fractal->NewHyperTreeGrid();

  vtkNew<vtkCutter> cut;
  vtkNew<vtkPlane> plane;
  plane->SetOrigin( .5, .5, .15 );
  plane->SetNormal( 0, 0, 1 );
  cut->SetInputData( htGrid );
  cut->SetCutFunction( plane.GetPointer() );
  vtkNew<vtkPolyDataWriter> writer;
  writer->SetFileName( "./hyperTreeGridCut.vtk" );
  writer->SetInputConnection( cut->GetOutputPort() );
  writer->Write();

  vtkNew<vtkContourFilter> contour;
  contour->SetInputData( htGrid );
  contour->SetNumberOfContours( 2 );
  contour->SetValue( 0, 2. );
  contour->SetValue( 1, 3. );
  contour->SetInputArrayToProcess( 0, 0, 0,
                                   vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                   "Test" );
  vtkNew<vtkPolyDataWriter> writer2;
  writer2->SetFileName( "./hyperTreeGridContour.vtk" );
  writer2->SetInputConnection( contour->GetOutputPort() );
  writer2->Write();

  vtkNew<vtkShrinkFilter> shrink;
  shrink->SetInputData( htGrid );
  shrink->SetShrinkFactor( 1. );
  vtkNew<vtkUnstructuredGridWriter> writer3;
  writer3->SetFileName( "./hyperTreeGridShrink.vtk" );
  writer3->SetInputConnection( shrink->GetOutputPort() );
  writer3->Write();

  vtkNew<vtkDataSetMapper> htGridMapper;
  htGridMapper->SetInputConnection( shrink->GetOutputPort() );
  vtkNew<vtkActor> htGridActor;
  htGridActor->SetMapper( htGridMapper.GetPointer() );

  // Create camera
  double bd[3];
  htGrid->GetBounds( bd );
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( htGrid->GetCenter() );
  camera->SetPosition( -.8 * bd[1], 2.1 * bd[3], -4.8 * bd[5] );

  // Create a renderer, add actors to it
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera( camera.GetPointer() );
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( htGridActor.GetPointer() );

  // Create a renderWindow
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer( renderer.GetPointer() );
  renWin->SetSize( 300, 300 );
  renWin->SetMultiSamples( 0 );

  // Create interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow( renWin.GetPointer() );

  // Render and test
  renWin->Render();
  
  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  // Clean up
  htGrid->Delete();

  return 0;
}
