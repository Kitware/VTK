/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DGeometry.cxx

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

int TestHyperTreeGridTernary3DGeometry( int argc, char* argv[] )
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaximumLevel( 5 );
  htGrid->SetGridSize( 3, 3, 2 );
  htGrid->SetGridScale( 1.5, 1., .7 );
  htGrid->SetDimension( 3 );
  htGrid->SetBranchFactor( 3 );
  htGrid->SetDescriptor( "RRR .R. .RR ..R ..R .R.|R.......................... ........................... ........................... .............R............. ....RR.RR........R......... .....RRRR.....R.RR......... ........................... ........................... ...........................|........................... ........................... ........................... ...RR.RR.......RR.......... ........................... RR......................... ........................... ........................... ........................... ........................... ........................... ........................... ........................... ............RRR............|........................... ........................... .......RR.................. ........................... ........................... ........................... ........................... ........................... ........................... ........................... ...........................|........................... ..........................." );

  // Geometry
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection( htGrid->GetOutputPort() );
  geometry->Update();
  vtkPolyData* pd = geometry->GetOutput();

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
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
  camera->SetPosition( -.8 * bd[1], 2.1 * bd[3], -4.8 * bd[5] );

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
  
  int retVal = vtkRegressionTestImageThreshold( renWin.GetPointer(), 30 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
}
