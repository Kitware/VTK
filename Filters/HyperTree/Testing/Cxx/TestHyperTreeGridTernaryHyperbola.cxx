/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernaryHyperbola.cxx

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
#include "vtkColorTransferFunction.h"
#include "vtkContourFilter.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkQuadric.h"

int TestHyperTreeGridTernaryHyperbola( int argc, char* argv[] )
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaximumLevel( 6 );
  htGrid->SetGridSize( 8, 12, 1 );
  htGrid->SetGridScale( 1.5, 1., .7 );
  htGrid->SetDimension( 2 );
  htGrid->SetBranchFactor( 3 );
  htGrid->UseDescriptorOff();
  htGrid->UseMaterialMaskOff();
  vtkNew<vtkQuadric> quadric;
  quadric->SetCoefficients( 1., -1., 0.,
                            0., 0., 0.,
                            -12., 12., 0.,
                            1. );
  htGrid->SetQuadric( quadric.GetPointer() );

  // Geometry
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection( htGrid->GetOutputPort() );
  geometry->Update();
  vtkPolyData* pd = geometry->GetOutput();
  pd->GetCellData()->SetActiveScalars( "Quadric" );

  // Contour
  vtkNew<vtkContourFilter> contour;
  contour->SetInputConnection( htGrid->GetOutputPort() );
  contour->SetNumberOfContours( 0 );
  contour->SetValue( 0, 0 );
  contour->SetInputArrayToProcess( 0, 0, 0,
                                   vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                   "Quadric" );
  //  Color transfer function
  vtkNew<vtkColorTransferFunction> colorFunction;
  colorFunction->AddRGBSegment( -30., 0., 0., 1.,
                                0.,  0., 1., 1.);
  colorFunction->AddRGBSegment( VTK_DBL_MIN,  1., 1., 0.,
                                30., 1., 0., 0.);

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection( geometry->GetOutputPort() );
  mapper1->UseLookupTableScalarRangeOn();
  mapper1->SetLookupTable( colorFunction.GetPointer() );
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection( geometry->GetOutputPort() );
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection( contour->GetOutputPort() );
  mapper3->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper( mapper1.GetPointer() );
  vtkNew<vtkActor> actor2;
  actor2->SetMapper( mapper2.GetPointer() );
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor( .7, .7, .7 );
  vtkNew<vtkActor> actor3;
  actor3->SetMapper( mapper3.GetPointer() );
  actor3->GetProperty()->SetColor( 0., 0., 0. );
  actor3->GetProperty()->SetLineWidth( 2 );

  // Camera
  double bd[6];
  pd->GetBounds( bd );
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( pd->GetCenter() );
  camera->SetPosition( .5 * bd[1], .5 * bd[3], 24. );

  // Scalar bar
  vtkNew<vtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable( colorFunction.GetPointer() );
  scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar->GetPositionCoordinate()->SetValue( .65, .05 );
  scalarBar->SetTitle( "Quadric" );
  scalarBar->SetWidth( 0.15 );
  scalarBar->SetHeight( 0.4 );
  scalarBar->SetTextPad( 4 );
  scalarBar->SetMaximumWidthInPixels( 60 );
  scalarBar->SetMaximumHeightInPixels( 200 );
  scalarBar->SetTextPositionToPrecedeScalarBar();
  scalarBar->GetTitleTextProperty()->SetColor( .4, .4, .4 );
  scalarBar->GetLabelTextProperty()->SetColor( .4, .4, .4 );
  scalarBar->SetDrawFrame( 1 );
  scalarBar->GetFrameProperty()->SetColor( .4, .4, .4 );
  scalarBar->SetDrawBackground( 1 );
  scalarBar->GetBackgroundProperty()->SetColor( 1., 1., 1. );

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera( camera.GetPointer() );
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( actor1.GetPointer() );
  renderer->AddActor( actor2.GetPointer() );
  renderer->AddActor( actor3.GetPointer() );
  renderer->AddActor( scalarBar.GetPointer() );

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
