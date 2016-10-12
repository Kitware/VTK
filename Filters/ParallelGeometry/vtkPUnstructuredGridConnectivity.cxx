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
#include "vtkPUnstructuredGridConnectivity.h"

#if !defined(VTK_LEGACY_REMOVE)

// VTK includes
#include "vtkBoundingBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkFieldDataSerializer.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"

// C/C++ includes
#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <cassert>
#include <algorithm>

//#define DEBUG

//------------------------------------------------------------------------------
//  INTERNAL DATASTRUCTURES
//------------------------------------------------------------------------------
namespace vtk
{
namespace details
{

//------------------------------------------------------------------------------
struct NodeLink {
  int Rank;            // the rank to communicate (send or receive)
  vtkIdType LocalIdx;  // the local node index w.r.t. the ghosted grid
  vtkIdType GlobalIdx; // the global node index w.r.t. across all partitions
};

//------------------------------------------------------------------------------
struct SndLink {
  int Rank;            // the rank to send to
  vtkIdType SourceIdx; // the local index w.r.t. the input grid
};

//------------------------------------------------------------------------------
struct RcvLink {
  int Rank;             // the rank to receive from
  vtkIdType SourceIdx;  // the local index w.r.t. the remote grid from
                        // which, to receive from
  vtkIdType TargetIdx;  // the local index w.r.t. the input grid in this
                        // process, where the data from the source will be
                        // copied.
};

//------------------------------------------------------------------------------
struct CommunicationLinks {

  // Description:
  // Default constructor
  CommunicationLinks()
  {
    this->RcvBuffersAllocated = false;
  }

  // Description:
  // Destructor
  ~CommunicationLinks()
  {
    // Clear all rank buffers
    std::set<int>::iterator rankIter = this->NeighboringRanks.begin();

    for( ; rankIter != this->NeighboringRanks.end(); ++rankIter)
    {
      int rank = *rankIter;
      if( this->RcvBuffers.find(rank) != this->RcvBuffers.end() )
      {
        delete [] this->RcvBuffers[rank];
        this->RcvBuffers[rank] = NULL;
      } // END if buffer entry for rank exists
    } // END for all neighboring ranks

    this->SndBuffers.clear();
    this->RcvBuffers.clear();
    this->SndBufferSizes.clear();
    this->RcvBufferSizes.clear();

    this->NeighboringRanks.clear();
    this->SndNodeLinks.clear();
    this->RcvNodeLinks.clear();
    this->SndCellLinks.clear();
    this->RcvCellLinks.clear();
  }

  // Description:
  // Returns the local ID on the ghosted grid for the given node global ID.
  vtkIdType GetTargetNodeId(const int vtkNotUsed(rmtRank),const vtkIdType globalIdx)
  {
    if(this->TargetNodeMapping.find(globalIdx)==this->TargetNodeMapping.end())
    {
      std::cerr << "ERROR: rmt node received has not target node mapping!\n";
      return -1;
    }
    return(this->TargetNodeMapping[globalIdx]);
  }

  // Description:
  // Returns the local ID on the ghosted grid for the given (rmtRank,rmtCellId)
  // pair.
  vtkIdType GetTargetCellId(const int rmtRank,const vtkIdType rmtCellId)
  {
    std::pair<int,vtkIdType> rmtInfo;
    rmtInfo.first  = rmtRank;
    rmtInfo.second = rmtCellId;
    if(this->TargetCellMapping.find(rmtInfo) == this->TargetCellMapping.end())
    {
      std::cerr << "ERROR: rmt cell received has no target cell mapping!\n";
      return -1;
    }
    return( this->TargetCellMapping[rmtInfo] );
  }

  // Description:
  // Enqueues a receive on the node with the given local/global ID from
  // the given remote rank.
  void EnqueueNodeRcv(
        const vtkIdType localIdx,
        const vtkIdType globalIdx,
        const int rmtRank)
  {
    this->NeighboringRanks.insert( rmtRank );

    NodeLink lnk;
    lnk.Rank      = rmtRank;
    lnk.GlobalIdx = globalIdx;
    lnk.LocalIdx  = localIdx;
    if(this->RcvNodeLinks.find(rmtRank) == this->RcvNodeLinks.end())
    {
      std::vector< NodeLink > lnks;
      lnks.push_back( lnk );
      this->RcvNodeLinks[ rmtRank ] = lnks;
    }
    else
    {
      this->RcvNodeLinks[ rmtRank ].push_back( lnk );
    }

    this->TargetNodeMapping[ globalIdx ] = localIdx;
  }

  // Description:
  // Enqueues a send on the node with the given local/global ID to the given
  // remote rank.
  void EnqueueNodeSend(
        const vtkIdType localIdx,
        const vtkIdType globalIdx,
        const int rmtRank)
  {
    this->NeighboringRanks.insert( rmtRank );

    NodeLink lnk;
    lnk.Rank      = rmtRank;
    lnk.GlobalIdx = globalIdx;
    lnk.LocalIdx  = localIdx;
    if(this->SndNodeLinks.find(rmtRank) == this->SndNodeLinks.end())
    {
      std::vector< NodeLink > lnks;
      lnks.push_back( lnk );
      this->SndNodeLinks[ rmtRank ] = lnks;
    }
    else
    {
      this->SndNodeLinks[ rmtRank ].push_back( lnk );
    }
  }

  // Description:
  // Enqueues a cell link to the communication lists.
  void EnqueueCellLink(
      const vtkIdType adjCell,
      const vtkIdType ghostCell,
      const vtkIdType rmtCell,
      const int rmtRank)
  {
    this->NeighboringRanks.insert( rmtRank );

    SndLink sndlnk;
    sndlnk.Rank      = rmtRank;
    sndlnk.SourceIdx = adjCell;
    if(this->SndCellLinks.find(rmtRank) == this->SndCellLinks.end())
    {
      std::vector< SndLink > sndlinks;
      sndlinks.push_back(sndlnk);
      this->SndCellLinks[rmtRank] = sndlinks;
    }
    else
    {
      this->SndCellLinks[rmtRank].push_back(sndlnk);
    }

    RcvLink rcvlnk;
    rcvlnk.Rank      = rmtRank;
    rcvlnk.SourceIdx = rmtCell;
    rcvlnk.TargetIdx = ghostCell;
    if(this->RcvCellLinks.find(rmtRank) == this->RcvCellLinks.end())
    {
      std::vector<RcvLink> rcvlinks;
      rcvlinks.push_back(rcvlnk);
      this->RcvCellLinks[rmtRank] = rcvlinks;
    }
    else
    {
      this->RcvCellLinks[rmtRank].push_back(rcvlnk);
    }

    std::pair<int,vtkIdType> rmtInfo;
    rmtInfo.first  = rmtRank;
    rmtInfo.second = rmtCell;
    this->TargetCellMapping[ rmtInfo ] = ghostCell;
  }

  // Maps a receiver node globalID to its localID w.r.t. the ghosted grid.
  // Used when filling in ghost zone nodes.
  std::map<vtkIdType,vtkIdType> TargetNodeMapping;

  // Maps a (rmtRank,rmtCellId) pair to the cellID w.r.t. the ghosted grid.
  // Used when filling in ghost zone cells.
  std::map< std::pair<int,vtkIdType>, vtkIdType > TargetCellMapping;

  // Flag that indicates if the receive buffers have been allocated.
  bool RcvBuffersAllocated;

  std::set<int> NeighboringRanks;

  // Holds the number of bytes to receive from each process
  std::map<int,int> RcvBufferSizes;

  // Holds the receive buffer for each process
  std::map<int,unsigned char*> RcvBuffers;

  // Holds the number of bytes to be sent to each process
  std::map<int,int> SndBufferSizes;

  // Holds the send buffers to each neighboring rank
  std::map< int,std::vector<unsigned char> > SndBuffers;

  // List of send node-links for each remote process
  std::map< int,std::vector< NodeLink > > SndNodeLinks;

  // List of receive node-links for each remote process
  std::map< int,std::vector< NodeLink > > RcvNodeLinks;

  // List of send node-links for each remote process
  std::map< int,std::vector< SndLink > > SndCellLinks;

  // List of receive node-links for each remote process
  std::map< int,std::vector< RcvLink > > RcvCellLinks;
};


//------------------------------------------------------------------------------
// Description:
// Computes a hash code for the given list of IDs.
// The hash code is a string composed by sorting the IDs of the cell/face
// and delimiting them by a '.'
std::string Hash(vtkIdType* ids, const vtkIdType N)
{
  std::sort(ids,ids+N);
  std::ostringstream oss;
  for(vtkIdType i=0; i < N; ++i)
  {
    oss << ids[i] << ".";
  } // END for all N
  return( oss.str() );
}

//------------------------------------------------------------------------------
// Description:
// Computes a hash code for the given cell/face.
// The hash code is a string composed by sorting the IDs of the cell/face
// and delimiting them by a '.'
std::string GetHashCode(vtkCell* c)
{
  std::vector<vtkIdType> nodeList;
  nodeList.reserve( c->GetNumberOfPoints() );

  for(vtkIdType nodeIdx=0; nodeIdx < c->GetNumberOfPoints(); ++nodeIdx)
  {
    nodeList.push_back( c->GetPointId(nodeIdx) );
  }
  assert("post: nodeList size mismatch!" &&
       (static_cast<vtkIdType>(nodeList.size())==c->GetNumberOfPoints()) );

  std::sort(nodeList.begin(),nodeList.end());

  std::ostringstream oss;
  for(unsigned int i=0; i < nodeList.size(); ++i)
  {
    oss << nodeList[ i ] << ".";
  }
  return( oss.str() );
}

//------------------------------------------------------------------------------
// Description:
// A simple struct that holds the face info.
struct FaceInfo
{
  std::vector<vtkIdType> FaceIds;
  vtkIdType CellAdjacency[2];
  int Count;
};

//------------------------------------------------------------------------------
// Description:
// A simple data-structure to allow performing queries easily.
struct MeshLinks {

