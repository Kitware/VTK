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
/**
 * @class   vtkAbstractGridConnectivity
 *
 *
 *  A superclass that defines the interface to be implemented by all
 *  concrete grid connectivity classes. Grid connectivity classes provide the
 *  mechanism to achieve the following:
 *  <ul>
 *    <li>
 *      <b> Handling of partitioned/distributed data </b>
 *      <p>
 *       Construct the neighboring topology information for each partition,e.g.,
 *       used for creating communication lists and in computing statistics,i.e.,
 *       average, mean, etc.
 *      </p>
 *      <b> Creation of ghost layers </b>
 *      <p>
 *       Provides the mechanism for automatically generating ghost information
 *       given a partitioned/distributed grid configuration.
 *      </p>
 *    </li>
 *  </ul>
 *
 * @sa
 *  vtkStructuredGridConnectivity vtkStructuredAMRGridConnectivity
*/

#ifndef vtkAbstractGridConnectivity_h
#define vtkAbstractGridConnectivity_h

// VTK includes
#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkObject.h"
#include "vtkPoints.h"            // for vtkPoints definition in STL vector
#include "vtkPointData.h"         // for vtkPointData definition in STL vector
#include "vtkCellData.h"          // for vtkCellData definition int STL vector
#include "vtkUnsignedCharArray.h" // for vtkUnsignedCharArray definition

// Forward declarations
class vtkPointData;
class vtkCellData;
class vtkUnsignedCharArray;
class vtkPoints;

// C++ include directives
#include <vector>  // For STL vector
#include <cassert> // For assert

class VTKFILTERSGEOMETRY_EXPORT vtkAbstractGridConnectivity : public vtkObject
{
public:
  vtkTypeMacro( vtkAbstractGridConnectivity, vtkObject );
  void PrintSelf(ostream &os,vtkIndent indent ) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the number of ghost layers
   */
  vtkSetMacro( NumberOfGhostLayers, unsigned int );
  vtkGetMacro( NumberOfGhostLayers, unsigned int);
  //@}

  /**
   * Sets the total number of grids in the domain.
   * Note: This method is implemented by concrete classes.
   * NOTE: Concrete classes implementing this pure virtual method must
   * set the number of grids and call AllocateUserRegisterDataStructures
   * in addition to defining any other additional functionality.
   */
  virtual void SetNumberOfGrids( const unsigned int N ) = 0;

  /**
   * Returns the total number of grids.
   */
  unsigned int GetNumberOfGrids() { return this->NumberOfGrids; };

  /**
   * Computes the grid neighboring topology for the domain
   */
  virtual void ComputeNeighbors( ) = 0;

  /**
   * Creates N layers of ghost layers where N is the number of cells that will
   * be added to each grid. If no parameter is supplied, N has a nominal value
   * of 1, in which case 1 layer of cells would be added.
   * NOTE: This method is implemented by concrete implementations
   */
  virtual void CreateGhostLayers( const int N=1 ) = 0;

  /**
   * Returns the ghosted points ghost array for the grid associated with the
   * given grid ID. The return pointer is a shallow-copy of the internal
   * data-structure. The pointer may be NULL iff there is no ghosted points
   * ghost array for the requested grid.
   */
  vtkUnsignedCharArray* GetGhostedPointGhostArray( const int gridID );

  /**
   * Returns the ghosted cells ghost array for the grid associated with the
   * given grid ID. The return pointer is a shallow-copy of the internal
   * data-structure. The pointer may be NULL iff there is no ghosted cells
   * ghost array for the requested grid.
   */
  vtkUnsignedCharArray* GetGhostedCellGhostArray( const int gridID );

  /**
   * Returns the ghosted grid point data for the grid associated with the
   * given grid ID. The return pointer is a shallow-copy of the internal
   * data-structure. The pointer may be NULL iff there is no ghosted point
   * data for the requested grid.
   */
  vtkPointData* GetGhostedGridPointData( const int gridID );

  /**
   * Returns the ghosted grid cell data for the grid associated with the
   * given grid ID. The return pointer is a shallow-copy of the internal
   * data-structure. The pointer may be NULL iff there is no ghosted cell
   * data for the requested grid.
   */
  vtkCellData* GetGhostedGridCellData( const int gridID );

  /**
   * Returns the ghosted grid points for the grid associated with the given
   * grid ID. The return pointer is a shallow-copy of the internal data
   * structure. The pointer may be NULL iff there are no ghosted points
   * created for the requested grid.
   */
  vtkPoints* GetGhostedPoints( const int gridID );

protected:
  vtkAbstractGridConnectivity();
  ~vtkAbstractGridConnectivity() VTK_OVERRIDE;

