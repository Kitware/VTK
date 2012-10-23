/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPStructuredGridConnectivity.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkPStructuredGridConnectivity.h -- Constructs grid connectivity
//
// .SECTION Description
//  vtkPStructuredGridConnectivity inherits from vtkStructuredGridConnectivity
//  and implements functionality to compute the neighboring topology within a
//  single, partitioned and distributed structured grid dataset.
//
// .SECTION Caveats
//  Initialize(), ComputeNeighbors() and CreateGhostLayers() are collective
//  operations, every process must call that method.
//
// .SECTION See Also
//  vtkStructuredGridConnectivity vtkGhostArray

#ifndef VTKPSTRUCTUREDGRIDCONNECTIVITY_H_
#define VTKPSTRUCTUREDGRIDCONNECTIVITY_H_

// VTK include directives
#include "vtkFiltersParallelGeometryModule.h" // For export macro
#include "vtkStructuredGridConnectivity.h"
#include "vtkMPICommunicator.h" // Needed for vtkMPICommunicator::Request

// C++ include directives
#include <vector> // For STL vector

// Forward declarations
class vtkMultiProcessController;
class vtkMPIController;
class vtkMultiProcessStream;
//class vtkMPICommunicator::Request;

class VTKFILTERSPARALLELGEOMETRY_EXPORT vtkPStructuredGridConnectivity :
  public vtkStructuredGridConnectivity
{
public:
  static vtkPStructuredGridConnectivity* New();
  vtkTypeMacro(vtkPStructuredGridConnectivity,vtkStructuredGridConnectivity);
  void PrintSelf(ostream& os, vtkIndent indent );

  // Description:
  // Set & Get the process controller
  vtkSetMacro( Controller,vtkMultiProcessController* );
  vtkGetMacro( Controller,vtkMultiProcessController* );

  // Description:
  // Sets the total number of domains distributed among processors
  void SetNumberOfGrids( const unsigned int N );

  // Description:
  // See vtkStructuredGridConnectivity::RegisterGrid
  void RegisterGrid( const int gridID, int extents[6],
      vtkUnsignedCharArray* nodesGhostArray,
      vtkUnsignedCharArray* cellGhostArray,
      vtkPointData* pointData,
      vtkCellData* cellData,
      vtkPoints* gridNodes );

  // Description:
  // Returns the number of local grids registers by the process that owns
  // the current vtkPStructuredGridConnectivity instance
  int GetNumberOfLocalGrids()
    { return static_cast<int>(this->GridIds.size()); };

  // Description:
  // Returns the rank of the given gridID. A nominal value of -1 for the
  // return value of this method indicates that possibly ComputeNeighbors
  // has not been called and consequently the GridRanks vector has not been
  // populated yet.
  int GetGridRank( const int gridID );

  // Description:
  // Returns true iff the grid is remote, otherwise false.
  bool IsGridRemote( const int gridID );

  // Description:
  // Returns true iff the grid corresponding to the given gridID is local.
  bool IsGridLocal( const int gridID );

  // Description:
  // Initializes this instance of vtkPStructuredGridConnectivity, essentially,
  // the acquires the local process ID from the registered controller. If a
  // controller is not registered, the global controller is set.
  void Initialize();

  // Description:
  // Computes the neighboring topology of a distributed structured grid
  // data-set.
  // See vtkStructuredGridConnectivity::ComputeNeighbors
  void ComputeNeighbors();

  // Description:
  // Creates ghost layers on the grids owned by this process using data from
  // both local and remote block neighbors.
  virtual void CreateGhostLayers( const int N=1 );

protected:
  vtkPStructuredGridConnectivity();
  ~vtkPStructuredGridConnectivity();

  vtkMultiProcessController *Controller;
  int Rank;
  bool Initialized;

  // BTX
  std::vector< int > GridRanks; // Corresponding rank for each grid
  std::vector< int > GridIds;   // List of GridIds, owned by this process

  // Data structures to store the remote ghost data of each grid for each one
  // of its neighbors. The first index is the global grid index. The second
  // is the neighbor index.
  std::vector< std::vector< vtkPoints* > >    RemotePoints;
  std::vector< std::vector< vtkPointData* > > RemotePointData;
  std::vector< std::vector< vtkCellData* > >  RemoteCellData;

  // Data structures to store the send/receive buffer sizes and corresponding
  // persistent buffers. The first index is the global grid index. The second
  // index is the neighbor index for the given grid.
  std::vector< std::vector< unsigned int > > SendBufferSizes;
  std::vector< std::vector< unsigned int > > RcvBufferSizes;
  std::vector< std::vector< unsigned char* > > SendBuffers;
  std::vector< std::vector< unsigned char* > > RcvBuffers;

  int TotalNumberOfSends;
  int TotalNumberOfRcvs;
  int TotalNumberOfMsgs;

  // Array of MPI requests
  vtkMPICommunicator::Request *MPIRequests;
  // ETX

  // Description:
  // Returns true if the two extents are equal, otherwise false.
  bool GridExtentsAreEqual( int rhs[6], int lhs[6] );

  // Description:
  // Returns true iff the grid corresponding to the given ID has point data.
  bool HasPointData(const int gridIdx);

  // Description:
  // Returns true iff the grid corresponding to the given ID has cell data.
  bool HasCellData(const int gridIdx);

  // Description:
  // Returns true iff the grid corresponding to the given ID has points.
  bool HasPoints(const int gridIdx);

  // Description:
  // Sets all message counters to 0.
  void InitializeMessageCounters();

  // Description:
  // Clears all internal VTK data-structures that are used to store the remote
  // ghost data.
  void ClearRemoteData();

  // Description:
  // Clears all raw send/rcv buffers
  void ClearRawBuffers();

  // Description:
  // Registers a remote grid with the given grid Id, structured extents and
  // process.
  void RegisterRemoteGrid( const int gridID, int extents[6], int process );

  // Description:
  // This method transfers all the remote neighbor data to the ghosted grid
  // instance of the grid corresponding to the given grid index.
  void TransferRemoteNeighborData(
      const int gridIdx,const int nei,const vtkStructuredNeighbor& Neighbor );

  // Description:
  // This method transfers the fields (point data and cell data) to the ghost
  // extents from the neighboring grids of the grid corresponding to the given
  // gridID.
  virtual void TransferGhostDataFromNeighbors(const int gridID);

  // Description:
  // Helper method to pack all the ghost data into send buffers.
  void PackGhostData();

  // Description:
  // Helper method to unpack the raw ghost data from the receive buffers in
  // to the VTK remote point data-structures.
  void UnpackGhostData();

  // Description:
  // Helper method to deserialize the buffer sizes coming from the given
  // process.
  void DeserializeBufferSizesForProcess(
      int *buffersizes, vtkIdType N, const int processId );

  // Description:
  // Helper method to serialize the buffer sizes for the grids of this process
  // to neighboring grids.
  void SerializeBufferSizes(int *&sizesbuf, vtkIdType &N);

  // Description:
  // Helper method to exchange buffer sizes.Each process sends the send buffer
  // size of each grid to each of its neighbors.
  void ExchangeBufferSizes();

  // Description:
  // Helper method for exchanging ghost data. It loops through all the grids,
  // and for each neighbor it constructs the corresponding send buffer.
  // size and posts a non-blocking receive.
  void ExchangeGhostDataInit();

  // Description:
  // Helper method to communicate ghost data. Loops through all the neighbors
  // and for every remote neighbor posts a non-blocking receive.
  void PostReceives();

  // Description:
  // Helper method to communicate ghost data. Loops through the neighbors and
  // for every remote neighbor posts a non-blocking send.
  void PostSends();

  // Description:
  // Helper method for exchanging ghost data. It loops through all the grids
  // and for each neighbor of each grid it serializes the data in the send
  // extent and posts a non-blocking send.
  void CommunicateGhostData();

  // Description:
  // Helper method for exchanging ghost data. It blocks until all requests
  // are complete (via a waitAll) and then de-serializes the receive buffers
  // to form the corresponding remote data-structures.
  void ExchangeGhostDataPost();

  // Description:
  // Exchanges ghost data of the grids owned by this process.
  void ExchangeGhostData();

  // Description:
  // Helper method to serialize the ghost points to send to a remote process.
  // Called from SerializeGhostData.
  void SerializeGhostPoints(
      const int gridIdx, int ext[6], vtkMultiProcessStream& bytestream );

  // Description:
  // Serializes the data array into a bytestream.
  void SerializeDataArray(
      vtkDataArray *dataArray, vtkMultiProcessStream& bytestream );

  // Description:
  // Helper method to serialize field data. Called from
  // SerializeGhostPointData and SerializeGhostCellData.
  void SerializeFieldData(
      int sourceExtent[6], int targetExtent[6], vtkFieldData *fieldData,
      vtkMultiProcessStream& bytestream );

  // Description:
  // Helper method to serialize ghost point data. Called from
  // SerializeGhostData.
  void SerializeGhostPointData(
      const int gridIdx, int ext[6], vtkMultiProcessStream& bytestream );

  // Description:
  // Helper method to serialize ghost cell data. Called from
  // SerializeGhostData.
  void SerializeGhostCellData(
      const int gridIdx, int ext[6], vtkMultiProcessStream& bytestream );

  // Description:
  // Helper method to de-serialize the ghost points received from a remote
  // process. Called from DeserializeGhostData.
  void DeserializeGhostPoints(
      const int gridIdx, const int nei,
      int ext[6], vtkMultiProcessStream& bytestream );

  // Description:
  // Helper method to deserialize the data array from a bytestream.
  void DeserializeDataArray(
      vtkDataArray *&dataArray,const int dataType,
      const int numberOfTuples, const int numberOfComponents,
      vtkMultiProcessStream& bytestream );

  // Description:
  // Helper method to de-serialize field data. Called from
  // DeserializeGhostPointData and DeserializeGhostCellData.
  void DeserializeFieldData(
      int ext[6], vtkFieldData *fieldData,
      vtkMultiProcessStream &bytestream );

  // Description:
  // Helper method to de-serialize the ghost point data received from a
  // remote process. Called from DeserializeGhostData.
  void DeserializeGhostPointData(
      const int gridIdx, const int nei,
      int ext[6], vtkMultiProcessStream& bytestream );

  // Description:
  // Helper method to de-serialize the ghost cell data received from a remote
  // process. Called from DeserializeGhostCellData.
  void DeserializeGhostCellData(
      const int gridIdx, const int nei,
      int ext[6], vtkMultiProcessStream& bytestream );

  // Description:
  // Given a grid ID and the corresponding send extent, this method serializes
  // the grid and data within the given extent. Upon return, the buffer is
  // allocated and contains the data in raw form, ready to be sent. Called
  // from vtkPStructuredGridConnectivity::PackGhostData().
  void SerializeGhostData(
      const int sndGridID, const int rcvGrid, int sndext[6],
      unsigned char*& buffer, unsigned int &size);

  // Description:
  // Given the raw buffer consisting of ghost data, this method deserializes
  // the object and returns the gridID and rcvext of the grid.
  void DeserializeGhostData(
      const int gridID, const int neiListID,
      const int neiGridIdx, int rcvext[6],
      unsigned char *buffer, unsigned int size );

  // Description:
  // Exchanges the grid extents among all processes and fully populates the
  // GridExtents vector.
  void ExchangeGridExtents();

  // Description:
  // Serializes the grid extents and information in a buffer to send over MPI
  // The data is serialized as follows: ID imin imax jmin jmax kmin kmax
  void SerializeGridExtents( int *&sndbuffer, vtkIdType &N );

  // Description:
  // Deserializes the received grid extent information to the GridExtents
  // internal data-structures.
  void DeserializeGridExtentForProcess(
      int *rcvbuffer, vtkIdType &N, const int processId );

private:
  vtkPStructuredGridConnectivity(const vtkPStructuredGridConnectivity& ); // Not implemented
  void operator=(const vtkPStructuredGridConnectivity& ); // Not implemented
};

