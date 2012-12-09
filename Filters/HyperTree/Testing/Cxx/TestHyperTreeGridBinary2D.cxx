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
#include "vtkContourFilter.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int TestHyperTreeGridBinary2D( int argc, char* argv[] )
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaximumLevel( maxLevel );
  htGrid->SetGridSize( 2, 3, 1 );
  htGrid->SetGridScale( 1.5, 1., .7 );
  htGrid->SetDimension( 2 );
  htGrid->SetBranchFactor( 2 );
  htGrid->DualOn();
  htGrid->SetDescriptor( "RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... ...R ..R. .... .R.. R...|.... .... .R.. ....|...." );

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
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection( geometry->GetOutputPort() );
  mapper1->SetScalarRange( pd->GetCellData()->GetScalars()->GetRange() );
  mapper1->SetResolveCoincidentTopologyToPolygonOffset();
  mapper1->SetResolveCoincidentTopologyPolygonOffsetParameters( 0, 1 );
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection( geometry->GetOutputPort() );
  mapper2->ScalarVisibilityOff();
  mapper2->SetResolveCoincidentTopologyToPolygonOffset();
  mapper2->SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection( contour->GetOutputPort() );
  mapper3->ScalarVisibilityOff();
  mapper3->SetResolveCoincidentTopologyToPolygonOffset();
  mapper3->SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper( mapper1.GetPointer() );
  vtkNew<vtkActor> actor2;
  actor2->SetMapper( mapper2.GetPointer() );
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor( .7, .7, .7 );
  vtkNew<vtkActor> actor3;
  actor3->SetMapper( mapper3.GetPointer() );
  actor3->GetProperty()->SetColor( .8, .2, .3 );
  actor3->GetProperty()->SetLineWidth( 2 );

  // Camera
  double bd[6];
  pd->GetBounds( bd );
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( pd->GetCenter() );
  camera->SetPosition( .5 * bd[1], .5 * bd[3], 6 );

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
  
  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
}
