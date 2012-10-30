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
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyDataWriter.h"
#include "vtkShrinkFilter.h"
#include "vtkStdString.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"

#include "vtksys/CommandLineArguments.hxx"

#include <vtksys/ios/sstream>

void SetInputParameters( int& dim,
                         int& branch,
                         int& nX,
                         int& nY,
                         int& nZ,
                         int max,
                         vtkStdString& str )
{
  // Ensure that parsed dimensionality makes sense
  if ( dim > 3 )
    {
    dim = 3;
    }
  else if ( dim < 1 )
    {
    dim = 1;
    }

  // Ensure that parsed branch factor makes sense
  if ( branch > 3 )
    {
    branch = 3;
    }
  else if ( branch < 2 )
    {
    branch = 2;
    }

  // Ensure that parsed grid sizes make sense
  if ( nX < 1 )
    {
    nX = 1;
    }
  if ( nY < 1 )
    {
    nY = 1;
    }
  if ( nZ < 1 )
    {
    nZ = 1;
    }

  // Ensure that parsed grid sizes are consistent with dimensionality
  if ( dim < 3 )
    {
    nZ = 1;
    if ( dim < 2 )
      {
      nY = 1;
      }
    }

  // Ensure that maximum level makes sense
  if ( max < 1 )
    {
    max = 1;
    }

  // Generate a descriptor if none was provided
  if ( str = "" )
    {
    // Calculate refined block size
    int blockSize = branch;
    for ( int i = 1; i < dim; ++ i )
      {
      blockSize *= branch;
      }

    // Initialize character stream
    vtksys_ios::ostringstream stream;

    // Seed random number generator
    vtkMath::RandomSeed( static_cast<int>( vtkTimerLog::GetUniversalTime() ) );

    // Initialize per-level cardinality
    int cardLevel = nX * nY * nZ;

    // Iterate over refinement levels
    for ( int l = 0; l < max - 1; ++ l )
      {
      // Initialize counters for this level
      int nRefined = 0;
      int nLeaves = 0;

      // Insert separator if not first level
      if ( l )
        {
        stream << '|';
        }

      // Iterate over entries in this level
      for ( int i = 0; i < cardLevel; ++ i )
        {
        // Generate next character based on pseudo-random clause
        double u = vtkMath::Random();
        if ( u < .3 )
          {
          // Refined cell
          stream << 'R';
          ++ nRefined;
          } // if ( u < .1 )
        else
          {
          // Leaf cell
          stream << '.';
          ++ nLeaves;
          } // else
        } // i
      
      // Update cardinality for next level 
      cardLevel = nRefined * blockSize;
      } // l

    // Last level contains only leaf cells
    if ( max > 1 )
      {
      // Insert separator if not first level
      stream << '|';
      }
    // Iterate over entries in this level
    for ( int i = 0; i < cardLevel; ++ i )
      {
      stream << '.';
      }

    // Finally dump stream into descriptor
    str = stream.str();
    
    } // if ( str = "" )
}

  int main( int argc, char* argv[] )
{
  // Set default argument values and options
  vtkStdString descriptor = "";
  int dim = 3;
  int branch = 3;
  int max = 3;
  int nX = 2;
  int nY = 3;
  int nZ = 2;
  double sX = 1.5;
  double sY = 1.;
  double sZ = .7;
  int nContours = 1;
  bool skipAxisCut = false;
  bool skipContour = false;
  bool skipCut = false;
  bool skipGeometry = false;
  bool skipShrink = false;
  bool printDescriptor = false;

  // Initialize command line argument parser
  vtksys::CommandLineArguments clArgs;
  clArgs.Initialize( argc, argv );
  clArgs.StoreUnusedArguments( false );

  // Parse command line parameters and options
  clArgs.AddArgument( "--dimension",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &dim, "Dimension of hyper tree grid" );

  clArgs.AddArgument( "--branch-factor",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &branch, "Branching factor of hyper tree grid" );

  clArgs.AddArgument( "--max-level",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &max, "Maximum depth of hyper tree grid" );

  clArgs.AddArgument("--descriptor",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &descriptor, "String describing the hyper tree grid");

  clArgs.AddArgument( "--grid-size-X",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &nX, "Size of hyper tree grid in X direction" );

  clArgs.AddArgument( "--grid-size-Y",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &nY, "Size of hyper tree grid in Y direction" );

  clArgs.AddArgument( "--grid-size-Z",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &nZ, "Size of hyper tree grid in Z direction" );

  clArgs.AddArgument( "--grid-scale-X",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &sX, "Scale of hyper tree grid in X direction" );

  clArgs.AddArgument( "--grid-scale-Y",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &sY, "Scale of hyper tree grid in Y direction" );

  clArgs.AddArgument( "--grid-scale-Z",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &sZ, "Scale of hyper tree grid in Z direction" );

  clArgs.AddArgument( "--contours",
                      vtksys::CommandLineArguments::SPACE_ARGUMENT,
                      &nContours, "Number of iso-contours to be calculated" );

  clArgs.AddArgument( "--skip-Axis-Cut",
                      vtksys::CommandLineArguments::NO_ARGUMENT,
                      &skipAxisCut, "Skip axis cut filter" );

  clArgs.AddArgument( "--skip-Contour",
                      vtksys::CommandLineArguments::NO_ARGUMENT,
                      &skipContour, "Skip contour filter" );

  clArgs.AddArgument( "--skip-Cut",
                      vtksys::CommandLineArguments::NO_ARGUMENT,
                      &skipCut, "Skip cut filter" );

  clArgs.AddArgument( "--skip-Geometry",
                      vtksys::CommandLineArguments::NO_ARGUMENT,
                      &skipGeometry, "Skip geometry filter" );

  clArgs.AddArgument( "--skip-Shrink",
                      vtksys::CommandLineArguments::NO_ARGUMENT,
                      &skipShrink, "Skip shrink filter" );

  clArgs.AddArgument( "--print-Descriptor",
                      vtksys::CommandLineArguments::NO_ARGUMENT,
                      &printDescriptor, "Print descriptor string" );

  // If incorrect arguments were provided, provide some help and terminate in error.
  if ( ! clArgs.Parse() )
    {
    cerr << "Usage: "
         << clArgs.GetHelp()
         << "\n";
    return 1;
    }

  // Verify and set input parameters
  SetInputParameters( dim, branch, nX, nY, nZ, max, descriptor );
  if ( printDescriptor )
    {
    cerr << "# Hyper tree grid descriptor: "
         << endl
         << descriptor
         << endl;
    }

  // Create hyper tree grid source
  vtkNew<vtkHyperTreeGridSource> source;
  source->DualOn();
  source->SetGridSize( nX, nY, nZ );
  source->SetGridScale( sX, sY, sZ );
  source->SetDimension( dim );
  source->SetAxisBranchFactor( branch );
  source->SetMaximumLevel( max );
  source->SetDescriptor( descriptor.c_str() );
  source->Update();
  vtkHyperTreeGrid* htGrid = source->GetOutput();
  cerr << "  Number of hyper tree dual grid cells: "
       << htGrid->GetNumberOfCells()
       << endl;

  if ( ! skipGeometry )
    {
    cerr << "# Geometry" << endl;
    vtkNew<vtkHyperTreeGridGeometry> geometry;
    geometry->SetInputConnection( source->GetOutputPort() );
    vtkNew<vtkPolyDataWriter> writer4;
    writer4->SetFileName( "./hyperTreeGridGeometry.vtk" );
    writer4->SetInputConnection( geometry->GetOutputPort() );
    writer4->Write();
    cerr << "  Number of surface cells: "
         << geometry->GetOutput()->GetNumberOfCells()
         << endl;
    }

  if ( ! skipContour )
    {
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
    writer0->SetFileName( "./hyperTreeGridContour.vtk" );
    writer0->SetInputConnection( contour->GetOutputPort() );
    writer0->Write();
    cerr << "  Number of cells in iso-contours: "
         << contour->GetOutput()->GetNumberOfCells()
         << endl;
    }

  if ( ! skipShrink )
    {
    cerr << "# Shrink" << endl;
    vtkNew<vtkShrinkFilter> shrink;
    shrink->SetInputData( htGrid );
    shrink->SetShrinkFactor( .5 );
    vtkNew<vtkUnstructuredGridWriter> writer1;
    writer1->SetFileName( "./hyperTreeGridShrink.vtk" );
    writer1->SetInputConnection( shrink->GetOutputPort() );
    writer1->Write();
    cerr << "  Number of shrunk cells: "
         << shrink->GetOutput()->GetNumberOfCells()
         << endl;
    }

  if ( ! skipAxisCut )
    {
    // Axis-aligned cut works only in 3D for now
    if ( dim == 3 )
      {
      cerr << "# HyperTreeGridAxisCut" << endl;
      vtkNew<vtkHyperTreeGridAxisCut> axisCut;
      axisCut->SetInputConnection( source->GetOutputPort() );
      axisCut->SetPlaneNormalAxis( 2 );
      axisCut->SetPlanePosition( .499 * nZ * sZ );
      vtkNew<vtkPolyDataWriter> writer2;
      writer2->SetFileName( "./hyperTreeGridAxisCut.vtk" );
      writer2->SetInputConnection( axisCut->GetOutputPort() );
      writer2->Write();
      cerr << "  Number of cells in axis cut: "
           << axisCut->GetOutput()->GetNumberOfCells()
           << endl;
      }
    }

  if ( ! skipCut )
    {
    cerr << "# Cut" << endl;
    vtkNew<vtkCutter> cut;
    vtkNew<vtkPlane> plane;
    plane->SetOrigin( .5, .5, .15 );
    plane->SetNormal( 0, 0, 1 );
    cut->SetInputData( htGrid );
    cut->SetCutFunction( plane.GetPointer() );
    vtkNew<vtkPolyDataWriter> writer3;
    writer3->SetFileName( "./hyperTreeGridCut.vtk" );
    writer3->SetInputConnection( cut->GetOutputPort() );
    writer3->Write();
    cerr << "  Number of cells in generic cut: "
         << cut->GetOutput()->GetNumberOfCells()
         << endl;
    }

  return 0;
}
