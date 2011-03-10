/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRDataTransferPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Description
//  A simple utility to demonstrate & test the parallel AMR functionality
//  and inter-block data transfer.

#include <cassert>
#include <mpi.h>

#include "vtkUniformGrid.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkAMRBox.h"
#include "vtkAMRConnectivityFilter.h"
#include "vtkAMRDataTransferFilter.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkXMLPHierarchicalBoxDataWriter.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkAMRUtilities.h"


// Global Variables
vtkMultiProcessController *Controller;

// Function Prototypes
vtkHierarchicalBoxDataSet* GetSerialAMRDataSet();
vtkHierarchicalBoxDataSet* GetParallelAMRDataSet();
vtkUniformGrid* GetGrid( double* origin,double* h,int* ndim );

//
// Main
//
int main( int argc, char **argv )
{
  Controller = vtkMPIController::New();
  Controller->Initialize( &argc, &argv );
  assert("pre: Controller != NULL" && (Controller != NULL ) );

  // STEP 0: Read In AMR data
  std::cout << "Constructing Sample AMR data!" << std::endl;
  std::cout.flush( );

  vtkHierarchicalBoxDataSet *amrData = NULL;

  if( Controller->GetNumberOfProcesses( ) == 3 )
    amrData = GetParallelAMRDataSet( );
  else if( Controller->GetNumberOfProcesses() == 1 )
    amrData = GetSerialAMRDataSet( );
  else
    {
      std::cerr << "Can only run with 1 or 3 MPI processes!\n";
      std::cerr.flush();
      return( -1 );
    }

  assert("pre: AMRData != NULL" && (amrData != NULL) );
  assert("pre: numLevels == 2" && (amrData->GetNumberOfLevels()==2) );

  std::cout << "Done reading!" << std::endl;
  std::cout.flush( );
  Controller->Barrier( );

  // STEP 1: Compute inter-block and inter-process connectivity
  std::cout << "Computing inter-block & inter-process connectivity!\n";
  std::cout.flush( );

  vtkAMRConnectivityFilter* connectivityFilter= vtkAMRConnectivityFilter::New( );
  connectivityFilter->SetController( Controller );
  connectivityFilter->SetAMRDataSet( amrData );
  connectivityFilter->ComputeConnectivity();

  std::cout << "Done computing connectivity!\n";
  std::cout.flush( );
  Controller->Barrier();

  connectivityFilter->Print( std::cout );
  Controller->Barrier();

  // STEP 2: Data transfer
  std::cout << "Transfering solution\n";
  std::cout.flush();

  vtkAMRDataTransferFilter* transferFilter = vtkAMRDataTransferFilter::New();

  transferFilter->SetController( Controller );
  transferFilter->SetAMRDataSet( amrData );
  transferFilter->SetNumberOfGhostLayers( 2 );
  transferFilter->SetRemoteConnectivity(
   connectivityFilter->GetRemoteConnectivity() );
  transferFilter->SetLocalConnectivity(
   connectivityFilter->GetLocalConnectivity() );
  transferFilter->Transfer();

  std::cout << "[DONE]\n";
  std::cout.flush();
  Controller->Barrier();

  // STEP 3: CleanUp
  amrData->Delete();
  connectivityFilter->Delete();
  transferFilter->Delete();

  Controller->Finalize();
  Controller->Delete();
  return 0;
}

//=============================================================================
//                    Function Prototype Implementation
//=============================================================================

//------------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* GetSerialAMRDataSet( )
{

  vtkHierarchicalBoxDataSet *data = vtkHierarchicalBoxDataSet::New( );
  data->Initialize();

  double origin[3];
  double h[3];
  int    ndim[3];
  int    dim      =  2;
  int    blockId  = -1;
  int    level    = -1;
  int    rank     = 0;

  // BLOCK 0
  ndim[0]               = 25;
  ndim[1]               = 25;
  ndim[2]               = 1;
  origin[0]             = 0.0;
  origin[1]             = 0.0;
  origin[2]             = 0.0;
  blockId               = 0;
  level                 = 0;
  h[0] = h[1] = h[2]    = 0.5;
  vtkUniformGrid *grid0 = GetGrid( origin,h,ndim );
  data->SetDataSet( level,blockId,grid0);
  grid0->Delete();

  // BLOCK 1
  ndim[0]            = 11;
  ndim[1]            = 7;
  ndim[2]            = 1;
  origin[0]          = 1.5;
  origin[1]          = 1.5;
  origin[2]          = 0.0;
  blockId            = 0;
  level              = 1;
  h[0] = h[1] = h[2] = 0.25;
  vtkUniformGrid *grid1 = GetGrid( origin,h,ndim );
  data->SetDataSet( level,blockId,grid1);
  grid1->Delete();

  // BLOCK 2
  ndim[0]               = 11;
  ndim[1]               = 7;
  ndim[2]               = 1;
  origin[0]             = 1.0;
  origin[1]             = 3.0;
  origin[2]             = 0.0;
  blockId               = 1;
  level                 = 1;
  h[0] = h[1] = h[2]    = 0.25;
  vtkUniformGrid *grid2 = GetGrid( origin,h,ndim );
  data->SetDataSet( level,blockId,grid2);
  grid2->Delete();

  vtkAMRUtilities::GenerateMetaData( data, NULL );
  data->GenerateVisibilityArrays();
  return( data );
}

//------------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* GetParallelAMRDataSet( )
{
  vtkHierarchicalBoxDataSet *data = vtkHierarchicalBoxDataSet::New( );
  data->Initialize();

  // TODO: imlpement this

  return( data );

}

//----------------------------------------------------------------------------
vtkUniformGrid* GetGrid( double *origin, double *spacing, int *ndim )
{
  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->Initialize();
  grd->SetOrigin( origin );
  grd->SetSpacing( spacing );
  grd->SetDimensions( ndim );

  vtkDoubleArray* xyz = vtkDoubleArray::New( );
  xyz->SetName( "XYZ" );
  xyz->SetNumberOfComponents( 1 );
  xyz->SetNumberOfTuples( grd->GetNumberOfCells() );
  for( int cellIdx=0; cellIdx < grd->GetNumberOfCells(); ++cellIdx )
    {
      vtkCell* myCell = grd->GetCell( cellIdx);
      assert( "post: myCell != NULL" && (myCell != NULL) );

      vtkPoints *cellPoints = myCell->GetPoints();

      double xyzCenter[3];
      xyzCenter[0] = 0.0;
      xyzCenter[1] = 0.0;
      xyzCenter[2] = 0.0;

      for( int cp=0; cp < cellPoints->GetNumberOfPoints(); ++cp )
        {
          double pnt[3];
          cellPoints->GetPoint( cp, pnt );
          xyzCenter[0] += pnt[0];
          xyzCenter[1] += pnt[1];
          xyzCenter[2] += pnt[2];
        } // END for all cell points

      xyzCenter[0] = xyzCenter[0] / (cellPoints->GetNumberOfPoints());
      xyzCenter[1] = xyzCenter[1] / (cellPoints->GetNumberOfPoints());
      xyzCenter[2] = xyzCenter[2] / (cellPoints->GetNumberOfPoints());

      double f = xyzCenter[0]*xyzCenter[0] +
          xyzCenter[1]*xyzCenter[1] + xyzCenter[2]*xyzCenter[2];
      xyz->SetTuple1(cellIdx,f);
    } // END for all cells

  grd->GetCellData()->AddArray(xyz);
  xyz->Delete();
  return grd;
}