  /**
   * Fills the ghost arrays for the given grid.
   */
  virtual void FillGhostArrays(
      const int gridId,
      vtkUnsignedCharArray* nodesArray,
      vtkUnsignedCharArray* cellsArray ) = 0;

  /**
   * Registers the ghostarrays for the given grid.
   */
  void RegisterGridGhostArrays(
       const int gridID,vtkUnsignedCharArray *nodesArray,
       vtkUnsignedCharArray *cellsArray );

  /**
   * Registers the grid's field data, i.e., the node and cell data.
   */
  void RegisterFieldData(
       const int gridID, vtkPointData *PointData, vtkCellData *CellData );

  /**
   * Registers the grid nodes for the grid associated with the given gridID.
   */
  void RegisterGridNodes( const int gridID, vtkPoints *nodes );


  //@{
  /**
   * Allocate/De-allocate the data-structures where the user-supplied grids
   * will be registered.
   */
  void AllocateUserRegisterDataStructures();
  void DeAllocateUserRegisterDataStructures();
  //@}

  //@{
  /**
   * Allocated/De-allocate the data-structures where the ghosted grid
   * data will be stored.
   */
  void AllocateInternalDataStructures();
  void DeAllocateInternalDataStructures();
  //@}

  // The total number of grids, set initially by the user.
  unsigned int NumberOfGrids;
  unsigned int NumberOfGhostLayers;

  // Arrays registered by the user for each grid
  std::vector< vtkUnsignedCharArray* > GridPointGhostArrays;
  std::vector< vtkUnsignedCharArray* > GridCellGhostArrays;
  std::vector< vtkPointData* > GridPointData;
  std::vector< vtkCellData* > GridCellData;
  std::vector< vtkPoints* > GridPoints;

  // Arrays computed internally for each grid
  bool AllocatedGhostDataStructures;
  std::vector< vtkPointData* > GhostedGridPointData;
  std::vector< vtkCellData* > GhostedGridCellData;
  std::vector< vtkUnsignedCharArray* > GhostedPointGhostArray;
  std::vector< vtkUnsignedCharArray* > GhostedCellGhostArray;
  std::vector< vtkPoints* > GhostedGridPoints;

private:
  vtkAbstractGridConnectivity(const vtkAbstractGridConnectivity&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAbstractGridConnectivity&) VTK_DELETE_FUNCTION;
};

//------------------------------------------------------------------------------
inline vtkUnsignedCharArray*
vtkAbstractGridConnectivity::GetGhostedPointGhostArray( const int gridID )
{
  if( !this->AllocatedGhostDataStructures )
  {
    return NULL;
  }


  assert( "pre: GridID is out-of-bound GridPointData" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids) ) );
  assert( "pre: Ghosted point ghost array" &&
       (this->NumberOfGrids == this->GhostedPointGhostArray.size() ) );

  return( this->GhostedPointGhostArray[ gridID ]  );
}

//------------------------------------------------------------------------------
inline vtkUnsignedCharArray*
vtkAbstractGridConnectivity::GetGhostedCellGhostArray( const int gridID )
{
  if( !this->AllocatedGhostDataStructures )
  {
    return NULL;
  }

  assert( "pre: GridID is out-of-bound GridPointData" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: Ghosted point ghost array" &&
       (this->NumberOfGrids == this->GhostedCellGhostArray.size() ) );

  return( this->GhostedCellGhostArray[ gridID ] );
}

//------------------------------------------------------------------------------
inline vtkPointData*
vtkAbstractGridConnectivity::GetGhostedGridPointData( const int gridID )
{
  if( !this->AllocatedGhostDataStructures )
  {
    return NULL;
  }

  assert( "pre: GridID is out-of-bound GridPointData" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: Ghosted point ghost array" &&
       (this->NumberOfGrids == this->GhostedGridPointData.size() ) );

  return( this->GhostedGridPointData[ gridID ] );
}

