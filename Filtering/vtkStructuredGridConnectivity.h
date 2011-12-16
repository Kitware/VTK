/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredGridConnectivity.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkStructuredGridConnectivity.h -- Constructs structured connectivity
//
// .SECTION Description
//  vtkStructuredGridConnectivity is a concrete instance of vtkObject that
//  implements functionality for computing the neighboring topology within a
//  single partitioned structured grid dataset. This class implementation does
//  not have any support for distributed data. For the parallel implementation
//  see vtkPStructuredGridConnectivity.
//
// .SECTION See Also
//  vtkGhostArray vtkPStructuredGridConnectivity

#ifndef vtkStructuredGridConnectivity_H_
#define vtkStructuredGridConnectivity_H_

// VTK include directives
#include "vtkAbstractGridConnectivity.h"
#include "vtkStructuredNeighbor.h" // For Structured Neighbor object definition


// C++ include directives
#include <vector> // For STL vector

// Forward Declarations
class vtkIdList;
class vtkUnsignedCharArray;
class vtkPointData;
class vtkCellData;
class vtkPoints;

class VTK_FILTERING_EXPORT vtkStructuredGridConnectivity :
  public vtkAbstractGridConnectivity
{
  public:
    static vtkStructuredGridConnectivity* New();
    vtkTypeMacro( vtkStructuredGridConnectivity, vtkAbstractGridConnectivity );
    void PrintSelf(ostream& os, vtkIndent  indent );

    // Description:
    // Set/Get the whole extent of the grid
    vtkSetVector6Macro(WholeExtent,int);
    vtkGetVector6Macro(WholeExtent,int);

    // Description:
    // Set/Get the total number of domains distributed among processors
    virtual void SetNumberOfGrids( const unsigned int N )
    {
      this->NumberOfGrids = N;
      this->AllocateUserRegisterDataStructures();

      this->GridExtents.resize( 6*N,-1);
      this->Neighbors.resize( N );
      this->BlockTopology.resize( N );
    }

    // Description:
    // Registers the current grid corresponding to the grid ID by its global
    // extent w.r.t. the whole extent.
    virtual void RegisterGrid( const int gridID, int extents[6],
        vtkUnsignedCharArray* nodesGhostArray,
        vtkUnsignedCharArray* cellGhostArray,
        vtkPointData* pointData,
        vtkCellData* cellData,
        vtkPoints* gridNodes );

    // Description:
    // Returns the grid extent of the grid corresponding to the given grid ID.
    void GetGridExtent( const int gridID, int extent[6] );

    // Description:
    // Computes neighboring information
    virtual void ComputeNeighbors();

    // Description:
    // Returns the number of neighbors for the grid corresponding to the given
    // grid ID.
    int GetNumberOfNeighbors( const int gridID )
      { return( this->Neighbors[ gridID ].size() ); };

    // Description:
    // Returns the list of neighboring blocks for the given grid and the
    // corresponding overlapping extents are filled in the 1-D flat array
    // strided by 6.
    //
    // NOTE: the flat array extents must be pre-allocated.
    vtkIdList* GetNeighbors( const int gridID, int *extents );

    // Description:
    // Filles the mesh property arrays, nodes and cells, for the grid
    // corresponding to the given grid ID.
    // NOTE: this method assumes that ComputeNeighbors() has been called.
    void FillGhostArrays(
       const int gridID,
       vtkUnsignedCharArray *nodesArray,
       vtkUnsignedCharArray *cellsArray );

    // Description:
    // Creates ghost layers.
    virtual void CreateGhostLayers( const int N=1 );

  protected:
    vtkStructuredGridConnectivity();
    virtual ~vtkStructuredGridConnectivity();

    // Description:
    // Returns true iff Lo <= idx <= Hi, otherwise false.
    bool InBounds( const int idx, const int Lo, const int Hi )
    { return( (idx>=Lo) && (idx<=Hi) ); };

    // Description
    // Returns the cardinality of a range S.
    int Cardinality( int S[2] ) { return( S[1]-S[0]+1 ); };

    // Description:
    // Given a point (i,j,k) belonging to the grid corresponding to the given
    // gridID, this method searches for the grids that this point is neighboring
    // with.
    void SearchNeighbors(
        const int gridID,
        const int i, const int j, const int k,
        vtkIdList *neiList );

    // Description:
    // Marks the node properties with the node with the given global i,j,k
    // grid coordinates w.r.t. to the grid defined by the given extent ext.
    void MarkNodeProperty(
        const int gridID,
        const int i, const int j, const int k,
        int ext[6], unsigned char &pfield );

    // Description:
    // Given a grid extent, this method computes the RealExtent.
    void GetRealExtent( int GridExtent[6],int RealExtent[6] );

    // Description:
    // Checks if the node corresponding to the given global i,j,k coordinates
    // is a ghost node or not.
    bool IsGhostNode(
        const int gridID, int GridExtent[6],
        const int i, const int j, const int k );

    // Description:
    // Checks if the node corresponding to the given global i,j,k coordinates
    // is on the boundary of the given extent.
    bool IsNodeOnBoundaryOfExtent(
        const int i, const int j, const int k, int ext[6] );

    // Description:
    // Checks if the node corresponding to the given global i,j,k coordinates
    // is on the shared boundary, i.e., a partition interface.
    //
    // NOTE: A node on a shared boundary, may also be on a real boundary.
    bool IsNodeOnSharedBoundary(
        const int i, const int j, const int k,
        int GridExtent[6],
        int RealExtent[6] );

    // Description:
    // Checks if the node corresponding to the given global i,j,k coordinates
    // touches the real boundaries of the domain given the whole extent.
    bool IsNodeOnBoundary( const int i, const int j, const int k );

    // Description:
    // Checks if the node, corresponding to the given global i,j,k coordinates
    // is within the interior of the given global grid extent.
    bool IsNodeInterior(
        const int i, const int j, const int k,
        int GridExtent[6] );

    // Description:
    // Checks if the node corresponding to the given global i,j,k coordinates
    // is within the given extent, inclusive of the extent bounds.
    bool IsNodeWithinExtent(
        const int i, const int j, const int k,
        int Extent[6] );

    // Description:
    // Creates a neighbor from i-to-j and from j-to-i.
    void SetNeighbors(
        const int i, const int j,
        int i2jOrientation[3], int j2iOrientation[3],
        int overlapExtent[6] );

    // Description:
    // Given an overlapping extent A and the corresponding overlap extent with
    // its neighbor, this method computes A's relative neighboring orientation
    // w.r.t to its neighbor. For example, A's neighbor is
    void DetermineNeighborOrientation(
        const int  idx, int A[2], int overlap[2], int orient[3] );

    // Description:
    // Detects if the two extents, ex1 and ex2, corresponding to the grids
    // with grid IDs i,j respectively, are neighbors, i.e, they either share
    // a corner, an edge or a plane in 3-D.
    void DetectNeighbors(
        const int i, const int j, int ex1[6], int ex2[6],
        int orientation[3], int ndim);

    // Description:
    // Checks if the intervals A,B overlap. The intersection of A,B is returned
    // in the overlap array and a return code is used to indicate the type of
    // overlap. The return values are defined as follows:
    // NO_OVERLAP      0
    // NODE_OVERLAP    1
    // EDGE_OVERLAP    2
    // PARTIAL_OVERLAP 3
    int IntervalOverlap( int A[2], int B[2], int overlap[2] );

    // Description:
    // Checks if the internals s,S partially overlap where |s| < |S|.
    // The intersection of s,S is stored in the supplied overlap array and a
    // return code is used to indicate the type of overlap. The return values
    // are defined as follows:
    // NO_OVERLAP      0
    // NODE_OVERLAP    1
    // PARTIAL_OVERLAP 3
    int DoPartialOverlap( int s[2], int S[2], int overlap[2] );

    // Description:
    // Checks if the intervals A,B partially overlap. The region of partial
    // overlap is returned in the provided overlap array and a return code is
    // used to indicate whether there is partial overlap or not. The return
    // values are defined as follows:
    // NO_OVERLAP      0
    // NODE_OVERLAP    1
    // PARTIAL_OVERLAP 3
    int PartialOverlap(
        int A[2], const int CofA,
        int B[2], const int CofB,
        int overlap[2] );

    // Description:
    // Establishes the neighboring information between the two grids
    // corresponding to grid ids "i" and "j" with i < j.
    void EstablishNeighbors( const int i, const int j );

    // Description:
    // Based on the user-supplied WholeExtent, this method determines the
    // topology of the structured domain, e.g., VTK_XYZ_GRID, VTK_XY_PLANE, etc.
    void AcquireDataDescription();

    // Description:
    // Checks if the block corresponding to the given grid ID has a block
    // adjacent to it in the given block direction.
    // NOTE: The block direction is essentially one of the 6 faces  of the
    // block defined as follows:
    // <ul>
    //  <li>IMIN=0</li>
    //  <li>IMAX=1</li>
    //  <li>JMIN=2</li>
    //  <li>JMAX=3</li>
    //  <li>KMIN=4</li>
    //  <li>KMAX=5</li>
    // </ul>
    bool HasBlockConnection( const int gridID, const int blockDirection );

    // Description:
    // Removes a block connection along the given direction for the block
    // corresponding to the given gridID.
    // NOTE: The block direction is essentially one of the 6 faces  of the
    // block defined as follows:
    // <ul>
    //  <li>IMIN=0</li>
    //  <li>IMAX=1</li>
    //  <li>JMIN=2</li>
    //  <li>JMAX=3</li>
    //  <li>KMIN=4</li>
    //  <li>KMAX=5</li>
    // </ul>
    void RemoveBlockConnection( const int gridID, const int blockDirection );

    // Description:
    // Adds a block connection along the given direction for the block
    // corresponding to the given gridID.
    // NOTE: The block direction is essentially one of the 6 faces  of the
    // block defined as follows:
    // <ul>
    //  <li>IMIN=0</li>
    //  <li>IMAX=1</li>
    //  <li>JMIN=2</li>
    //  <li>JMAX=3</li>
    //  <li>KMIN=4</li>
    //  <li>KMAX=5</li>
    // </ul>
    void AddBlockConnection( const int gridID, const int blockDirection );

    // Description:
    // Clears all block connections for the  block corresponding to the given
    // grid ID.
    void ClearBlockConnections( const int gridID );

    // Description:
    // Prints the extent, used for debugging
    void PrintExtent( int extent[6] );

    int DataDescription;
    int WholeExtent[6];
    std::vector< int > GridExtents;
    std::vector< unsigned char  > BlockTopology;
    std::vector< std::vector<vtkStructuredNeighbor> > Neighbors;

  private:
    vtkStructuredGridConnectivity( const vtkStructuredGridConnectivity& ); // Not implemented
    void operator=(const vtkStructuredGridConnectivity& ); // Not implemented
};

#endif /* vtkStructuredGridConnectivity_H_ */
