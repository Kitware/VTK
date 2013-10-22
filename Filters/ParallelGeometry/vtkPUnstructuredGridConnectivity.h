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
// .NAME vtkPUnstructuredGridConnectivity.h -- Unstructured grid connectivity.
//
// .SECTION Description
//  vtkPUnstructuredGridConnectivity implements functionality for generating
//  ghost zones for a distributed unstructured grid. Generating ghost zones is
//  implemented in two stages. First, we build the ghost zones, which amounts
//  to building the connectivity of the ghosted grid and communication links,
//  and second, we update the ghost zones by communicating the fields on the
//  ghost cells and nodes. The main steps involved in this process are as
//  follows:
//  <ol>
//    <li> Each process computes a bounding box of the grid it owns. </li>
//    <li> The bounding boxes are then distributed to all processes by
//         an AllGather collective call. </li>
//    <li> Each process loops through the list of bounding boxes and
//         computes box intersections with its local bounding box. </li>
//    <li> The list of intersecting bounding boxes, yields an abbreviated list
//         of candidate neighbors. </li>
//    <li> Given the local grid, each process then extracts the boundary grid,
//         which consists of nodes/cells on the boundary, global node IDs and
//         the local cell IDs w.r.t. the local grid. </li>
//    <li> Boundary grids are then exchanged among candidate neighbors using
//         point-to-point communication. </li>
//    <li> Next, each process constructs the topology of the ghost zones and
//         communication links, using the local boundary grid and the list of
//         remote boundary grids. </li>
//    <li> The communication links store a source/target pair for nodes/cells
//         among connected grids and remain persistent in memory. </li>
//    <li> Last, the fields (node- and/or cell-centered) are updated, using
//         point-to-point communication by processing the communication
//         links. </li>
//  </ol>
//
// .SECTION Caveats
//  <ul>
//    <li> The code currently assumes one grid per rank. </li>
//    <li> GlobalID information must be available. </li>
//    <li> The grid must be globally conforming, i.e., no hanging nodes. </li>
//    <li> Only topologically face-adjacent ghost cells are considered. </li>
//    <li> PointData and CellData must match across partitions/processes. </li>
//  </ul>
//
// .SECTION See Also
//  vtkPUnstructuredGridGhostDataGenerator

#ifndef VTKPUNSTRUCTUREDGRIDCONNECTIVITY_H_
#define VTKPUNSTRUCTUREDGRIDCONNECTIVITY_H_

#include "vtkFiltersParallelGeometryModule.h" // For export macro
#include "vtkObject.h"

// Forward Declarations
class vtkCell;
class vtkCellData;
class vtkIdList;
class vtkIdTypeArray;
class vtkMPIController;
class vtkMultiProcessStream;
class vtkPointData;
class vtkPoints;
class vtkUnstructuredGrid;

// Forward Declaration of internal data-structures
namespace vtk
{
namespace details
{

struct GridInfo;
struct MeshLinks;
struct CommunicationLinks;

} // END namespace details
} // END namespace vkt

