// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredGridConnectivity
 *
 *
 *  vtkStructuredGridConnectivity is a concrete instance of vtkObject that
 *  implements functionality for computing the neighboring topology within a
 *  single partitioned structured grid dataset. This class implementation does
 *  not have any support for distributed data. For the parallel implementation
 *  see vtkPStructuredGridConnectivity.
 *
 * @sa
 *  vtkPStructuredGridConnectivity
 */

#ifndef vtkStructuredGridConnectivity_h
#define vtkStructuredGridConnectivity_h

#define VTK_NO_OVERLAP 0
#define VTK_NODE_OVERLAP 1
#define VTK_EDGE_OVERLAP 2
#define VTK_PARTIAL_OVERLAP 3

// VTK include directives
#include "vtkAbstractGridConnectivity.h"
#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkStructuredData.h"        // For data description definitions
#include "vtkStructuredNeighbor.h"    // For Structured Neighbor object definition

// C++ include directives
#include <cassert>  // For assert()
#include <iostream> // For cout
#include <map>      // For STL map
#include <utility>  // For STL pair and overloaded relational operators
#include <vector>   // For STL vector

// Forward Declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkUnsignedCharArray;
class vtkPointData;
class vtkCellData;
class vtkPoints;

class VTKFILTERSGEOMETRY_EXPORT vtkStructuredGridConnectivity : public vtkAbstractGridConnectivity
{
public:
  static vtkStructuredGridConnectivity* New();
  vtkTypeMacro(vtkStructuredGridConnectivity, vtkAbstractGridConnectivity);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the whole extent of the grid
   */
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  ///@}

  ///@{
  /**
   * Returns the data dimension based on the whole extent
   */
  vtkGetMacro(DataDimension, int);
  ///@}

  /**
   * Set/Get the total number of domains distributed among processors
   */
  void SetNumberOfGrids(unsigned int N) override;

  /**
   * Registers the current grid corresponding to the grid ID by its global
   * extent w.r.t. the whole extent.
   */
  virtual void RegisterGrid(int gridID, int extents[6], vtkUnsignedCharArray* nodesGhostArray,
    vtkUnsignedCharArray* cellGhostArray, vtkPointData* pointData, vtkCellData* cellData,
    vtkPoints* gridNodes);

  /**
   * Returns the grid extent of the grid corresponding to the given grid ID.
   */
  void GetGridExtent(int gridID, int extent[6]);

  /**
   * Sets the ghosted grid extent for the grid corresponding to the given
   * grid ID to the given extent.
   */
  void SetGhostedGridExtent(int gridID, int ext[6]);

  /**
   * Returns the ghosted grid extent for the block corresponding the
   */
  void GetGhostedGridExtent(int gridID, int ext[6]);

  /**
   * Computes neighboring information
   */
  void ComputeNeighbors() override;

  /**
   * Returns the number of neighbors for the grid corresponding to the given
   * grid ID.
   */
  int GetNumberOfNeighbors(int gridID)
  {
    return (static_cast<int>(this->Neighbors[gridID].size()));
  }

  /**
   * Returns the neighbor corresponding to the index nei for the grid with the
   * given (global) grid ID.
   */
  vtkStructuredNeighbor GetGridNeighbor(int gridID, int nei);

  /**
   * Returns the list of neighboring blocks for the given grid and the
   * corresponding overlapping extents are filled in the 1-D flat array
   * strided by 6.

   * NOTE: the flat array extents must be pre-allocated.
   */
  vtkIdList* GetNeighbors(int gridID, int* extents);

  /**
   * Fills the mesh property arrays, nodes and cells, for the grid
   * corresponding to the given grid ID.
   * NOTE: this method assumes that ComputeNeighbors() has been called.
   */
  void FillGhostArrays(
    int gridID, vtkUnsignedCharArray* nodesArray, vtkUnsignedCharArray* cellsArray) override;

  /**
   * Creates ghost layers.
   */
  void CreateGhostLayers(int N = 1) override;

protected:
  vtkStructuredGridConnectivity();
  ~vtkStructuredGridConnectivity() override;

