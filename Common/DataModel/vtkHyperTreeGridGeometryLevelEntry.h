/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometryLevelEntry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridGeometryLevelEntry
 *
 * @brief   Cursor cache data with coordinates and level info
 *
 * cf. vtkHyperTreeGridEntry
 *
 * @sa
 * vtkHyperTreeGridEntry
 * vtkHyperTreeGridLevelEntry
 * vtkHyperTreeGridGeometryEntry
 * vtkHyperTreeGridGeometryLevelEntry
 * vtkHyperTreeGridNonOrientedSuperCursor
 * vtkHyperTreeGridNonOrientedSuperCursorLight
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien, Jerome Dubois and
 * Guenole Harel, CEA 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGeometryLevelEntry_h
#define vtkHyperTreeGridGeometryLevelEntry_h

#ifndef __VTK_WRAP__

#include "assert.h"

#include "vtkObject.h"
#include "vtkSmartPointer.h"

#include "vtkHyperTreeGridOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

class vtkHyperTree;
class vtkHyperTreeGrid;

class vtkHyperTreeGridGeometryLevelEntry
{
public:
  /**
   * Display info about the entry
   */
  void PrintSelf( ostream& os, vtkIndent indent );

  /**
   * Constructor
   */
  vtkHyperTreeGridGeometryLevelEntry()
  {
    this->Tree = nullptr;
    this->Level = 0;
    this->Index = 0;
    for (unsigned int d = 0; d < 3; ++d )
    {
      this->Origin[d] = 0.;
    }
  }

  /**
   * Destructor
   */
  ~vtkHyperTreeGridGeometryLevelEntry() = default;

  /**
   * Dump information
   */
  void Dump( ostream& os );

  /**
   * Initialize cache entry from explicit required data
   */
  void Initialize(
    vtkHyperTree *tree,
    unsigned int level,
    vtkIdType index,
    const double* origin
  )
  {
    this->Tree = tree;
    this->Level = level;
    this->Index = index;
    for (unsigned int d = 0; d < 3; ++d )
    {
      this->Origin[d] = origin[d];
    }
  }

  /**
   * Initialize cache entry at root of given tree index in grid.
   */
  vtkHyperTree* Initialize( vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false );

  /**
   * Reset function
   */
  void Reset() {
    this->Tree = nullptr;
    this->Index = 0;
  }

  /**
   * Copy function
   */
  void Copy( const vtkHyperTreeGridGeometryLevelEntry* entry)
  {
    this->Initialize( entry->Tree, entry->Level, entry->Index, entry->Origin);
  }

  /**
   * Create a vtkHyperTreeGridOrientedCursor from input grid and
   * current entry data.
   */
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor>
  GetHyperTreeGridOrientedGeometryCursor( vtkHyperTreeGrid* grid )
  {
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursor = vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor>::New();
    cursor->Initialize( grid, this->Tree, this->Level, this->Index, this->Origin );
    return cursor;
  }

  /**
   * Create a vtkHyperTreeGridNonOrientedCursor from input grid and
   * current entry data.
   */
  vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor>
  GetHyperTreeGridNonOrientedGeometryCursor( vtkHyperTreeGrid* grid )
  {
    assert ( "pre: level==0" && this->Level == 0 );
    vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor> cursor = vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor>::New();
    cursor->Initialize( grid, this->Tree, this->Level, this->Index, this->Origin );
    return cursor;
  }

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId() const { return this->Index; };

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex() const;

  /**
   * Set the global index for the root cell of the HyperTree.
   */
  void SetGlobalIndexStart( vtkIdType index );

  /**
   * Set the global index for the current cell of the HyperTree.
   */
  void SetGlobalIndexFromLocal( vtkIdType index );

  /**
   * Is the cursor pointing to a leaf?
   */
  bool IsLeaf() const;

  /**
   * Change the current cell's status: if leaf then becomes coarse and
   * all its children are created, cf. HyperTree.
   */
  void SubdivideLeaf();

  /**
   * Is the cursor pointing to a coarse with all childrens being leaves ?
   */
  bool IsTerminalNode() const;

  /**
   * Is the cursor at tree root?
   */
  bool IsRoot() { return ( this->Index == 0 ); }

  /**
   * Move the cursor to child `child' of the current vertex.
   * \pre not_leaf: !IsLeaf()
   * \pre valid_child: ichild>=0 && ichild<this->GetNumberOfChildren()
   */
  void ToChild(
    const vtkHyperTreeGrid* grid,
    unsigned char ichild
  );

  /**
   * Get HyperTree from current cache entry.
   */
  vtkHyperTree* GetTree() const { return this->Tree; }

  /**
   * Get level info from current cache entry.
   */
  unsigned int GetLevel() const { return this->Level; }

  /**
   * Getter for origin coordinates of the current cell.
   */
  double* GetOrigin() { return this->Origin; }
  const double* GetOrigin() const { return this->Origin; }

  /**
   * Getter for bounding box of the current cell.
   */
  void GetBounds( double bounds[6] ) const;

  /**
   * Getter for center of the current cell.
   */
  void GetPoint( double point[3] ) const;

private:

  /**
   * pointer to the HyperTree containing the current cell.
   */
  vtkHyperTree *Tree;

  /**
   * level of the current cell in the HyperTree.
   */
  unsigned int Level;

  /**
   * index of the current cell in the HyperTree.
   */
  vtkIdType Index;

  /**
   * origin coiordinates of the current cell
   */
  double Origin[3];
};

#endif// __VTK_WRAP__

#endif// vtkHyperTreeGridGeometryLevelEntry
// VTK-HeaderTest-Exclude: vtkHyperTreeGridGeometryLevelEntry.h
