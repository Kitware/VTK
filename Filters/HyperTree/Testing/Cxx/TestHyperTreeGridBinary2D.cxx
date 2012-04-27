/*=========================================================================

  Copyright (c) Kitware Inc.
  All rights reserved.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay and Charles Law, Kitware 2012
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int TestHyperTreeGridBinary2D( int argc, char* argv[] )
{
  vtkNew<vtkHyperTreeGridSource> fractal;
  fractal->SetMaximumLevel( 3 );
  fractal->SetGridSize( 4, 3, 1 );
  fractal->SetDimension( 2 );
  fractal->SetAxisBranchFactor( 2 );

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
  camera->SetPosition( .5 * bd[1], .5 * bd[3], 13 );

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

  return !retVal;
}