  /**
   * Returns true iff Lo <= idx <= Hi, otherwise false.
   */
  bool InBounds(int idx, int Lo, int Hi) { return ((idx >= Lo) && (idx <= Hi)); }

  /**
   * Returns true iff Lo < idx < Hi, otherwise false.
   */
  bool StrictlyInsideBounds(int idx, int Lo, int Hi) { return ((idx > Lo) && (idx < Hi)); }

  /**
   * Returns true iff A is a subset of B, otherwise false.
   */
  bool IsSubset(int A[2], int B[2])
  {
    return (this->InBounds(A[0], B[0], B[1]) && this->InBounds(A[1], B[0], B[1]));
  }

  /**
   * Returns the cardinality of a range S.
   */
  int Cardinality(int S[2]) { return (S[1] - S[0] + 1); }

  ///@{
  /**
   * Returns the number of nodes per cell according to the given dimension.
   */
  int GetNumberOfNodesPerCell(int dim)
  {
    int numNodes = 0;
    switch (dim)
    {
      case 1:
        numNodes = 2; // line cell
        break;
      case 2:
        numNodes = 4; // quad cell
        break;
      case 3:
        numNodes = 8; // hex cell
        break;
      default:
        assert("ERROR: code should not reach here!" && false);
    } // END switch
    return (numNodes);
  }
  ///@}

  /**
   * Fills the ghost array for the nodes
   */
  void FillNodesGhostArray(int gridID, int dataDescription, int GridExtent[6], int RealExtent[6],
    vtkUnsignedCharArray* nodesArray);

  /**
   * Fills the ghost array for the grid cells
   */
  void FillCellsGhostArray(int dataDescription, int numNodesPerCell, int dims[3], int CellExtent[6],
    vtkUnsignedCharArray* nodesArray, vtkUnsignedCharArray* cellsArray);

  /**
   * Given a point (i,j,k) belonging to the grid corresponding to the given
   * gridID, this method searches for the grids that this point is neighboring
   * with.
   */
  void SearchNeighbors(int gridID, int i, int j, int k, vtkIdList* neiList);

  /**
   * Marks the node properties with the node with the given global i,j,k
   * grid coordinates w.r.t. to the grid defined by the given extent ext.
   */
  void MarkNodeProperty(
    int gridID, int i, int j, int k, int ext[6], int RealExtent[6], unsigned char& pfield);

  /**
   * Marks the cell property for the cell composed by the nodes with the
   * given ghost fields.
   */
  void MarkCellProperty(unsigned char& pfield, unsigned char* nodeGhostFields, int numNodes);

  /**
   * Given a grid extent, this method computes the RealExtent.
   */
  void GetRealExtent(int gridID, int GridExtent[6], int RealExtent[6]);

  /**
   * Checks if the node corresponding to the given global i,j,k coordinates
   * is a ghost node or not.
   */
  bool IsGhostNode(int GridExtent[6], int RealExtent[6], int i, int j, int k);

  /**
   * Checks if the node corresponding to the given global i,j,k coordinates
   * is on the boundary of the given extent.
   */
  bool IsNodeOnBoundaryOfExtent(int i, int j, int k, int ext[6]);

  /**
   * Checks if the node corresponding to the given global i,j,k coordinates
   * is on the shared boundary, i.e., a partition interface.
   * NOTE: A node on a shared boundary, may also be on a real boundary.
   */
  bool IsNodeOnSharedBoundary(int gridID, int RealExtent[6], int i, int j, int k);

  /**
   * Checks if the node corresponding to the given global i,j,k coordinates
   * touches the real boundaries of the domain given the whole extent.
   */
  bool IsNodeOnBoundary(int i, int j, int k);

  /**
   * Checks if the node, corresponding to the given global i,j,k coordinates
   * is within the interior of the given global grid extent.
   */
  bool IsNodeInterior(int i, int j, int k, int GridExtent[6]);