  // Description:
  // Checks if the given face exists
  bool HasFace(const std::string& face)
    {return(this->FaceLinks.find(face)!=this->FaceLinks.end());};

  // Description:
  // Clears all data-structures
  void Clear()
  {
    this->Global2LocalNodeIdx.clear();
    this->FaceLinks.clear();
  }

  // Description:
  // Links faces in the mesh to cells
  void AddFaceLink(const std::string& face, vtkIdType cellIdx)
  {
    if( this->HasFace(face) )
    {
      this->FaceLinks[face].insert(cellIdx);
    }
    else
    {
      std::set<vtkIdType> cells;
      cells.insert(cellIdx);
      this->FaceLinks[face] = cells;
    }
  }

  // Description:
  // Given a global ID of a node, this method returns the
  // corresponding local ID w.r.t. the input grid. A -1
  // is returned if the node does not exist.
  vtkIdType GetLocalNodeID(const vtkIdType globalIdx)
  {
    if( this->Global2LocalNodeIdx.find(globalIdx) !=
        this->Global2LocalNodeIdx.end())
    {
      return this->Global2LocalNodeIdx[globalIdx];
    }
    return -1;
  }

  // Description:
  // Builds cell links for the given *boundary* grid.
  void BuildLinks(vtkUnstructuredGrid* grid)
  {
    vtkIdType numCells = grid->GetNumberOfCells();

    vtkPointData* PD = grid->GetPointData();
    assert("pre: point data does not have LOCAL ID" &&
            PD->HasArray("LOCAL_ID"));
    assert("pre: point data does not have GLOBAL ID" &&
            PD->HasArray("GLOBAL_ID"));


    vtkCellData* CD = grid->GetCellData();
    assert("pre: cell data does not have local CELL ID" &&
            CD->HasArray("LOCAL_CELL_ID"));

    vtkIdType* globalIdPtr =
     static_cast<vtkIdType*>(PD->GetArray("GLOBAL_ID")->GetVoidPointer(0));

    vtkIdType* localIdPtr =
     static_cast<vtkIdType*>(PD->GetArray("LOCAL_ID")->GetVoidPointer(0));

    vtkIdType* cellIdPtr =
     static_cast<vtkIdType*>(CD->GetArray("LOCAL_CELL_ID")->GetVoidPointer(0));

    // Add global2Local node index
    for(vtkIdType p=0; p < grid->GetNumberOfPoints(); ++p)
    {
      this->Global2LocalNodeIdx[ globalIdPtr[p] ] = localIdPtr[p];
    } // END for all points

    // Add face-adjacency information

    // nodes vector used as temporary storage for edge/face nodes in order
    // to construct a corresponding hashcode to uniquely identify an edge
    // or face, regardless of orientation.
    std::vector<vtkIdType>  nodes;

    for(vtkIdType c=0; c < numCells; ++c)
    {
      ///@todo: optimize this -- use table lookup to get face ids, at least
      ///  for all linear cells.
      vtkCell* cell          = grid->GetCell(c);
      vtkIdType localCellIdx = cellIdPtr[ c ];
      // Add face links
      for(int f=0; f < cell->GetNumberOfFaces(); ++f)
      {
        vtkCell* face      = cell->GetFace( f );
        int N              = face->GetNumberOfPoints();
        vtkIdType* nodePtr = face->GetPointIds()->GetPointer(0);

        nodes.resize(N);
        for(int i=0; i < N; ++i)
        {
          nodes[i] = globalIdPtr[ nodePtr[i] ];
        } // END for all face nodes
        std::string hashCode = Hash(&nodes[0],N);
        this->AddFaceLink(hashCode,localCellIdx);
      } // END for all cell faces
    } // END for all cells
  } // END BuildLinks

  // Maps global nodes Ids on the local boundary grid to the local nodes
  // in the input mesh
  std::map< vtkIdType,vtkIdType > Global2LocalNodeIdx;

  // Maps a face, identified using global IDs, to the local cell IDs
  // from the input mesh.
  std::map< std::string, std::set<vtkIdType> > FaceLinks;
};

//------------------------------------------------------------------------------
// Description:
// A simple struct to hold auxiliary information
struct GridInfo
{
  // The cartesian bounds of the grid in this process.
  double GridBounds[6];

  // List of candidate ranks to exchange boundary grid information.
  std::vector<int> CandidateRanks;

  // For each candidate rank, stores the size of the buffer that needs to be
  // allocated to communicate the boundary grids.
  std::vector<int> RmtBGridSizes;

  // Stores the remote boundary grid at each corresponding candidate rank.
  std::vector<vtkUnstructuredGrid*> RmtBGrids;

  // Flat vector to store the global grid bounds. The bounds of process i
  // are stored within a contiguous region [i*6,i*6+5]
  std::vector<double> GlobalGridBounds;

  // List of boundary node IDs on the surface mesh of the input mesh.
  // Stored in a set so that we can easily lookup if a cell is on a boundary.
  std::set<vtkIdType> SurfaceNodes;

  // List of faces and metadata(i.e., FaceInfo) on the surface mesh of the
  // input grid. Note the connectivity of the surface mesh is w.r.t. to the
  // local IDs of the nodes in the input grid.
  std::map<std::string,FaceInfo> SurfaceMesh;

  // List of faces and metadata(i.e., FaceInfo) from the input grid.
  std::map<std::string,FaceInfo> FaceList;

  // Mapping of local node IDs, w.r.t., the input grid, to the corresponding
  // node IDs on the BoundaryGrid.
  std::map<vtkIdType,vtkIdType> BndryNodeMap;

  // A grid that consists of only the boundary cells of the input grid. Each
  // node in the boundary grid has
  vtkUnstructuredGrid* BoundaryGrid;

  // MeshLinks for the boundary grid in this process, used to enable queries
  // based on global IDs.
  MeshLinks BoundaryGridLinks;

  // History to keep track of nodes that are inserted to the ghosted grid
  // mapping the global ID to the ID of the node on the ghosted grid.
  std::map<vtkIdType,vtkIdType> NodeHistory;

  // History of cell hashcodes that are inserted to the ghosted grid in order
  // to avoid inserting duplicate cells in the ghosted grid.
  std::set<std::string> CellHistory;

  // Description:
  // Constructor.
  GridInfo() { this->BoundaryGrid = NULL; }

  // Description:
  // Destructor
  ~GridInfo() { this->Clear(); }

  // Description:
  // Clears all data from this GridInfo instance.
  void Clear()
  {
    if( this->BoundaryGrid != NULL)
    {
      this->BoundaryGrid->Delete();
      this->BoundaryGrid = NULL;
    }

    for(unsigned int i=0; i < this->RmtBGrids.size(); ++i)
    {
      if( this->RmtBGrids[ i ] != NULL )
      {
        this->RmtBGrids[ i ]->Delete();
        this->RmtBGrids[ i ] = NULL;
      }
    }
    this->RmtBGrids.clear();

    this->BoundaryGridLinks.Clear();

    this->RmtBGridSizes.clear();
    this->GlobalGridBounds.clear();
    this->CandidateRanks.clear();
    this->FaceList.clear();
    this->SurfaceMesh.clear();
    this->BndryNodeMap.clear();
    this->SurfaceNodes.clear();

    this->NodeHistory.clear();
    this->CellHistory.clear();
  }

  // Description:
  // Updates the face list
  void UpdateFaceList(vtkCell* face, vtkIdType cellidx)
  {
    std::string hashCode = GetHashCode(face);
    if( this->FaceList.find(hashCode)==this->FaceList.end())
    {
      FaceInfo f;
      for(vtkIdType nodeIdx=0; nodeIdx < face->GetNumberOfPoints(); ++nodeIdx)
      {
        f.FaceIds.push_back( face->GetPointId(nodeIdx) );
      } // END for all nodes on the face
      f.CellAdjacency[0] = cellidx;
      f.Count = 1;
      this->FaceList[hashCode] = f;
    } // END if
    else
    {
      // this is the 2nd time we encounter this face
      assert(this->FaceList[hashCode].Count==1);
      this->FaceList[hashCode].CellAdjacency[1] = cellidx;
      this->FaceList[hashCode].Count++;
    } // END else
  }

};

} // END namespace details

} // END namespace vtk

//------------------------------------------------------------------------------

vtkStandardNewMacro(vtkPUnstructuredGridConnectivity);

//------------------------------------------------------------------------------
vtkPUnstructuredGridConnectivity::vtkPUnstructuredGridConnectivity()
{
  VTK_LEGACY_BODY(
    vtkPUnstructuredGridConnectivity::vtkPUnstructuredGridConnectivity,
    "VTK 7.0");

 this->InputGrid         = NULL;
 this->GhostedGrid       = NULL;
 this->Controller        = NULL;
 this->GlobalIDFieldName = NULL;
 this->AuxiliaryData     = new vtk::details::GridInfo();
 this->CommLists         = new vtk::details::CommunicationLinks();
}

