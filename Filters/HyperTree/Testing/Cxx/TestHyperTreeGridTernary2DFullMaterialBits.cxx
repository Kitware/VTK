/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary2DMaterialBits.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay and Joachim Pouderoux, Kitware 2013
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
#include "vtkBitArray.h"
#include "vtkIdTypeArray.h"
#include "vtkPointData.h"
#include "vtkTimerLog.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"
#include <sstream>

void GenerateDescriptorAndMaterial(int depth, int sx, int sy, int sz, int branch,
                                   std::stringstream &d, std::stringstream &m)
{
  vtkIdType l = sx * sy * sz;
  vtkIdType s = 1;
  for ( int j = 0; j < depth; j++ )
  {
    for ( int i = 0; i < l; i++ )
    {
      if ( j > 0 && i%s == 0 )
      {
        d << " ";
        if ( j > 1 )
        {
          m << " ";
        }
      }
      d << "R";
      if ( j > 0 )
      {
        m << "1";
      }

    }
    s *= branch * branch;
    l *= branch * branch;
    d << " |";
    if ( j > 0 )
    {
      m << " |";
    }
  }

  for ( int i = 0; i < l; i++ )
  {
    if ( i%(s/branch) == 0 )
    {
      d << " ";
      m << " ";
    }
    d << ".";
    m << "1";
  }
}

void GenerateDescriptorAndMaterial(int depth, int sx, int sy, int sz, int branch,
                                  vtkBitArray *d, vtkBitArray* m)
{
  vtkIdType l = sx * sy * sz;
  vtkIdType s = 1;
  for ( int j = 0; j < depth - 1; j++ )
  {
    for ( int i = 0; i < l; i++ )
    {
      d->InsertNextValue(1);
      if ( j > 0 )
      {
        m->InsertNextValue(1);
      }
    }
    s *= branch * branch;
    l *= branch * branch;
  }

  for ( int i = 0; i < l; i++ )
  {
    d->InsertNextValue(0);
    m->InsertNextValue(1);
  }
}

int TestHyperTreeGridTernary2DFullMaterialBits( int argc, char* argv[] )
{
  int sx = 10;
  int sy = 10;
  int sz = 1;
  int depth = 2;
  int branch = 3;

  vtkNew<vtkTimerLog> timer;

  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = depth;
  htGrid->SetMaximumLevel( maxLevel );
  htGrid->SetGridSize( sx, sy, sz );
  htGrid->SetGridScale( 1., 1., 1. );
  htGrid->SetDimension( 2 );
  htGrid->SetBranchFactor( branch );
  htGrid->UseMaterialMaskOn();
  vtkNew<vtkIdTypeArray> zero;
  for ( int i = 0; i < sx * sy * sz; i++ )
  {
    zero->InsertNextValue(i);
  }
  htGrid->SetLevelZeroMaterialIndex( zero.GetPointer() );
  vtkNew<vtkBitArray> desc;
  vtkNew<vtkBitArray> mat;
  timer->StartTimer();
  cout << "Generating descriptors..." << endl;
  GenerateDescriptorAndMaterial( depth, sx, sy, sz, branch, desc.GetPointer(), mat.GetPointer() );
  timer->StopTimer();
  htGrid->SetDescriptorBits( desc.GetPointer() );
  htGrid->SetMaterialMaskBits( mat.GetPointer() );
  cout << " Done in " << timer->GetElapsedTime() << "s (" << desc->GetNumberOfTuples() << " nodes)" << endl;

  cout << "Constructing HTG " << sx << "x" << sy << "x" << sz << "  branch: " << branch << "  depth: " << depth << "..." << endl;
  timer->StartTimer();
  htGrid->Update();
  timer->StopTimer();
  vtkIdType nbCells = htGrid->GetOutput()->GetNumberOfCells();
  cout << " Done in " << timer->GetElapsedTime() << "s" << endl;
  cout << "#pts " << htGrid->GetOutput()->GetNumberOfPoints() << endl;
  timer->StartTimer();
  timer->StopTimer();
  cout << "#cells " << nbCells << endl;

  cout << "HTG takes " << htGrid->GetOutput()->GetActualMemorySize() << "KB in memory." << endl;

  // Prepare an array of ids
  vtkNew<vtkIdTypeArray> idArray;
  idArray->SetName( "Ids" );
  idArray->SetNumberOfComponents( 1 );
  vtkIdType nbPoints = htGrid->GetOutput()->GetNumberOfPoints();
  idArray->SetNumberOfValues( nbPoints );
  for ( vtkIdType i = 0; i < nbPoints; ++ i )
  {
    idArray->SetValue( i, i );
  }
  htGrid->GetOutput()->GetPointData()->SetScalars( idArray.GetPointer() );

  // Geometry
  cout << "Constructing geometry..." << endl;
  timer->StartTimer();
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputData( htGrid->GetOutput() );
  geometry->Update();
  vtkPolyData* pd = geometry->GetOutput();
  timer->StopTimer();
  cout << " Done in " << timer->GetElapsedTime() << "s" << endl;

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection( geometry->GetOutputPort() );
  mapper1->SetScalarRange( pd->GetCellData()->GetScalars()->GetRange() );
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection( geometry->GetOutputPort() );
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkDataSetMapper> mapper3;
  mapper3->SetInputConnection( htGrid->GetOutputPort() );
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
  actor3->GetProperty()->SetRepresentationToWireframe();
  actor3->GetProperty()->SetColor( .0, .0, .0 );
  vtkNew<vtkActor> actor4;
  actor4->SetMapper( mapper4.GetPointer() );
  actor4->GetProperty()->SetRepresentationToPoints();
  actor4->GetProperty()->SetPointSize(4);
  actor4->GetProperty()->SetColor( 0., 1., 0. );

  vtkNew<vtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable(mapper1->GetLookupTable());
  scalarBar->SetLabelFormat("%.0f");
  scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar->GetPositionCoordinate()->SetValue( .80, .32 );
  scalarBar->SetTitle( "  id  " );
  scalarBar->SetNumberOfLabels( 4 );
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

  // Camera
  double bd[6];
  pd->GetBounds( bd );
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange( 1., 100. );
  camera->SetFocalPoint( pd->GetCenter() );
  camera->SetPosition( .5 * bd[1], .5 * bd[3], 2 * bd[1] );

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera( camera.GetPointer() );
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( actor1.GetPointer() );
  renderer->AddActor( actor2.GetPointer() );
  renderer->AddActor( actor3.GetPointer() );
  renderer->AddActor( actor4.GetPointer() );
  renderer->AddActor2D( scalarBar.GetPointer() );

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
