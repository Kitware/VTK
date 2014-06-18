/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2011-12
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarBarActor.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkTextProperty.h"

#include "vtkTestUtilities.h"

int TestScalarBar( int argc, char *argv[] )
{
 char* fname =
    vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/combxyz.bin");
  char* fname2 =
    vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/combq.bin");

  // Start by loading some data.
  vtkSmartPointer<vtkMultiBlockPLOT3DReader> pl3d =
    vtkSmartPointer<vtkMultiBlockPLOT3DReader>::New();
  pl3d->SetXYZFileName( fname );
  pl3d->SetQFileName( fname2 );
  pl3d->SetScalarFunctionNumber( 100 );
  pl3d->SetVectorFunctionNumber( 202 );
  pl3d->Update();

  delete [] fname;
  delete [] fname2;

  // An outline is shown for context.
  vtkSmartPointer<vtkStructuredGridGeometryFilter> outline =
    vtkSmartPointer<vtkStructuredGridGeometryFilter>::New();
  outline->SetInputData( pl3d->GetOutput()->GetBlock( 0 ));
  outline->SetExtent( 0, 100, 0, 100, 9, 9 );

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection( outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor =
    vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper( outlineMapper );

  // Create the RenderWindow, Renderer and all Actors
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer( ren1 );

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( renWin );

  vtkSmartPointer<vtkScalarBarActor> scalarBar1 =
    vtkSmartPointer<vtkScalarBarActor>::New();
  vtkScalarsToColors* lut = outlineMapper->GetLookupTable();
  lut->SetAnnotation(0.0, "Zed");
  lut->SetAnnotation(1.0, "Uno");
  lut->SetAnnotation(0.1, "$\\frac{1}{10}$");
  lut->SetAnnotation(0.125, "$\\frac{1}{8}$");
  lut->SetAnnotation(0.5, "Half");
  scalarBar1->SetTitle("Density");
  scalarBar1->SetLookupTable(lut);
  scalarBar1->DrawAnnotationsOn();
  scalarBar1->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar1->GetPositionCoordinate()->SetValue( .6, .05 );
  scalarBar1->SetWidth( 0.15 );
  scalarBar1->SetHeight( 0.5 );
  scalarBar1->SetTextPositionToPrecedeScalarBar();
  scalarBar1->GetTitleTextProperty()->SetColor( 0., 0., 1. );
  scalarBar1->GetLabelTextProperty()->SetColor( 0., 0., 1. );
  scalarBar1->SetDrawFrame( 1 );
  scalarBar1->GetFrameProperty()->SetColor( 0., 0., 0. );
  scalarBar1->SetDrawBackground( 1 );
  scalarBar1->GetBackgroundProperty()->SetColor( 1., 1., 1. );

  vtkSmartPointer<vtkScalarBarActor> scalarBar2 =
    vtkSmartPointer<vtkScalarBarActor>::New();
  scalarBar2->SetTitle("Density");
  scalarBar2->SetLookupTable(lut);
  scalarBar2->DrawAnnotationsOff();
  scalarBar2->SetOrientationToHorizontal();
  scalarBar2->SetWidth( 0.5 );
  scalarBar2->SetHeight( 0.15 );
  scalarBar2->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar2->GetPositionCoordinate()->SetValue( 0.05, 0.05 );
  scalarBar2->SetTextPositionToPrecedeScalarBar();
  scalarBar2->GetTitleTextProperty()->SetColor( 1., 0., 0. );
  scalarBar2->GetLabelTextProperty()->SetColor( .8, 0., 0. );
  scalarBar2->SetDrawFrame( 1 );
  scalarBar2->GetFrameProperty()->SetColor( 1., 0., 0. );
  scalarBar2->SetDrawBackground( 1 );
  scalarBar2->GetBackgroundProperty()->SetColor( .5, .5, .5 );

  vtkSmartPointer<vtkScalarBarActor> scalarBar3 =
    vtkSmartPointer<vtkScalarBarActor>::New();
  scalarBar3->SetTitle("Density");
  scalarBar3->SetLookupTable(lut);
  scalarBar3->DrawAnnotationsOff();
  scalarBar3->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar3->GetPositionCoordinate()->SetValue( .8, .05 );
  scalarBar3->SetWidth( 0.15 );
  scalarBar3->SetHeight( 0.5 );
  scalarBar3->SetTextPositionToSucceedScalarBar();
  scalarBar3->GetTitleTextProperty()->SetColor( 0., 0., 1. );
  scalarBar3->GetLabelTextProperty()->SetColor( 0., 0., 1. );
  scalarBar3->SetDrawFrame( 1 );
  scalarBar3->GetFrameProperty()->SetColor( 0., 0., 0. );
  scalarBar3->SetDrawBackground( 0 );

  vtkSmartPointer<vtkScalarBarActor> scalarBar4 =
    vtkSmartPointer<vtkScalarBarActor>::New();
  scalarBar4->SetTitle("Density");
  scalarBar4->SetLookupTable(lut);
  scalarBar4->DrawAnnotationsOff();
  scalarBar4->SetOrientationToHorizontal();
  scalarBar4->SetWidth( 0.5 );
  scalarBar4->SetHeight( 0.15 );
  scalarBar4->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar4->GetPositionCoordinate()->SetValue( .05, .8 );
  scalarBar4->SetTextPositionToSucceedScalarBar();
  scalarBar4->GetTitleTextProperty()->SetColor( 0., 0., 1. );
  scalarBar4->GetLabelTextProperty()->SetColor( 0., 0., 1. );
  scalarBar4->SetDrawFrame( 1 );
  scalarBar4->GetFrameProperty()->SetColor( 1., 1., 1. );
  scalarBar4->SetDrawBackground( 0 );

  vtkSmartPointer<vtkCamera> camera =
    vtkSmartPointer<vtkCamera>::New();
  camera->SetFocalPoint( 8, 0, 30 );
  camera->SetPosition( 6, 0, 50 );
  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor( outlineActor );
  ren1->AddActor( scalarBar1 );
  ren1->AddActor( scalarBar2 );
  ren1->AddActor( scalarBar3 );
  ren1->AddActor( scalarBar4 );
  ren1->GradientBackgroundOn();
  ren1->SetBackground( .5,.5,.5 );
  ren1->SetBackground2( .0,.0,.0 );
  ren1->SetActiveCamera( camera );

  // render the image
  renWin->SetWindowName( "VTK - Scalar Bar options" );
  renWin->SetSize( 700, 500 );
  renWin->SetMultiSamples( 0 );
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
}
