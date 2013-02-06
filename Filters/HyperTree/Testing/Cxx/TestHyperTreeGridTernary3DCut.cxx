/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DCut.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2013
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridToUnstructuredGrid.h"
#include "vtkHyperTreeGridSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCutter.h"
#include "vtkDataSetMapper.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnstructuredGrid.h"

int TestHyperTreeGridTernary3DCut( int argc, char* argv[] )
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaximumLevel( 5 );
  htGrid->SetGridSize( 3, 3, 2 );
  htGrid->SetGridScale( 1.5, 1., .7 );
  htGrid->SetDimension( 3 );
  htGrid->SetBranchFactor( 3 );
  htGrid->SetDescriptor( "RRR .R. .RR ..R ..R .R.|R.......................... ........................... ........................... .............R............. ....RR.RR........R......... .....RRRR.....R.RR......... ........................... ........................... ...........................|........................... ........................... ........................... ...RR.RR.......RR.......... ........................... RR......................... ........................... ........................... ........................... ........................... ........................... ........................... ........................... ............RRR............|........................... ........................... .......RR.................. ........................... ........................... ........................... ........................... ........................... ........................... ........................... ...........................|........................... ..........................." );

  // Hyper tree grid to unstructured grid filter
  vtkNew<vtkHyperTreeGridToUnstructuredGrid> htg2ug;
  htg2ug->SetInputConnection( htGrid->GetOutputPort() );

  // Cuts
  vtkNew<vtkPlane> plane1;
  plane1->SetOrigin( 3.35, 0., 0. );
  plane1->SetNormal( 1., -.2, .2 );
  vtkNew<vtkCutter> cut1;
  cut1->SetInputConnection( htGrid->GetOutputPort() );
  cut1->SetCutFunction( plane1.GetPointer() );
  vtkNew<vtkPlane> plane2;
  plane2->SetOrigin( 0., .6, .4 );
  plane2->SetNormal( -.2, -.6, 1. );
  vtkNew<vtkCutter> cut2;
  cut2->SetInputConnection( htGrid->GetOutputPort() );
  cut2->SetCutFunction( plane2.GetPointer() );

  // Mappers
  cut1->Update();
  double* range1 = cut1->GetOutput()->GetPointData()->GetScalars()->GetRange();
  cut2->Update();
  double* range2 = cut2->GetOutput()->GetPointData()->GetScalars()->GetRange();
  double range[2];
  range[0] = range1[0] < range2[0] ? range1[0] : range2[0];
  range[1] = range1[1] > range2[1] ? range1[1] : range2[1];
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection( cut1->GetOutputPort() );
  mapper1->SetScalarRange( range );
  vtkNew<vtkDataSetMapper> mapper2;
  mapper2->SetInputConnection( htg2ug->GetOutputPort() );
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkDataSetMapper> mapper3;
  mapper3->SetInputConnection( cut2->GetOutputPort() );
  mapper3->SetScalarRange( range );
 
  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper( mapper1.GetPointer() );
  vtkNew<vtkActor> actor2;
  actor2->SetMapper( mapper2.GetPointer() );
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor( .8, .8, .8 );
  vtkNew<vtkActor> actor3;
  actor3->SetMapper( mapper3.GetPointer() );

  // Camera
  vtkHyperTreeGrid* ht = htGrid->GetOutput();
  double bd[6];
  ht->GetBounds( bd );
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( ht->GetCenter() );
  camera->SetPosition( -.8 * bd[1], 2.1 * bd[3], -4.8 * bd[5] );

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera( camera.GetPointer() );
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( actor1.GetPointer() );
  renderer->AddActor( actor2.GetPointer() );
  renderer->AddActor( actor3.GetPointer() );

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
  
  int retVal = vtkRegressionTestImageThreshold( renWin.GetPointer(), 50 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
}
