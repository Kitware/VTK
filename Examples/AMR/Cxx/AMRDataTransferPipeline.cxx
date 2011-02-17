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
#include "vtkAssertUtils.hpp"

// Global Variables
vtkMultiProcessController *Controller;

// Function Prototypes
vtkHierarchicalBoxDataSet* GetAMRDataSet( );
vtkUniformGrid* GetGrid( double* origin,double* h,int* ndim );

int main( int argc, char **argv )
{
  Controller = vtkMPIController::New();
  Controller->Initialize( &argc, &argv );
  vtkAssertUtils::assertNotNull( Controller,__FILE__,__LINE__ );

  // STEP 0: Read In AMR data
  std::cout << "Reading data!" << std::endl;
  std::cout.flush( );

  vtkHierarchicalBoxDataSet *amrData = GetAMRDataSet();
  vtkAssertUtils::assertNotNull( amrData, __FILE__, __LINE__ );
  vtkAssertUtils::assertEquals(
   amrData->GetNumberOfLevels(),2,__FILE__,__LINE__);
  vtkAssertUtils::assertEquals(
   amrData->GetNumberOfDataSets(0),1,__FILE__,__LINE__);
  vtkAssertUtils::assertEquals(
   amrData->GetNumberOfDataSets(1),2,__FILE__,__LINE__ );


  std::cout << "Done reading!" << std::endl;
  std::cout.flush( );
  Controller->Barrier( );

  // STEP 1: Compute inter-block and inter-process connectivity
  std::cout << "Computing inter-block & inter-process connectivity!\n";
  std::cout.flush( );

  vtkAMRConnectivityFilter* connectivityFilter =
      vtkAMRConnectivityFilter::New( );
  vtkAssertUtils::assertNotNull( connectivityFilter,__FILE__,__LINE__);
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
  vtkAssertUtils::assertNotNull( transferFilter,__FILE__,__LINE__);

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

  Controller->Finalize();
  return 0;
}

//=============================================================================
//                    Function Prototype Implementation
//=============================================================================

