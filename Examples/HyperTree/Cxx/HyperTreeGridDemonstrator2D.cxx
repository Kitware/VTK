/*=========================================================================

Copyright (c) Kitware Inc.
All rights reserved.

=========================================================================*/
// .SECTION Description
// This program illustrates the use of various filters acting upon hyper
// tree grid data sets. It generates output files in VTK format.
//
// .SECTION Usage
//   --branch-factor opt  Branching factor of hyper tree grid
//   --dimension opt      Dimension of hyper tree grid
//   --grid-size-X opt    Size of hyper tree grid in X direction
//   --grid-size-Y opt    Size of hyper tree grid in Y direction
//   --grid-size-Z opt    Size of hyper tree grid in Z direction
//   --descriptor         String of characters specifying tree structure
//   --max-level opt      Maximum depth of hyper tree grid
//   --contours           Number of iso-contours to be calculated
//   --skip-Axis-Cut      Skip axis cut filter
//   --skip-Contour       Skip contour filter
//   --skip-Cut           Skip cut filter
//   --skip-Geometry      Skip geometry filter
//   --skip-Shrink        Skip shrink filter
//
// .SECTION Thanks
// This example was written by Philippe Pebay and Charles Law, Kitware 2012
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisCut.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkHyperTreeGridGeometry.h"

#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataSetWriter.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyDataWriter.h"
#include "vtkShrinkFilter.h"
#include "vtkStdString.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"

int main( int argc, char* argv[] )
{
  // Set hyper tree grid source parameters
  vtkStdString descriptor = "RRRRR.|.... .R.. .R.R R... ....|.R.. ...R .... ....|.... ....";
  int dim = 2;
  int branch = 2;
  int max = 4;
  int nX = 2;
  int nY = 3;
  int nZ = 1;
  int nContours = 2;

  // Create hyper tree grid source
  vtkNew<vtkHyperTreeGridSource> fractal;
  fractal->DualOn();
  if ( dim == 3 )
    {
    fractal->SetGridSize( nX, nY, nZ );
    }
  fractal->SetGridSize( nX, nY, nZ );
  fractal->SetDimension( dim );
  fractal->SetAxisBranchFactor( branch );
  fractal->SetMaximumLevel( max );
  fractal->SetDescriptor( descriptor.c_str() );
  fractal->Update();
  vtkHyperTreeGrid* htGrid = fractal->GetOutput();
  cerr << "  Number of hyper tree dual grid cells: "
       << htGrid->GetNumberOfCells()
       << endl;

  cerr << "# Geometry" << endl;
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection( fractal->GetOutputPort() );
  vtkNew<vtkPolyDataWriter> writer4;
  writer4->SetFileName( "./hyperTreeGridGeometry2D.vtk" );
  writer4->SetInputConnection( geometry->GetOutputPort() );
  writer4->Write();
  cerr << "  Number of surface cells: "
       << geometry->GetOutput()->GetNumberOfCells()
       << endl;
  
  cerr << "# Contour" << endl;
  vtkNew<vtkContourFilter> contour;
  contour->SetInputData( htGrid );
  double* range = htGrid->GetPointData()->GetScalars()->GetRange();
  cerr << "  Calculating "
       << nContours
       << " iso-contours across ["
       << range[0]
       << ", "
       << range[1]
       << "] range:"
       << endl;
  contour->SetNumberOfContours( nContours );
  double resolution = ( range[1] - range[0] ) / ( nContours + 1. );
  double isovalue = resolution;
  for ( int i = 0; i < nContours; ++ i, isovalue += resolution )
    {
    cerr << "    Contour "
         << i
         << " at iso-value: "
         << isovalue
         << endl;
    contour->SetValue( i, isovalue );
    }
  vtkNew<vtkPolyDataWriter> writer0;
  writer0->SetFileName( "./hyperTreeGridContour2D.vtk" );
  writer0->SetInputConnection( contour->GetOutputPort() );
  writer0->Write();
  cerr << "  Number of cells in iso-contours: "
       << contour->GetOutput()->GetNumberOfCells()
       << endl;
  

  cerr << "# Shrink" << endl;
  vtkNew<vtkShrinkFilter> shrink;
  shrink->SetInputData( htGrid );
  shrink->SetShrinkFactor( 1. );
  vtkNew<vtkUnstructuredGridWriter> writer1;
  writer1->SetFileName( "./hyperTreeGridShrink2D.vtk" );
  writer1->SetInputConnection( shrink->GetOutputPort() );
  writer1->Write();
  cerr << "  Number of shrunk cells: "
       << shrink->GetOutput()->GetNumberOfCells()
       << endl;

  // Axis-aligned cut works only in 3D for now
  if ( dim == 3 )
    {
    cerr << "# HyperTreeGridAxisCut" << endl;
    vtkNew<vtkHyperTreeGridAxisCut> axisCut;
    axisCut->SetInputConnection( fractal->GetOutputPort() );
    axisCut->SetPlaneNormalAxis( 2 );
    axisCut->SetPlanePosition( .1 );
    vtkNew<vtkPolyDataWriter> writer2;
    writer2->SetFileName( "./hyperTreeGridAxisCut2D.vtk" );
    writer2->SetInputConnection( axisCut->GetOutputPort() );
    writer2->Write();
    cerr << "  Number of cells in axis cut: "
         << axisCut->GetOutput()->GetNumberOfCells()
         << endl;
    }

  cerr << "# Cut" << endl;
  vtkNew<vtkCutter> cut;
  vtkNew<vtkPlane> plane;
  plane->SetOrigin( .5, .5, .15 );
  plane->SetNormal( 0, 0, 1 );
  cut->SetInputData( htGrid );
  cut->SetCutFunction( plane.GetPointer() );
  vtkNew<vtkPolyDataWriter> writer3;
  writer3->SetFileName( "./hyperTreeGridCut2D.vtk" );
  writer3->SetInputConnection( cut->GetOutputPort() );
  writer3->Write();
  cerr << "  Number of cells in generic cut: "
       << cut->GetOutput()->GetNumberOfCells()
       << endl;

  return 0;
}
