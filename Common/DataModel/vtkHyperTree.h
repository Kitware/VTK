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
/**
 * @class   vtkHyperTree
 * @brief   An object structured as a tree where each node has
 * exactly either 2^d or 3^d children.
 *
 *
 * A hypertree is a dataset where each node has either exactly f^d
 * children or no child at all if the node is a leaf, where f in {2,3}
 * is the branching factor of the tree and d in {1,2,3} is the
 * dimension of the dataset.
 * Such trees have particular names when f=2: bintree (d=1), quadtree
 * (d=2), and octree (d=3). When f=3, we respectively call them
 * 3-tree, 9-tree, and 27-tree.
 *
 * The original octree class name came from the following paper:
 * \verbatim
 * @ARTICLE{yau-srihari-1983,
 *  author={Mann-May Yau and Sargur N. Srihari},
 *  title={A Hierarchical Data Structure for Multidimensional Digital Images},
 *  journal={Communications of the ACM},
 *  month={July},
 *  year={1983},
 *  volume={26},
 *  number={7},
 *  pages={504--515}
 *  }
 * \endverbatim
 *
 * Each node is a cell. Attributes are associated with cells, not with points.
 * The geometry is implicitly given by the size of the root node on each axis
 * and position of the center and the orientation. (TODO: review center
 * position and orientation). The geometry is then not limited to a hybercube
 * but can have a rectangular shape.
 * Attributes are associated with leaves. For LOD (Level-Of-Detail) purpose,
 * attributes can be computed on none-leaf nodes by computing the average
 * values from its children (which can be leaves or not).
 *
 * By construction, a hypertree is efficient in memory usage when the
 * geometry is sparse. The LOD feature allows for quick culling of part of the
 * dataset.
 *
 * This is an abstract class used as a superclass by a templated compact class.
 * All methods are pure virtual. This is done to hide templates.
 *
 * @par Case with f=2:
 * * d=3 case (octree)
 * for each node, each child index (from 0 to 7) is encoded in the
 * following orientation.
 * It is easy to access each child as a cell of a grid.
 * Note also that the binary representation is relevant, each bit code
 * a side:
 * bit 0 encodes -x side (0) or +x side (1)
 * bit 1 encodes -y side (0) or +y side (1)
 * bit 2 encodes -z side (0) or +z side (2)
 * -z side is first, in counter-clockwise order:
 *  0: -y -x sides
 *  1: -y +x sides
 *  2: +y -x sides
 *  3: +y +x sides
 * \verbatim
 *              +y
 * +-+-+        ^
 * |2|3|        |
 * +-+-+  O +z  +-> +x
 * |0|1|
 * +-+-+
 * \endverbatim
 *
 * @par Case with f=2:
 * +z side is last, in counter-clockwise order:
 *  4: -y -x sides
 *  5: -y +x sides
 *  6: +y -x sides
 *  7: +y +x sides
 * \verbatim
 *              +y
 * +-+-+        ^
 * |6|7|        |
 * +-+-+  O +z  +-> +x
 * |4|5|
 * +-+-+
 * \endverbatim
 *
 * @par Case with f=2:
 * The cases with fewer dimensions are consistent with the octree case:
 *
 * @par Case with f=2:
 * * d=2 case (quadtree):
 * in counter-clockwise order:
 *  0: -y -x edges
 *  1: -y +x edges
 *  2: +y -x edges
 *  3: +y +x edges
 * \verbatim
 *         +y
 * +-+-+   ^
 * |2|3|   |
 * +-+-+  O+-> +x
 * |0|1|
 * +-+-+
 * \endverbatim
 *
 * @par Case with f=2:
 * * d=1 case (bintree):
 * \verbatim
 * +0+1+  O+-> +x
 * \endverbatim
 *
 * @warning
 * It is not a spatial search object. If you are looking for this kind of
 * octree see vtkCellLocator instead.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTree_h
#define vtkHyperTree_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkHyperTreeCursor;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTree : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTree, vtkObject);
  void PrintSelf( ostream&, vtkIndent ) override;

  /**
   * Restore the initial state: only one node and one leaf: the root.
   */
  virtual void Initialize() = 0;

  /**
   * Return the number of levels.
   */
  virtual vtkIdType GetNumberOfLevels() = 0;

  /**
   * Return the number of vertices in the tree.
   */
  virtual vtkIdType GetNumberOfVertices() = 0;

  /**
   * Return the number of nodes (non-leaf vertices) in the tree.
   */
  virtual vtkIdType GetNumberOfNodes() = 0;

  /**
   * Return the number of leaf vertices in the tree.
   */
  virtual vtkIdType GetNumberOfLeaves() = 0;

  /**
   * Return the branch factor of the tree.
   */
  virtual int GetBranchFactor() = 0;

  /**
   * Return the dimension of the tree.
   */
  virtual int GetDimension() = 0;

  /**
   * Return the number of children per node of the tree.
   */
  virtual vtkIdType GetNumberOfChildren() = 0;

  //@{
  /**
   * Set/Get scale of the tree in each direction.
   */
  virtual void SetScale( double[3] ) = 0;
  virtual void GetScale( double[3] ) = 0;
  virtual double GetScale( unsigned int ) = 0;
  //@}

  /**
   * Return an instance of a templated hypertree for given branch
   * factor and dimension.
   * This is done to hide templates.
   */
  VTK_NEWINSTANCE
  static vtkHyperTree* CreateInstance( unsigned int branchFactor,
                                       unsigned int dimension );

  /**
   * Find the Index of the parent of a vertex in the hypertree.
   * This is done to hide templates.
   */
  virtual void FindParentIndex( vtkIdType& );

  /**
   * Find the Index, Parent Index and IsLeaf() parameters of the child
   * of a node in the hypertree.
   * This is done to hide templates.
   */
  virtual void FindChildParameters( int, vtkIdType&, bool& );

  /**
   * Return pointer to new instance of hyper tree cursor
   */
  virtual vtkHyperTreeCursor* NewCursor() = 0;

  /**
   * Subdivide node pointed by cursor, only if its a leaf.
   * At the end, cursor points on the node that used to be leaf.
   * \pre leaf_exists: leaf!=0
   * \pre is_a_leaf: leaf->CurrentIsLeaf()
   */
  virtual void SubdivideLeaf( vtkHyperTreeCursor* leaf ) = 0;

  /**
   * Return memory used in kibibytes (1024 bytes).
   * NB: Ignore the attribute array because its size is added by the data set.
   */
  virtual unsigned int GetActualMemorySize() = 0;

  /**
   * Set the start global index for the current tree.
   * The global index of a node will be this index + the node index.
   */
  virtual void SetGlobalIndexStart( vtkIdType ) = 0;

  /**
   * Set the mapping between local & global Ids used by HyperTreeGrids.
   */
  virtual void SetGlobalIndexFromLocal( vtkIdType local, vtkIdType global ) = 0;

  /**
   * Get the global id of a local node.
   * Use the mapping function if available or the start global index.
   */
  virtual vtkIdType GetGlobalIndexFromLocal( vtkIdType local ) = 0;

protected:
  vtkHyperTree()
  {
  }

private:
  vtkHyperTree(const vtkHyperTree&) = delete;
  void operator=(const vtkHyperTree&) = delete;
};

#endif