  /**
   * Checks if the node corresponding to the given global i,j,k coordinates
   * is within the given extent, inclusive of the extent bounds.
   */
  bool IsNodeWithinExtent(int i, int j, int k, int GridExtent[6])
  {
    bool status = false;

    switch (this->DataDescription)
    {
      case VTK_X_LINE:
        if ((GridExtent[0] <= i) && (i <= GridExtent[1]))
        {
          status = true;
        }
        break;
      case VTK_Y_LINE:
        if ((GridExtent[2] <= j) && (j <= GridExtent[3]))
        {
          status = true;
        }
        break;
      case VTK_Z_LINE:
        if ((GridExtent[4] <= k) && (k <= GridExtent[5]))
        {
          status = true;
        }
        break;
      case VTK_XY_PLANE:
        if ((GridExtent[0] <= i) && (i <= GridExtent[1]) && (GridExtent[2] <= j) &&
          (j <= GridExtent[3]))
        {
          status = true;
        }
        break;
      case VTK_YZ_PLANE:
        if ((GridExtent[2] <= j) && (j <= GridExtent[3]) && (GridExtent[4] <= k) &&
          (k <= GridExtent[5]))
        {
          status = true;
        }
        break;
      case VTK_XZ_PLANE:
        if ((GridExtent[0] <= i) && (i <= GridExtent[1]) && (GridExtent[4] <= k) &&
          (k <= GridExtent[5]))
        {
          status = true;
        }
        break;
      case VTK_XYZ_GRID:
        if ((GridExtent[0] <= i) && (i <= GridExtent[1]) && (GridExtent[2] <= j) &&
          (j <= GridExtent[3]) && (GridExtent[4] <= k) && (k <= GridExtent[5]))
        {
          status = true;
        }
        break;
      default:
        std::cout << "Data description is: " << this->DataDescription << "\n";
        std::cout.flush();
        assert("pre: Undefined data-description!" && false);
    } // END switch

    return (status);
  }

  /**
   * Creates a neighbor from i-to-j and from j-to-i.
   */
  void SetNeighbors(
    int i, int j, int i2jOrientation[3], int j2iOrientation[3], int overlapExtent[6]);

  /**
   * Given two overlapping extents A,B and the corresponding overlap extent
   * this method computes A's relative neighboring orientation
   * w.r.t to its neighbor, B. The resulting orientation is stored in an
   * integer 3-tuple that holds the orientation of A relative to B alone each
   * axis, i, j, k. See vtkStructuredNeighbor::NeighborOrientation for a list
   * of valid orientation values.
   */
  void DetermineNeighborOrientation(int idx, int A[2], int B[2], int overlap[2], int orient[3]);

  /**
   * Detects if the two extents, ex1 and ex2, corresponding to the grids
   * with grid IDs i,j respectively, are neighbors, i.e, they either share
   * a corner, an edge or a plane in 3-D.
   */
  void DetectNeighbors(int i, int j, int ex1[6], int ex2[6], int orientation[3], int ndim);

  /**
   * Checks if the intervals A,B overlap. The intersection of A,B is returned
   * in the overlap array and a return code is used to indicate the type of
   * overlap. The return values are defined as follows:
   * VTK_NO_OVERLAP      0
   * VTK_NODE_OVERLAP    1
   * VTK_EDGE_OVERLAP    2
   * VTK_PARTIAL_OVERLAP 3
   */
  int IntervalOverlap(int A[2], int B[2], int overlap[2]);

  /**
   * Checks if the internals s,S partially overlap where |s| < |S|.
   * The intersection of s,S is stored in the supplied overlap array and a
   * return code is used to indicate the type of overlap. The return values
   * are defined as follows:
   * VTK_NO_OVERLAP      0
   * VTK_NODE_OVERLAP    1
   * VTK_PARTIAL_OVERLAP 3
   */
  int DoPartialOverlap(int s[2], int S[2], int overlap[2]);

  /**
   * Checks if the intervals A,B partially overlap. The region of partial
   * overlap is returned in the provided overlap array and a return code is
   * used to indicate whether there is partial overlap or not. The return
   * values are defined as follows:
   * VTK_NO_OVERLAP      0
   * VTK_NODE_OVERLAP    1
   * VTK_PARTIAL_OVERLAP 3
   */
  int PartialOverlap(int A[2], int CardinalityOfA, int B[2], int CardinalityOfB, int overlap[2]);

