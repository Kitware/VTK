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
 * @class   vtkHyperTreeCursor
 * @brief   Objects that can traverse hypertree nodes.
 *
 * Objects that can traverse hyper3TREE nodes. It is an abstract class.
 * Cursors are created by the hyper3TREE.
 * @sa
 * vtkDataObject vtkFieldData vtkHyper3TREEAlgorithm
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux and Charles Law,
 * Kitware 2013
 * This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeCursor_h
#define vtkHyperTreeCursor_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

enum
{
  VTK_3TREE_CHILD_ZMIN_YMIN_XMIN=0,
  VTK_3TREE_CHILD_ZMIN_YMIN_XMAX,
  VTK_3TREE_CHILD_ZMIN_YMAX_XMIN,
  VTK_3TREE_CHILD_ZMIN_YMAX_XMAX,
  VTK_3TREE_CHILD_ZMAX_YMIN_XMIN,
  VTK_3TREE_CHILD_ZMAX_YMIN_XMAX,
  VTK_3TREE_CHILD_ZMAX_YMAX_XMIN,
  VTK_3TREE_CHILD_ZMAX_YMAX_XMAX
};

const int VTK_2TREE_CHILD_SW=VTK_3TREE_CHILD_ZMIN_YMIN_XMIN;
const int VTK_2TREE_CHILD_SE=VTK_3TREE_CHILD_ZMIN_YMIN_XMAX;
const int VTK_2TREE_CHILD_NW=VTK_3TREE_CHILD_ZMIN_YMAX_XMIN;
const int VTK_2TREE_CHILD_NE=VTK_3TREE_CHILD_ZMIN_YMAX_XMAX;

const int VTK_1TREE_TREE_CHILD_LEFT=VTK_2TREE_CHILD_SW;
const int VTK_1TREE_TREE_CHILD_RIGHT=VTK_2TREE_CHILD_SE;

class vtkHyperTree;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeCursor : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeCursor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return the HyperTree on which the cursor points to.
   */
  virtual vtkHyperTree* GetTree() = 0;

  /**
   * Return the index of the current leaf in the data arrays.
   * \pre is_leaf: IsLeaf()
   */
  virtual vtkIdType GetLeafId() = 0;

  /**
   * Return the index of the current node in the data arrays.
   */
  virtual vtkIdType GetNodeId() = 0;

  /**
   * Is the node pointed by the cursor a leaf?
   */
  virtual bool IsLeaf() = 0;

  // Are the children of the current node all leaves?
  // This query can be called also on a leaf node.
  // \post compatible: result implies !IsLeaf()
  virtual bool IsTerminalNode() = 0;

  /**
   * Is the node pointed by the cursor the root?
   */
  virtual bool IsRoot() = 0;

  /**
   * Return the level of the node pointed by the cursor.
   * \post positive_result: result>=0
   */
  virtual int GetCurrentLevel() = 0;

  /**
   * Return the child number of the current node relative to its parent.
   * \pre not_root: !IsRoot().
   * \post valid_range: result>=0 && result<GetNumberOfChildren()
   */
  virtual int GetChildIndex() = 0;

  /**
   * Move the cursor to the root node.
   * \pre can be root
   * \post is_root: IsRoot()
   */
  virtual void ToRoot() = 0;

  /**
   * Move the cursor to the parent of the current node.
   * \pre not_root: !IsRoot()
   */
  virtual void ToParent() = 0;

  /**
   * Move the cursor to child `child' of the current node.
   * \pre not_leaf: !IsLeaf()
   * \pre valid_child: child>=0 && child<this->GetNumberOfChildren()
   */
  virtual void ToChild( int child ) = 0;

  /**
   * Move the cursor to the same node pointed by `other'.
   * \pre other_exists: other!=0
   * \pre same_hyper3TREE: this->SameTree(other);
   * \post equal: this->IsEqual(other)
   */
  virtual void ToSameNode( vtkHyperTreeCursor* other ) = 0;

  /**
   * Is `this' equal to `other'?
   * \pre other_exists: other!=0
   * \pre same_hyper3TREE: this->SameTree(other);
   */
  virtual bool IsEqual( vtkHyperTreeCursor* other ) = 0;

  /**
   * Create a copy of `this'.
   * \post results_exists:result!=0
   * \post same_tree: result->SameTree(this)
   */
  virtual vtkHyperTreeCursor* Clone() = 0;

  /**
   * Are `this' and `other' pointing on the same hyper3TREE?
   * \pre other_exists: other!=0
   */
  virtual int SameTree( vtkHyperTreeCursor* other ) = 0;

  /**
   * Return the index in dimension `d', as if the node was a cell of a
   * uniform grid of 1<<GetCurrentLevel() cells in each dimension.
   * \pre valid_range: d>=0 && d<GetDimension()
   * \post valid_result: result>=0 && result<(1<<GetCurrentLevel())
   */
  virtual int GetIndex( int d ) = 0;

  /**
   * Return the number of children for each node of the tree.
   * \post positive_number: result>0
   */
  virtual int GetNumberOfChildren() = 0;

  /**
   * Return the dimension of the tree.
   * \post positive_result: result>0
   */
  virtual int GetDimension() = 0;

  /**
   * Move to the node described by its indices in each dimension and
   * at a given level. If there is actually a node or a leaf at this
   * location, Found() returns true. Otherwise, Found() returns false and the
   * cursor moves to the closest parent of the query. It can be the root in the
   * worst case.
   * \pre indices_exists: indices!=0
   * \pre valid_size: sizeof(indices)==GetDimension()
   * \pre valid_level: level>=0
   */
  virtual void MoveToNode( int* indices, int level ) = 0;

  /**
   * Did the last call to MoveToNode succeed?
   */
  virtual bool Found() = 0;

protected:
  // Constructor.
  vtkHyperTreeCursor();
  ~vtkHyperTreeCursor() VTK_OVERRIDE;

private:
  vtkHyperTreeCursor(const vtkHyperTreeCursor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHyperTreeCursor&) VTK_DELETE_FUNCTION;
};
#endif
