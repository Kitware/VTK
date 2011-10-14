/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRResamplingToUniformGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME AMRResamplingToUniformGrid.cxx -- Resamples AMR data to a uniform grid.
//
// .SECTION Description
//  A simple code that resample an AMR dataset to a uniform grid. The resampled
//  may then be used for volume rendering for example.
//

#include <iostream>
#include <cassert>
#include <cmath>

#include "vtkAMREnzoReader.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkAMRBox.h"
#include "AMRCommon.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"

// Description:
// Resamples the given AMR dataset to a uniform grid with resolution  equivalent
// the maxLevel of resolution
vtkUniformGrid* ReSample(vtkHierarchicalBoxDataSet *amrds, const int maxLevel);

// Description:
// Transfers the solution from the AMR dataset to the given re-sampled grid.
void TransferSolution( vtkUniformGrid *g, vtkHierarchicalBoxDataSet *amrds );

// Description:
// Checks if a donor cell can be found
bool FoundDonor(
  const double nx,const double ny,const double nz,
  vtkUniformGrid *donorGrid, float &yvel);

//------------------------------------------------------------------------------

int main( int argc, char **argv )
{
  if( argc != 3 )
    {
      std::cout << "Usage: ARMResamplingToUniformGrid <file> <max-resolution> \n";
      std::cout.flush();
      return( -1 );
    }

  // STEP 0: Read AMR Data
  std::cout << "Reading input data...";
  std::cout.flush();

//  vtkAMREnzoReader *myReader = vtkAMREnzoReader::New();
//  myReader->SetMaxLevel( atoi(argv[2] )  );
//  myReader->SetFileName( argv[1] );
//  myReader->Update();
//
//  myReader->SetCellArrayStatus( "y-velocity", 1 );
//  myReader->Update();
//
//  assert( myReader->GetCellArrayStatus( "y-velocity" ) == 1 );
//
//  vtkHierarchicalBoxDataSet *amrds = myReader->GetOutput();

  vtkHierarchicalBoxDataSet *amrds =
      AMRCommon::ReadAMRData( std::string( argv[1] ) );

  std::cout << "[DONE]\n";
  std::cout.flush();

  // STEP 1: Re-sample AMR data to pre-scribed resolution
  std::cout << "Re-sampling AMR data to uniform grid...";
  std::cout.flush();

  vtkUniformGrid *rsampledGrid = ReSample( amrds, atoi( argv[2] ) );

  std::cout << "[DONE]\n";
  std::cout.flush();

  // STEP 3: Transfer solution
  std::cout << "Transfer the solution...";
  std::cout.flush();

  TransferSolution( rsampledGrid, amrds );
  std::cout << "[DONE]\n";
  std::cout.flush();

  // STEP 4: Write re-sampled dataset
  double h[3];
  rsampledGrid->GetSpacing( h );
  h[ 0 ] *= 100;
  h[ 1 ] *= 100;
  h[ 2 ] *= 100;
  rsampledGrid->SetSpacing( h );
  std::cout << "Writing re-sampled grid...";
  std::cout.flush();
  AMRCommon::WriteUniformGrid( rsampledGrid, "RESAMPLED_GRID" );
  std::cout << "[DONE]\n";
  std::cout.flush();

  // STEP 3: CleanUp
  rsampledGrid->Delete();
//  amrds->Delete();
//  myReader->Delete();
  return( 0 );
}

//------------------------------------------------------------------------------
bool FoundDonor(
  const double nx,const double ny,const double nz,
  vtkUniformGrid *donorGrid, float &yvel)
{

  double x[3];
  x[0] = nx;
  x[1] = ny;
  x[2] = nz;

  int ijk[3];
  double pcoords[3];
  int status = donorGrid->ComputeStructuredCoordinates(x, ijk, pcoords );
  if( status == 1 )
    {
      vtkIdType cellIdx = vtkStructuredData::ComputeCellId(
          donorGrid->GetDimensions(), ijk );
      yvel = donorGrid->GetCellData()->
          GetArray( "GaussianPulse" )->GetComponent( cellIdx, 0 );
      return true;
    }


  return false;
}

//------------------------------------------------------------------------------
void TransferSolution( vtkUniformGrid *g, vtkHierarchicalBoxDataSet *amrds )
{
  assert( "pre: Uniform grid is NULL" && ( g != NULL )  );
  assert( "pre: AMR datasets are NULL" && ( amrds != NULL )  );

  // NOTE: We assume y-velocity as the only attribute
  vtkFloatArray *da = vtkFloatArray::New();
  da->SetName( "GaussianPulse" );
  da->SetNumberOfComponents( 1 );
  da->SetNumberOfTuples( g->GetNumberOfPoints() );
  g->GetPointData()->AddArray( da );
  g->GetPointData()->SetScalars( da );


  vtkIdType pIdx = 0;
  for( ; pIdx < g->GetNumberOfPoints(); ++pIdx )
    {

      double centroid[3];
      g->GetPoint( pIdx, centroid );

      bool found            = false;
      unsigned int levelIdx = 0;
      for( ; !found && levelIdx < amrds->GetNumberOfLevels(); ++levelIdx )
        {
          unsigned int dataIdx = 0;
          for( ; !found && dataIdx < amrds->GetNumberOfDataSets( levelIdx ); ++dataIdx )
            {
              vtkUniformGrid *donorGrid = amrds->GetDataSet( levelIdx, dataIdx );
              assert( "pre: donor grid should not be NULL!" &&
                  ( donorGrid != NULL ) );

              float yvel = 0.0;
              if( FoundDonor(
                    centroid[0], centroid[1], centroid[2],
                    donorGrid, yvel ) )
                {
                  da->SetComponent( pIdx, 0, yvel*100 );
                  found = true;
                  break;
                }
            } // END for all datasets
        } // END for all levels

      if( !found )
        {
          std::cerr << "Cannot find point-in-cell: ";
          std::cerr << "( " << centroid[0] << ", " << centroid[1] << ", ";
          std::cerr << centroid[2] << std::endl;
          std::cerr.flush();
        }

    } // END for all cells

  da->Delete();

}


//------------------------------------------------------------------------------
vtkUniformGrid* ReSample(vtkHierarchicalBoxDataSet *amrds, const int maxLevel )
{
  assert( "pre: AMR dataset is NULL!" && (amrds != NULL) );
  assert( "pre: level index is out-of-bounds" &&
    ( (maxLevel >= 0) && (maxLevel < amrds->GetNumberOfLevels() ) ) );
  assert( "pre: Number of datasets at requested level must be greater than zero" &&
    ( amrds->GetNumberOfDataSets(maxLevel) > 0) );

  // STEP 0: Get the level metadata
  vtkAMRBox lmd;
  amrds->GetMetaData( maxLevel, 0, lmd );

  amrds->GetGlobalAMRBoxWithSpacing(
      lmd, const_cast< double* >( lmd.GetGridSpacing() ) );

  vtkUniformGrid *myGrid = vtkUniformGrid::New();
  myGrid->Initialize();
  double origin[3];
  lmd.GetBoxOrigin( origin );
  myGrid->SetOrigin( origin );

  double h[3];
  lmd.GetGridSpacing( h );
  myGrid->SetSpacing( h );

  int ndim[3];
  lmd.GetNumberOfNodes( ndim );
  myGrid->SetDimensions( ndim );
  return( myGrid );
}