  /**
   * Establishes the neighboring information between the two grids
   * corresponding to grid ids "i" and "j" with i < j.
   */
  void EstablishNeighbors(int i, int j);

  /**
   * Based on the user-supplied WholeExtent, this method determines the
   * topology of the structured domain, e.g., VTK_XYZ_GRID, VTK_XY_PLANE, etc.
   */
  void AcquireDataDescription();

  /**
   * Checks if the block corresponding to the given grid ID has a block
   * adjacent to it in the given block direction.
   * NOTE: The block direction is essentially one of the 6 faces of the
   * block defined as follows:
   * <ul>
   * <li> FRONT  = 0 (+k direction) </li>
   * <li> BACK   = 1 (-k direction) </li>
   * <li> RIGHT  = 2 (+i direction) </li>
   * <li> LEFT   = 3 (-i direction) </li>
   * <li> TOP    = 4 (+j direction) </li>
   * <li> BOTTOM = 5 (-j direction) </li>
   * </ul>
   */
  bool HasBlockConnection(int gridID, int blockDirection);

  /**
   * Removes a block connection along the given direction for the block
   * corresponding to the given gridID.
   * NOTE: The block direction is essentially one of the 6 faces of the
   * block defined as follows:
   * <ul>
   * <li> FRONT  = 0 (+k direction) </li>
   * <li> BACK   = 1 (-k direction) </li>
   * <li> RIGHT  = 2 (+i direction) </li>
   * <li> LEFT   = 3 (-i direction) </li>
   * <li> TOP    = 4 (+j direction) </li>
   * <li> BOTTOM = 5 (-j direction) </li>
   * </ul>
   */
  void RemoveBlockConnection(int gridID, int blockDirection);

  /**
   * Adds a block connection along the given direction for the block
   * corresponding to the given gridID.
   * NOTE: The block direction is essentially one of the 6 faces of the
   * block defined as follows:
   * <ul>
   * <li> FRONT  = 0 (+k direction) </li>
   * <li> BACK   = 1 (-k direction) </li>
   * <li> RIGHT  = 2 (+i direction) </li>
   * <li> LEFT   = 3 (-i direction) </li>
   * <li> TOP    = 4 (+j direction) </li>
   * <li> BOTTOM = 5 (-j direction) </li>
   * </ul>
   */
  void AddBlockConnection(int gridID, int blockDirection);

  /**
   * Clears all block connections for the block corresponding to the given
   * grid ID.
   */
  void ClearBlockConnections(int gridID);

  /**
   * Returns the number of faces of the block corresponding to the given grid
   * ID that are adjacent to at least one other block. Note, this is not the
   * total number of neighbors for the block. This method simply checks how
   * many out of the 6 block faces have connections. Thus, the return value
   * has an upper-bound of 6.
   */
  int GetNumberOfConnectingBlockFaces(int gridID);

  /**
   * Sets the block topology connections for the grid corresponding to gridID.
   */
  void SetBlockTopology(int gridID);

  /**
   * Given i-j-k coordinates and the grid defined by tis extent, ext, this
   * method determines IJK orientation with respect to the block boundaries,
   * i.e., the 6 block faces. If the node is not on a boundary, then
   * orientation[i] = BlockFace::NOT_ON_BLOCK_FACE for all i in [0,2].
   */
  void GetIJKBlockOrientation(int i, int j, int k, int ext[6], int orientation[3]);

  /**
   * A helper method that computes the 1-D i-j-k orientation to facilitate the
   * implementation of GetNodeBlockOrientation.
   */
  int Get1DOrientation(int idx, int ExtentLo, int ExtentHi, int OnLo, int OnHi, int NotOnBoundary);

  /**
   * Creates the ghosted extent of the grid corresponding to the given
   * gridID.
   */
  void CreateGhostedExtent(int gridID, int N);