//------------------------------------------------------------------------------
inline vtkCellData*
vtkAbstractGridConnectivity::GetGhostedGridCellData( const int gridID )
{
  if( !this->AllocatedGhostDataStructures )
  {
    return NULL;
  }

  assert( "pre: GridID is out-of-bound GridPointData" &&
            (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: Ghosted point ghost array" &&
          (this->NumberOfGrids == this->GhostedGridCellData.size() ) );

  return( this->GhostedGridCellData[ gridID ] );
}

//------------------------------------------------------------------------------
inline vtkPoints*
vtkAbstractGridConnectivity::GetGhostedPoints( const int gridID )
{
  if( !this->AllocatedGhostDataStructures )
  {
    return NULL;
  }

  assert( "pre: GridID is out-of-bound GridPointData" &&
            (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: Ghosted point ghost array" &&
          (this->NumberOfGrids == this->GhostedGridPoints.size() ) );

  return( this->GhostedGridPoints[ gridID ] );
}

//------------------------------------------------------------------------------
inline void
vtkAbstractGridConnectivity::AllocateUserRegisterDataStructures()
{
  // Sanity Check
  assert( "pre: Allocating UserRegister for N > 0 grids" &&
          (this->NumberOfGrids > 0) );

  this->GridPointGhostArrays.resize( this->NumberOfGrids, NULL );
  this->GridCellGhostArrays.resize( this->NumberOfGrids, NULL );
  this->GridPointData.resize( this->NumberOfGrids, NULL );
  this->GridCellData.resize( this->NumberOfGrids, NULL );
  this->GridPoints.resize( this->NumberOfGrids, NULL );
}

//------------------------------------------------------------------------------
inline void
vtkAbstractGridConnectivity::DeAllocateUserRegisterDataStructures()
{
  assert( "pre: Data-structure has not been properly allocated" &&
             (this->GridPointGhostArrays.size() == this->NumberOfGrids  ) );
  assert( "pre: Data-structure has not been properly allocated" &&
             (this->GridCellGhostArrays.size() == this->NumberOfGrids  ) );
  assert( "pre: Data-structure has not been properly allocated" &&
             (this->GridPointData.size() == this->NumberOfGrids  ) );
  assert( "pre: Data-structure has not been properly allocated" &&
             (this->GridCellData.size() == this->NumberOfGrids  ) );
  assert( "pre: Data-structure has not been properly allocated" &&
             (this->GridPoints.size() == this->NumberOfGrids  ) );

  for( unsigned int i=0; i < this->NumberOfGrids; ++i )
  {
// NOTE: Ghost arrays are not deleted here b/c when they are registered, they
// are not shallow-copied.
//    if( this->GridPointGhostArrays[i] != NULL )
//      {
//      this->GridPointGhostArrays[i]->Delete();
//      }
//    if( this->GridCellGhostArrays[i] != NULL )
//      {
//      this->GridCellGhostArrays[i]->Delete();
//      }
    if( this->GridPointData[i] != NULL )
    {
      this->GridPointData[i]->Delete();
    }
    if( this->GridCellData[i] != NULL )
    {
      this->GridCellData[i]->Delete();
    }
    if( this->GridPoints[i] != NULL )
    {
      this->GridPoints[i]->Delete();
    }
  } // END for all grids

  this->GridPointGhostArrays.clear();
  this->GridCellGhostArrays.clear();
  this->GridPointData.clear();
  this->GridCellData.clear();
  this->GridPoints.clear();
}

//------------------------------------------------------------------------------
inline void
vtkAbstractGridConnectivity::AllocateInternalDataStructures()
{
  assert( "pre: Allocating Internal data-structured for N > 0 grids" &&
          (this->NumberOfGrids > 0) );

  this->GhostedGridPointData.resize( this->NumberOfGrids, NULL );
  this->GhostedGridCellData.resize( this->NumberOfGrids, NULL );
  this->GhostedPointGhostArray.resize( this->NumberOfGrids, NULL );
  this->GhostedCellGhostArray.resize( this->NumberOfGrids, NULL );
  this->GhostedGridPoints.resize( this->NumberOfGrids, NULL );
  this->AllocatedGhostDataStructures = true;
}

//------------------------------------------------------------------------------
inline void
vtkAbstractGridConnectivity::DeAllocateInternalDataStructures()
{
  if( !this->AllocatedGhostDataStructures )
  {
    return;
  }

  assert( "pre: Data-structure has not been properly allocated" &&
          (this->GhostedGridPointData.size() == this->NumberOfGrids) );
  assert( "pre: Data-structure has not been properly allocated" &&
          (this->GhostedGridCellData.size() == this->NumberOfGrids) );
  assert( "pre: Data-structure has not been properly allocated" &&
          (this->GhostedPointGhostArray.size() == this->NumberOfGrids) );
  assert( "pre: Data-structure has not been properly allocated" &&
          (this->GhostedCellGhostArray.size() == this->NumberOfGrids ) );
  assert( "pre: Data-structure has not been properly allocated" &&
          (this->GhostedGridPoints.size() == this->NumberOfGrids ) );

  for( unsigned int i=0; i < this->NumberOfGrids; ++i )
  {
    if( this->GhostedGridPointData[i] != NULL )
    {
      this->GhostedGridPointData[i]->Delete();
    }
    if( this->GhostedGridCellData[i] != NULL )
    {
      this->GhostedGridCellData[i]->Delete();
    }
    if( this->GhostedPointGhostArray[i] != NULL )
    {
      this->GhostedPointGhostArray[i]->Delete();
    }
    if( this->GhostedCellGhostArray[i] != NULL )
    {
      this->GhostedCellGhostArray[i]->Delete();
    }
    if( this->GhostedGridPoints[i] != NULL )
    {
      this->GhostedGridPoints[i]->Delete();
    }
  } // END for all grids

  this->GhostedGridPointData.clear();
  this->GhostedGridCellData.clear();
  this->GhostedPointGhostArray.clear();
  this->GhostedCellGhostArray.clear();
  this->GhostedGridPoints.clear();

  this->AllocatedGhostDataStructures = false;
}

//------------------------------------------------------------------------------
inline void vtkAbstractGridConnectivity::RegisterGridGhostArrays(
        const int gridID,
        vtkUnsignedCharArray *nodesArray,
        vtkUnsignedCharArray *cellsArray )
{
  // Sanity check
  assert( "pre: GridID is out-of-bound GridPointData" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: GridPointGhostArrays has not been allocated" &&
       (this->GridPointGhostArrays.size() == this->NumberOfGrids) );
  assert( "pre: GridCellGhostArrays has not been allocated" &&
       (this->GridCellGhostArrays.size() == this->NumberOfGrids ) );

  // NOTE: We should really shallow copy the objects here
  this->GridPointGhostArrays[ gridID ] = nodesArray;
  this->GridCellGhostArrays[ gridID ]  = cellsArray;
}

//------------------------------------------------------------------------------
inline void vtkAbstractGridConnectivity::RegisterFieldData(
        const int gridID, vtkPointData *PointData, vtkCellData *CellData )
{
  // Sanity check
  assert( "pre: GridID is out-of-bound GridPointData" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: GridPointData has not been allocated!" &&
       (this->GridPointData.size() == this->NumberOfGrids ) );
  assert( "pre: GridCellData has not been allocated!" &&
       (this->GridCellData.size() == this->NumberOfGrids ) );

  // Note: The size of these vectors is allocated in SetNumberOfGrids
  if( PointData != NULL )
  {
    assert( "pre: GridPointData[gridID] must be NULL" &&
             this->GridPointData[ gridID ]==NULL );
    this->GridPointData[ gridID ] = vtkPointData::New();
    this->GridPointData[ gridID ]->ShallowCopy( PointData );
  }
  else
  {
    this->GridPointData[ gridID ] = NULL;
  }

  if( CellData != NULL )
  {
    assert( "pre: GridCellData[gridID] must be NULL" &&
            this->GridCellData[gridID]==NULL );
    this->GridCellData[ gridID ]  = vtkCellData::New();
    this->GridCellData[ gridID ]->ShallowCopy( CellData );
  }
  else
  {
    this->GridCellData[ gridID ] = NULL;
  }
}

//------------------------------------------------------------------------------
inline void vtkAbstractGridConnectivity::RegisterGridNodes(
        const int gridID, vtkPoints *nodes )
{
  // Sanity check
  assert( "pre: GridID is out-of-bound GridPointData" &&
       (gridID >= 0) && (gridID < static_cast<int>(this->NumberOfGrids)));
  assert( "pre: GridPoints has not been allocated!" &&
       (this->GridPoints.size() == this->NumberOfGrids) );

  if( nodes != NULL )
  {
    assert( "pre:GridPoints[gridID] must be NULL" &&
            this->GridPoints[gridID]==NULL );
    this->GridPoints[ gridID ] = vtkPoints::New();
    this->GridPoints[ gridID ]->SetDataTypeToDouble();
    this->GridPoints[ gridID ]->ShallowCopy( nodes );
  }
  else
  {
    this->GridPoints[ gridID ] = NULL;
  }
}

#endif /* vtkAbstractGridConnectivity_h */
