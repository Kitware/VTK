/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DClip.cxx

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
#include "vtkClipDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkShrinkFilter.h"
#include "vtkUnstructuredGrid.h"

int TestHyperTreeGridTernary3DClip( int argc, char* argv[] )
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

  // Clip
  vtkNew<vtkPlane> plane;
  plane->SetOrigin( 0., .5, .4 );
  plane->SetNormal( -.2, -.6, 1. );
  vtkNew<vtkClipDataSet> clip;
  clip->SetInputConnection( htGrid->GetOutputPort() );
  clip->SetClipFunction( plane.GetPointer() );

  // Shrink
  vtkNew<vtkShrinkFilter> shrink;
  shrink->SetInputConnection( clip->GetOutputPort() );
  shrink->SetShrinkFactor( .8 );

  // Mappers
  clip->Update();
  double* range = clip->GetOutput()->GetPointData()->GetScalars()->GetRange();
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection( clip->GetOutputPort() );
  mapper1->SetScalarRange( range );
  vtkNew<vtkDataSetMapper> mapper2;
  mapper2->SetInputConnection( htg2ug->GetOutputPort() );
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkDataSetMapper> mapper3;
  mapper3->SetInputConnection( shrink->GetOutputPort() );
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
  //renderer->AddActor( actor1.GetPointer() );
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

  int retVal = vtkRegressionTestImageThreshold( renWin.GetPointer(), 40 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
}