  /**
   * Gets the ghosted extent from the given grid extent along the dimension
   * given by minIdx and maxIdx. This method is a helper method for the
   * implementation of CreateGhostedExtent.
   */
  void GetGhostedExtent(int* ghostedExtent, int GridExtent[6], int minIdx, int maxIdx, int N);

  /**
   * This method creates the ghosted mask arrays, i.e., the NodeGhostArrays
   * and the CellGhostArrays for the grid corresponding to the given gridID.
   */
  void CreateGhostedMaskArrays(int gridID);

  /**
   * This method initializes the ghost data according to the computed ghosted
   * grid extent for the grid with the given grid ID. Specifically, PointData,
   * CellData and grid coordinates are allocated for the ghosted grid
   * accordingly.
   */
  void InitializeGhostData(int gridID);

  /**
   * Adds/creates all the arrays in the reference grid point data, RPD, to
   * the user-supplied point data instance, PD, where the number of points
   * is given by N.
   */
  void AllocatePointData(vtkPointData* RPD, int N, vtkPointData* PD);

  /**
   * Adds/creates all the arrays in the reference grid cell data, RCD, to the
   * user-supplied cell data instance, CD, where the number of cells is given
   * by N.
   */
  void AllocateCellData(vtkCellData* RCD, int N, vtkCellData* CD);

  /**
   * This method transfers the registered grid data to the corresponding
   * ghosted grid data.
   */
  void TransferRegisteredDataToGhostedData(int gridID);

  /**
   * This method computes, the send and rcv extents for each neighbor of
   * each grid.
   */
  void ComputeNeighborSendAndRcvExtent(int gridID, int N);

  /**
   * This method transfers the fields (point data and cell data) to the
   * ghost extents from the neighboring grids of the grid corresponding
   * to the given gridID.
   */
  virtual void TransferGhostDataFromNeighbors(int gridID);

  /**
   * This method transfers the fields
   */
  void TransferLocalNeighborData(int gridID, const vtkStructuredNeighbor& Neighbor);

  /**
   * Copies the coordinates from the source points to the target points.
   */
  void CopyCoordinates(
    vtkPoints* source, vtkIdType sourceIdx, vtkPoints* target, vtkIdType targetIdx);

  /**
   * Loops through all arrays in the source and for each array, it copies the
   * tuples from sourceIdx to the target at targetIdx. This method assumes
   * that the source and target have a one-to-one array correspondence, that
   * is array i in the source corresponds to array i in the target.
   */
  void CopyFieldData(
    vtkFieldData* source, vtkIdType sourceIdx, vtkFieldData* target, vtkIdType targetIdx);

  /**
   * Given a global grid ID and the neighbor grid ID, this method returns the
   * neighbor index w.r.t. the Neighbors list of the grid with grid ID
   * gridIdx.
   */
  int GetNeighborIndex(int gridIdx, int NeighborGridIdx);

  /**
   * Prints the extent, used for debugging
   */
  void PrintExtent(int extent[6]);

  int DataDimension;
  int DataDescription;
  int WholeExtent[6];

  std::vector<int> GridExtents;
  std::vector<int> GhostedExtents;
  std::vector<unsigned char> BlockTopology;
  std::vector<std::vector<vtkStructuredNeighbor>> Neighbors;
  std::map<std::pair<int, int>, int> NeighborPair2NeighborListIndex;

private:
  vtkStructuredGridConnectivity(const vtkStructuredGridConnectivity&) = delete;
  void operator=(const vtkStructuredGridConnectivity&) = delete;
};

//=============================================================================
//  INLINE METHODS
//=============================================================================

