/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeCursor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridCursor
 * @brief   Objects for depth-first traversal HyperTreeGrids.
 *
 *
 * Objects that can perform depth-first traversal of hyper tree grids,
 * take into account more parameters (related to the grid structure) than
 *  the compact hyper tree cursor implemented in vtkHyperTree can.
 * This is an abstract class.
 * Cursors are created by the HyperTreeGrid implementation.
 *
 * @sa
 * vtkHyperTreeCursor vtkHyperTree vtkHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Guénolé Harel and Jacques-Bernard Lekien, 2014
 * This class was re-written by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridCursor_h
#define vtkHyperTreeGridCursor_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHyperTreeCursor.h"

class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridCursor : public vtkHyperTreeCursor
{
public:
  vtkTypeMacro(vtkHyperTreeGridCursor, vtkHyperTreeCursor);
  void PrintSelf( ostream& os, vtkIndent indent ) override;
  static vtkHyperTreeGridCursor* New();

  /**
   * Create a copy of `this'.
   * \post results_exists:result!=0
   */
  vtkHyperTreeGridCursor* Clone() override;

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  virtual void Initialize( vtkHyperTreeGrid*, vtkIdType );

  //@{
  /**
   * Set the hyper tree grid to which the cursor is pointing.
   */
  virtual void SetGrid( vtkHyperTreeGrid* );
  vtkGetObjectMacro(Grid,vtkHyperTreeGrid);
  //@}

  //@{
  /**
   * Set the hyper tree to which the cursor is pointing.
   */
  void SetTree( vtkHyperTree* ) override;
  vtkHyperTree* GetTree() override {return this->Tree;};
  //vtkGetObjectMacro(Tree,vtkHyperTree);
  //@}

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId() override;

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
   */
  virtual vtkIdType GetGlobalNodeIndex();

  /**
   * Is the cursor pointing to a leaf?
   */
  bool IsLeaf() override;

  /**
   * Is the cursor at tree root?
   */
  bool IsRoot() override;

  /**
   * Get the level of the tree vertex pointed by the cursor.
   */
  unsigned int GetLevel() override {return this->Level;};

  /**
   * Return the child number of the current vertex relative to its parent.
   * \pre not_root: !IsRoot().
   * \post valid_range: result>=0 && result<GetNumberOfChildren()
   */
  int GetChildIndex() override;

  /**
   * Move the cursor to the root vertex.
   * \pre can be root
   * \post is_root: IsRoot()
   */
  void ToRoot() override;

  /**
   * Move the cursor to the parent of the current vertex.
   * \pre not_root: !IsRoot()
   */
  void ToParent() override;

  /**
   * Move the cursor to child `child' of the current vertex.
   * \pre not_leaf: !IsLeaf()
   * \pre valid_child: child>=0 && child<this->GetNumberOfChildren()
   */
  void ToChild( int child ) override;

  /**
   * Move the cursor to the same vertex pointed by `other'.
   * \pre other_exists: other!=0
   * \pre same_hypertree: this->SameTree(other);
   * \post equal: this->IsEqual(other)
   * NB: not implemented
   */
  void ToSameVertex( vtkHyperTreeCursor* vtkNotUsed(other) ) override { return; }

  /**
   * Is `this' equal to `other'?
   * \pre other_exists: other!=0
   * \pre same_hypertree: this->SameTree(other);
   * NB: not implemented
   */
  bool IsEqual( vtkHyperTreeCursor* vtkNotUsed(other) ) override { return false; }

  /**
   * Are `this' and `other' pointing on the same hypertree?
   * \pre other_exists: other!=0
   * NB: not implemented
   */
  int SameTree( vtkHyperTreeCursor* vtkNotUsed(other) ) override { return 0; }

  /**
   * Return the number of children for each node (non-vertex leaf) of the tree.
   * \post positive_number: result>0
   */
  int GetNumberOfChildren() override;

  /**
   * Return the dimension of the tree.
   * \post positive_result: result>0
   */
  int GetDimension() override;

  /**
   * Compute the origin of the cursor.
   * NB: The basic hyper tree grid cursor does not have an origin.
   */
  virtual double* GetOrigin() { return nullptr; }

  /**
   * Compute the size of the cursor.
   * NB: The basic hyper tree grid cursor does not have a size.
   */
  virtual double* GetSize() { return nullptr; }

  /**
   * Compute the bounds of the cursor.
   * NB: The basic hyper tree grid cursor does not have bounds.
   */
  virtual void GetBounds( double pt[6] )
    { pt[0] = pt[1] = pt[2] = pt[3] = pt[4] = pt[5] = 0.; }

  /**
   * Compute the center coordinates of the cursor.
   * NB: The basic hyper tree grid cursor is always centered at 0.
   */
  virtual void GetPoint( double pt[3] ) { pt[0] = pt[1] = pt[2] = 0.; }

  /**
   * Return the number of neighborhood cursors
   * The neighborhood definition depends on the type of cursor.
   * NB: Only super cursors keep track of neighborhoods.
   */
  virtual unsigned int GetNumberOfCursors() { return 0; }

  /**
   * Return the cursor pointing into i-th neighbor.
   * The neighborhood definition depends on the type of cursor.
   * NB: Only super cursors keep track of neighborhoods.
   */
  virtual vtkHyperTreeGridCursor* GetCursor( unsigned int )
    { return nullptr; }

  /**
   * Return the list of cursors pointing to the leaves touching a
   * given corner of the cell.
   * Return whether the considered cell is the owner of said corner.
   * NB: Only the Moore super cursor implements this functionality.
   */
  virtual bool GetCornerCursors( unsigned int, unsigned int, vtkIdList* )
    { return false; }

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridCursor();

  /**
   * Destructor
   */
  ~vtkHyperTreeGridCursor() override;

  // Hyper tree grid to which the cursor is attached
  vtkHyperTreeGrid* Grid;

  // Hyper tree to which the cursor is attached
  vtkHyperTree* Tree;

  // Level in the tree at which the cursor is positioned
  unsigned int Level;

  // Index either in the nodes or parent (if leaf)
  vtkIdType Index;

  // Is center of cursor at a leaf?
  bool Leaf;

private:
  vtkHyperTreeGridCursor(const vtkHyperTreeGridCursor&) = delete;
  void operator=(const vtkHyperTreeGridCursor&) = delete;
};
#endif
