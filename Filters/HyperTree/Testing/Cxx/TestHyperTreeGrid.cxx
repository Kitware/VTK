/*=========================================================================

  Copyright (c) Kitware Inc.
  All rights reserved.

=========================================================================*/
// .SECTION Thanks
// This test was written by Charles Law and Philippe Pebay, Kitware 2012

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisCut.h"
#include "vtkHyperTreeGridFractalSource.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGenerator.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkPolyDataMapper.h"
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

int TestHyperTreeGrid( int argc, char* argv[] )
{
  // Initialize return value of test
  int testIntValue = 0;

  // Dimension of hyper trees
  int dim = 3;

  //vtkNew<vtkHyperTreeGenerator> fractal;
  //vtkNew<vtkHyperTreeGridFractalSource> fractal;
  vtkHyperTreeGridFractalSource* fractal = vtkHyperTreeGridFractalSource::New();
  fractal->SetMaximumLevel( 3 );
  fractal->DualOn();
  if ( dim == 3 )
    {
    fractal->SetGridSize( 3, 4, 2 );
    }
  else if ( dim == 2 )
    {
    fractal->SetGridSize( 3, 4, 1 );
    }
  else if ( dim == 1 )
    {
    fractal->SetGridSize( 3, 1, 1 );
    }
  else
    {
    return 1;
    }

  fractal->SetDimension( dim );
  fractal->SetAxisBranchFactor( 3 );
  //vtkHyperTreeGrid* htGrid = fractal->NewHyperTreeGrid();
  fractal->Update();
  vtkHyperTreeGrid* htGrid = fractal->GetOutput();

  cerr << "# Contour" << endl;
  vtkNew<vtkContourFilter> contour;
  contour->SetInputData( htGrid );
  contour->SetNumberOfContours( 2 );
  contour->SetValue( 0, 4. );
  contour->SetValue( 1, 18. );
  contour->SetInputArrayToProcess( 0, 0, 0,
                                   vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                   "Cell Value" );
  vtkNew<vtkPolyDataWriter> writer0;
  writer0->SetFileName( "./hyperTreeGridContour.vtk" );
  writer0->SetInputConnection( contour->GetOutputPort() );
  writer0->Write();

  cerr << "# Shrink" << endl;
  vtkNew<vtkShrinkFilter> shrink;
  shrink->SetInputData( htGrid );
  shrink->SetShrinkFactor( 1. );
  vtkNew<vtkUnstructuredGridWriter> writer1;
  writer1->SetFileName( "./hyperTreeGridShrink.vtk" );
  writer1->SetInputConnection( shrink->GetOutputPort() );
  writer1->Write();

  // Axis-aligned cut works only in 3D for now
  if ( dim == 3 )
    {
    cerr << "# HyperTreeGridAxisCut" << endl;
    vtkNew<vtkHyperTreeGridAxisCut> axisCut;
    axisCut->SetInputConnection( fractal->GetOutputPort() );
    axisCut->SetPlaneNormalAxis( 2 );
    axisCut->SetPlanePosition( .1 );
    vtkNew<vtkPolyDataWriter> writer3;
    writer3->SetFileName( "./hyperTreeGridAxisCut.vtk" );
    writer3->SetInputConnection( axisCut->GetOutputPort() );
    writer3->Write();
    }

  cerr << "# Cut" << endl;
  vtkNew<vtkCutter> cut;
  vtkNew<vtkPlane> plane;
  plane->SetOrigin( .5, .5, .15 );
  plane->SetNormal( 0, 0, 1 );
  cut->SetInputData( htGrid );
  cut->SetCutFunction( plane.GetPointer() );
  vtkNew<vtkPolyDataWriter> writer2;
  writer2->SetFileName( "./hyperTreeGridCut.vtk" );
  writer2->SetInputConnection( cut->GetOutputPort() );
  writer2->Write();

  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection( fractal->GetOutputPort() );
  geometry->Update();
  vtkPolyData* pd = geometry->GetOutput();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection( geometry->GetOutputPort() );
  mapper->SetScalarRange( pd->GetCellData()->GetScalars()->GetRange() );
 
  vtkNew<vtkActor> actor;
  actor->SetMapper( mapper.GetPointer() );

  // Create camera
  double bd[3];
  pd->GetBounds( bd );
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( pd->GetCenter() );
  camera->SetPosition( -.8 * bd[1], 2.1 * bd[3], -4.8 * bd[5] );

  // Create a renderer, add actors to it
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera( camera.GetPointer() );
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( actor.GetPointer() );

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
  //htGrid->Delete();
  fractal->Delete();

  return testIntValue;
}