class VTKFILTERSPARALLELGEOMETRY_EXPORT vtkPUnstructuredGridConnectivity :
  public vtkObject
{
public:
  static vtkPUnstructuredGridConnectivity* New();
  vtkTypeMacro(vtkPUnstructuredGridConnectivity,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the underlying MPI controller used for communication.
  vtkSetMacro(Controller,vtkMPIController*);
  vtkGetMacro(Controller,vtkMPIController*);

  // Description:
  // Set/Get the name of the GlobalID field. By default, "GlobalID" is assumed.
  vtkSetStringMacro(GlobalIDFieldName);
  vtkGetStringMacro(GlobalIDFieldName);

  // Description:
  // Returns the ghosted grid.
  vtkGetMacro(GhostedGrid,vtkUnstructuredGrid*);

  // Description:
  // Registers the grid in this process
  void RegisterGrid(vtkUnstructuredGrid* gridPtr);

  // Description:
  // Builds the ghost-zone connectivity. This method sets up the necessary
  // communication lists for updating the ghost zones.
  // NOTE: the local grid, must be registered, by calling RegisterGrid(),
  // prior to calling this method.
  void BuildGhostZoneConnectivity();

  // Description:
  // Exchanges ghost zone data (i.e., node-centered or cell-centered fields).
  // NOTE: This method must be called after BuildGhostZoneConnectivity.
  void UpdateGhosts();

protected:
  vtkPUnstructuredGridConnectivity();
  virtual ~vtkPUnstructuredGridConnectivity();

  char* GlobalIDFieldName;            // The field of the global IDs.
  vtkUnstructuredGrid* InputGrid;     // The input grid, to be ghosted.
  vtkUnstructuredGrid* GhostedGrid;   // This is the output from this class.
  vtkMPIController* Controller;       // Supplied MPI controller.

// BTX
  vtk::details::GridInfo* AuxiliaryData; // Data used to build the ghost zones.
  vtk::details::CommunicationLinks* CommLists; // Persistent comm lists.
// ETX

  // Description:
  // Given the deserialized cell-centered ghost data from the given neighboring
  // rank, this method fills in the cell-centered fields of the ghost zone.
  void FillGhostZoneCells(
        const int neiRank,
        vtkCellData* ghostData,
        vtkIdType* cellIdx,
        const unsigned int numGhostCells);

  // Description:
  // Given the deserialized node-centered ghost data from the given neighboring
  // rank, this method fills in the node-centered fields of the ghost zone.
  void FillGhostZoneNodes(
        const int neiRank,
        vtkPointData* ghostData,
        vtkIdType* globalIdx,
        const unsigned int numGhostNodes);

  // Description:
  // Deserializes the raw buffers received from each neighboring rank and
  // updates the ghosted grid instance by filling in the values for the
  // ghost zones.
  void DeSerializeGhostZones();

  // Description:
  // This method exchanges the buffer sizes among neighboring processes and
  // allocates a persistent buffer for the communication. This exchange and
  // memory allocation happens only the first time the data is exchanged
  void CreatePersistentRcvBuffers();

  // Description:
  // This method serializes the local data (node-centered and/or cell-centered)
  // for each rank that this process/grid communicates with.
  void SerializeGhostZones();

  // Description:
  // Synchs the data on the input grid in this process to the ghosted grid
  // instance.
  void SynchLocalData();

  // Description:
  // Loops through the nodes of the ghost cell and the local adjacent cell
  // and determines what
  void EnqueueNodeLinks(
        const int rmtRank,
        const vtkIdType ghostCell,
        const vtkIdType adjCell,
        vtkIdList* shared);

  // Description:
  // Given the cell, c, this method checks if it is connected to the grid
  // assigned to this process. The method will return -1, if the cell is
  // not connected. If the cell is connected, adjCell will hold the cell
  // index of the cell w.r.t. the input grid that the ghost cell is
  // face-adjacent to as well as the shared global Ids of the face between
  // the ghost and the face-adjacent, boundary cell.
  bool IsCellConnected(
      vtkCell* c,vtkIdType* globalId, const vtkIdType N,
      vtkIdType& adjCell,
      vtkIdList* sharedIds);

  // Description:
  // Inserts the ghost cell nodes in to the ghosted instance of the grid.
  void InsertGhostCellNodes(
        vtkCell* ghostCell,
        vtkIdTypeArray* ghostGridGlobalIdx,
        vtkIdType* globalIdArray,
        vtkUnstructuredGrid* bGrid,
        vtkIdType* cellPts);

  // Description:
  // Process the remote boundary grid and injects cells in to the ghosted
  // grid if a match is found.
  void ProcessRemoteGrid(
      const int rmtRank,vtkUnstructuredGrid* bGrid);

  // Description:
  // Builds the ghosted grid and communication lists
  void BuildGhostedGridAndCommLists();

  // Description:
  // Serializes the unstructured grid into a bytestream.
  void SerializeUnstructuredGrid(
      vtkUnstructuredGrid* g, vtkMultiProcessStream& bytestream);

  // Description:
  // De-serializes the unstructured grid from the given bytestream.
  void DeSerializeUnstructuredGrid(
      vtkUnstructuredGrid* g, vtkMultiProcessStream& bytestream);

  // Description:
  // Writes the given unstructured grid to an ASCII file.
  // NOTE: Used for debugging.
  void WriteUnstructuredGrid(vtkUnstructuredGrid* grid, const char* fileName);

  // Description:
  // Loops through the auxiliary FaceList, constructed in MarkFaces, and
  // extracts the faces and nodes on the boundary.
  void ExtractSurfaceMesh();

  // Description:
  // Loops through the input grid cell faces and updates the auxiliary
  // data-structures to associates a count with each face.
  void MarkFaces();

  // Description:
  // Extracts the boundary cell from the input grid and inserts it in to
  // the boundary grid.
  void ExtractBoundaryCell(
      const vtkIdType cellIdx,
      const vtkIdType numCellNodes,
      vtkIdType* cellNodes,
      vtkPoints* nodes,
      vtkIdTypeArray* localIdx,
      vtkIdTypeArray* globaIdx
      );

  // Description:
  // Checks if the cell, composed by the supplied nodes, is on the boundary.
  // A cell is on the boundary iff any of its nodes touch the boundary.
  bool IsCellOnBoundary(vtkIdType* cellNodes, vtkIdType N);

  // Description:
  // Exchanged the boundary grids among candidate ranks.
  void ExchangeBoundaryGrids();

  // Description:
  // Exchange boundary grid sizes
  void ExchangeBoundaryGridSizes(int size);

  // Description:
  // Collides the bounds of this process with the bounding boxes of all
  // other processes. The processes whose bounding boxes intersect yield
  // the list of candidate ranks with which boundary grids will be exchanged.
  void BoundingBoxCollision();

  // Description:
  // Exchanges the grid bounds of this process with all other processes.
  // Upon completion, each process will have the global grid bounds of
  // every process.
  void ExchangeGridBounds();

  // Description:
  // Extracts the boundary grid geometry from the input grid.
  // Note: this method only extract the mesh and global/local ID information.
  void ExtractBoundaryGrid();

private:
  vtkPUnstructuredGridConnectivity(const vtkPUnstructuredGridConnectivity&); // Not implemented
  void operator=(const vtkPUnstructuredGridConnectivity&); // Not implemented
};

#endif /* VTKPUNSTRUCTUREDGRIDCONNECTIVITY_H_ */
