/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAbstractGridConnectivity.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAbstractGridConnectivity.h -- Superclass for GridConnectivity
//
// .SECTION Description
//  A superclass that defines the interface to be implemented by all
//  concrete classes.
//
// .SECTION See Also
//  vtkStructuredGridConnectivity

#ifndef VTKABSTRACTGRIDCONNECTIVITY_H_
#define VTKABSTRACTGRIDCONNECTIVITY_H_

// VTK includes
#include "vtkObject.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkUnsignedCharArray.h"

// C++ include directives
#include <vtkstd/vector> // For STL vector
#include <cassert> // For assert

// Forward declarations
class vtkPointData;
class vtkCellData;
class vtkUnsignedCharArray;

class VTK_COMMON_EXPORT vtkAbstractGridConnectivity : public vtkObject
{
  public:
    vtkTypeMacro( vtkAbstractGridConnectivity, vtkObject );
    void PrintSelf( std::ostream &os,vtkIndent indent );

    // Description:
    // Sets the total number of grids in the domain.
    // Note: This method is implemented by concrete classes.
    //
    // NOTE: Concrete classes implementing this pure virtual method must
    // at least do the following:
    // (1) Set the NumberOfGrids ivar to N
    // (2) Allocate the GridPointData and GridCellData vectors
    // (3) Allocate the GridPointGhostArrays and GridCellGhostArrays vectors
    virtual void SetNumberOfGrids( const int N ) = 0;

    // Description:
    // Returns the total number of grids.
    int GetNumberOfGrids() { return this->NumberOfGrids; };

    // Description:
    // Computes the grid neighboring topology for the domain
    virtual void ComputeNeighbors( ) = 0;

    // Description:
    // Fills the ghost arrays for the given grid.
    virtual void FillGhostArrays(
        const int gridId,
        vtkUnsignedCharArray* nodesArray,
        vtkUnsignedCharArray* cellsArray ) = 0;

    // Description:
    // Creates N layers of ghost layers where N is the number of cells that will be
    // added to each grid. If no parameter is supplied, N has a nominal value
    // of 1, in which case 1 layer of cells would be added.
    //
    // NOTE: This method is implemented by concrete implementations
    virtual void CreateGhostLayers( const int N=1 ) = 0;

    // Description:
    // Communicates the data at the ghost nodes.
    virtual void CommunicateGhostNodes() = 0;

    // Description:
    // Communicates the data at the ghost cells.
    virtual void CommunicateGhostCells() = 0;

    // Description:
    // Registers the ghostarrays for the given grid.
    virtual void RegisterGridGhostArrays(
        const int gridID,
        vtkUnsignedCharArray *nodesArray,
        vtkUnsignedCharArray *cellsArray )
    {
      // Sanity check
      assert( "GridID is out-of-bound GridPointData" &&
               (gridID >= 0) && (gridID < this->NumberOfGrids ) );
      assert( "GridID is out-of-bound GridPointData" &&
               (gridID >= 0) && (gridID < this->NumberOfGrids ) );

      this->GridPointGhostArrays[ gridID ] = nodesArray;
      this->GridCellGhostArrays[ gridID ]  = cellsArray;
// Cannot shallow copy vtkDataArrays ? I'll just push on for now
//      this->GridPointGhostArrays[ gridID ] = vtkUnsignedCharArray::New();
//      this->GridPointGhostArrays[ gridID ]->ShallowCopy( nodesArray );
    }

    // Description:
    // Registers the grid's field data, i.e., the node and cell data.
    virtual void RegisterFieldData(
        const int gridID, vtkPointData *PointData, vtkCellData *CellData )
      {
        // Sanity check
        assert( "GridID is out-of-bound GridPointData" &&
                 (gridID >= 0) && (gridID < this->NumberOfGrids ) );
        assert( "GridID is out-of-bound GridPointData" &&
                 (gridID >= 0) && (gridID < this->NumberOfGrids ) );


        // Note: The size of these vectors is allocated in SetNumberOfGrids
        this->GridPointData[ gridID ] = vtkPointData::New();
        this->GridPointData[ gridID ]->ShallowCopy( PointData );

        this->GridCellData[ gridID ]  = vtkCellData::New();
        this->GridCellData[ gridID ]->ShallowCopy( CellData );
      }

  protected:
    vtkAbstractGridConnectivity();
    virtual ~vtkAbstractGridConnectivity();

    // The total number of grids, set initially by the user.
    int NumberOfGrids;
    int NumberOfGhostLayers;

    // BTX

    // Arrays registered by the user for each grid
    vtkstd::vector< vtkUnsignedCharArray* > GridPointGhostArrays;
    vtkstd::vector< vtkUnsignedCharArray* > GridCellGhostArrays;
    vtkstd::vector< vtkPointData* > GridPointData;
    vtkstd::vector< vtkCellData* >  GridCellData;

    // Arrays computed internally for each grid
    vtkstd::vector< vtkPointData* > GhostedGridPointData;
    vtkstd::vector< vtkCellData* >  GhostedGridCellData;
    vtkstd::vector< vtkUnsignedCharArray* > GhostedPointGhostArray;
    vtkstd::vector< vtkUnsignedCharArray* > GhostedCellGhostArray;

    // ETX

  private:
    vtkAbstractGridConnectivity(const vtkAbstractGridConnectivity&);// Not implemented
    void operator=(const vtkAbstractGridConnectivity&); // Not implemented
};

#endif /* VTKABSTRACTGRIDCONNECTIVITY_H_ */