//------------------------------------------------------------------------------
inline int vtkStructuredGridConnectivity::GetNeighborIndex(int gridIdx, int NeighborGridIdx)
{
  assert("pre: Grid index is out-of-bounds!" && (gridIdx >= 0) &&
    (gridIdx < static_cast<int>(this->NumberOfGrids)));
  assert("pre: Neighbor grid index is out-of-bounds!" && (NeighborGridIdx >= 0) &&
    (NeighborGridIdx < static_cast<int>(this->NumberOfGrids)));

  std::pair<int, int> gridPair = std::make_pair(gridIdx, NeighborGridIdx);
  assert("pre: Neighboring grid pair does not exist in hash!" &&
    (this->NeighborPair2NeighborListIndex.find(gridPair) !=
      this->NeighborPair2NeighborListIndex.end()));

  return (this->NeighborPair2NeighborListIndex[gridPair]);
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::GetGhostedExtent(
  int* ghostedExtent, int GridExtent[6], int minIdx, int maxIdx, int N)
{
  assert("pre: Number of ghost layers must be N >= 1" && (N >= 1));
  assert("pre: ghosted extent pointer is nullptr" && ghostedExtent != nullptr);

  ghostedExtent[minIdx] = GridExtent[minIdx] - N;
  ghostedExtent[maxIdx] = GridExtent[maxIdx] + N;

  // Clamp the ghosted extent to be within the WholeExtent
  ghostedExtent[minIdx] = (ghostedExtent[minIdx] < this->WholeExtent[minIdx])
    ? this->WholeExtent[minIdx]
    : ghostedExtent[minIdx];
  ghostedExtent[maxIdx] = (ghostedExtent[maxIdx] > this->WholeExtent[maxIdx])
    ? this->WholeExtent[maxIdx]
    : ghostedExtent[maxIdx];
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::SetGhostedGridExtent(int gridID, int ext[6])
{
  assert("pre: gridID is out-of-bounds" && (gridID >= 0) &&
    (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: ghosted-extents vector has not been allocated" &&
    (this->NumberOfGrids == this->GhostedExtents.size() / 6));

  for (int i = 0; i < 6; ++i)
  {
    this->GhostedExtents[gridID * 6 + i] = ext[i];
  }
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::GetGridExtent(int gridID, int ext[6])
{
  assert("pre: gridID out-of-bounds!" &&
    (gridID >= 0 && gridID < static_cast<int>(this->NumberOfGrids)));
  for (int i = 0; i < 6; ++i)
  {
    ext[i] = this->GridExtents[gridID * 6 + i];
  }
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::GetGhostedGridExtent(int gridID, int ext[6])
{
  assert("pre: gridID out-of-bounds!" &&
    (gridID >= 0 && gridID < static_cast<int>(this->NumberOfGrids)));

  if (this->GhostedExtents.empty())
  {
    ext[0] = ext[2] = ext[4] = -1;
    ext[1] = ext[3] = ext[5] = 0;
    vtkErrorMacro("No ghosted extents found for registered grid extends!!!");
    return;
  }

  assert("GhostedExtents are not aligned with registered grid extents" &&
    (this->GhostedExtents.size() == this->GridExtents.size()));
  for (int i = 0; i < 6; ++i)
  {
    ext[i] = this->GhostedExtents[gridID * 6 + i];
  }
}

//------------------------------------------------------------------------------
inline bool vtkStructuredGridConnectivity::IsNodeOnBoundaryOfExtent(int i, int j, int k, int ext[6])
{
  if (!this->IsNodeWithinExtent(i, j, k, ext))
  {
    return false;
  }

  bool status = false;
  switch (this->DataDescription)
  {
    case VTK_X_LINE:
      if (i == ext[0] || i == ext[1])
      {
        status = true;
      }
      break;
    case VTK_Y_LINE:
      if (j == ext[2] || j == ext[3])
      {
        status = true;
      }
      break;
    case VTK_Z_LINE:
      if (k == ext[4] || k == ext[5])
      {
        status = true;
      }
      break;
    case VTK_XY_PLANE:
      if ((i == ext[0] || i == ext[1]) || (j == ext[2] || j == ext[3]))
      {
        status = true;
      }
      break;
    case VTK_YZ_PLANE:
      if ((j == ext[2] || j == ext[3]) || (k == ext[4] || k == ext[5]))
      {
        status = true;
      }
      break;
    case VTK_XZ_PLANE:
      if ((i == ext[0] || i == ext[1]) || (k == ext[4] || k == ext[5]))
      {
        status = true;
      }
      break;
    case VTK_XYZ_GRID:
      if ((i == ext[0] || i == ext[1]) || (j == ext[2] || j == ext[3]) ||
        (k == ext[4] || k == ext[5]))
      {
        status = true;
      }
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert("pre: Undefined data-description!" && false);
  } // END switch

  return (status);
}

//------------------------------------------------------------------------------
inline bool vtkStructuredGridConnectivity::IsNodeInterior(int i, int j, int k, int GridExtent[6])
{
  bool status = false;

  switch (this->DataDescription)
  {
    case VTK_X_LINE:
      if ((GridExtent[0] < i) && (i < GridExtent[1]))
      {
        status = true;
      }
      break;
    case VTK_Y_LINE:
      if ((GridExtent[2] < j) && (j < GridExtent[3]))
      {
        status = true;
      }
      break;
    case VTK_Z_LINE:
      if ((GridExtent[4] < k) && (k < GridExtent[5]))
      {
        status = true;
      }
      break;
    case VTK_XY_PLANE:
      if ((GridExtent[0] < i) && (i < GridExtent[1]) && (GridExtent[2] < j) && (j < GridExtent[3]))
      {
        status = true;
      }
      break;
    case VTK_YZ_PLANE:
      if ((GridExtent[2] < j) && (j < GridExtent[3]) && (GridExtent[4] < k) && (k < GridExtent[5]))
      {
        status = true;
      }
      break;
    case VTK_XZ_PLANE:
      if ((GridExtent[0] < i) && (i < GridExtent[1]) && (GridExtent[4] < k) && (k < GridExtent[5]))
      {
        status = true;
      }
      break;
    case VTK_XYZ_GRID:
      if ((GridExtent[0] < i) && (i < GridExtent[1]) && (GridExtent[2] < j) &&
        (j < GridExtent[3]) && (GridExtent[4] < k) && (k < GridExtent[5]))
      {
        status = true;
      }
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert("pre: Undefined data-description!" && false);
  } // END switch

  return (status);
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::DetermineNeighborOrientation(
  int idx, int A[2], int B[2], int overlap[2], int orient[3])
{
  assert("pre: idx is out-of-bounds" && (idx >= 0) && (idx < 3));

  // A. Non-overlapping cases
  if (overlap[0] == overlap[1])
  {
    if (A[1] == B[0])
    {
      orient[idx] = vtkStructuredNeighbor::HI;
    }
    else if (A[0] == B[1])
    {
      orient[idx] = vtkStructuredNeighbor::LO;
    }
    else
    {
      orient[idx] = vtkStructuredNeighbor::UNDEFINED;
      assert("ERROR: Code should not reach here!" && false);
    }
  } // END non-overlapping cases
  // B. Sub-set cases
  else if (this->IsSubset(A, B))
  {
    if ((A[0] == B[0]) && (A[1] == B[1]))
    {
      orient[idx] = vtkStructuredNeighbor::ONE_TO_ONE;
    }
    else if (this->StrictlyInsideBounds(A[0], B[0], B[1]) &&
      this->StrictlyInsideBounds(A[1], B[0], B[1]))
    {
      orient[idx] = vtkStructuredNeighbor::SUBSET_BOTH;
    }
    else if (A[0] == B[0])
    {
      orient[idx] = vtkStructuredNeighbor::SUBSET_HI;
    }
    else if (A[1] == B[1])
    {
      orient[idx] = vtkStructuredNeighbor::SUBSET_LO;
    }
    else
    {
      orient[idx] = vtkStructuredNeighbor::UNDEFINED;
      assert("ERROR: Code should not reach here!" && false);
    }
  }
  // C. Super-set cases
  else if (this->IsSubset(B, A))
  {
    orient[idx] = vtkStructuredNeighbor::SUPERSET;
  }
  // D. Partially-overlapping (non-subset) cases
  else if (!(this->IsSubset(A, B) || this->IsSubset(A, B)))
  {
    if (this->InBounds(A[0], B[0], B[1]))
    {
      orient[idx] = vtkStructuredNeighbor::LO;
    }
    else if (this->InBounds(A[1], B[0], B[1]))
    {
      orient[idx] = vtkStructuredNeighbor::HI;
    }
    else
    {
      orient[idx] = vtkStructuredNeighbor::UNDEFINED;
      assert("ERROR: Code should not reach here!" && false);
    }
  }
  else
  {
    orient[idx] = vtkStructuredNeighbor::UNDEFINED;
    assert("ERROR: Code should not reach here!" && false);
  }
}

//------------------------------------------------------------------------------
inline int vtkStructuredGridConnectivity::Get1DOrientation(
  int idx, int ExtentLo, int ExtentHi, int OnLo, int OnHi, int NotOnBoundary)
{
  if (idx == ExtentLo)
  {
    return OnLo;
  }
  else if (idx == ExtentHi)
  {
    return OnHi;
  }
  return NotOnBoundary;
}

//------------------------------------------------------------------------------
inline bool vtkStructuredGridConnectivity::HasBlockConnection(int gridID, int blockDirection)
{
  // Sanity check
  assert("pre: gridID is out-of-bounds" && (gridID >= 0) &&
    (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: BlockTopology has not been properly allocated" &&
    (this->NumberOfGrids == this->BlockTopology.size()));
  assert("pre: blockDirection is out-of-bounds" && (blockDirection >= 0) && (blockDirection < 6));
  bool status = false;
  if (this->BlockTopology[gridID] & (1 << blockDirection))
  {
    status = true;
  }
  return (status);
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::RemoveBlockConnection(int gridID, int blockDirection)
{
  // Sanity check
  assert("pre: gridID is out-of-bounds" && (gridID >= 0) &&
    (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: BlockTopology has not been properly allocated" &&
    (this->NumberOfGrids == this->BlockTopology.size()));
  assert("pre: blockDirection is out-of-bounds" && (blockDirection >= 0) && (blockDirection < 6));

  this->BlockTopology[gridID] &= ~(1 << blockDirection);
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::AddBlockConnection(int gridID, int blockDirection)
{
  // Sanity check
  assert("pre: gridID is out-of-bounds" && (gridID >= 0) &&
    (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: BlockTopology has not been properly allocated" &&
    (this->NumberOfGrids == this->BlockTopology.size()));
  assert("pre: blockDirection is out-of-bounds" && (blockDirection >= 0) && (blockDirection < 6));
  this->BlockTopology[gridID] |= (1 << blockDirection);
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::ClearBlockConnections(int gridID)
{
  // Sanity check
  assert("pre: gridID is out-of-bounds" && (gridID >= 0) &&
    (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: BlockTopology has not been properly allocated" &&
    (this->NumberOfGrids == this->BlockTopology.size()));
  for (int i = 0; i < 6; ++i)
  {
    this->RemoveBlockConnection(gridID, i);
  } // END for all block directions
}

//------------------------------------------------------------------------------
inline int vtkStructuredGridConnectivity::GetNumberOfConnectingBlockFaces(int gridID)
{
  // Sanity check
  assert("pre: gridID is out-of-bounds" && (gridID >= 0) &&
    (gridID < static_cast<int>(this->NumberOfGrids)));
  assert("pre: BlockTopology has not been properly allocated" &&
    (this->NumberOfGrids == this->BlockTopology.size()));

  int count = 0;
  for (int i = 0; i < 6; ++i)
  {
    if (this->HasBlockConnection(gridID, i))
    {
      ++count;
    }
  }
  assert("post: count must be in [0,5]" && (count >= 0 && count <= 6));
  return (count);
}

//------------------------------------------------------------------------------
inline void vtkStructuredGridConnectivity::SetNumberOfGrids(unsigned int N)
{
  if (N == 0)
  {
    vtkErrorMacro("Number of grids cannot be 0.");
    return;
  }

  this->NumberOfGrids = N;
  this->AllocateUserRegisterDataStructures();

  this->GridExtents.resize(6 * N, -1);
  this->Neighbors.resize(N);
  this->BlockTopology.resize(N);
}
VTK_ABI_NAMESPACE_END
#endif /* vtkStructuredGridConnectivity_h */