//------------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* GetAMRDataSet( )
{
  vtkAssertUtils::assertNotNull( Controller, __FILE__, __LINE__ );

  vtkHierarchicalBoxDataSet *data = vtkHierarchicalBoxDataSet::New( );
  vtkAssertUtils::assertNotNull( data, __FILE__, __LINE__ );
  data->Initialize();

  double origin[3];
  double h[3];
  int    ndim[3];
  int    dim      =  2;
  int    blockId  = -1;
  int    level    = -1;
  int    rank     = Controller->GetLocalProcessId();
  int    numProcs = Controller->GetNumberOfProcesses();
  switch( rank )
    {
    case 0:
      {
        // BLOCK 0
        vtkAssertUtils::assertEquals( dim, 2, __FILE__, __LINE__ );
        ndim[0]     = 25;
        ndim[1]     = 25;
        ndim[2]     = 1;
        origin[0]   = 0.0;
        origin[1]   = 0.0;
        origin[2]   = 0.0;
        blockId     = 0;
        level       = 0;
        h[0] = h[1] = h[2] = 0.5;
        vtkAMRBox rmtBox0(origin,dim,ndim,h,blockId,level,rank);
        vtkUniformGrid *myGrid = GetGrid( origin,h,ndim );
        vtkAssertUtils::assertNotNull( myGrid, __FILE__, __LINE__ );
        data->SetDataSet( level,0,rmtBox0,myGrid);

        // BLOCK 1
        ndim[0]     = 10;
        ndim[1]     = 7;
        ndim[2]     = 1;
        origin[0]   = 1.5;
        origin[1]   = 1.5;
        origin[2]   = 0.0;
        blockId     = 1;
        level       = 1;
        h[0] = h[1] = h[2] = 0.25;
        vtkAMRBox rmtBox1(origin,dim,ndim,h,blockId,level,blockId%numProcs);
        data->SetDataSet( level,0,rmtBox1,NULL);

        // BLOCK 2
        ndim[0]     = 10;
        ndim[1]     = 7;
        ndim[2]     = 1;
        origin[0]   = 1.0;
        origin[1]   = 3.0;
        origin[2]   = 0.0;
        blockId     = 2;
        level       = 1;
        h[0] = h[1] = h[2] = 0.25;
        vtkAMRBox rmtBox2(origin,dim,ndim,h,blockId,level,blockId%numProcs);
        data->SetDataSet( level,1,rmtBox2,NULL);
      }
      break;
    case 1:
      {
        // BLOCK 1
        vtkAssertUtils::assertEquals( dim, 2, __FILE__, __LINE__ );
        ndim[0]     = 10;
        ndim[1]     = 7;
        ndim[2]     = 1;
        origin[0]   = 1.5;
        origin[1]   = 1.5;
        origin[2]   = 0.0;
        blockId     = 1;
        level       = 1;
        h[0] = h[1] = h[2] = 0.25;
        vtkAMRBox rmtBox1(origin,dim,ndim,h,blockId,level,rank);
        vtkUniformGrid *myGrid = GetGrid( origin,h,ndim );
        vtkAssertUtils::assertNotNull( myGrid, __FILE__, __LINE__ );
        data->SetDataSet( level,0,rmtBox1,myGrid);

        // BLOCK 0
        ndim[0]     = 25;
        ndim[1]     = 25;
        ndim[2]     = 1;
        origin[0]   = 0.0;
        origin[1]   = 0.0;
        origin[2]   = 0.0;
        blockId     = 0;
        level       = 0;
        h[0] = h[1] = h[2] = 0.5;
        vtkAMRBox rmtBox0(origin,dim,ndim,h,blockId,level,blockId%numProcs);
        data->SetDataSet( level,0,rmtBox0,NULL);

        // BLOCK 2
        ndim[0]     = 10;
        ndim[1]     = 7;
        ndim[2]     = 1;
        origin[0]   = 1.0;
        origin[1]   = 3.0;
        origin[2]   = 0.0;
        blockId     = 2;
        level       = 1;
        h[0] = h[1] = h[2] = 0.25;
        vtkAMRBox rmtBox2(origin,dim,ndim,h,blockId,level,blockId%numProcs);
        data->SetDataSet( level,1,rmtBox2,NULL);
      }
      break;
    case 2:
      {
        // BLOCK 2
        ndim[0]     = 10;
        ndim[1]     = 7;
        ndim[2]     = 1;
        origin[0]   = 1.0;
        origin[1]   = 3.0;
        origin[2]   = 0.0;
        blockId     = 2;
        level       = 1;
        h[0] = h[1] = h[2] = 0.25;
        vtkAMRBox rmtBox2(origin,dim,ndim,h,blockId,level,blockId%numProcs);
        vtkUniformGrid *myGrid = GetGrid( origin,h,ndim );
        vtkAssertUtils::assertNotNull( myGrid, __FILE__, __LINE__ );
        data->SetDataSet( level,1,rmtBox2,NULL);

        // BLOCK 0
        ndim[0]     = 25;
        ndim[1]     = 25;
        ndim[2]     = 1;
        origin[0]   = 0.0;
        origin[1]   = 0.0;
        origin[2]   = 0.0;
        blockId     = 0;
        level       = 0;
        h[0] = h[1] = h[2] = 0.5;
        vtkAMRBox rmtBox0(origin,dim,ndim,h,blockId,level,blockId%numProcs);
        data->SetDataSet( level,0,rmtBox0,NULL);

        // BLOCK 1
       ndim[0]     = 10;
       ndim[1]     = 7;
       ndim[2]     = 1;
       origin[0]   = 1.5;
       origin[1]   = 1.5;
       origin[2]   = 0.0;
       blockId     = 1;
       level       = 1;
       h[0] = h[1] = h[2] = 0.25;
       vtkAMRBox rmtBox1(origin,dim,ndim,h,blockId,level,blockId%numProcs);
       data->SetDataSet( level,0,rmtBox1,NULL);
      }
      break;
    default:
      /* Note: Code should not reach here! */
      return NULL;
    }

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
      vtkAssertUtils::assertNotNull( myCell, __FILE__, __LINE__ );

      vtkPoints *cellPoints = myCell->GetPoints();
      vtkAssertUtils::assertNotNull( cellPoints, __FILE__, __LINE__ );

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
  return grd;
}
