/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernarySphereMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2012
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkQuadric.h"
#include "vtkTimerLog.h"

int TestHyperTreeGridTernarySphereMaterial( int argc, char* argv[] )
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaximumLevel( 4 );
  htGrid->SetGridSize( 5, 5, 6 );
  htGrid->SetGridScale( 1.5, 1., .7 );
  htGrid->SetDimension( 3 );
  htGrid->SetBranchFactor( 3 );
  htGrid->UseDescriptorOff();
  htGrid->UseMaterialMaskOn();
  vtkNew<vtkQuadric> quadric;
  quadric->SetCoefficients( 1., 1., 1.,
                            0, 0., 0.,
                            0.0, 0., 0.,
                            -25. );
  htGrid->SetQuadric( quadric.GetPointer() );
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  htGrid->Update();
  timer->StopTimer();
  cerr << "Creation time : " << timer->GetElapsedTime() << endl;
  timer->StartTimer();
  vtkNew<vtkHyperTreeGrid> htgCopy;
  htgCopy->ShallowCopy( htGrid->GetOutput() );
  timer->StopTimer();
  cerr << "Copy time : " << timer->GetElapsedTime() << endl;
  // Geometry
  timer->StartTimer();
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputData( htgCopy.GetPointer() );
  geometry->Update();
  vtkPolyData* pd = geometry->GetOutput();
  timer->StopTimer();
  cerr << "Geometry time : " << timer->GetElapsedTime() << endl;

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection( geometry->GetOutputPort() );
  mapper1->SetScalarRange( pd->GetCellData()->GetScalars()->GetRange() );
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection( geometry->GetOutputPort() );
  mapper2->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper( mapper1.GetPointer() );
  vtkNew<vtkActor> actor2;
  actor2->SetMapper( mapper2.GetPointer() );
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor( .7, .7, .7 );

  // Camera
  double bd[6];
  pd->GetBounds( bd );
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( pd->GetCenter() );
  camera->SetPosition( -.7 * bd[1], .9 * bd[3], -2.5 * bd[5] );

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera( camera.GetPointer() );
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( actor1.GetPointer() );
  renderer->AddActor( actor2.GetPointer() );

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer( renderer.GetPointer() );
  renWin->SetSize( 400, 400 );
  renWin->SetMultiSamples( 0 );

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow( renWin.GetPointer() );

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold( renWin.GetPointer(), 110 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    iren->Start();
  }

  return !retVal;
}
