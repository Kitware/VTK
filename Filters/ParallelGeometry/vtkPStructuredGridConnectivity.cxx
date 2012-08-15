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
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMultiProcessStream.h"

#include <cassert>
#include <fstream>
#include <sstream>

vtkStandardNewMacro(vtkPStructuredGridConnectivity);

//------------------------------------------------------------------------------
vtkPStructuredGridConnectivity::vtkPStructuredGridConnectivity()
{
  this->Controller         = vtkMultiProcessController::GetGlobalController();
  this->Initialized        = false;
  this->MPIRequests        = NULL;
  this->TotalNumberOfSends = 0;
  this->TotalNumberOfRcvs  = 0;
  this->TotalNumberOfMsgs  = 0;
}

//------------------------------------------------------------------------------
vtkPStructuredGridConnectivity::~vtkPStructuredGridConnectivity()
{
  // STEP 0: Delete MPI requests list
  if( this->MPIRequests == NULL )
    {
    delete [] this->MPIRequests;
    }

  // STEP 1: Clear all remote data
  this->ClearRemoteData();

  // STEP 2: Clear all raw buffers
  this->ClearRawBuffers();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::PrintSelf(
    std::ostream& os,vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
  os << "Controller: " << this->Controller << std::endl;
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

  // STEP 0: Compute neighbor send and receive extent
  for( unsigned int i=0; i < this->NumberOfGrids; ++i )
    {
    this->CreateGhostedExtent( i, N );
    this->ComputeNeighborSendAndRcvExtent( i, N );
    }

// BEGIN DEBUG
//  std::ostringstream oss;
//  oss << "Process" << this->Rank << "-neis.log";
//  std::ofstream ofs;
//  ofs.open( oss.str().c_str() );
//  assert("ERROR: cannot write log file" && ofs.is_open() );
//  this->Print( ofs );
//  ofs.close();
// END DEBUG

  this->Controller->Barrier();

  // STEP 1: Exchange ghost-data
  this->ExchangeGhostData();
  this->Controller->Barrier();

  // STEP 4: Create ghost-layers
  for( unsigned int i=0; i < this->NumberOfGrids; ++i )
    {
    if( this->IsGridLocal( i )  )
      {
      this->CreateGhostedMaskArrays( i );
      this->InitializeGhostData( i );
      this->TransferRegisteredDataToGhostedData( i );
      this->TransferGhostDataFromNeighbors( i );
      }
    } // END for all grids

  // STEP 5: Synchronize
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::TransferRemoteNeighborData(
    const int gridIdx, const int nei, const vtkStructuredNeighbor& Neighbor )
{
  // Sanity check
  assert( "pre: gridID is out-of-bounds!" &&
          (gridIdx >= 0) && (gridIdx < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: Neighbor grid ID is out-of-bounds!" &&
          (Neighbor.NeighborID >= 0) &&
          (Neighbor.NeighborID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: RemotePoints has not been properly allocated!" &&
          this->RemotePoints.size() == this->NumberOfGrids );
  assert( "pre: RemotePointData has not been properly allocated!" &&
          this->RemotePointData.size() == this->NumberOfGrids );
  assert( "pre: RemoteCellData has not been properly allocated!" &&
          this->RemoteCellData.size() == this->NumberOfGrids );


  // STEP 0: Get the ghosted grid (node) extent and cell extent
  int GhostedGridExtent[6];
  this->GetGhostedGridExtent( gridIdx, GhostedGridExtent );

  int GhostedGridCellExtent[6];
  vtkStructuredData::GetCellExtentFromNodeExtent(
      GhostedGridExtent, GhostedGridCellExtent );

  // STEP 1: Get Neighboring cell extent
  int RcvCellExtent[6];
  vtkStructuredData::GetCellExtentFromNodeExtent(
      const_cast<int*>(Neighbor.RcvExtent), RcvCellExtent);

  // STEP 2: Transfer the data
  int ijk[3];
  for( int i=Neighbor.RcvExtent[0]; i <= Neighbor.RcvExtent[1]; ++i )
    {
    for( int j=Neighbor.RcvExtent[2]; j <= Neighbor.RcvExtent[3]; ++j )
      {
      for( int k=Neighbor.RcvExtent[4]; k <= Neighbor.RcvExtent[5]; ++k )
        {
        assert( "pre: RcvExtent is outside of the GhostExtent!" &&
                this->IsNodeWithinExtent(i,j,k,GhostedGridExtent) );

        ijk[0]=i; ijk[1]=j; ijk[2]=k;

        if( this->HasPoints(gridIdx) )
          {
          // Compute the source (node) index into the remote neighbor data
          vtkIdType srcIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  const_cast<int*>(Neighbor.RcvExtent),ijk);

          // Compute the target (node) index into the ghost data
          vtkIdType targetIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  GhostedGridExtent, ijk, this->DataDescription );

          this->CopyCoordinates(
              this->RemotePoints[gridIdx][nei], srcIdx,
              this->GhostedGridPoints[gridIdx], targetIdx );
          } // END if there are grid points registered

        if( this->HasPointData(gridIdx) )
          {
          // Compute the source (node) index into the remote neighbor data
          vtkIdType srcIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  const_cast<int*>(Neighbor.RcvExtent),ijk);

          // Compute the target (node) index into the ghost data
          vtkIdType targetIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  GhostedGridExtent, ijk, this->DataDescription );

          // Transfer node data from remote to the ghosted grid data
          this->CopyFieldData(
              this->RemotePointData[gridIdx][nei],srcIdx,
              this->GhostedGridPointData[gridIdx],targetIdx);
          } // END if has remote point data

        if( this->HasCellData(gridIdx) &&
            this->IsNodeWithinExtent(i,j,k,RcvCellExtent) )
          {
          // Compute the source cell index, Note, since we are passing a cell
          // extent to ComputePointIdForExtent, the result will be a cell ID
          // and not a point ID.
          vtkIdType sourceCellIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  RcvCellExtent, ijk );

          // Compute the target cell index. Note, since we are passing a cell
          // extent to ComputePointIdForExtent, the result will be a cell ID
          // and not a point ID.
          vtkIdType targetCellIdx =
              vtkStructuredData::ComputePointIdForExtent(
                  GhostedGridCellExtent, ijk, this->DataDescription);

          // Transfer the cell data
          this->CopyFieldData(
              this->RemoteCellData[gridIdx][nei], sourceCellIdx,
              this->GhostedGridCellData[gridIdx], targetCellIdx );
          } // END if has remote cell data && is within the cell extent

        } // END for all k
      } // END for all j
    } // END for all i
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::TransferGhostDataFromNeighbors(
    const int gridID)
{
  assert( "pre: gridID is out-of-bounds!" &&
          (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids) ) );
  assert( "pre: Neighbors are not properly allocated" &&
          (this->NumberOfGrids==this->Neighbors.size() ) );
  assert( "pre: grid must be local!" && this->IsGridLocal(gridID) );

  int NumNeis = static_cast<int>(this->Neighbors[ gridID ].size());
  for( int nei=0; nei < NumNeis; ++nei )
    {
    int neiGridIdx = this->Neighbors[gridID][nei].NeighborID;
    if( this->IsGridLocal(neiGridIdx) )
      {
      this->TransferLocalNeighborData( gridID,this->Neighbors[gridID][nei] );
      }
    else
      {
      this->TransferRemoteNeighborData(
          gridID, nei, this->Neighbors[gridID][nei]);
      }
    } // END for all neighbors
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::PackGhostData()
{
  assert("pre: SendBuffers is not properly allocated!" &&
         this->SendBuffers.size()==this->NumberOfGrids );
  assert("pre: RcvBuffers is not properly allocated!" &&
         this->RcvBuffers.size()==this->NumberOfGrids );

  for( unsigned int idx=0; idx < this->GridIds.size(); ++idx )
    {
    int gridIdx = this->GridIds[ idx ];
    assert( "ERROR: grid index is out-of-bounds!" &&
            (gridIdx >= 0) &&
            (gridIdx < static_cast<int>(this->NumberOfGrids)));

    int NumNeis = this->GetNumberOfNeighbors( gridIdx );
    this->SendBuffers[gridIdx].resize( NumNeis, NULL );
    this->RcvBuffers[gridIdx].resize( NumNeis, NULL);
    this->RcvBufferSizes[gridIdx].resize( NumNeis, 0 );
    this->SendBufferSizes[gridIdx].resize( NumNeis, 0 );

    for(int nei=0; nei < NumNeis; ++nei )
      {
      this->RcvBufferSizes[gridIdx][nei] = 0;

      int neiGridIdx = this->Neighbors[ gridIdx ][ nei ].NeighborID;
      assert( "ERROR: neighbor grid index is out-of-bounds" &&
              (neiGridIdx >= 0) &&
              (neiGridIdx < static_cast<int>(this->NumberOfGrids) ) );

      if( this->IsGridRemote( neiGridIdx ) )
        {
        this->TotalNumberOfSends++;
        this->TotalNumberOfRcvs++;

        this->SerializeGhostData(
            gridIdx, neiGridIdx,
            this->Neighbors[gridIdx][nei].SendExtent,
            this->SendBuffers[gridIdx][nei],
            this->SendBufferSizes[gridIdx][nei] );
        } // END if the neighboring grid is remote

      } // END for all neighbors
    } // END for all local grids

  this->TotalNumberOfMsgs = this->TotalNumberOfRcvs + this->TotalNumberOfSends;
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeBufferSizes(
    int *&sizesbuf, vtkIdType &N)
{
  assert("pre: sizes buffer must be NULL" && (sizesbuf==NULL) );
  assert("pre: Number of sends should be at least 1" &&
         (this->TotalNumberOfSends >= 1) );

  int k = 0;
  for( unsigned int idx=0; idx < this->GridIds.size(); ++idx )
    {
    k += this->GetNumberOfNeighbors( this->GridIds[idx] );
    }
  N        = 3*k;
  sizesbuf = new int[ N ];
  assert("pre: Cannot allocate sizes buffer" && (sizesbuf != NULL) );

  int bidx = 0; // index to the buffer
  for( unsigned int idx=0; idx < this->GridIds.size(); ++idx )
    {
    int gridIdx = this->GridIds[ idx ];
    assert("pre: grid index is out-of-bounds" &&
           (gridIdx >= 0) && (gridIdx < static_cast<int>(this->NumberOfGrids)));

    int NumNeis = this->GetNumberOfNeighbors( gridIdx );
    for( int nei=0; nei < NumNeis; ++nei )
      {
      // Sender grid
      sizesbuf[bidx] = gridIdx;
      ++bidx;

      // Receiver grid
      sizesbuf[bidx] = this->Neighbors[gridIdx][nei].NeighborID;
      ++bidx;

      // Buffer size
      sizesbuf[bidx] = this->SendBufferSizes[gridIdx][nei];
      ++bidx;
      } // END for all neighbors
    } // END for all local grids
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeBufferSizesForProcess(
    int *buffersizes, vtkIdType N, const int vtkNotUsed(processId) )
{
  assert("pre: Controller should not be NULL" && (this->Controller != NULL) );
  assert("pre: Cannot deserialize empty buffer size" && (buffersizes != NULL) );
  assert("pre: Buffer size should not be empty!" && (N > 0) );
  assert("pre: Buffer size must be a multiple of 3" && ( (N%3)==0) );
  assert("pre: RcvBuffersizes is not properly allocated!" &&
          this->RcvBufferSizes.size() == this->NumberOfGrids);

  int NumTuples = N/3;
  for( int i=0; i < NumTuples; ++i )
    {
    int senderGrid = buffersizes[ i*3 ];
    int rcvGrid    = buffersizes[ i*3+1 ];
    int size       = buffersizes[ i*3+2 ];

    if( this->IsGridLocal( rcvGrid ) )
      {
      int neiIndex = this->GetNeighborIndex(rcvGrid,senderGrid);
      assert("ERROR: rcver grid is out-of-bounds!" &&
             (rcvGrid >= 0) &&
             (rcvGrid < static_cast<int>(this->NumberOfGrids)));
      assert("ERROR: neighbor index is out-of-bounds!" &&
        (neiIndex >= 0) &&
        (neiIndex < static_cast<int>(this->RcvBufferSizes[rcvGrid].size())));

      this->RcvBufferSizes[ rcvGrid ][ neiIndex ] = size;
      } // END if the grid is local
    } // END for all tuples

}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeBufferSizes()
{
  vtkIdType N      = 0;
  int *sizesbuffer = NULL;
  this->SerializeBufferSizes( sizesbuffer, N );
  assert("ERROR: sizesbuffer is NULL!" && (sizesbuffer != NULL));
  assert("ERROR: N > 0" && (N > 0));

  // STEP 1: Get the number of ints each process will send with an all gather
  vtkIdType numRanks   = this->Controller->GetNumberOfProcesses();
  vtkIdType *rcvcounts = new vtkIdType[ numRanks ];
  this->Controller->AllGather( &N, rcvcounts, 1);

  // STEP 2: Calculate the receive buffer size & Allocate
  vtkIdType rcvBufferSize = rcvcounts[0];
  for( int i=1; i < numRanks; ++i )
    {
    rcvBufferSize += rcvcounts[i];
    }
  int *rcvbuffer = new int[ rcvBufferSize ];
  assert( "pre: receive buffer should not be NULL" && (rcvbuffer != NULL) );

  // STEP 3: Calculate offset to the rcvbuffer for each rank
  vtkIdType *offSet = new vtkIdType[ numRanks ];
  offSet[0]   = 0;
  for( int i=1; i < numRanks; ++i )
    {
    offSet[ i ] = offSet[ i-1 ] + rcvcounts[ i-1 ];
    }

  // STEP 4: AllGatherv of all the remote buffer size information
  this->Controller->AllGatherV( sizesbuffer, rcvbuffer, N, rcvcounts, offSet );

  // STEP 5: Deserialize Grid Extent(s) for each remote process
  for( int i=0; i < numRanks; ++i )
    {
    if( i != this->Rank )
      {
      this->DeserializeBufferSizesForProcess(
          rcvbuffer+offSet[i], rcvcounts[i], i);
      }// END if remote rank
    } // END for all ranks

  // STEP 6: Deallocate
  delete [] sizesbuffer;
  delete [] rcvcounts;
  delete [] rcvbuffer;
  delete [] offSet;

  // STEP 7: Synch processes
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::UnpackGhostData()
{
  assert("pre: RcvBuffers is not properly allocated!" &&
         this->RcvBuffers.size()==this->NumberOfGrids );

  for( unsigned int idx=0; idx < this->GridIds.size(); ++idx )
    {
    int gridIdx = this->GridIds[ idx ];
    assert("ERROR: grid index is out-of-bounds!" &&
           (gridIdx >= 0) &&
           (gridIdx < static_cast<int>(this->NumberOfGrids)));

    int NumNeis = this->GetNumberOfNeighbors( gridIdx );
    assert("ERROR: rcv buffers for grid are not properly allocated" &&
           (static_cast<int>(this->RcvBuffers[gridIdx].size())==NumNeis));

    for( int nei=0; nei < NumNeis; ++nei )
      {
      // Get the global grid index of the neighboring grid
      int neiGridIdx = this->Neighbors[ gridIdx ][ nei ].NeighborID;
      assert( "ERROR: neighbor grid index is out-of-bounds" &&
                    (neiGridIdx >= 0) &&
                    (neiGridIdx < static_cast<int>(this->NumberOfGrids) ) );

//      int neiListIndex = this->GetNeighborIndex( gridIdx, neiGridIdx );
//      assert("ERROR: neiListIndex mismatch!" && (neiListIndex==nei) );

      if( this->IsGridRemote(neiGridIdx) )
        {
        this->DeserializeGhostData(
            gridIdx,nei,neiGridIdx,this->Neighbors[ gridIdx ][ nei ].RcvExtent,
            this->RcvBuffers[ gridIdx ][ nei ],
            this->RcvBufferSizes[ gridIdx ][ nei ] );
        } // END if the grid is remote
      } // END for all neighbors

    } // END for all local grids
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeGhostDataInit()
{
  // STEP 0: Pack ghost data
  this->PackGhostData();

  // STEP 1: Exchange buffer size
  this->ExchangeBufferSizes();

  // STEP 2: Synchronize
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::PostReceives()
{
  // STEP 0: Acquire MPI controller from supplied Multi-Process controller
  vtkMPIController *myMPIController =
        vtkMPIController::SafeDownCast(this->Controller);
  assert("pre: Cannot acquire MPI controller" && (myMPIController != NULL) );

  // STEP 1: Loop through all local grids and post receives
  int rqstIdx = 0;
  for( unsigned int idx=0; idx < this->GridIds.size(); ++idx )
    {
    int gridIdx = this->GridIds[ idx ];
    assert("ERROR: grid index is out-of-bounds" &&
           (gridIdx >= 0) &&
           (gridIdx < static_cast<int>(this->NumberOfGrids) ) );
    assert("ERROR: grid must be local" && this->IsGridLocal( gridIdx ) );
    assert("ERROR: grid rcv buffers must be 1-to-1 with the grid neighbors" &&
           this->Neighbors[gridIdx].size()==this->RcvBuffers[gridIdx].size() );
    assert("ERROR: grid rcv buffers must be 1-to-1 with the rcv buffer sizes" &&
      this->RcvBufferSizes[gridIdx].size()==this->RcvBuffers[gridIdx].size() );

    size_t NumNeis = this->Neighbors[ gridIdx ].size();
    for( size_t nei=0; nei < NumNeis; ++nei )
      {
      int neiGridIdx = this->Neighbors[gridIdx][nei].NeighborID;
      assert("ERROR: Neighbor grid index is out-of-bounds!" &&
             (neiGridIdx >= 0) &&
             (neiGridIdx < static_cast<int>(this->NumberOfGrids) ) );
      if( this->IsGridLocal( neiGridIdx ) )
        {
        // The neighboring grid is local, thus, the ghost data are transfered
        // directly using vtkStructuredGrid::TransferLocalNeighborData().
        // Consequently, there is no need for any communication.
        continue;
        }

      int NeighborRank = this->GetGridRank( neiGridIdx );

      assert("pre: RcvBuffer must be NULL!" &&
             (this->RcvBuffers[gridIdx][nei] == NULL));

      this->RcvBuffers[gridIdx][nei] =
           new unsigned char[ this->RcvBufferSizes[gridIdx][nei]  ];
      assert("ERROR: Could not allocate RcvBuffer!" &&
             this->RcvBuffers[gridIdx][nei] );
      assert("pre: RequestIndex is out-of-bounds!" &&
             (rqstIdx >= 0) && (rqstIdx < this->TotalNumberOfMsgs) );

      unsigned char *bufferPtr = this->RcvBuffers[gridIdx][nei];
      int length               = this->RcvBufferSizes[ gridIdx][nei];
      myMPIController->NoBlockReceive(
          bufferPtr,length,NeighborRank,neiGridIdx,this->MPIRequests[rqstIdx]);
      ++rqstIdx;
      } // END for all neis
    } // END for all local grids

    assert(this->TotalNumberOfRcvs  == rqstIdx);
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::PostSends()
{
  // STEP 0: Acquire MPI controller from supplied Multi-Process controller
  vtkMPIController *myMPIController =
        vtkMPIController::SafeDownCast(this->Controller);
  assert("pre: Cannot acquire MPI controller" && (myMPIController != NULL) );

  // STEP 1: Loop through all local grids and post receives
  int rqstIdx = this->TotalNumberOfRcvs;
  for( unsigned int idx=0; idx < this->GridIds.size(); ++idx )
    {
    int gridIdx = this->GridIds[ idx ];
    assert("ERROR: grid index is out-of-bounds" &&
           (gridIdx >= 0) &&
           (gridIdx < static_cast<int>(this->NumberOfGrids) ) );
    assert("ERROR: grid must be local" && this->IsGridLocal(gridIdx) );
    assert("ERROR: grid rcv buffers must be 1-to-1 with the grid neighbors" &&
           this->Neighbors[gridIdx].size()==this->SendBuffers[gridIdx].size() );
    assert("ERROR: grid rcv buffers must be 1-to-1 with the rcv buffer sizes" &&
      this->SendBufferSizes[gridIdx].size()==this->SendBuffers[gridIdx].size() );

    int NumNeis = static_cast<int>(this->Neighbors[gridIdx].size());
    for( int nei=0; nei < NumNeis; ++nei )
      {
      int neiGridIdx = this->Neighbors[gridIdx][nei].NeighborID;
      assert("ERROR: Neighbor grid index is out-of-bounds!" &&
             (neiGridIdx >= 0) &&
             (neiGridIdx < static_cast<int>(this->NumberOfGrids) ) );
      if( this->IsGridLocal( neiGridIdx ) )
        {
        // The neighboring grid is local, thus, the ghost data are transfered
        // directly using vtkStructuredGrid::TransferLocalNeighborData().
        // Consequently, there is no need for any communication.
        continue;
        }

      int NeighborRank = this->GetGridRank( neiGridIdx );

      assert("pre: RcvBuffer must be NULL!" &&
             (this->SendBuffers[gridIdx][nei] != NULL));
      assert("pre: RequestIndex is out-of-bounds!" &&
             (rqstIdx >= 0) && (rqstIdx < this->TotalNumberOfMsgs) );
      unsigned char *bufferPtr = this->SendBuffers[gridIdx][nei];
      int length               = this->SendBufferSizes[gridIdx][nei];
      myMPIController->NoBlockSend(
          bufferPtr,length,NeighborRank,gridIdx,this->MPIRequests[rqstIdx]);
      ++rqstIdx;
      } // END for all neis
    } // END for all local grids

  assert(rqstIdx == this->TotalNumberOfMsgs);
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::CommunicateGhostData()
{
  // STEP 0: Sanity Checks!
  assert("pre: Instance has not been initialized!" && this->Initialized );
  assert("pre: RcvBuffers is not properly allocated" &&
         (this->RcvBuffers.size() == this->NumberOfGrids) );
  assert("pre: RcvBufferSizes is not properly allocated" &&
         (this->RcvBufferSizes.size() == this->NumberOfGrids ) );
  assert("pre: Neighbors have not been computed!" &&
          this->Neighbors.size() == this->NumberOfGrids );
  assert("pre: MPI requests array must be NULL!" &&
         (this->MPIRequests==NULL));

  // STEP 1: Allocate the MPI requests array
  this->MPIRequests = new vtkMPICommunicator::Request[this->TotalNumberOfMsgs];
  assert("pre:Could not alloc MPI requests array" && (this->MPIRequests!=NULL));

  // STEP 2: Allocate receive buffers and post receives
  this->PostReceives();
  this->Controller->Barrier();

  // STEP 3: Post sends
  this->PostSends();
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeGhostDataPost()
{
  vtkMPIController *myMPIController =
      vtkMPIController::SafeDownCast(this->Controller);
  assert("pre: Cannot acquire MPI controller" && (myMPIController != NULL) );

  // STEP 0: Block until all communication is completed
  myMPIController->WaitAll(this->TotalNumberOfMsgs,this->MPIRequests);

  // STEP 1: Process receive buffers
  this->UnpackGhostData();

  // STEP 2: De-allocate receive buffers
  this->ClearRawBuffers();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeGhostData()
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );

  // STEP 0:
  this->InitializeMessageCounters();

  // STEP 1: Allocate internal data-structures
  this->RemotePoints.resize( this->NumberOfGrids);
  this->RemotePointData.resize( this->NumberOfGrids);
  this->RemoteCellData.resize( this->NumberOfGrids);
  for( unsigned int i=0; i < this->NumberOfGrids; ++i )
    {
    this->RemotePoints[ i ].resize( this->GetNumberOfNeighbors(i),    NULL );
    this->RemotePointData[ i ].resize( this->GetNumberOfNeighbors(i), NULL );
    this->RemoteCellData[ i ].resize( this->GetNumberOfNeighbors(i),  NULL );
    }

  this->SendBuffers.resize(this->NumberOfGrids);
  this->RcvBuffers.resize(this->NumberOfGrids);
  this->SendBufferSizes.resize(this->NumberOfGrids);
  this->RcvBufferSizes.resize(this->NumberOfGrids);

  // STEP 2: Serialize the ghost data and exchange buffer sizes
  this->ExchangeGhostDataInit();

  // STEP 3: Allocate rcv buffers and perform non-blocking communication
  this->CommunicateGhostData();

  // STEP 4: Block until communication is complete and raw rcv buffers are
  // de-serialized into the VTK data-structures.
  this->ExchangeGhostDataPost();

  // STEP 5: Synchronize with all processes
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeGhostPoints(
    const int gridIdx, int ext[6], vtkMultiProcessStream& bytestream )
{
  assert("pre: gridIdx is out-of-bounds" &&
         (gridIdx >= 0) && (gridIdx < static_cast<int>(this->NumberOfGrids)));
  assert("pre: GridPoints is not properly allocated" &&
          this->GridPoints.size() == this->NumberOfGrids);

  // STEP 0: Check if the user has registered points for this grid instance
  if( this->GridPoints[ gridIdx ] == NULL )
    {
    // If not points are registered put a 0 in the bytestream and return!
    bytestream << 0;
    return;
    }

  // STEP 1: Otherwise, put a "1" in the bytestream to indicate that the are
  // points included in the bytestream
  bytestream << 1;

  // STEP 2: Get the grid extent of the send grid
  int GridExtent[6];
  this->GetGridExtent( gridIdx, GridExtent );

  // STEP 3: Compute the number of nodes in the send extent
  int dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
  int N = vtkStructuredData::GetNumberOfNodes(ext, dataDescription );
//  bytestream << N;

  // STEP 4: Allocate and store points in a temporary array
  double *pnts = new double[ 3*N ];
  assert( "Cannot allocate temporary pnts array" && (pnts != NULL) );

  int ijk[3];
  double x[3];
  for( int i=ext[0]; i <= ext[1]; ++i )
    {
    for( int j=ext[2]; j <= ext[3]; ++j )
      {
      for( int k=ext[4]; k <= ext[5]; ++k )
        {
        assert("pre: IJK must be within grid extent!" &&
               this->IsNodeWithinExtent(i,j,k,GridExtent) );

        ijk[0]=i; ijk[1]=j; ijk[2]=k;

        // Compute the source index
        vtkIdType sourceIdx =
            vtkStructuredData::ComputePointIdForExtent(GridExtent,ijk);
        assert("pre: sourceIdx is out-of-bounds" &&
               (sourceIdx >= 0) &&
               (sourceIdx < this->GridPoints[gridIdx]->GetNumberOfPoints()) );

        this->GridPoints[ gridIdx ]->GetPoint(sourceIdx,x);

        // Compute the target index
        vtkIdType targetIdx =
            vtkStructuredData::ComputePointIdForExtent(ext,ijk,dataDescription);
        assert("pre: targetIdx is out-of-bounds" &&
               (targetIdx >= 0) && (targetIdx < N) );

        // Store the point
        pnts[ targetIdx*3   ] = x[ 0 ];
        pnts[ targetIdx*3+1 ] = x[ 1 ];
        pnts[ targetIdx*3+2 ] = x[ 2 ];
        } // END for all k
      } // END for all j
    } // END for all i

  // STEP 5: Push the points on the bytestream
  bytestream.Push( pnts, 3*N );
  delete [] pnts;
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeGhostPoints(
       const int gridIdx, const int nei,
       int ext[6], vtkMultiProcessStream& bytestream )
{
  assert("pre:Cannot deserialize an empty bytestream" && !bytestream.Empty());
  assert("pre:Grid index is out-of-bounds" &&
         (gridIdx >= 0 ) &&
         (gridIdx < static_cast<int>(this->GetNumberOfGrids())));
  assert("pre:Neighbor list index is out-of-bounds" &&
         (nei >=0) && (nei < this->GetNumberOfNeighbors(gridIdx)));
  assert("pre:Remote points is not properly allocated!" &&
          this->RemotePoints.size() == this->NumberOfGrids );


  // STEP 0: Check if there are serialized points in the bytestream
  int hasPoints;
  bytestream >> hasPoints;

  if( hasPoints == 0 )
    {
    return;
    }

  assert("pre:Remote points for grid is not properly allocated!" &&
     (this->GetNumberOfNeighbors(gridIdx)==
         static_cast<int>(this->RemotePoints[gridIdx].size())));

  // STEP 1: If there are points, deserialize them
  int dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
  int N = vtkStructuredData::GetNumberOfNodes(ext,dataDescription);

  // STEP 2: Pop the points from the bytestream
  double *pnts      = NULL;
  unsigned int size = 0;
  bytestream.Pop( pnts, size );
  assert("pre: deserialize ghosts points array is NULL" &&
         (pnts != NULL) );
  assert("pre: points array is not of the expected size!" &&
         (static_cast<int>(size) == 3*N) );

  this->RemotePoints[ gridIdx ][ nei ] = vtkPoints::New();
  this->RemotePoints[ gridIdx ][ nei ]->SetDataTypeToDouble();
  this->RemotePoints[ gridIdx ][ nei ]->SetNumberOfPoints( N );
  for( int i=0; i < N ; ++i )
    {
    this->RemotePoints[ gridIdx ][ nei ]->SetPoint(
        i, pnts[i*3], pnts[i*3+1], pnts[i*3+2] );
    }

  delete [] pnts;
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeDataArray(
    vtkDataArray *dataArray, vtkMultiProcessStream& bytestream )
{
  assert("pre: Cannot serialize a NULL data array" && (dataArray != NULL) );

  // STEP 0: Compute number of elements in flat array
  int k       = dataArray->GetNumberOfComponents();
  assert("pre: number of components must be at least 1" && (k>=1) );
  vtkIdType N = dataArray->GetNumberOfTuples();
  assert("pre: number of elements must be at least 1" && (N>=1) );

  unsigned int size = N*k;

  // STEP 1: Push the raw data into the bytestream
  // TODO: Add more cases here
  switch( dataArray->GetDataType( ) )
    {
    case VTK_FLOAT:
      bytestream.Push(
          static_cast<float*>(dataArray->GetVoidPointer(0)), size );
      break;
    case VTK_DOUBLE:
      bytestream.Push(
          static_cast<double*>(dataArray->GetVoidPointer(0)), size );
      break;
    case VTK_INT:
      bytestream.Push(
          static_cast<int*>(dataArray->GetVoidPointer(0)), size );
      break;
    default:
      vtkErrorMacro("Cannot serialize data array of this type");
    }
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeDataArray(
    vtkDataArray *&dataArray,
    const int dataType,
    const int numberOfTuples,
    const int numberOfComponents,
    vtkMultiProcessStream& bytestream )
{
  assert("pre: Cannot deserialize an empty bytestream" && !bytestream.Empty());

  switch( dataType )
    {
    case VTK_FLOAT:
      {
      // STEP 0: Get the raw data
      unsigned int size = 0;
      float *data       = NULL;
      bytestream.Pop( data, size );
      assert( "pre: de-serialized data array is not of the expected size" &&
              (size == static_cast<unsigned int>((numberOfTuples*numberOfComponents)) ) );

      // STEP 1: Allocate vtkdata array
      dataArray = vtkDataArray::CreateDataArray( dataType );
      dataArray->SetNumberOfComponents( numberOfComponents );
      dataArray->SetNumberOfTuples( numberOfTuples );

      // STEP 2: Copy the data
      float *dataArrayPtr = static_cast<float*>(dataArray->GetVoidPointer(0));
      for( int i=0; i < numberOfTuples; ++i )
        {
        for( int j=0; j < numberOfComponents; ++j )
          {
          dataArrayPtr[ i*numberOfComponents+j ] =
              data[ i*numberOfComponents+j ];
          } // END for all components
        }// END for all tuples
      delete [] data;
      } // END VTK_FLOAT case
      break;
    case VTK_DOUBLE:
      {
      // STEP 0: Get the raw data
      unsigned int size = 0;
      double *data      = NULL;
      bytestream.Pop(data,size);
      assert( "pre: de-serialized data array is not of the expected size" &&
              (size == static_cast<unsigned int>((numberOfTuples*numberOfComponents)) ) );

      // STEP 1: Allocate vtkdata array
      dataArray = vtkDataArray::CreateDataArray( dataType );
      dataArray->SetNumberOfComponents(numberOfComponents);
      dataArray->SetNumberOfTuples(numberOfTuples);

      // STEP 2: Copy the data
      double *dataArrayPtr = static_cast<double*>(dataArray->GetVoidPointer(0));
      for( int i=0; i < numberOfTuples; ++i )
        {
        for( int j=0; j < numberOfComponents; ++j )
          {
          dataArrayPtr[ i*numberOfComponents+j ] =
              data[ i*numberOfComponents+j ];
          } // END for all components
        } // END for all tuples
      delete [] data;
      } // END VTK_DOUBLE case
      break;
    case VTK_INT:
      {
      // STEP 0: Get the raw data
      unsigned int size = 0;
      int *data         = NULL;
      bytestream.Pop(data,size);
      assert( "pre: de-serialized data array is not of the expected size" &&
              (size == static_cast<unsigned int>((numberOfTuples*numberOfComponents)) ) );

      // STEP 1: Allocate vtkdata array
      dataArray = vtkDataArray::CreateDataArray( dataType );
      dataArray->SetNumberOfComponents( numberOfComponents );
      dataArray->SetNumberOfTuples( numberOfTuples );

      // STEP 2: Copy the data
      int *dataArrayPtr = static_cast<int*>(dataArray->GetVoidPointer(0));
      for( int i=0; i < numberOfTuples; ++i )
        {
        for( int j=0; j < numberOfComponents; ++j )
          {
          dataArrayPtr[ i*numberOfComponents+j ] =
              data[ i*numberOfComponents+j ];
          } // END for all components
        } // END for all tuples
      delete [] data;
      } // END VTK_INT case
      break;
    default:
      vtkErrorMacro("Cannot de-serialize data array of this type");
      assert(false);
    }
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeFieldData(
    int GridExtent[6], int ext[6], vtkFieldData *fieldData,
    vtkMultiProcessStream& bytestream )
{
  assert("pre: FieldData is NULL" && (fieldData != NULL) );

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  // For each array:
  for( int array=0; array < fieldData->GetNumberOfArrays(); ++array )
    {
    vtkDataArray *myArray = fieldData->GetArray( array );
    assert( "pre: attempting to serialize a NULL array!" && (myArray!=NULL) );

    int dataType  = myArray->GetDataType();
    int numComp   = myArray->GetNumberOfComponents();
    int numTuples = vtkStructuredData::GetNumberOfNodes(ext);

    // STEP 1: Write the datatype and number of components
    bytestream << dataType << numTuples << numComp;
    bytestream << std::string( myArray->GetName() );

    // STEP 2: Extract the ghost data within the given ext
    // Allocate the ghost array where the data will be extracted
    vtkDataArray *ghostArray =
        vtkDataArray::CreateDataArray( myArray->GetDataType() );

    ghostArray->SetName( myArray->GetName() );
    ghostArray->SetNumberOfComponents( numComp );
    ghostArray->SetNumberOfTuples( numTuples );

    int ijk[3];
    for( int i=ext[0]; i <= ext[1]; ++i )
      {
      for( int j=ext[2]; j <= ext[3]; ++j )
        {
        for( int k=ext[4]; k <= ext[5]; ++k )
          {
          ijk[0]=i; ijk[1]=j; ijk[2]=k;
          assert("pre: IJK is outside the grid extent!" &&
                  this->IsNodeWithinExtent(i,j,k,GridExtent) );

          // Compute the source index from the grid extent. Note, this could
          // be a cell index if the incoming GridExtent and ext are cell extents.
          vtkIdType sourceIdx =
              vtkStructuredData::ComputePointIdForExtent(GridExtent,ijk);
          assert("pre: source index is out-of-bounds!" &&
                 (sourceIdx >=  0) &&
                 (sourceIdx < myArray->GetNumberOfTuples() ) );

          // Compute the target index from the grid extent. Note, this could
          // be a cell index if the incoming GridExtent and ext are cell extents.
          vtkIdType targetIdx =
              vtkStructuredData::ComputePointIdForExtent(ext,ijk);
          assert("pre: target index is out-of-bounds!" &&
                 (targetIdx >= 0) &&
                 (targetIdx < ghostArray->GetNumberOfTuples() ) );

          ghostArray->SetTuple( targetIdx, sourceIdx, myArray );
          } // END for k
        } // END for j
      } // END for i

    // STEP 3: Serialize the ghost array
    this->SerializeDataArray( ghostArray, bytestream );
    ghostArray->Delete();
    } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeFieldData(
    int* vtkNotUsed(ext), vtkFieldData* fieldData, vtkMultiProcessStream& bytestream )
{
  assert("pre: Cannot deserialize an empty bytestream" && !bytestream.Empty());
  assert("pre: field data should not be NULL!" && (fieldData != NULL) );

  int NumberOfArrays;
  bytestream >> NumberOfArrays;
  assert("ERROR: number of arrays must be greater or equal to 1" &&
         (NumberOfArrays >= 1) );

  vtkDataArray *dataArray;
   int dataType;
   int numTuples;
   int numComponents;
   std::string arrayName;

  for( int i=0; i < NumberOfArrays; ++i )
    {
    dataArray = NULL;
    bytestream >> dataType >> numTuples >> numComponents;
    bytestream >> arrayName;

    this->DeserializeDataArray(
        dataArray, dataType, numTuples, numComponents, bytestream );
    assert("ERROR: data array should not be NULL!" && (dataArray != NULL));

    dataArray->SetName( arrayName.c_str() );
    fieldData->AddArray( dataArray );
    dataArray->Delete();
    } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeGhostPointData(
    const int gridIdx, int ext[6], vtkMultiProcessStream& bytestream )
{
  assert("pre: Grid to be serialized must be local" &&
          this->IsGridLocal( gridIdx ) );
  assert("pre: gridIdx is out-of-bounds" &&
         (gridIdx >= 0) && (gridIdx < static_cast<int>(this->NumberOfGrids)));
  assert("pre: GridPointData is not properly allocated" &&
         (this->GridPointData.size()==this->NumberOfGrids));

  if( (this->GridPointData[gridIdx] == NULL) ||
      (this->GridPointData[gridIdx]->GetNumberOfArrays() == 0) )
    {
    bytestream << 0;
    return;
    }

  // STEP 0: Get the grid's node extent
  int GridExtent[6];
  this->GetGridExtent( gridIdx, GridExtent );

  // STEP 1: Serialize the node data
  bytestream << 1;
  this->SerializeFieldData(
      GridExtent, ext, this->GridPointData[gridIdx], bytestream );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeGhostPointData(
    const int gridIdx, const int nei,int ext[6],
    vtkMultiProcessStream& bytestream )
{
  assert("pre: Cannot deserialize an empty bytestream" && !bytestream.Empty());
  assert("pre:Grid index is out-of-bounds" &&
          (gridIdx >= 0 ) &&
          (gridIdx < static_cast<int>(this->GetNumberOfGrids())));
  assert("pre: Grid to be serialized must be local" &&
         this->IsGridLocal(gridIdx) );
  assert("pre:Neighbor list index is out-of-bounds" &&
         (nei >=0) && (nei < this->GetNumberOfNeighbors(gridIdx)));
  assert("pre:Remote point data is not properly allocated!" &&
         this->RemotePointData.size() == this->NumberOfGrids );

  // STEP 0: Check if there are point data in the byte-stream
  int hasPointData;
  bytestream >> hasPointData;
  if( hasPointData == 0 )
    {
    return;
    }

  assert("pre:Remote point data for grid is not properly allocated!" &&
      (this->GetNumberOfNeighbors(gridIdx)==
       static_cast<int>(this->RemotePointData[gridIdx].size())));

  // STEP 1: De-serialize the point data
  this->RemotePointData[gridIdx][nei] = vtkPointData::New();
  this->DeserializeFieldData(ext,this->RemotePointData[gridIdx][nei],bytestream);
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeGhostCellData(
    const int gridIdx, int ext[6], vtkMultiProcessStream& bytestream )
{
  assert("pre: Grid to be serialized must be local" &&
          this->IsGridLocal( gridIdx ) );
  assert("pre: gridIdx is out-of-bounds" &&
         (gridIdx >= 0) && (gridIdx < static_cast<int>(this->NumberOfGrids)));
  assert("pre: GridCellData is not properly allocated" &&
         (this->GridCellData.size()==this->NumberOfGrids));

  if( this->GridCellData[gridIdx] == NULL ||
      this->GridCellData[gridIdx]->GetNumberOfArrays() == 0 )
    {
    bytestream << 0;
    return;
    }

  // STEP 0: Get the grid node/cell extent
  int GridExtent[6];
  this->GetGridExtent( gridIdx, GridExtent );
  int GridCellExtent[6];
  vtkStructuredData::GetCellExtentFromNodeExtent(GridExtent,GridCellExtent);

  // STEP 1: Get the cell extent of the sub-extent
  int cellExtent[6];
  vtkStructuredData::GetCellExtentFromNodeExtent( ext, cellExtent );

  // STEP 2: Serialize the cell data
  bytestream << 1;
  this->SerializeFieldData(
      GridCellExtent, cellExtent,
      this->GridCellData[gridIdx], bytestream );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeGhostCellData(
    const int gridIdx, const int nei, int ext[6],
    vtkMultiProcessStream& bytestream )
{
  assert("pre: Cannot deserialize an empty bytestream" && !bytestream.Empty());
  assert("pre:Grid index is out-of-bounds" &&
          (gridIdx >= 0 ) &&
          (gridIdx < static_cast<int>(this->GetNumberOfGrids())));
  assert("pre: Grid to be serialized must be local" &&
         this->IsGridLocal(gridIdx) );
  assert("pre:Neighbor list index is out-of-bounds" &&
         (nei >=0) && (nei < this->GetNumberOfNeighbors(gridIdx)));
  assert("pre:Remote point data is not properly allocated!" &&
         this->RemoteCellData.size() == this->NumberOfGrids );

  // STEP 0: Check if there are cell data in the byte-stream
  int hasCellData;
  bytestream >> hasCellData;
  if( hasCellData == 0)
    {
    return;
    }

  assert("pre:Remote cell data for grid is not properly allocated!" &&
      (this->GetNumberOfNeighbors(gridIdx)==
       static_cast<int>(this->RemoteCellData[gridIdx].size())));

  // STEP 1: De-serialize the cell data
  this->RemoteCellData[gridIdx][nei] = vtkCellData::New();
  int cellext[6];
  vtkStructuredData::GetCellExtentFromNodeExtent( ext, cellext );
  this->DeserializeFieldData(
      cellext,this->RemoteCellData[gridIdx][nei],bytestream);
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeGhostData(
  const int sendGridID, const int rcvGrid, int sndext[6],
  unsigned char*& buffer, unsigned int& size)
{
  // Pre-conditions
  assert("pre: Grid to be serialized must be local" &&
          this->IsGridLocal( sendGridID ) );
  assert("pre: Receive grid should not be local" &&
          !this->IsGridLocal(rcvGrid) );
  assert("pre: sendGridID out-of-bounds!" &&
     (sendGridID >= 0  && sendGridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: rcvGridID is out-of-bounds!" &&
     (rcvGrid >= 0) && rcvGrid < static_cast<int>(this->NumberOfGrids) );

  vtkMultiProcessStream bytestream;

  // STEP 0: Write the header
  bytestream << sendGridID << rcvGrid;
  bytestream.Push( sndext, 6 );

   // STEP 1: Serialize the points
  this->SerializeGhostPoints( sendGridID, sndext, bytestream );

  // STEP 2: Serialize point data (if any)
  this->SerializeGhostPointData(sendGridID, sndext, bytestream );

  // STEP 3: Serialize cell data (if any)
  this->SerializeGhostCellData( sendGridID, sndext, bytestream );

  // STEP 4: Get the raw data buffer
  bytestream.GetRawData( buffer, size );

// BEGIN DEBUG
//  std::ostringstream oss;
//  oss << "SendLog_" << this->Rank << ".txt";
//  std::ofstream ofs;
//  ofs.open( oss.str().c_str() );
//  ofs << "SEND MESSAGE SIZE: " << size << "\n";
//  ofs << sendGridID << " " << rcvGrid << std::endl;
//  for( int i=0; i < 6; ++i )
//    {
//    ofs << sndext[ i ] << " ";
//    }
//  ofs << std::endl;
//  ofs.close();
// END DEBUG

  // Post-conditions
  assert("post: buffer should not be NULL!" && (buffer != NULL) );
  assert("post: size > 0" && (size > 0) );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeGhostData(
    const int gridID, const int neiGridID, const int vtkNotUsed(neiGridIdx),
    int rcvext[6],unsigned char *buffer, unsigned int size )
{
  assert("pre: raw buffer is NULL!" && (buffer != NULL) );
  assert("pre: raw buffer size > 0" && (size > 0) );

  // STEP 0: Constructs the byte-stream object with raw data
  vtkMultiProcessStream bytestream;
  bytestream.SetRawData( buffer, size );

  // STEP 1: Extract the header
  int remoteGrid;
  int rcvGrid;
  bytestream >> remoteGrid >> rcvGrid;
//  assert("pre: Remote grid must match the Neighbor grid index" &&
//         (neiGridIdx==remoteGrid) );
  assert("pre: Serialized receiver grid must match this grid instance" &&
         (rcvGrid == gridID) );

  // STEP 2: Extract the rcv extent
  int *ext = NULL;
  unsigned int s = 0;
  bytestream.Pop(ext,s);

// BEGIN DEBUG
//  std::ostringstream oss;
//  oss << "RcvLog_" << this->Rank << ".txt";
//  std::ofstream ofs;
//  ofs.open( oss.str().c_str() );
//  ofs << "RCV MESSAGE SIZE: " << size << "\n";
//  ofs <<  remoteGrid << " " << rcvGrid << std::endl;
//  for( int i=0; i < 6; ++i )
//    {
//    ofs << ext[ i ] << " ";
//    }
//  ofs << std::endl;
//  ofs.close();
// END DEBUG

  assert("ERROR: parsed extent is not of expected size" && (s==6));
  assert("ERROR: parsed extent does not matched expected receive extent" &&
          this->GridExtentsAreEqual(ext,rcvext) );

//  std::ostringstream oss;
//  oss << "Extents-" << this->Rank << "-" << neiGridIdx << ".txt";
//  std::ofstream ofs;
//  ofs.open( oss.str().c_str() );
//  assert( ofs.is_open() );
//
//  ofs << "GRID " << gridID << std::endl;
//  bool error = false;
//  for( int i=0; i < 6; ++i )
//    {
//    ofs << i << " ";
//    if( ext[i] == rcvext[i] )
//      {
//      ofs << ext[i] << "==" << rcvext[i] << "!\n";
//      }
//    else
//      {
//      ofs << ext[i] << "!=" << rcvext[i] << "!!!\n";
//      error = true;
//      }
//    }
//
//  ofs.close();
//  assert("ERROR: Serialize send extent must match receive extent" && !error );
  delete [] ext;

  // STEP 2: De-serialize the grid points
  this->DeserializeGhostPoints(gridID, neiGridID, rcvext, bytestream);

  // STEP 3: De-serialize the ghost point data
  this->DeserializeGhostPointData(gridID, neiGridID, rcvext, bytestream);

  // STEP 4: De-serialize the ghost cell data
  this->DeserializeGhostCellData(gridID, neiGridID, rcvext, bytestream);
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeGridExtents()
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  assert( "pre: Controller is NULL!" && (this->Controller != NULL) );

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
    {
    rcvBufferSize += rcvcounts[i];
    }
  int *rcvbuffer = new int[ rcvBufferSize ];
  assert( "pre: receive buffer should not be NULL" && (rcvbuffer != NULL) );

  // STEP 3: Calculate offset to the rcvbuffer for each rank
  vtkIdType *offSet = new vtkIdType[ numRanks ];
  offSet[0]   = 0;
  for( int i=1; i < numRanks; ++i )
    {
    offSet[ i ] = offSet[ i-1 ] + rcvcounts[ i-1 ];
    }

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
