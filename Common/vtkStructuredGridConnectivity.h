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
//  not have any support for distributed data. For the parallel implmementation
//  see vtkPStructuredGridConnectivity.
//
// .SECTION See Also
//  vtkMeshPropertyEncoder vtkMeshProperty vtkPStructuredGridConnectivity

#ifndef vtkStructuredGridConnectivity_H_
#define vtkStructuredGridConnectivity_H_

// VTK include directives
#include "vtkObject.h" // Base class
#include "vtkStructuredNeighbor.h" // For Structured Neighbor object definition


// C++ include directives
#include <vector> // For STL vector

// Forward Declarations
class vtkIdList;


class VTK_COMMON_EXPORT vtkStructuredGridConnectivity : public vtkObject
{
  public:
    static vtkStructuredGridConnectivity* New();
    vtkTypeMacro( vtkStructuredGridConnectivity, vtkObject );
    void PrintSelf( std::ostream& os, vtkIndent  indent );

    // Description:
    // Set/Get the whole extent of the grid
    vtkSetVector6Macro(WholeExtent,int);
    vtkGetVector6Macro(WholeExtent,int);

    // Description:
    // Set/Get the total number of domains distributed among processors
    virtual void SetNumberOfGrids( const int N )
    {
      this->NumberOfGrids = N;
      this->GridExtents.resize( 6*N,-1);
      this->Neighbors.resize( N );
    }
    int GetNumberOfGrids() { return this->NumberOfGrids; };

    // Description:
    // Registers the current grid corresponding to the grid ID by its global
    // extent w.r.t. the whole extent.
    virtual void RegisterGrid( const int gridID, int extents[6] );

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
    void FillMeshPropertyArrays(
       const int gridID, unsigned char *nodesArray, unsigned char *cellsArray );

  protected:
    vtkStructuredGridConnectivity();
    virtual ~vtkStructuredGridConnectivity();

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
    void SetNeighbors( const int i, const int j, int overlapExtent[6] );

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
    // NO_OVERLAP   0
    // NODE_OVERLAP 1
    // EDGE_OVERLAP 2
    int IntervalOverlap( int A[2], int B[2], int overlap[2] );

    // Description:
    // Establishes the neighboring information between the two grids
    // corresponding to grid ids "i" and "j" with i < j.
    void EstablishNeighbors( const int i, const int j );

    // Description:
    // Based on the user-supplied WholeExtent, this method determines the
    // topology of the structured domain, e.g., VTK_XYZ_GRID, VTK_XY_PLANE, etc.
    void AcquireDataDescription();

    // Description:
    // Prints the extent, used for debugging
    void PrintExtent( int extent[6] );

    int DataDescription;
    int NumberOfGrids;
    int WholeExtent[6];
    std::vector< int > GridExtents;
    std::vector< std::vector<vtkStructuredNeighbor> > Neighbors;

  private:
    vtkStructuredGridConnectivity( const vtkStructuredGridConnectivity& );
    void operator=(const vtkStructuredGridConnectivity& );
};

#endif /* vtkStructuredGridConnectivity_H_ */
