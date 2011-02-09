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
#include "vtkAMRBox.h"
#include "vtkAMRConnectivityFilter.h"
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
void WriteAMRData( vtkHierarchicalBoxDataSet *amrdata, const char* filename );

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
   amrData->GetNumberOfDataSets(1),1,__FILE__,__LINE__ );

  WriteAMRData( amrData, "Input" );

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

  Controller->Finalize();
  return 0;
}

//=============================================================================
//                    Function Prototype Implementation
//=============================================================================

void WriteAMRData( vtkHierarchicalBoxDataSet* amrdata, const char* filename )
{
  vtkAssertUtils::assertNotNull(amrdata,__FILE__,__LINE__);

//  vtkXMLPHierarchicalBoxDataWriter *myWriter =
//          vtkXMLPHierarchicalBoxDataWriter::New( );
//  myWriter->SetInput( amrdata );
//  myWriter->SetFileName( filename );
//  myWriter->Write();
}

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
        vtkAssertUtils::assertEquals( dim, 2, __FILE__, __LINE__ );
        ndim[0]     = 11;
        ndim[1]     = 10;
        ndim[2]     = 1;
        origin[0]   = 0.0;
        origin[1]   = 0.0;
        origin[2]   = 0.0;
        blockId     = 0;
        level       = 0;
        h[0] = h[1] = h[2] = 0.5;
        vtkAMRBox myBox(origin,dim,ndim,h,blockId,level,rank);
        vtkUniformGrid *myGrid = GetGrid( origin,h,ndim );
        vtkAssertUtils::assertNotNull( myGrid, __FILE__, __LINE__ );
        data->SetDataSet( level,0,myBox,myGrid);

        ndim[0]     = 10;
        ndim[1]     = 7;
        ndim[2]     = 1;
        origin[0]   = 1.5;
        origin[1]   = 1.5;
        origin[2]   = 0.0;
        blockId     = 1;
        level       = 1;
        h[0] = h[1] = h[2] = 0.25;
        vtkAMRBox rmtBox(origin,dim,ndim,h,blockId,level,blockId%numProcs);
        data->SetDataSet( level,0,rmtBox,NULL);
      }
      break;
    case 1:
      {
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
        vtkAMRBox myBox(origin,dim,ndim,h,blockId,level,rank);
        vtkUniformGrid *myGrid = GetGrid( origin,h,ndim );
        vtkAssertUtils::assertNotNull( myGrid, __FILE__, __LINE__ );
        data->SetDataSet( level,0,myBox,myGrid);

        ndim[0]     = 11;
        ndim[1]     = 10;
        ndim[2]     = 1;
        origin[0]   = 0.0;
        origin[1]   = 0.0;
        origin[2]   = 0.0;
        blockId     = 0;
        level       = 0;
        h[0] = h[1] = h[2] = 0.5;
        vtkAMRBox rmtBox(origin,dim,ndim,h,blockId,level,blockId%numProcs);
        data->SetDataSet( level,0,rmtBox,NULL);
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
  return grd;
}