//=============================================================================
//  INLINE METHODS
//=============================================================================


inline bool vtkPStructuredGridConnectivity::GridExtentsAreEqual(
    int rhs[6], int lhs[6] )
{
  for( int i=0; i < 6; ++i )
    {
    if( rhs[i] != lhs[i] )
      {
      return false;
      }
    }
  return true;
}

//------------------------------------------------------------------------------
inline bool vtkPStructuredGridConnectivity::HasPointData(const int gridIdx)
{
  // Sanity check
  assert("pre: grid index is out-of-bounds!" &&
         (gridIdx >= 0) && (gridIdx < static_cast<int>(this->NumberOfGrids)));

  if( (this->GridPointData[gridIdx] != NULL) &&
      (this->GridPointData[gridIdx]->GetNumberOfArrays() > 0) )
    {
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
inline bool vtkPStructuredGridConnectivity::HasCellData(const int gridIdx)
{
  // Sanity check
  assert("pre: grid index is out-of-bounds!" &&
         (gridIdx >= 0) && (gridIdx < static_cast<int>(this->NumberOfGrids)));

  if( (this->GridCellData[gridIdx] != NULL) &&
      (this->GridCellData[gridIdx]->GetNumberOfArrays( ) > 0) )
    {
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
inline bool vtkPStructuredGridConnectivity::HasPoints(const int gridIdx)
{
  // Sanity check
  assert("pre: grid index is out-of-bounds!" &&
         (gridIdx >= 0) && (gridIdx < static_cast<int>(this->NumberOfGrids)));

  if( this->GridPoints[gridIdx] != NULL )
    {
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
inline void vtkPStructuredGridConnectivity::InitializeMessageCounters()
{
  this->TotalNumberOfMsgs=this->TotalNumberOfRcvs=this->TotalNumberOfSends=0;
}

//------------------------------------------------------------------------------
inline void vtkPStructuredGridConnectivity::ClearRawBuffers()
{
  this->SendBufferSizes.clear();
  this->RcvBufferSizes.clear();

  // STEP 0: Clear send buffers
  for( unsigned int i=0; i < this->SendBuffers.size(); ++i )
    {
    for( unsigned int j=0; j < this->SendBuffers[i].size(); ++j )
      {
      if( this->SendBuffers[i][j] != NULL )
        {
        delete [] this->SendBuffers[i][j];
        }
      } // END for all neighbors
    this->SendBuffers[i].clear();
    } // END for all grids
  this->SendBuffers.clear();

  // STEP 1: Clear rcv buffers
  for( unsigned int i=0; i < this->RcvBuffers.size(); ++i )
    {
    for( unsigned int j=0; j < this->RcvBuffers[i].size(); ++j )
      {
      if( this->RcvBuffers[i][j] != NULL )
        {
        delete [] this->RcvBuffers[i][j];
        }
      } // END for all neighbors
    this->RcvBuffers[i].clear();
    } // END for all grids
  this->RcvBuffers.clear();
}

//------------------------------------------------------------------------------
inline void vtkPStructuredGridConnectivity::ClearRemoteData()
{
  // STEP 0: Clear remote points
  for( unsigned int i=0; i < this->RemotePoints.size(); ++i )
    {
    for( unsigned int j=0; j < this->RemotePoints[i].size(); ++j )
      {
      if( this->RemotePoints[ i ][ j ] != NULL )
        {
        this->RemotePoints[ i ][ j ]->Delete();
        }
      } // END for all j
    this->RemotePoints[ i ].clear();
    } // END for all i
  this->RemotePoints.clear();

  // STEP 1: Clear remote point data
  for( unsigned int i=0; i < this->RemotePointData.size(); ++i )
    {
    for( unsigned int j=0; j < this->RemotePointData[i].size(); ++j )
      {
      if( this->RemotePointData[ i ][ j ] != NULL )
        {
        this->RemotePointData[ i ][ j ]->Delete();
        }
      } // END for all j
    this->RemotePointData[ i ].clear();
    } // END for all i
  this->RemotePointData.clear();

  // STEP 2: Clear remote cell data
  for( unsigned int i=0; i < this->RemoteCellData.size(); ++i )
    {
    for( unsigned int j=0; j < this->RemoteCellData[i].size(); ++j )
      {
      if( this->RemoteCellData[ i ][ j ] != NULL )
        {
        this->RemoteCellData[ i ][ j ]->Delete();
        }
      } // END for all j
    this->RemoteCellData[ i ].clear();
    }
  this->RemoteCellData.clear();
}

//------------------------------------------------------------------------------
inline bool vtkPStructuredGridConnectivity::IsGridRemote(const int gridID )
{
  return( !this->IsGridLocal(gridID) );
}

//------------------------------------------------------------------------------
inline bool vtkPStructuredGridConnectivity::IsGridLocal(const int gridID)
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  assert( "pre: gridID is out-of-bounds" &&
          (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids) ) );
  assert( "pre: GridRanks is not properly allocated" &&
          this->NumberOfGrids == this->GridRanks.size() );
  return( (this->GridRanks[ gridID ] == this->Rank) );
}

//------------------------------------------------------------------------------
inline int vtkPStructuredGridConnectivity::GetGridRank( const int gridID )
{
  assert( "pre: Instance has not been intialized!" && this->Initialized );
  assert( "pre: gridID out-of-bounds!" &&
          (gridID >= 0  && gridID < static_cast<int>(this->NumberOfGrids)));
  return( this->GridRanks[ gridID ] );
}
#endif /* VTKPSTRUCTUREDGRIDCONNECTIVITY_H_ */
