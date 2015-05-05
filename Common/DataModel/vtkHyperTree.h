/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperTree - An object structured as a tree where each node has
// exactly either 2^n or 3^n children.
//
// .SECTION Description
// An hypertree is a dataset where each node has either exactly 2^n or 3^n children
// or no child at all if the node is a leaf. `n' is the dimension of the
// dataset (1 (binary tree), 2 (quadtree) or 3 (octree) ).
// The class name comes from the following paper:
//
// \verbatim
// @ARTICLE{yau-srihari-1983,
//  author={Mann-May Yau and Sargur N. Srihari},
//  title={A Hierarchical Data Structure for Multidimensional Digital Images},
//  journal={Communications of the ACM},
//  month={July},
//  year={1983},
//  volume={26},
//  number={7},
//  pages={504--515}
//  }
// \endverbatim
//
// Each node is a cell. Attributes are associated with cells, not with points.
// The geometry is implicitly given by the size of the root node on each axis
// and position of the center and the orientation. (TODO: review center
// position and orientation). The geometry is then not limited to an hybercube
// but can have a rectangular shape.
// Attributes are associated with leaves. For LOD (Level-Of-Detail) purpose,
// attributes can be computed on none-leaf nodes by computing the average
// values from its children (which can be leaves or not).
//
// By construction, an hypertree is efficient in memory usage when the
// geometry is sparse. The LOD feature allows to cull quickly part of the
// dataset.
//
// This is an abstract class used as a superclass by a templated compact class.
// All methods are pure virtual. This is done to hide templates.
//
// .SECTION Case with 2^n children
// * 3D case (octree)
// for each node, each child index (from 0 to 7) is encoded in the following
// orientation. It is easy to access each child as a cell of a grid.
// Note also that the binary representation is relevant, each bit code a
// side: bit 0 encodes -x side (0) or +x side (1)
// bit 1 encodes -y side (0) or +y side (1)
// bit 2 encodes -z side (0) or +z side (2)
// - the -z side first
// - 0: -y -x sides
// - 1: -y +x sides
// - 2: +y -x sides
// - 3: +y +x sides
// \verbatim
//              +y
// +-+-+        ^
// |2|3|        |
// +-+-+  O +z  +-> +x
// |0|1|
// +-+-+
// \endverbatim
//
// - then the +z side, in counter-clockwise
// - 4: -y -x sides
// - 5: -y +x sides
// - 6: +y -x sides
// - 7: +y +x sides
// \verbatim
//              +y
// +-+-+        ^
// |6|7|        |
// +-+-+  O +z  +-> +x
// |4|5|
// +-+-+
// \endverbatim
//
// The cases with fewer dimensions are consistent with the octree case:
//
// * Quadtree:
// in counter-clockwise
// - 0: -y -x edges
// - 1: -y +x edges
// - 2: +y -x edges
// - 3: +y +x edges
// \verbatim
//         +y
// +-+-+   ^
// |2|3|   |
// +-+-+  O+-> +x
// |0|1|
// +-+-+
// \endverbatim
//
// * Binary tree:
// \verbatim
// +0+1+  O+-> +x
// \endverbatim
//
// .SECTION Caveats
// It is not a spatial search object. If you are looking for this kind of
// octree see vtkCellLocator instead.
//
// .SECTION Thanks
// This class was written by Philippe Pebay, Joachim Pouderoux and Charles Law, Kitware 2013
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#ifndef vtkHyperTree_h
#define vtkHyperTree_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkHyperTreeCursor;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTree : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTree, vtkObject);
  virtual void Initialize() = 0;
  virtual vtkHyperTreeCursor* NewCursor() = 0;
  virtual vtkIdType GetNumberOfLeaves() = 0;
  virtual vtkIdType GetNumberOfNodes() = 0;
  virtual vtkIdType GetNumberOfIndex() = 0;
  virtual int GetBranchFactor() = 0;
  virtual int GetDimension() = 0;
  virtual void SetScale( double[3] ) = 0;
  virtual void GetScale( double[3] ) = 0;
  virtual double GetScale( unsigned int ) = 0;

  // Description:
  // Return the number of levels.
  // \post result_greater_or_equal_to_one: result>=1
  virtual vtkIdType GetNumberOfLevels() = 0;

  // Description:
  // Subdivide node pointed by cursor, only if its a leaf.
  // At the end, cursor points on the node that used to be leaf.
  // \pre leaf_exists: leaf!=0
  // \pre is_a_leaf: leaf->CurrentIsLeaf()
  virtual void SubdivideLeaf( vtkHyperTreeCursor* leaf ) = 0;

  // Description:
  // Return the actual memory size in kibibytes (1024 bytes).
  // NB: Ignores the attribute array.
  virtual unsigned int GetActualMemorySize() = 0;

  // Description:
  // Return an instance of a templated hypertree for given branch
  // factor and dimension
  // This is done to hide templates.
  static vtkHyperTree* CreateInstance( unsigned int branchFactor,
                                       unsigned int dimension );

  // Description:
  // Find the Index, Parent Index and IsLeaf() parameters of a child for hypertree.
  // This is done to hide templates.
  virtual void FindChildParameters( int, vtkIdType&, bool& );

  // Description:
  // Set the start global index for the current tree.
  // The global index of a node will be this index + the node index.
  virtual void SetGlobalIndexStart( vtkIdType ) = 0;

  // Description:
  // Set the mapping between local & global ids used by HyperTreeGrids.
  virtual void SetGlobalIndexFromLocal( vtkIdType local, vtkIdType global ) = 0;

  // Description:
  // Get the global id of a local node.
  // Use the mapping function if available or the start global index.
  virtual vtkIdType GetGlobalIndexFromLocal( vtkIdType local ) = 0;

protected:
  vtkHyperTree()
  {
  }

private:
  vtkHyperTree(const vtkHyperTree&);  // Not implemented.
  void operator=(const vtkHyperTree&);    // Not implemented.
};


#endif