//------------------------------------------------------------------------------
vtkPUnstructuredGridConnectivity::~vtkPUnstructuredGridConnectivity()
{
  this->InputGrid  = NULL;
  this->Controller = NULL;

  if( this->GhostedGrid != NULL )
  {
    this->GhostedGrid->Delete();
  }

  delete this->AuxiliaryData;
  delete this->CommLists;
  delete [] this->GlobalIDFieldName;
  this->GlobalIDFieldName = NULL;
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::PrintSelf(
      ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::RegisterGrid(
      vtkUnstructuredGrid* gridPtr)
{
  assert("pre: gridPtr != NULL" && (gridPtr != NULL) );
  if( this->InputGrid != NULL )
  {
    vtkErrorMacro("Only one grid per process is currently supported!");
  }
  this->InputGrid = gridPtr;
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::BuildGhostZoneConnectivity()
{
  // Sanity check
  assert("pre: controller is NULL!" && (this->Controller != NULL) );
  assert("pre: input grid is NULL!" && (this->InputGrid != NULL) );
  assert("pre: auxiliary data is NULL!" && (this->AuxiliaryData != NULL) );

  if(this->Controller->GetNumberOfProcesses() <= 1 )
  {
    // short-circuit
    return;
  }

  if(this->GlobalIDFieldName == NULL)
  {
    // We assume "GlobalID" as the default
    this->GlobalIDFieldName = new char[9];
    strncpy(this->GlobalIDFieldName,"GlobalID", 9);
  }

  // STEP 0: Ensure the input grid has GlobalID information
  if( !this->InputGrid->GetPointData()->HasArray(this->GlobalIDFieldName) )
  {
    vtkErrorMacro("Input grid has no global ID information");
  }

  // STEP 1: Build auxiliary data-structures and extract boundary grid
  this->ExtractBoundaryGrid();
  assert("post: boundary grid is NULL!" &&
          (this->AuxiliaryData->BoundaryGrid != NULL) );

  // STEP 2: Exchange grid bounds
  this->AuxiliaryData->BoundaryGrid->GetBounds(this->AuxiliaryData->GridBounds);
  this->ExchangeGridBounds();

  // STEP 3: BoundingBox collision. This establishes the list of CandidateRanks
  // to communicate the boundary grids.
  this->BoundingBoxCollision();

  // STEP 4: Exchange Boundary grids
  this->ExchangeBoundaryGrids();

  // STEP 5: Build Ghosted grid and communication lists
  this->BuildGhostedGridAndCommLists();
  this->Controller->Barrier();

  // STEP 6: Clear all auxiliary data
  this->AuxiliaryData->Clear();
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::UpdateGhosts()
{
  assert("pre: controller is NULL!" && (this->Controller != NULL) );

  if(this->Controller->GetNumberOfProcesses() <= 1)
  {
    // return;
  }

  // STEP 0: copies local data from the input grid to the ghosted grid
  this->SynchLocalData();

  // STEP 1: serialize data
  this->SerializeGhostZones();

  // STEP 2: create persistent receive buffers. This only executes the first
  // time UpdateGhosts() is called. Afterwards, the method returns immediately.
  this->CreatePersistentRcvBuffers();

  // STEP 3: Allocate MPI request objects for non-blocking point-to-point comm.
  int numNeis = static_cast<int>(this->CommLists->NeighboringRanks.size());
  vtkMPICommunicator::Request* rqsts;
  rqsts = new vtkMPICommunicator::Request[2*numNeis];
  int rqstIdx = 0;

  // STEP 4: post receives
  std::set<int>::iterator rankIter = this->CommLists->NeighboringRanks.begin();
  for( ;rankIter != this->CommLists->NeighboringRanks.end(); ++rankIter)
  {
    int rank = *rankIter;
    assert("pre: cannot find rcv buffer size for rank!" &&
           (this->CommLists->RcvBufferSizes.find(rank)!=
                this->CommLists->RcvBufferSizes.end()));
    assert("pre: cannot find buffer for rank!" &&
            (this->CommLists->RcvBuffers.find(rank)!=
                this->CommLists->RcvBuffers.end()));
    assert("pre: rcv buffer for rank is NULL" &&
           (this->CommLists->RcvBuffers[rank] != NULL));

    this->Controller->NoBlockReceive(
        this->CommLists->RcvBuffers[rank],
        this->CommLists->RcvBufferSizes[rank],
        rank,
        0,
        rqsts[rqstIdx]
        );
    ++rqstIdx;
  } // END for all ranks

  // STEP 5: post sends
  rankIter = this->CommLists->NeighboringRanks.begin();
  for( ;rankIter != this->CommLists->NeighboringRanks.end(); ++rankIter)
  {
    int rank = *rankIter;
    assert("pre: cannot find snd buffer size for rank!" &&
           (this->CommLists->SndBufferSizes.find(rank)!=
               this->CommLists->SndBufferSizes.end()));
    assert("pre: cannot find snd buffer for rank!" &&
           (this->CommLists->SndBuffers.find(rank)!=
               this->CommLists->SndBuffers.end()));

    this->Controller->NoBlockSend(
        &this->CommLists->SndBuffers[rank][0],
        this->CommLists->SndBufferSizes[rank],
        rank,
        0,
        rqsts[rqstIdx]
        );
    ++rqstIdx;
  } // END for all ranks

  // STEP 6: wait all
  this->Controller->WaitAll(2*numNeis,rqsts);
  delete [] rqsts;

  // STEP 6: Update ghosted grid
  this->DeSerializeGhostZones();

  // STEP 7: Barrier synchronization
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::FillGhostZoneCells(
      const int neiRank,
      vtkCellData* ghostData,
      vtkIdType* cellIdx,
      const unsigned int numGhostCells)
{

#ifdef NDEBUG
  static_cast<void>(numGhostCells);
#endif

  // Sanity checks
  assert("pre: ghostData should not be NULL!" && (ghostData != NULL) );
  assert("pre: cellIdx should not be NULL!" && (cellIdx != NULL) );
  assert("pre: CommLists object is NULL!" && (this->CommLists != NULL) );
  assert("pre: GhostedGrid is NULL!" && (this->GhostedGrid != NULL) );

  vtkCellData* CD = this->GhostedGrid->GetCellData();

  // Loop through all arrays
  for(int arrayIdx=0; arrayIdx < ghostData->GetNumberOfArrays(); ++arrayIdx)
  {

    vtkDataArray* ghostArray = ghostData->GetArray(arrayIdx);
    assert("pre: array by that name not found on ghosted grid!" &&
            CD->HasArray(ghostArray->GetName()));

#ifndef NDEBUG
    assert("pre: numtuples mismatch!" &&
           (numGhostCells==ghostArray->GetNumberOfTuples()));
#endif

    vtkDataArray* targetArray = CD->GetArray( ghostArray->GetName() );
    assert("pre: numcomponents mismatch between target and ghost array!" &&
     ghostArray->GetNumberOfComponents()==targetArray->GetNumberOfComponents());

    // loop through all the tuples of the array & copy values to the ghostzone
    for(vtkIdType tuple=0; tuple < ghostArray->GetNumberOfTuples(); ++tuple)
    {
      vtkIdType cellId = cellIdx[ tuple ];
      vtkIdType target = this->CommLists->GetTargetCellId(neiRank,cellId);
      CD->CopyTuple(ghostArray,targetArray,tuple,target);
    } // END for all tuples

  } // END for all arrays

}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::FillGhostZoneNodes(
      const int neiRank,
      vtkPointData* ghostData,
      vtkIdType* globalIdx,
      const unsigned int numGhostNodes)
{

#ifdef NDEBUG
  static_cast<void>(numGhostNodes);
#endif

  // Sanity checks
  assert("pre: ghostData should not be NULL!" && (ghostData != NULL) );
  assert("pre: globalIdx should not be NULL!" && (globalIdx != NULL) );
  assert("pre: CommLists object is NULL!" && (this->CommLists != NULL) );
  assert("pre: GhostedGrid is NULL!" && (this->GhostedGrid != NULL) );

  vtkPointData* PD = this->GhostedGrid->GetPointData();

  // Loop through all arrays
  for(int arrayIdx=0; arrayIdx < ghostData->GetNumberOfArrays(); ++arrayIdx)
  {

    vtkDataArray* ghostArray = ghostData->GetArray( arrayIdx );
    if(strcmp(ghostArray->GetName(),this->GlobalIDFieldName)!=0)
    {
      assert("pre: array by that name not found on ghosted grid!" &&
              PD->HasArray(ghostArray->GetName()));

#ifndef NDEBUG
      assert("pre: numtuples mismatch!" &&
             (numGhostNodes==ghostArray->GetNumberOfTuples()));
#endif

      vtkDataArray* targetArray = PD->GetArray( ghostArray->GetName() );
      assert("pre: numcomponents mismatch between target and ghost array!" &&
              ghostArray->GetNumberOfComponents()==
                  targetArray->GetNumberOfComponents());

      // loop through all the tuples of the array & copy values to the
      // ghostzone, i.e., the target array.
      for(vtkIdType tuple=0; tuple < ghostArray->GetNumberOfTuples(); ++tuple)
      {
        vtkIdType globalId = globalIdx[ tuple ];
        vtkIdType targetId = this->CommLists->GetTargetNodeId(neiRank,globalId);
        PD->CopyTuple(ghostArray,targetArray,tuple,targetId);
      } // END for all tuples

    } // END if not the global ID field name

  } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::CreatePersistentRcvBuffers()
{
  // Sanity Checks
  assert("pre: CommLists object is NULL!" && (this->CommLists != NULL));
  assert("pre: numranks != numstreams" &&
    (this->CommLists->NeighboringRanks.size()==
     this->CommLists->SndBufferSizes.size()));

  // short-circuit here if the buffers have been already allocated
  if( this->CommLists->RcvBuffersAllocated )
  {
    return;
  }

  // Allocate MPI request objects for non-blocking point-to-point comm.
  int numNeis = static_cast<int>(this->CommLists->NeighboringRanks.size());
  vtkMPICommunicator::Request* rqsts;
  rqsts = new vtkMPICommunicator::Request[2*numNeis];

  // Post receives
  int rqstIdx = 0;
  std::set<int>::iterator rankIter = this->CommLists->NeighboringRanks.begin();
  for(;rankIter != this->CommLists->NeighboringRanks.end(); ++rankIter)
  {
    int rank = *rankIter;
    this->CommLists->RcvBufferSizes[rank] = 0;
    this->Controller->NoBlockReceive(
        &this->CommLists->RcvBufferSizes[rank],1,rank,0,rqsts[rqstIdx]);
    ++rqstIdx;
  } // END for all neighboring ranks, post receives

  // Post sends
  rankIter = this->CommLists->NeighboringRanks.begin();
  for(;rankIter != this->CommLists->NeighboringRanks.end(); ++rankIter)
  {
    int rank = *rankIter;
    assert("pre: cannot find send bytestream for rank" &&
           (this->CommLists->SndBufferSizes.find(rank)!=
                this->CommLists->SndBufferSizes.end()));
    this->Controller->NoBlockSend(
        &this->CommLists->SndBufferSizes[rank],1,rank,0,rqsts[rqstIdx]);
    ++rqstIdx;
  } // END for all neighboring ranks, post sends

  // Wait all
  this->Controller->WaitAll(2*numNeis,rqsts);
  delete [] rqsts;

  // Allocate buffers for each neighboring rank
  rankIter = this->CommLists->NeighboringRanks.begin();
  for(;rankIter != this->CommLists->NeighboringRanks.end(); ++rankIter)
  {
    int rank = *rankIter;
    assert("pre: cannot find buffersize for rank!" &&
           (this->CommLists->RcvBufferSizes.find(rank) !=
              this->CommLists->RcvBufferSizes.end()) );

    // Get buffer size (communicated from the remote rank earlier)
    int size = this->CommLists->RcvBufferSizes[rank];
    assert("pre: buffer should not exist!" &&
           (this->CommLists->RcvBuffers.find(rank)==
              this->CommLists->RcvBuffers.end()) );

    // Allocate receive buffer
    this->CommLists->RcvBuffers[ rank ] = new unsigned char[size];
  } // END for all neighboring ranks

  // Set RcvBuffersAllocated to true
  this->CommLists->RcvBuffersAllocated = true;
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::DeSerializeGhostZones()
{
  assert("pre: ghosted grid is NULL!" && (this->GhostedGrid != NULL) );
  assert("pre: Persistent CommLists object is NULL!" &&
          (this->CommLists != NULL));

  vtkMultiProcessStream bytestream;
  std::set<int>::iterator rankIter = this->CommLists->NeighboringRanks.begin();
  for( ;rankIter != this->CommLists->NeighboringRanks.end(); ++rankIter)
  {
    int rank = *rankIter;
    assert("pre: no rcv buffer for rank!" &&
            (this->CommLists->RcvBuffers.find(rank)!=
                this->CommLists->RcvBuffers.end()));
    assert("pre: no rcv buffer size for rank!" &&
            (this->CommLists->RcvBufferSizes.find(rank)!=
              this->CommLists->RcvBufferSizes.end()));
    assert("pre: rcvbuffer is NULL!" &&
            this->CommLists->RcvBuffers[rank] != NULL);

    bytestream.Reset();
    bytestream.SetRawData(
        this->CommLists->RcvBuffers[rank],
        this->CommLists->RcvBufferSizes[rank]);

    // Deserialize node-centered fields
    unsigned int numNodeLinks = 0;
    bytestream >> numNodeLinks;

    // Deserialize globalID information
    vtkIdType* globalIdx = new vtkIdType[numNodeLinks];
    bytestream.Pop(globalIdx,numNodeLinks);

    // Deserialize ghostzone pointdata for this rank
    vtkPointData* ghostPD = vtkPointData::New();
    vtkFieldDataSerializer::Deserialize(bytestream,ghostPD);

    // Deserialize cell-centered fields
    unsigned int numCellLinks = 0;
    bytestream >> numCellLinks;

    // Deserialize cellID information
    vtkIdType* cellIdx = new vtkIdType[numCellLinks];
    bytestream.Pop(cellIdx,numCellLinks);

    // Deserialize ghostzone celldata for this rank
    vtkCellData* ghostCD = vtkCellData::New();
    vtkFieldDataSerializer::Deserialize(bytestream,ghostCD);

    // Fill the ghost zones
    this->FillGhostZoneNodes(rank,ghostPD,globalIdx,numNodeLinks);
    this->FillGhostZoneCells(rank,ghostCD,cellIdx,numCellLinks);

    // clear all dynamically allocated memory
    ghostPD->Delete();
    ghostCD->Delete();
    delete [] globalIdx;
    delete [] cellIdx;
  } // END for all neighboring ranks
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::SerializeGhostZones()
{
  assert("pre: ghosted grid is NULL!" && (this->GhostedGrid != NULL) );
  assert("pre: Persistent CommLists object is NULL!" &&
          (this->CommLists != NULL));

  vtkPointData* PD = this->GhostedGrid->GetPointData();
  vtkCellData*  CD = this->GhostedGrid->GetCellData();

  vtkMultiProcessStream bytestream;
  std::set<int>::iterator rankIter = this->CommLists->NeighboringRanks.begin();
  for(;rankIter != this->CommLists->NeighboringRanks.end(); ++rankIter)
  {
    int rank = *rankIter;

    assert("pre: rank not found in SndNodeLinks!" &&
        (this->CommLists->SndNodeLinks.find(rank)!=
         this->CommLists->SndNodeLinks.end()));
    assert("pre: rank not found SndCellLinks" &&
         (this->CommLists->SndCellLinks.find(rank)!=
          this->CommLists->SndCellLinks.end()));

    // clear all data
    bytestream.Reset();

    // Serialize node-centered fields
    std::vector<vtk::details::NodeLink>* nodelinks =
            &this->CommLists->SndNodeLinks[rank];
    bytestream << static_cast<unsigned int>(nodelinks->size());

    // extract the local/global IDs of the nodes
    std::vector< vtkIdType > globalIdx;
    globalIdx.resize(nodelinks->size());
    vtkIdList* tupleIds = vtkIdList::New();
    tupleIds->SetNumberOfIds( static_cast<vtkIdType>(nodelinks->size()) );
    unsigned int lnk = 0;
    for(; lnk < static_cast<unsigned int>(nodelinks->size()); ++lnk)
    {
      globalIdx[lnk] = (*nodelinks)[lnk].GlobalIdx;
      tupleIds->SetId(static_cast<vtkIdType>(lnk),(*nodelinks)[lnk].LocalIdx);
    } // END for all links

    // serialize the global IDs s.t. the remote rank knows which node to
    // update once the data is transferred.
    bytestream.Push(&globalIdx[0],static_cast<unsigned int>(nodelinks->size()));

    // serialize the selected tuples for this remote rank
    vtkFieldDataSerializer::SerializeTuples(tupleIds,PD,bytestream);
    tupleIds->Delete();

    // Serialize cell-centered fields
    std::vector<vtk::details::SndLink>* celllinks =
        &this->CommLists->SndCellLinks[rank];
    bytestream << static_cast<unsigned int>(celllinks->size());

    // extract the cell ids to send to this remote rank
    vtkIdList* cellIds = vtkIdList::New();
    cellIds->SetNumberOfIds(celllinks->size());
    for(lnk=0; lnk < celllinks->size(); ++lnk)
    {
      cellIds->SetId(lnk,(*celllinks)[lnk].SourceIdx);
    } // END for all links

    // serialize the cellIds s.t. the remote rank knows which cell to update
    // once the data is transfered
    bytestream.Push(cellIds->GetPointer(0),cellIds->GetNumberOfIds());

    // serialize the data on the selected cells
    vtkFieldDataSerializer::SerializeTuples(cellIds,CD,bytestream);
    cellIds->Delete();

    // Set the bytestream for this rank
    this->CommLists->SndBufferSizes[rank] = bytestream.RawSize();
    if(this->CommLists->SndBuffers.find(rank) ==
        this->CommLists->SndBuffers.end())
    {
      std::vector<unsigned char> buffer;
      this->CommLists->SndBuffers[rank] = buffer;
    } // END if no snd buffer entry found for rank

    this->CommLists->SndBuffers[rank].resize(bytestream.RawSize());
    bytestream.GetRawData(this->CommLists->SndBuffers[rank]);
  } // END for all neighboring ranks
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::SynchLocalData()
{
  // Sanity checks
  assert("pre: input grid is NULL!" && (this->InputGrid != NULL) );
  assert("pre: ghosted grid is NULL!" && (this->GhostedGrid != NULL) );

  // STEP 0: Get pointers to input point-data and cell-data
  vtkPointData* sourcePD = this->InputGrid->GetPointData();
  assert("pre: source point-data is NULL!" && (sourcePD != NULL) );
  vtkCellData*  sourceCD = this->InputGrid->GetCellData();
  assert("pre: source cell-data is NULL!" && (sourceCD != NULL) );

  // STEP 1: Get pointers to ghosted grid point-data and cell-data
  vtkPointData* targetPD  = this->GhostedGrid->GetPointData();
  assert("pre: target point-data is NULL!" && (targetPD != NULL) );
  vtkCellData* targetCD = this->GhostedGrid->GetCellData();
  assert("pre: target cell-data is NULL!" && (targetCD != NULL) );

  // STEP 2: Copy point-data
  for(int arrayIdx=0; arrayIdx < sourcePD->GetNumberOfArrays(); ++arrayIdx)
  {
    vtkDataArray* field = sourcePD->GetArray( arrayIdx );

    // NOTE: The global IDs are copied upon construction since when the
    // ghosted grid is constructed (in BuildGhostedGridAndCommLists() )
    // global IDs need to be taken in to account!
    if( strcmp(field->GetName(),this->GlobalIDFieldName)!=0 )
    {
      int ncomp         = field->GetNumberOfComponents();
      assert("pre: ncomp must be at lease 1" && (ncomp >= 1) );

      vtkIdType ntuples = this->GhostedGrid->GetNumberOfPoints();
      // ghosted may have more points than input so we can
      // only safely copy the number of input point values
      vtkIdType intuples = this->InputGrid->GetNumberOfPoints();

      vtkDataArray* ghostedField = NULL;
      if( !targetPD->HasArray(field->GetName()))
      {
        ghostedField = vtkDataArray::CreateDataArray(field->GetDataType());
        ghostedField->SetName(field->GetName());
        ghostedField->SetNumberOfComponents(ncomp);
        ghostedField->SetNumberOfTuples(ntuples);
        targetPD->AddArray(ghostedField);
        ghostedField->Delete();
      } // END if array does not exist
      ghostedField = targetPD->GetArray(field->GetName());
      assert("pre: ghosted field is NULL!" && (ghostedField != NULL) );
      memcpy(ghostedField->GetVoidPointer(0),field->GetVoidPointer(0),
             intuples*ncomp*field->GetDataTypeSize());
    } // END if the array is not a global ID field
  } // END for all point data arrays

  // STEP 3: Copy cell-data
  for(int arrayIdx=0; arrayIdx < sourceCD->GetNumberOfArrays(); ++arrayIdx)
  {
    vtkDataArray* field = sourceCD->GetArray( arrayIdx );
    int ncomp         = field->GetNumberOfComponents();
    assert("pre: ncomp must be at lease 1" && (ncomp >= 1) );
    vtkIdType ntuples = this->GhostedGrid->GetNumberOfCells();
    // ghosted may have more points than input so we can
    // only safely copy the number of input point values
    vtkIdType intuples = this->InputGrid->GetNumberOfCells();

    vtkDataArray* ghostedField = NULL;
    if(!targetCD->HasArray(field->GetName()))
    {
      ghostedField = vtkDataArray::CreateDataArray(field->GetDataType());
      ghostedField->SetName(field->GetName());
      ghostedField->SetNumberOfComponents(ncomp);
      ghostedField->SetNumberOfTuples(ntuples);
      targetCD->AddArray(ghostedField);
      ghostedField->Delete();
    } // END if array does not exists

    ghostedField = targetCD->GetArray(field->GetName());
    assert("pre: ghosted field is NULL!" && (ghostedField != NULL) );
    memcpy(ghostedField->GetVoidPointer(0),field->GetVoidPointer(0),
           intuples*ncomp*field->GetDataTypeSize());
  } // END for all cell data arrays

  // STEP 4: Finally, mark ghost cells. The ghost cells are marked only
  // the first time UpdateGhosts() is called.
  if( !targetCD->HasArray("GHOSTCELL") )
  {
    vtkIntArray* ghostCellArray = vtkIntArray::New();
    ghostCellArray->SetName("GHOSTCELL");
    ghostCellArray->SetNumberOfComponents(1);
    ghostCellArray->SetNumberOfTuples(this->GhostedGrid->GetNumberOfCells());
    int* ghostCellPtr = static_cast<int*>(ghostCellArray->GetVoidPointer(0));
    vtkIdType ncells = this->GhostedGrid->GetNumberOfCells();
    for(vtkIdType cellIdx=0; cellIdx < ncells; ++cellIdx)
    {
      if(cellIdx < this->InputGrid->GetNumberOfCells())
      {
        // cell is not a ghost
        ghostCellPtr[cellIdx] = 0;
      } // END if the cell is local
      else
      {
        // cell is a ghost cell
        ghostCellPtr[cellIdx] = 1;
      } // END else
    } // END for all cells
    this->GhostedGrid->GetCellData()->AddArray(ghostCellArray);
    ghostCellArray->Delete();
  } // END if no ghostcell array
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::EnqueueNodeLinks(
      const int rmtRank,
      const vtkIdType ghostCell,
      const vtkIdType adjCell,
      vtkIdList* shared)
{
  // Sanity Checks
  assert("pre: ghosted grid is NULL!" && (this->GhostedGrid != NULL) );
  assert("pre: ghostCell out-of-bounds!" &&
     (ghostCell >= 0) && (ghostCell < this->GhostedGrid->GetNumberOfCells()));
  assert("pre: adjCell out-of-bounds!" &&
     (adjCell >= 0) && (adjCell < this->GhostedGrid->GetNumberOfCells()));
  assert("pre: ghost grid must have global IDs" &&
      this->GhostedGrid->GetPointData()->HasArray(this->GlobalIDFieldName));

  // STEP 0: Put the shared nodes in a set, s.t. we can do easy look up
  std::set<vtkIdType> sharedNodes;
  for(vtkIdType idx=0; idx < shared->GetNumberOfIds(); ++idx)
  {
    sharedNodes.insert(shared->GetId(idx));
  }
  assert("post: shared nodes mismatch!" &&
      (shared->GetNumberOfIds()==static_cast<vtkIdType>(sharedNodes.size())));

  // STEP 1: Get pointer to the global ID array on the ghosted grid
  vtkIdType* globalIdxArray =
    static_cast<vtkIdType*>(
       this->GhostedGrid->GetPointData()->
         GetArray(this->GlobalIDFieldName)->GetVoidPointer(0)
         );

  // STEP 2: local variables used to traverse the nodes of the adjCell and
  // ghostCell
  vtkIdType npts = 0;
  vtkIdType* pts = NULL;

  // STEP 3: Get pointer to the connectivity list of the adjacent cell
  this->GhostedGrid->GetCellPoints(adjCell,npts,pts);
  assert("post: adjCell pts is NULL!" && (pts != NULL) );
  assert("post: npts >= 1" && (npts >= 1) );

  // STEP 4: Loop through all adjacent cell nodes. The nodes of the adjacent
  // cell that are not on the shared interface with the ghost cell are
  // enqueued to be *sent* to the remote process of the ghost cell.
  for(vtkIdType idx=0; idx < npts; ++idx)
  {
    vtkIdType localId  = pts[idx];
    vtkIdType globalId = globalIdxArray[ localId ];
    if(sharedNodes.find(globalId) == sharedNodes.end() )
    {
      this->CommLists->EnqueueNodeSend(localId,globalId,rmtRank);
    } // END if node not at a shared interface
  } // END for all adjacent cell nodes

  // STEP 5: Get pointer to the connectivity list of the ghost cell
  npts = 0;
  pts  = NULL;
  this->GhostedGrid->GetCellPoints(ghostCell,npts,pts);
  assert("post: adjCell pts is NULL!" && (pts != NULL) );
  assert("post: npts >= 1" && (npts >= 1) );

  // STEP 6: Loop through all ghost cell nodes. The nodes of the ghost cell
  // that are not on the shared interface with the local adjacent cell are
  // enqueued to *receive* from the remote process that owns the ghost cell.
  for(vtkIdType idx=0; idx < npts; ++idx)
  {
    vtkIdType localId  = pts[idx];
    vtkIdType globalId = globalIdxArray[ localId ];
    if( sharedNodes.find(globalId) == sharedNodes.end() )
    {
      this->CommLists->EnqueueNodeRcv(localId,globalId,rmtRank);
    } // END if node not at a shared interface
  } // END for all ghost cell nodes
}

//------------------------------------------------------------------------------
bool vtkPUnstructuredGridConnectivity::IsCellConnected(
      vtkCell* c, vtkIdType* globalId, const vtkIdType NumPoints,
      vtkIdType& adjCell,
      vtkIdList* shared)
{
#ifdef NDEBUG
  static_cast<void>(NumPoints);
#endif

  adjCell = -1;

  // nodes vector used as temporary storage for edge/face nodes in order
  // to construct a corresponding hashcode to uniquely identify an edge
  // or face, regardless of orientation.
  std::vector<vtkIdType>  nodes;

  // Check faces
  for(int f=0; f < c->GetNumberOfFaces(); ++f)
  {
    vtkCell* face      = c->GetFace( f );
    int N              = face->GetNumberOfPoints();
    vtkIdType* nodePtr = face->GetPointIds()->GetPointer(0);

     nodes.resize(N);
     shared->SetNumberOfIds(N);
     for(int i=0; i < N; ++i)
     {

#ifndef NDEBUG
       assert("pre: face node out-of-bounds!" &&
               (nodePtr[i] >= 0) && (nodePtr[i] < NumPoints) );
#endif

       nodes[i] = globalId[ nodePtr[i] ];
       shared->SetId(i,nodes[i]);
     } // END for all face nodes

     std::string hashCode = vtk::details::Hash(&nodes[0],N);
     if( this->AuxiliaryData->BoundaryGridLinks.HasFace( hashCode ) )
     {
       assert("pre: boundary faces must have at most one cell" &&
        this->AuxiliaryData->BoundaryGridLinks.FaceLinks[hashCode].size()==1);
       adjCell =
         *(this->AuxiliaryData->BoundaryGridLinks.FaceLinks[hashCode].begin());
       return true;
     } // END if
  } // END for all faces


  // cell is not connected to the boundary grid of this process
  return(false);
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::InsertGhostCellNodes(
      vtkCell* ghostCell,
      vtkIdTypeArray* ghostGridGlobalIdx,
      vtkIdType* globalIdArray,
      vtkUnstructuredGrid* bGrid,
      vtkIdType* cellPts)
{
  assert("pre: ghost cell is NULL!" && (ghostCell != NULL) );
  assert("pre: null global ID array!" && (globalIdArray != NULL) );
  assert("pre: remote boundary grid is NULL!" && (bGrid != NULL) );
  assert("pre: cellPts buffer is NULL!" && (cellPts != NULL) );

  double pnt[3];
  for(vtkIdType node=0; node < ghostCell->GetNumberOfPoints(); ++node)
  {
    // mesh index of the point w.r.t. the boundary grid
    vtkIdType meshId   = ghostCell->GetPointId(node);

    // global ID of the node
    vtkIdType globalId = globalIdArray[ meshId ];

    // get the local ID of the node, if it is one of the boundary nodes
    vtkIdType localId  =
        this->AuxiliaryData->BoundaryGridLinks.GetLocalNodeID(globalId);

    if( localId != -1 )
    {
      // node is a boundary node
      cellPts[ node ] = localId;
    } // END if
    else if( this->AuxiliaryData->NodeHistory.find( globalId ) !=
             this->AuxiliaryData->NodeHistory.end() )
    {
      // we have previously inserted that node
      cellPts[ node ] = this->AuxiliaryData->NodeHistory[ globalId ];
    } // END else if
    else
    {
      // insert the node & update the history
      bGrid->GetPoint(meshId,pnt);
      vtkIdType idx = this->GhostedGrid->GetPoints()->InsertNextPoint(pnt);
      cellPts[ node ] = idx;
      assert("post: new node id mismatch!" &&
             (this->GhostedGrid->GetNumberOfPoints()-1)==idx);

      // Update node history
      this->AuxiliaryData->NodeHistory[ globalId ] = idx;

      // Update global ID array on ghosted grid
      ghostGridGlobalIdx->InsertNextValue( globalId );

      assert("post: ghost grid global ID array size mismatch" &&
             (this->GhostedGrid->GetNumberOfPoints()==
              ghostGridGlobalIdx->GetNumberOfTuples()) );
    } // END else
  } // END for all nodes
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::ProcessRemoteGrid(
      const int rmtRank, vtkUnstructuredGrid* bGrid)
{
  // Sanity Checks
  assert("pre: remote bgrid is NULL!" && (bGrid != NULL) );
  assert("pre: ghosted grid instance is NULL!" && (this->GhostedGrid != NULL));
  assert("pre: controller is NULL!" && (this->Controller != NULL) );
  assert("pre: remote rank is out-of-bounds!" &&
          (rmtRank >= 0) &&
          (rmtRank < this->Controller->GetNumberOfProcesses()) );
  assert("pre: remote bgrid doesn't have global ID!"&&
          bGrid->GetPointData()->HasArray("GLOBAL_ID"));
  assert("preS: remote bgrid doesn't have local cell ID!" &&
          bGrid->GetCellData()->HasArray("LOCAL_CELL_ID"));

  // Get the GlobalID array of the output GhostGrid. This method grows that
  // array accordingly as ghost nodes are inserted.
  vtkIdTypeArray* ghostGridGlobalIdArray =
      vtkArrayDownCast<vtkIdTypeArray>(
         this->GhostedGrid->GetPointData()->GetArray(this->GlobalIDFieldName));
  assert("pre: cannot get global ID field from GhostedGrid" &&
         (ghostGridGlobalIdArray != NULL) );

  // Get pointer to the GlobalID array on the boundary grid.
  vtkIdType* globalIdx =
      static_cast<vtkIdType*>(
          bGrid->GetPointData()->GetArray("GLOBAL_ID")->GetVoidPointer(0));

  // Get pointer to the local cell ID w.r.t. the remote grid, of the cells
  // on the boundary grid.
  vtkIdType* rmtCellIdx =
      static_cast<vtkIdType*>(
          bGrid->GetCellData()->GetArray("LOCAL_CELL_ID")->GetVoidPointer(0));

  // Loop through all remote boundary grid cells, check to see if they are
  // abutting with the boundary grid of the input grid and if so, update
  // the ghosted grid.
  vtkIdType adjCell = -1;
  std::vector<vtkIdType> cellPts;
  vtkIdList* sharedIds = vtkIdList::New();
  for(vtkIdType c=0; c < bGrid->GetNumberOfCells(); ++c)
  {
    vtkCell* cell     = bGrid->GetCell(c);
    vtkIdType rmtCell = rmtCellIdx[c];

    if(this->IsCellConnected(
          cell,globalIdx,bGrid->GetNumberOfPoints(),adjCell,sharedIds))
    {
      // Sanity checks
      assert("pre: number of sharedIds must be at least 2" &&
              (sharedIds->GetNumberOfIds()>=2) );
      assert("pre: adjCell is out-of-bounds from input grid!" &&
        (adjCell >= 0) && (adjCell < this->InputGrid->GetNumberOfCells()));
      assert("pre: adjCell is out-of-bounds from ghosted grid!" &&
        (adjCell >= 0) && (adjCell < this->GhostedGrid->GetNumberOfCells()));

      // Insert cell points
      cellPts.resize( cell->GetNumberOfPoints() );
      this->InsertGhostCellNodes(
          cell,ghostGridGlobalIdArray,globalIdx,bGrid,&cellPts[0]);

      // Insert ghost cell, if this cell is not inserted by another partition
      std::vector<vtkIdType> cellNodesCopy = cellPts;
      std::string hc = vtk::details::Hash(&cellNodesCopy[0],cellNodesCopy.size());
      if( this->AuxiliaryData->CellHistory.find(hc) ==
          this->AuxiliaryData->CellHistory.end() )
      {
        vtkIdType ghostCellIdx =
            this->GhostedGrid->InsertNextCell(
                cell->GetCellType(),cell->GetNumberOfPoints(),&cellPts[0]);
        assert("post: ghostCellIdx mismatch!" &&
              (ghostCellIdx == this->GhostedGrid->GetNumberOfCells()-1) );

        // update cell communication list
        this->CommLists->EnqueueCellLink(adjCell,ghostCellIdx,rmtCell,rmtRank);

        // Enqueue node links
        this->EnqueueNodeLinks(rmtRank,ghostCellIdx,adjCell,sharedIds);

        // update history s.t. we avoid adding duplicate cells.
        this->AuxiliaryData->CellHistory.insert(hc);
      } // END if

    } // END if the cell is connected
  } // END for all cells

  // Delete sharedIds object
  sharedIds->Delete();

  // sanity check!
  assert("post: ghost grid global ID array size mismatch" &&
         (this->GhostedGrid->GetNumberOfPoints()==
          ghostGridGlobalIdArray->GetNumberOfTuples()) );
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::BuildGhostedGridAndCommLists()
{
  assert("pre: ghosted grid should be NULL!" && (this->GhostedGrid==NULL));

  // STEP 0: Deep-Copy the topology of the input grid to the ghosted grid.
  this->GhostedGrid = vtkUnstructuredGrid::New();
  vtkUnstructuredGrid* tmpPtr = vtkUnstructuredGrid::New();
  tmpPtr->CopyStructure(this->InputGrid);
  this->GhostedGrid->DeepCopy(tmpPtr);
  tmpPtr->Delete();

  // STEP 1: Deep-Copy the global IDs
  vtkIdTypeArray* globalIdx = vtkIdTypeArray::New();
  globalIdx->SetName(this->GlobalIDFieldName);
  globalIdx->SetNumberOfComponents(1);
  globalIdx->SetNumberOfTuples(this->GhostedGrid->GetNumberOfPoints());
  memcpy(
    globalIdx->GetVoidPointer(0),
    this->InputGrid->GetPointData()
      ->GetArray(this->GlobalIDFieldName)->GetVoidPointer(0),
    sizeof(vtkIdType)*this->GhostedGrid->GetNumberOfPoints()
    );

  assert("pre: globalIdx size mismatch!" &&
      globalIdx->GetNumberOfTuples()==this->GhostedGrid->GetNumberOfPoints());
  this->GhostedGrid->GetPointData()->AddArray(globalIdx);
  globalIdx->Delete();

  // STEP 2: Loop through all remote boundary grids, find the cells that
  // are face-adjacent and insert them to the ghosted grid.
  unsigned int i=0;
  for(;i<static_cast<unsigned int>(this->AuxiliaryData->RmtBGrids.size()); ++i)
  {
    int rmtRank = this->AuxiliaryData->CandidateRanks[ i ];
    this->ProcessRemoteGrid(
        rmtRank,this->AuxiliaryData->RmtBGrids[i]);
  } // END for all remote grids
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::ExchangeBoundaryGridSizes(int size)
{
  int numCandidates =
      static_cast<int>(this->AuxiliaryData->CandidateRanks.size());
  this->AuxiliaryData->RmtBGridSizes.resize(numCandidates,0);

  vtkMPICommunicator::Request* rqsts;
  rqsts = new vtkMPICommunicator::Request[2*numCandidates];

  // STEP 0: Post receives for each candidate rank
  int idx = 0;
  for(int i=0; i < numCandidates; ++i)
  {
   int rmtRank = this->AuxiliaryData->CandidateRanks[ i ];
   this->Controller->NoBlockReceive(
       &this->AuxiliaryData->RmtBGridSizes[i],1,rmtRank,
       0,rqsts[idx]);
   ++idx;
  } // END for all candidate ranks

  // STEP 1: Post sends
  for(int i=0; i < numCandidates; ++i)
  {
    int rmtRank = this->AuxiliaryData->CandidateRanks[ i ];
    this->Controller->NoBlockSend(
        &size,1,rmtRank,0,rqsts[idx]);
    ++idx;
  }

  // STEP 2: Block until communication
  this->Controller->WaitAll(2*numCandidates,rqsts);
  delete [] rqsts;
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::ExchangeBoundaryGrids()
{
  assert("pre: Boundary Grid should not be NULL!" &&
          (this->AuxiliaryData->BoundaryGrid != NULL));

  // STEP 0: Serialize the local grid
  vtkMultiProcessStream bytestream;
  this->SerializeUnstructuredGrid(
      this->AuxiliaryData->BoundaryGrid,bytestream);

  // STEP 1: Point-to-Point exchange boundary grid sizes
  this->ExchangeBoundaryGridSizes(bytestream.RawSize());

  // STEP 2: Post receives
  int numCandidates =
      static_cast<int>(this->AuxiliaryData->CandidateRanks.size());
  std::vector< unsigned char* > RawData;
  RawData.resize(numCandidates);

  vtkMPICommunicator::Request* rqsts;
  rqsts = new vtkMPICommunicator::Request[2*numCandidates];

  int idx = 0;
  for(int i=0; i < numCandidates; ++i)
  {
    int rmtRank = this->AuxiliaryData->CandidateRanks[ i ];
    int size    = this->AuxiliaryData->RmtBGridSizes[ i ];
    RawData[i]  = new unsigned char[size];
    this->Controller->NoBlockReceive(
        RawData[i],size,rmtRank,0,rqsts[idx]);
    ++idx;
  } // END for all candidates

  // STEP 3: Post sends
  unsigned char* data = NULL;
  unsigned int size   = 0;
  bytestream.GetRawData(data,size);

  for(int i=0; i < numCandidates; ++i)
  {
    int rmtRank = this->AuxiliaryData->CandidateRanks[ i ];
    this->Controller->NoBlockSend(
        data,size,rmtRank,0,rqsts[idx]);
    ++idx;
  } // END for all candidates

  // STEP 4: Block until communication is complete
  this->Controller->WaitAll(2*numCandidates,rqsts);
  delete [] rqsts;
  delete [] data;

  // STEP 5: De-serialize remote boundary grids
  this->AuxiliaryData->RmtBGrids.resize(numCandidates,NULL);
  vtkMultiProcessStream tmpStream;
  for(int i=0; i < numCandidates; ++i)
  {
    int sz = this->AuxiliaryData->RmtBGridSizes[ i ];
    tmpStream.Reset();
    tmpStream.SetRawData(RawData[i],sz);

    this->AuxiliaryData->RmtBGrids[ i ] = vtkUnstructuredGrid::New();
    this->DeSerializeUnstructuredGrid(
        this->AuxiliaryData->RmtBGrids[i],tmpStream);

#ifdef DEBUG
    std::ostringstream oss;
    oss << "BOUNDARY_GRID-P" << this->AuxiliaryData->CandidateRanks[i] << "-";
    oss << "AT-RANK";
    this->WriteUnstructuredGrid(
        this->AuxiliaryData->RmtBGrids[i],oss.str().c_str());
#endif

    delete [] RawData[i];
    RawData[i] = NULL;
  } // END for all candidates

  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::BoundingBoxCollision()
{
  // Sanity checks
  assert("pre: controller is NULL!" && (this->Controller != NULL) );
  int N = this->Controller->GetNumberOfProcesses();

  assert("pre: auxiliary data is NULL!" && (this->AuxiliaryData != NULL) );
  assert("pre: bounding box list size mismarch!" &&
    (static_cast<int>(this->AuxiliaryData->GlobalGridBounds.size())==6*N) );

  int myRank = this->Controller->GetLocalProcessId();

  this->AuxiliaryData->CandidateRanks.reserve(N);

  vtkBoundingBox localBox(this->AuxiliaryData->GridBounds);
  vtkBoundingBox rmtBox;
  for(int i=0; i < N; ++i)
  {
    if(i != myRank)
    {
      rmtBox.SetBounds(
          this->AuxiliaryData->GlobalGridBounds[i*6],   // xmin
          this->AuxiliaryData->GlobalGridBounds[i*6+1], // xmax
          this->AuxiliaryData->GlobalGridBounds[i*6+2], // ymin
          this->AuxiliaryData->GlobalGridBounds[i*6+3], // ymax
          this->AuxiliaryData->GlobalGridBounds[i*6+4], // zmin
          this->AuxiliaryData->GlobalGridBounds[i*6+5]  // zmax
          );

      if(localBox.Intersects(rmtBox))
      {
        this->AuxiliaryData->CandidateRanks.push_back( i );
      } // END if
    } // END if remote rank
  } // END for all remote bounding boxes
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::ExchangeGridBounds()
{
  // Sanityc checks
  assert("pre: controller is NULL!" && (this->Controller != NULL) );

  // STEP 0: Allocate buffers. Each process sends 6 doubles and receives 6
  // doubles from each remote process. Hence, the rcv buffer is allocated
  // as N*6.
  int N = this->Controller->GetNumberOfProcesses();
  this->AuxiliaryData->GlobalGridBounds.resize( N*6 );

  // STEP 1: Communicates the bounds. Upon completion, GlobalGridBounds stores
  // the bounds of each process in a flat vector strided by 6. The bounds of
  // of process, P_i, are stored contiguously in the region [i*6, i*6+5] of the
  // GlobalGridBounds array.
  this->Controller->AllGather(
      this->AuxiliaryData->GridBounds,
      &this->AuxiliaryData->GlobalGridBounds[0],6);
}

//------------------------------------------------------------------------------
bool vtkPUnstructuredGridConnectivity::IsCellOnBoundary(
      vtkIdType* cellNodes, vtkIdType N)
{
  assert("pre: null cell nodes array!" && (cellNodes != NULL) );

  for(int i=0; i < N; ++i)
  {
    if(this->AuxiliaryData->SurfaceNodes.find(cellNodes[i]) !=
        this->AuxiliaryData->SurfaceNodes.end() )
    {
      return true;
    } // END if
  } // END for all nodes of the cell

  return false;
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::MarkFaces()
{
  vtkIdType numCells = this->InputGrid->GetNumberOfCells();
  for(vtkIdType cellIdx=0; cellIdx < numCells; ++cellIdx)
  {
    vtkCell* cell = this->InputGrid->GetCell( cellIdx );
    assert("pre: cell is NULL!" && (cell != NULL) );

    /// @todo: optimize this using lookup tables, at least for linear cells,
    /// since we only need the IDs of the faces
    for(int faceIdx=0; faceIdx < cell->GetNumberOfFaces(); ++faceIdx)
    {
      vtkCell* face = cell->GetFace( faceIdx );
      this->AuxiliaryData->UpdateFaceList(face,cellIdx);
    } // END for all faces
  }  // END for all cells

}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::ExtractSurfaceMesh()
{
  std::map<std::string,vtk::details::FaceInfo>::iterator iter;

  iter = this->AuxiliaryData->FaceList.begin();
  for(;iter != this->AuxiliaryData->FaceList.end(); ++iter)
  {
    assert("pre: a face can only be adjacent to at most two cells!" &&
            (iter->second.Count <= 2) );

    if(iter->second.Count==1)
    {
     assert("pre: duplicate boundary face!" &&
         this->AuxiliaryData->SurfaceMesh.find(iter->first)==
             this->AuxiliaryData->SurfaceMesh.end());

     this->AuxiliaryData->SurfaceMesh[ iter->first ] = iter->second;
     for(unsigned int i=0; i < iter->second.FaceIds.size(); ++i)
     {
       this->AuxiliaryData->SurfaceNodes.insert(iter->second.FaceIds[i]);
     } // END for all bndry face ids
    } // END if face on boundary

  } // END for all faces on the input mesh
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::ExtractBoundaryCell(
    const vtkIdType cellIdx,
    const vtkIdType numCellNodes,
    vtkIdType* cellNodes,
    vtkPoints* nodes,
    vtkIdTypeArray* localIdx,
    vtkIdTypeArray* globalIdx)
{
  // Sanity checks
  assert("pre: input grid is NULL!" && (this->InputGrid != NULL) );
  assert("pre: auxiliary data is NULL!" && (this->AuxiliaryData != NULL) );
  assert("pre: nodes is NULL!" && (nodes != NULL) );
  assert("pre: localIdx is NULL!" && (localIdx != NULL) );
  assert("pre: globalIdx is NULL!" && (globalIdx != NULL) );
  assert("pre: cellIdx is out-of-bounds!" &&
      (cellIdx >= 0) && (cellIdx < this->InputGrid->GetNumberOfCells()));

  // STEP 0: Get the global ID information from the input grid
  vtkPointData* PD = this->InputGrid->GetPointData();
  assert("pre: PD is NULL!" && (PD != NULL) );
  vtkDataArray* G  = PD->GetArray(this->GlobalIDFieldName);
  assert("pre: Global array, G, is NULL!" && (G != NULL) );
  vtkIdType* globalInfo = static_cast<vtkIdType*>(G->GetVoidPointer(0));

  // STEP 1: Get the cell type from the input grid
  int cellType = this->InputGrid->GetCellType(cellIdx);

  // STEP 2: Create vector for the cell connectivity that will be inserted
  // in the boundary grid instance.
  std::vector< vtkIdType > cellConnectivity;
  cellConnectivity.resize(numCellNodes);

  // STEP 3: Loop through the cell nodes and first update the nodal information
  // of the boundary and the cell connectivity for this cell.
  double pt[3];
  for(vtkIdType nodeIdx=0; nodeIdx < numCellNodes; ++nodeIdx)
  {
    vtkIdType ptIdx = cellNodes[ nodeIdx ]; // local idx w.r.t. input grid
    if( this->AuxiliaryData->BndryNodeMap.find(ptIdx) ==
        this->AuxiliaryData->BndryNodeMap.end() )
    {
      // insert new point on the boundary grid from the input grid
      this->InputGrid->GetPoint(ptIdx,pt);

      vtkIdType idx             = nodes->InsertNextPoint(pt);
      cellConnectivity[nodeIdx] = idx;
      localIdx->InsertNextValue(ptIdx);
      globalIdx->InsertNextValue(globalInfo[ptIdx]);

      // update the node map
      this->AuxiliaryData->BndryNodeMap[ptIdx] = idx;
    }
    else
    {
      // node has already been inserted to the boundary grid, just update
      // the connectivity
      cellConnectivity[nodeIdx] = this->AuxiliaryData->BndryNodeMap[ptIdx];
    }
  } // END for all cell nodes

  // STEP 4: Insert the cell in to the boundary grid
  this->AuxiliaryData->BoundaryGrid->InsertNextCell(
      cellType,numCellNodes,&cellConnectivity[0]);

  // sanity checks
#ifndef NDEBUG
  vtkIdType N = nodes->GetNumberOfPoints();
  assert("post: array size mismatch!" && (localIdx->GetNumberOfTuples()==N));
  assert("post: array size mismatch!" && (globalIdx->GetNumberOfTuples()==N));
#endif
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::ExtractBoundaryGrid()
{
  // Sanity checks
  assert("pre: input grid is NULL!" && (this->InputGrid != NULL) );
  assert("pre: auxiliary data is NULL!" && (this->AuxiliaryData != NULL) );

  vtkIdType numCells = this->InputGrid->GetNumberOfCells();

  // Allocate data-structure for boundary grid
  this->AuxiliaryData->BoundaryGrid = vtkUnstructuredGrid::New();
  this->AuxiliaryData->BoundaryGrid->Allocate(numCells,numCells*8);

  // Create global ID array -- for each node in the boundary grid we store
  // the corresponding global ID from the input grid.
  vtkIdTypeArray* globalidx = vtkIdTypeArray::New();
  globalidx->SetName("GLOBAL_ID");
  globalidx->SetNumberOfComponents(1);
  globalidx->Allocate(this->InputGrid->GetNumberOfPoints()); // pre-allocate

  // Create the local ID array -- for each node in the boundary grid we store
  // the local ID w.r.t. to the input grid.
  vtkIdTypeArray* localidx  = vtkIdTypeArray::New();
  localidx->SetName("LOCAL_ID");
  localidx->SetNumberOfComponents(1);
  localidx->Allocate(this->InputGrid->GetNumberOfPoints()); // pre-allocate

  // Create the local cell ID array -- for each cell in the boundary grid we
  // store the corresponding local cell ID w.r.t. to the input grid.
  vtkIdTypeArray* localCellIdx = vtkIdTypeArray::New();
  localCellIdx->SetName("LOCAL_CELL_ID");
  localCellIdx->SetNumberOfComponents(1);
  localCellIdx->Allocate(this->InputGrid->GetNumberOfCells()); // pre-allocate

  // Allocate boundary grid nodes
  vtkPoints* points = vtkPoints::New();
  points->SetDataTypeToDouble();
  points->Allocate(this->InputGrid->GetNumberOfPoints()); // pre-allocate

  // STEP 0: Loop through all cells and mark faces -- O(N)
  this->MarkFaces();

  // STEP 1: Loop through all marked faces and extract the surface
  // mesh of the input grid -- O(N)
  this->ExtractSurfaceMesh();

  // STEP 2: Loop through all cells and extract cells on the boundary -- O(N)
  vtkIdType numNodes = 0;    // numNodes in cell
  vtkIdType* nodes   = NULL; // pointer to the cell Ids
  for(vtkIdType cellIdx=0; cellIdx < numCells; ++cellIdx)
  {
    // Get point IDs of the cell. Note, this method returns a "read-only"
    // pointer to the underlying connectivity array for the cell in query.
    // No memory is allocated.
    this->InputGrid->GetCellPoints(cellIdx,numNodes,nodes);
    assert("pre: nodes ptr should not be NULL!" && (nodes != NULL) );

    if( this->IsCellOnBoundary(nodes,numNodes) )
    {
      this->ExtractBoundaryCell(
          cellIdx,numNodes,nodes,
          points,
          localidx,
          globalidx
          );
      localCellIdx->InsertNextValue(cellIdx);
    } // END if cell on boundary
  } // END for all cells

  // STEP 3: Return any memory that was allocated but not used.
  points->Squeeze();
  localidx->Squeeze();
  globalidx->Squeeze();
  localCellIdx->Squeeze();
  this->AuxiliaryData->BoundaryGrid->Squeeze();

  // sanity checks
#ifndef NDEBUG
  vtkIdType nc  = this->AuxiliaryData->BoundaryGrid->GetNumberOfCells();
  vtkIdType numPoints = points->GetNumberOfPoints();
  assert("array size mismatch!" && (localidx->GetNumberOfTuples()==numPoints));
  assert("array size mismatch!" && (globalidx->GetNumberOfTuples()==numPoints));
  assert("post: array size mismatch!" &&
         (localCellIdx->GetNumberOfTuples()==nc) );
#endif

  this->AuxiliaryData->BoundaryGrid->SetPoints(points);
  this->AuxiliaryData->BoundaryGrid->GetPointData()->AddArray(localidx);
  this->AuxiliaryData->BoundaryGrid->GetPointData()->AddArray(globalidx);
  this->AuxiliaryData->BoundaryGrid->GetCellData()->AddArray(localCellIdx);

  points->Delete();
  globalidx->Delete();
  localidx->Delete();
  localCellIdx->Delete();

  // Build links on the boundary grid
  this->AuxiliaryData->BoundaryGridLinks.BuildLinks(
      this->AuxiliaryData->BoundaryGrid);

  // Write the unstructured grid, iff debugging is turned on
#ifdef DEBUG
  this->WriteUnstructuredGrid(
      this->AuxiliaryData->BoundaryGrid,"BG");
#endif
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::SerializeUnstructuredGrid(
      vtkUnstructuredGrid* g, vtkMultiProcessStream& bytestream)
{
  assert("pre: cannot serialize a null grid!" && (g != NULL) );
  assert("pre: byte-stream should be empty" && bytestream.Empty() );

  // serialize the number of points and cells in the grid
  bytestream << g->GetNumberOfPoints();
  bytestream << g->GetNumberOfCells();

  // serialize the nodes of the grid
  double* nodes = static_cast<double*>(g->GetPoints()->GetVoidPointer(0));
  bytestream.Push(nodes,3*g->GetNumberOfPoints());

  // serialize the cell connectivity information of the grid
  vtkIdType n       = 0;    // number of nodes of each cell
  vtkIdType* cnodes = NULL; // pointer to the cell connectivity array
  for(vtkIdType cellIdx=0; cellIdx < g->GetNumberOfCells(); ++cellIdx)
  {
    // push the cell type
    bytestream << g->GetCellType(cellIdx);

    // get the cell points
    g->GetCellPoints(cellIdx,n,cnodes);

    // push the number of nodes per cell
    bytestream << n;

    // push the cell connectivity
    bytestream.Push(cnodes,n);
  } // END for all cells

  // serialize the point data
  vtkFieldDataSerializer::Serialize(g->GetPointData(),bytestream);

  // serialize the cell data
  vtkFieldDataSerializer::Serialize(g->GetCellData(),bytestream);
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::DeSerializeUnstructuredGrid(
    vtkUnstructuredGrid* g, vtkMultiProcessStream& bytestream)
{
  assert("pre: input grid is NULL!" && (g != NULL) );
  assert("pre: byte-stream should not be empty!" && !bytestream.Empty() );

  unsigned int N; // auxiliary local variable used to satisfy bytestream API.

  // deserialize the number of points & number of cells
  vtkIdType numPoints = 0;
  vtkIdType numCells  = 0;
  bytestream >> numPoints;
  bytestream >> numCells;

  // deserialize the grid points
  vtkPoints* pnts = vtkPoints::New();
  pnts->SetDataTypeToDouble();
  pnts->SetNumberOfPoints(numPoints);

  double* nodes = static_cast<double*>(pnts->GetVoidPointer(0));
  N = 3*numPoints;
  assert( N==3*numPoints );
  bytestream.Pop(nodes,N);

  g->SetPoints( pnts );
  pnts->Delete();

  // pre-allocate internal buffer for connectivity
  g->Allocate(numCells,8);

  // deserialize the grid connectivity
  int cellType      = 0;
  vtkIdType n       = 0;
  std::vector<vtkIdType> cnodes;
  for(vtkIdType cellIdx=0; cellIdx < numCells; ++cellIdx)
  {
    bytestream >> cellType >> n;
    cnodes.resize( n );
    vtkIdType* cnodesPtr = &cnodes[0];
    N = n;
    bytestream.Pop(cnodesPtr,N);

    g->InsertNextCell(cellType,n,cnodesPtr);
  } // END for all cells

  g->Squeeze();

  // De-serialize point data
  vtkFieldDataSerializer::Deserialize(bytestream,g->GetPointData());

  // De-serialize cell data
  vtkFieldDataSerializer::Deserialize(bytestream,g->GetCellData());
}

//------------------------------------------------------------------------------
void vtkPUnstructuredGridConnectivity::WriteUnstructuredGrid(
      vtkUnstructuredGrid* g, const char* fileName)
{
  assert("pre: input grid is NULL!" && (g != NULL) );
  assert("pre: fileName is NULL!" && (fileName != NULL) );

  std::ostringstream oss;
  oss << fileName << "-" << this->Controller->GetLocalProcessId() << ".vtk";

  vtkUnstructuredGridWriter* writer = vtkUnstructuredGridWriter::New();
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(g);
  writer->Update();
  writer->Delete();
}

#endif //VTK_LEGACY_REMOVE
