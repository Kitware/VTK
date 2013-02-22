/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary2DMaterial.cxx

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

#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int TestHyperTreeGridTernary2DMaterial( int argc, char* argv[] )
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaximumLevel( maxLevel );
  htGrid->SetGridSize( 2, 3, 1 );
  htGrid->SetGridScale( 1.5, 1., .7 );
  htGrid->SetDimension( 2 );
  htGrid->SetBranchFactor( 3 );
  htGrid->UseMaterialMaskOn();
  htGrid->SetDescriptor( "RRRRR.|......... ..R...... RRRRRRRRR R........ R........|..R...... ........R ......RRR ......RRR ..R..R..R RRRRRRRRR R..R..R.. ......... ......... ......... ......... .........|......... ......... ......... ......... ......... ......... ......... ......... ........R ..R..R..R ......... ......RRR ......R.. ......... RRRRRRRRR R..R..R.. ......... ......... ......... ......... ......... ......... .........|......... ......... ......... ......... ......... ......... ......... ......... ......... RRRRRRRRR ......... ......... ......... ......... ......... ......... ......... ......... ......... .........|......... ......... ......... ......... ......... ......... ......... ......... ........." );
  htGrid->SetMaterialMask( "111111|000000000 111111111 111111111 111111111 111111111|111111111 000000001 000000111 011011111 001001001 111111111 100100100 001001001 111111111 111111111 111111111 001111111|111111111 001001001 111111111 111111111 111111111 111111111 111111111 111111111 001001111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111|111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111|111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111" );

  // Geometry
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection( htGrid->GetOutputPort() );
  geometry->Update();
  vtkPolyData* pd = geometry->GetOutput();

  // Contour
  vtkNew<vtkContourFilter> contour;
  int nContours = 3;
  contour->SetNumberOfContours( nContours );
  contour->SetInputConnection( htGrid->GetOutputPort() );
  double resolution = ( maxLevel - 1 ) / ( nContours + 1. );
  double isovalue = resolution;
  for ( int i = 0; i < nContours; ++ i, isovalue += resolution )
    {
    contour->SetValue( i, isovalue );
    }

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection( geometry->GetOutputPort() );
  mapper1->SetScalarRange( pd->GetCellData()->GetScalars()->GetRange() );
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection( geometry->GetOutputPort() );
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection( contour->GetOutputPort() );
  mapper3->ScalarVisibilityOff();
  vtkNew<vtkDataSetMapper> mapper4;
  mapper4->SetInputConnection( htGrid->GetOutputPort() );
  mapper4->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper( mapper1.GetPointer() );
  vtkNew<vtkActor> actor2;
  actor2->SetMapper( mapper2.GetPointer() );
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor( .7, .7, .7 );
  vtkNew<vtkActor> actor3;
  actor3->SetMapper( mapper3.GetPointer() );
  actor3->GetProperty()->SetColor( .8, .4, .3 );
  actor3->GetProperty()->SetLineWidth( 3 );
  vtkNew<vtkActor> actor4;
  actor4->SetMapper( mapper4.GetPointer() );
  actor4->GetProperty()->SetRepresentationToWireframe();
  actor4->GetProperty()->SetColor( .0, .0, .0 );

  // Camera
  double bd[6];
  pd->GetBounds( bd );
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( pd->GetCenter() );
  camera->SetPosition( .5 * bd[1], .5 * bd[3], 6. );

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera( camera.GetPointer() );
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( actor1.GetPointer() );
  renderer->AddActor( actor2.GetPointer() );
  renderer->AddActor( actor3.GetPointer() );
  renderer->AddActor( actor4.GetPointer() );

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
  
  int retVal = vtkRegressionTestImageThreshold( renWin.GetPointer(), 70 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
}
