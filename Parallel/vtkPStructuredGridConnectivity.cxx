/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPStructuredGridConnectivity.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPStructuredGridConnectivity.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"

#include <cassert>

vtkStandardNewMacro(vtkPStructuredGridConnectivity);

//------------------------------------------------------------------------------
vtkPStructuredGridConnectivity::vtkPStructuredGridConnectivity()
{
  this->Controller       = vtkMultiProcessController::GetGlobalController();
  this->Initialized      = false;
}

//------------------------------------------------------------------------------
vtkPStructuredGridConnectivity::~vtkPStructuredGridConnectivity()
{

}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::PrintSelf(
    std::ostream& os,vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::Initialize()
{
  if( !this->Initialized )
    {
    this->Rank        = this->Controller->GetLocalProcessId();
    this->Initialized = true;
    }

}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SetNumberOfGrids( const unsigned int N )
{
  this->Superclass::SetNumberOfGrids( N );
  this->GridRanks.resize( N,-1 );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::RegisterGrid(
            const int gridID, int extents[6],
            vtkUnsignedCharArray* nodesGhostArray,
            vtkUnsignedCharArray* cellGhostArray,
            vtkPointData* pointData,
            vtkCellData* cellData,
            vtkPoints* gridNodes )
{
  assert( "pre: gridID out-of-bounds!" &&
          (gridID >= 0  && gridID < static_cast<int>(this->NumberOfGrids)));

  this->Superclass::RegisterGrid( gridID, extents, nodesGhostArray,
      cellGhostArray, pointData, cellData, gridNodes );
  this->GridIds.push_back( gridID );
  this->GridRanks[ gridID ] = this->Rank;
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::RegisterRemoteGrid(
    const int gridID, int extents[6], int process )
{
  // Sanity check
  assert( "pre: gridID out-of-bounds!" &&
          (gridID >= 0) && (gridID < static_cast<int>(this->GridRanks.size())));

  // NOTE: remote grids only have their extents since that information is
  // required to determine neighboring.
  this->Superclass::RegisterGrid(
      gridID, extents, NULL, NULL, NULL, NULL, NULL );
  this->GridRanks[ gridID ] = process;
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ComputeNeighbors()
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  assert( "pre: Null multi-process controller" && (this->Controller != NULL) );

  this->ExchangeGridExtents();
  this->Controller->Barrier();

  this->Superclass::ComputeNeighbors();
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::CreateGhostLayers( const int N )
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  if( N==0 )
    {
    vtkWarningMacro(
        "N=0 ghost layers requested! No ghost layers will be created!" );
    this->Controller->Barrier();
    return;
    }

  this->NumberOfGhostLayers += N;
  this->AllocateInternalDataStructures();
  this->GhostedExtents.resize(this->NumberOfGrids*6, -1 );

  // TODO: implement this
  this->ExchangeGhostData();
  this->Controller->Barrier();

  for( unsigned int i=0; i < this->NumberOfGrids; ++i )
    {
    this->CreateGhostedExtent( i, N );
    this->CreateGhostedMaskArrays( i );
    if( this->IsGridLocal( i )  )
      {
      this->InitializeGhostedFieldData( i );
      this->TransferRegisteredDataToGhostedData( i );
      // TODO: transfer neighbor data
      }

    } // END for all grids
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeGhostData()
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  // TODO: implement this
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeGridExtents()
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  assert( "pre: Controlles is NULL!" && (this->Controller != NULL) );

  // STEP 0: Serialize the data buffer
  int *buffer   = NULL;
  vtkIdType  N  = 0;
  this->SerializeGridExtents( buffer, N );
  assert( "pre: buffer != NULL" && (buffer != NULL) );
  assert( "pre: N > 0" && (N > 0)  );

  // STEP 1: Get the number of ints each process will send with an all gather
  vtkIdType numRanks   = this->Controller->GetNumberOfProcesses();
  vtkIdType *rcvcounts = new vtkIdType[ numRanks ];
  this->Controller->AllGather( &N, rcvcounts, 1);

  // STEP 2: Calculate the receive buffer size & Allocate
  vtkIdType rcvBufferSize = rcvcounts[0];
  for( int i=1; i < numRanks; ++i )
    rcvBufferSize += rcvcounts[i];
  int *rcvbuffer = new int[ rcvBufferSize ];
  assert( "pre: receive buffer should not be NULL" && (rcvbuffer != NULL) );

  // STEP 3: Calculate offset to the rcvbuffer for each rank
  vtkIdType *offSet = new vtkIdType[ numRanks ];
  offSet[0]   = 0;
  for( int i=1; i < numRanks; ++i )
    offSet[ i ] = offSet[ i-1 ] + rcvcounts[ i-1 ];

  // STEP 4: AllGatherv of all the extent information
  this->Controller->AllGatherV( buffer, rcvbuffer, N, rcvcounts, offSet );


  // STEP 5: Deserialize Grid Extent(s) for each remote process
  for( int i=0; i < numRanks; ++i )
    {
    if( i != this->Rank )
      {
      this->DeserializeGridExtentForProcess(rcvbuffer+offSet[i],rcvcounts[i],i);
      }// END if remote rank
    } // END for all ranks

  // STEP 6: Deallocate
  delete [] buffer;
  delete [] rcvcounts;
  delete [] rcvbuffer;
  delete [] offSet;

  // STEP 7: Synch processes
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeGridExtents(
    int *&sndbuffer, vtkIdType &N )
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  assert( "pre: send buffer is expected ot be NULL" && sndbuffer == NULL );

  //Each local extent is serialized with 7 ints:ID imin imax jmin jmax kmin kmax
  N         = this->GetNumberOfLocalGrids()*7;
  sndbuffer = new int[ N ];

  for( int i=0; i < this->GetNumberOfLocalGrids(); ++i )
    {
    int gridID = this->GridIds[ i ];
    int ext[6];
    this->GetGridExtent( gridID, ext );

    sndbuffer[ i*7 ] = gridID;
    for( int j=0; j < 6; ++j )
      {
      sndbuffer[ (i*7)+(j+1) ] = ext[ j ];
      }
    } // END for all local grids
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeGridExtentForProcess(
    int *rcvbuffer, vtkIdType &N, const int processId )
{
  // Sanity checks
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  assert( "pre: Process controller should not be NULL!" &&
          (this->Controller != NULL) );
  assert( "pre: rcvbuffer should not be NULL" && (rcvbuffer != NULL) );
  assert( "pre: must be called for a remote process" &&
          (processId != this->Rank) );
  assert( "pre: processId out-of-bounds!" && (processId >= 0) &&
          (processId < this->Controller->GetNumberOfProcesses())  );
  assert( "pre: extents must be a multiple of 7" && ( (N%7) == 0) );

  int numGrids = N/7;
  for( int i=0; i < numGrids; ++i )
    {
    int gridID = rcvbuffer[i*7];
    int ext[6];
    for( int j=0; j < 6; ++j )
      {
      ext[ j ] = rcvbuffer[ (i*7)+(j+1) ];
      }
    this->RegisterRemoteGrid( gridID, ext, processId );
    }

}
