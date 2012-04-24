/*=========================================================================

  Copyright (c) Kitware Inc.
  All rights reserved.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2012

#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridFractalSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int TestHyperTreeGridGeometry( int argc, char* argv[] )
{
  vtkNew<vtkHyperTreeGridFractalSource> fractal;
  fractal->SetMaximumLevel( 3 );
  fractal->SetGridSize( 3, 4, 2 );
  fractal->SetDimension( 3 );
  fractal->SetAxisBranchFactor( 3 );

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

  return 0;
}
