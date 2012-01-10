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
#include "vtkMultiProcessStream.h"

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

  // STEP 0: Compute neighbor send and receive extent
  for( unsigned int i=0; i < this->NumberOfGrids; ++i )
    {
    this->ComputeNeighborSendAndRcvExtent( i, N );
    }
  this->Controller->Barrier();

  // STEP 1: Exchange ghost-data
  this->ExchangeGhostData();
  this->Controller->Barrier();

  // STEP 4: Create ghost-layers
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

  // STEP 5: Synchronize
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeGhostData()
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  this->RemotePoints.resize( this->NumberOfGrids, NULL );
  this->RemotePointData.resize( this->NumberOfGrids, NULL );
  this->RemoteCellData.resize( this->NumberOfGrids, NULL );

  // TODO: implement this
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
  bytestream << N;

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

  // STEP 2: Push the raw data into the bytestream
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

    int dataType = myArray->GetDataType();
    int numComp  = myArray->GetNumberOfComponents();

    // STEP 1: Write the datatype and number of components
    bytestream << dataType;
    bytestream << numComp;
    bytestream << std::string( myArray->GetName() );

    // STEP 2: Extract the ghost data within the given ext
    // Allocate the ghost array where the data will be extracted
    vtkDataArray *ghostArray =
        vtkDataArray::CreateDataArray(myArray->GetDataType() );

    ghostArray->SetName( myArray->GetName() );
    ghostArray->SetNumberOfComponents( numComp );
    ghostArray->SetNumberOfTuples(vtkStructuredData::GetNumberOfNodes(ext));

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
              vtkStructuredData::ComputePointIdForExtent(ijk,GridExtent);
          assert("pre: source index is out-of-bounds!" &&
                  (sourceIdx >=  0) &&
                  (sourceIdx < myArray->GetNumberOfTuples() ) );

          // Compute the target index from the grid extent. Note, this could
          // be a cell index if the incoming GridExtent and ext are cell extents.
          vtkIdType targetIdx =
              vtkStructuredData::ComputePointIdForExtent(ijk,ext);
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
void vtkPStructuredGridConnectivity::SerializeGhostData(
  const int sendGridID, const int rcvGrid, int sndext[6],
  unsigned char*& buffer, unsigned int& size)
{
  // Pre-conditions
  assert("pre: Grid to be serialized must be local" &&
          this->IsGridLocal( sendGridID ) );
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

  // Post-conditions
  assert("post: buffer should not be NULL!" && (buffer != NULL) );
  assert("post: size > 0" && (size > 0) );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeGhostData(
    unsigned char *buffer, unsigned int size, int &gridID, int rcvext[6] )
{
  assert("pre: raw buffer is NULL!" && (buffer != NULL) );
  assert("pre: raw buffer size > 0" && (size > 0) );

  // TODO:

  assert("post: gridID is out-of-bounds" &&
         (gridID >= 0) && gridID < static_cast<int>(this->NumberOfGrids) );
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
