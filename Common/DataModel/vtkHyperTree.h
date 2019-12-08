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
 * @brief   A data object structured as a tree.
 *
 * An hypertree grid is a dataobject containing a rectilinear grid of
 * elements that can be either null or a hypertree.
 * An hypertree is a dataobject describing a decomposition tree.
 * A VERTICE is an element of this tree.
 * A NODE, also called COARSE cell, is a specific vertice which is
 * refined and than has either exactly f^d children, where f in {2,3}
 * is the branching factor, the same value for all trees in this
 * hypertree grid, and d in {1,2,3} is the spatial dimension. It is
 * called coarse because there are smaller child cells.
 * A LEAF, also called FINE cell, is a vertice without children, not
 * refined. It is called fine because in the same space there are no
 * finer cells.
 * In a tree, we can find coarse cells smaller than fine cell but not
 * in the same space.
 *
 * Such trees have particular names for f=2:
 * - bintree (d=1),
 * - quadtree (d=2),
 * - octree (d=3).
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
 * Attributes are associated with (all) cells, not with points. The
 * attributes that are associated with coarses, it's used for LoD
 * (Level-of-Detail). The attributes on coarse cells can be given by the
 * code or/and computed by the use of a specifc filter exploiting the
 * values from its children (which can be leaves or not).
 *
 * The geometry is implicitly given by the size of the root node on each
 * axis and position of the origin. In fact, in 3D, the geometry is then
 * not limited to a cube but can have a rectangular shape.
 *
 * By construction, an hypertree is efficient in memory usage. The LoD
 * feature allows for quick culling of part of the dataobject.
 *
 * This is an abstract class used as a superclass by a custom templated
 * compact class. Other versions of this code could be made available
 * to meet other needs without questioning cursors and filters.
 * All methods are pure virtual. This is done to hide templates.
 *
 * @par Case octree with f=2, d=3:
 * For each node (coarse cell), 8 children are encoded in a child index
 * (from 0 to 7) in the following orientation described in hypertree grid.
 * It is easy to access each child as a cell of a grid.
 * Note also that the binary representation is relevant, each bit codes
 * a side:
 * bit 0 encodes -x side (0) or +x side (1)
 * bit 1 encodes -y side (0) or +y side (1)
 * bit 2 encodes -z side (0) or +z side (1)
 * -z side is first, in counter-clockwise order:
 *  0: -y -x sides
 *  1: -y +x sides
 *  2: +y -x sides
 *  3: +y +x sides
 * +z side is last, in counter-clockwise order:
 *  4: -y -x sides
 *  5: -y +x sides
 *  6: +y -x sides
 *  7: +y +x sides
 * \verbatim
 *              +y
 * +-+-+        ^
 * |2|3|        |
 * +-+-+  O +z  +-> +x
 * |0|1|
 * +-+-+
 *              +y
 * +-+-+        ^
 * |6|7|        |
 * +-+-+  1 +z  +-> +x
 * |4|5|
 * +-+-+
 * \endverbatim
 *
 * @par Case quadtree with f=2, d=2:
 * Just use 2 bits.
 * \verbatim
 *              +y
 * +-+-+        ^
 * |2|3|        |
 * +-+-+        +-> +x
 * |0|1|
 * +-+-+
 * \endverbatim
 *
 * @par Case bintree with f=2, d=1:
 * Just use 1 bits.
 * \verbatim
 *             O+-> +x
 * \endverbatim
 *
 * It's more difficult with f=3.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and
 * Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * Among others, this class was simplified, optimized (memory), documented and
 * completed for to improve IO XML by Jacques-Bernard Lekien 2018-19
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTree_h
#define vtkHyperTree_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include <cassert> // Used internally
#include <memory>  // std::shared_ptr

class vtkBitArray;
class vtkUnsignedLongArray;

class vtkIdList;

class vtkHyperTreeGridScales;

//=============================================================================
struct vtkHyperTreeData
{
  // Index of this tree in the hypertree grid
  vtkIdType TreeIndex;

  // Number of levels in the tree
  unsigned int NumberOfLevels;

  // Number of vertices in this tree (coarse and leaves)
  vtkIdType NumberOfVertices;

  // Number of nodes (non-leaf vertices) in the tree
  vtkIdType NumberOfNodes;

  // Offset start for the implicit global index mapping fixed by
  // SetGlobalIndexStart after create a tree.
  // If you don't choose implicit global index mapping then this
  // value is -1. Then, you must to describ explicit global index
  // mapping by using then SetGlobalIndexFromLocal for each cell
  // in tree.
  // The extra cost is equivalent to the cost of a field of values
  // of cells.
  vtkIdType GlobalIndexStart;
};

//=============================================================================
class VTKCOMMONDATAMODEL_EXPORT vtkHyperTree : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTree, vtkObject);

  void PrintSelf(ostream&, vtkIndent) override;

  /**
   * Restore the initial state: only one vertice is then a leaf:
   * the root cell for the hypertree.
   * @param branchFactor
   * @param dimension
   * @param numberOfChildren
   */
  void Initialize(unsigned char, unsigned char, unsigned char);

  /**
   * Restore a state from read data, without using a cursor
   * Call after create hypertree with initialize.
   *
   * @param numberOfLevels: the maximum number of levels.
   * @param nbVertices: the number of vertices of the future tree
   * (coarse and leaves), fixed either the information loading
   * (for load reduction) or defined by the fixed level of reader.
   * @param nbVerticesOfLastLevel: the number of vertices of last
   * valid level.
   * @param isParent: a binary decomposition tree by level with
   * constraint all describe children. It is useless to declare
   * all the lastest values to False, especially the last level
   * may not be defined.
   * @param isMasked: a binary mask corresponding. It is useless
   * to declare all the latest values to False.
   * @param outIsMasked: the mask of hypertree grid including
   * this hypertree which is a vtkBitArray.
   */
  virtual void InitializeForReader(vtkIdType numberOfLevels, vtkIdType nbVertices,
    vtkIdType nbVerticesOfLastLevel, vtkBitArray* isParent, vtkBitArray* isMasked,
    vtkBitArray* outIsMasked) = 0;

  /**
   * Initialize a state from write data.
   * Call after create the hypertree grid.
   *
   * @param inIsMasked: the mask of hypertree grid including
   * this hypertree which is a vtkBitArray.
   * @param nbVerticesbyLevel: the number of vertices in tree
   * (coarse and leaves) by each level.
   * @param isParent: modify the vtkBitArray, a binary decomposition
   * tree by level with constraint all describe children. It is
   * useless to declare all the lastest values to False, especially
   * the last level may not be defined.
   * @param isMasked: modify the vtkBitArray, a binary mask
   * corresponding. It is useless to declare all the latest values
   * to False.
   */
  virtual void GetByLevelForWriter(vtkBitArray* inIsMasked, vtkUnsignedLongArray* nbVerticesbyLevel,
    vtkBitArray* isParent, vtkBitArray* isMasked, vtkIdList* ids) = 0;

  /**
   * Copy the structure by sharing the decomposition description
   * of the tree.
   * \pre ht_exist: ht!=nullptr
   */
  void CopyStructure(vtkHyperTree* ht);

  /**
   * Return a freeze instance (a priori compact but potentially
   * unmodifiable).
   * This method is calling by the Squeeze method of hypertree grid.
   * The mode parameter will allow to propose different instances.
   * Today, there is none, the freeze call does not do anything.
   */
  virtual vtkHyperTree* Freeze(const char* mode) = 0;

  //@{
  /**
   * Set/Get tree index in hypertree grid.
   * Services for internal use between hypertree grid and hypertree.
   */
  void SetTreeIndex(vtkIdType treeIndex) { this->Datas->TreeIndex = treeIndex; }
  vtkIdType GetTreeIndex() const { return this->Datas->TreeIndex; }
  //@}

  /**
   * Return the number of levels.
   */
  unsigned int GetNumberOfLevels() const
  {
    assert("post: result_greater_or_equal_to_one" && this->Datas->NumberOfLevels >= 1);
    return this->Datas->NumberOfLevels;
  }

  /**
   * Return the number of all vertices (coarse and fine) in the tree.
   */
  vtkIdType GetNumberOfVertices() const { return this->Datas->NumberOfVertices; }

  /**
   * Return the number of nodes (coarse) in the tree.
   */
  vtkIdType GetNumberOfNodes() const { return this->Datas->NumberOfNodes; }

  /**
   * Return the number of leaf (fine) in the tree.
   */
  vtkIdType GetNumberOfLeaves() const
  {
    return this->Datas->NumberOfVertices - this->Datas->NumberOfNodes;
  }

  /**
   * Return the branch factor of the tree.
   */
  int GetBranchFactor() const { return this->BranchFactor; }

  /**
   * Return the spatial dimension of the tree.
   */
  int GetDimension() const { return this->Dimension; }

  /**
   * Return the number of children per node of the tree.
   * This value is branchfactoring scale spatial dimenion (f^d).
   */
  vtkIdType GetNumberOfChildren() const { return this->NumberOfChildren; }

  //@{
  /**
   * Set/Get scale of the tree in each direction for the ground
   * level (0).
   */
  void GetScale(double s[3]) const;

  double GetScale(unsigned int d) const;
  //@}

  /**
   * In an hypertree, all cells are the same size by level. This
   * fonction initialize this cache system is particular used by
   * the symetric filter.
   */
  std::shared_ptr<vtkHyperTreeGridScales> InitializeScales(
    const double* scales, bool reinitialize = false) const;

  /**
   * Return an instance of an implementation of a hypertree for
   * given branch factor and dimension.
   * Other versions of this code could be made available to meet
   * other needs without questioning cursors and filters.
   * Since an instance, an other instance can be creating by call
   * the method Freeze (by default, nothing more, instance currently
   * is returning).
   */
  VTK_NEWINSTANCE
  static vtkHyperTree* CreateInstance(unsigned char branchFactor, unsigned char dimension);
  /**
   * Return memory used in bytes.
   * NB: Ignore the attribute array because its size is added by the data set.
   */
  virtual unsigned long GetActualMemorySizeBytes() = 0;

  /**
   * Return memory used in kibibytes (1024 bytes).
   * NB: Ignore the attribute array because its size is added by the data set.
   */
  unsigned int GetActualMemorySize()
  {
    // in kilibytes
    return static_cast<unsigned int>(this->GetActualMemorySizeBytes() / 1024);
  }

  /**
   * Return if implicit global index maping has been used.
   * If true, the initialize has been done by SetGlobalIndexStart (one call
   * by hypertree).
   * If false, the initialize has been done by SetGlobalIndexFromLocal (one
   * call by cell of hypertree).
   * GetGlobalIndexFromLocel get the good value of global index mapping for
   * one cell what ever the initialize metho used.
   */
  virtual bool IsGlobalIndexImplicit() = 0;

  /**
   * Set the start implicit global index mapping for the first cell in the
   * current tree.
   * The implicit global index mapping of a node will be computed by
   * this start index + the node index (local offset in tree).
   * The node index begin by 0, the origin cell in tree. The follow values
   * are organizing by fatrie as i to i+NumberOfChildren, for all children
   * of one coarse cell, i is 1+8*s with s in integer. The order of fatrie
   * depend of order to call SubdivideLeaf.
   * This global index mapping permits to access a value of
   * field for a cell, in implicit, the order values depends of implicit
   * order linking with the order build of this tree.
   * WARNING: See of hypertree grid, for to use a implicit global index
   * mapping, you have to build hypertree by hypertree without to recome
   * in hypertree also build.
   * For this tree, in debug, assert is calling if tried call
   * SetGlobalIndexFromLocal.
   * \pre not_global_index_start_if_use_global_index_from_local
   */
  virtual void SetGlobalIndexStart(vtkIdType start) = 0;

  /**
   * Get the start global index for the current tree for implicit global
   * index mapping.
   */
  vtkIdType GetGlobalIndexStart() const { return this->Datas->GlobalIndexStart; }

  /**
   * Set the mapping between a node index in tree and a explicit global
   * index mapping.
   * This global index mapping permits to access a value of
   * field for a cell, in explicit, the index depend of order values.
   * For this tree, in debug, assert is calling if tried call
   * SetGlobalIndexStart.
   * \pre not_global_index_from_local_if_use_global_index_start
   */
  virtual void SetGlobalIndexFromLocal(vtkIdType index, vtkIdType global) = 0;

  /**
   * Get the global id of a local node identified by index.
   * Use the explicit mapping function if available or the implicit
   * mapping build with start global index.
   * \pre not_valid_index
   * \pre not_positive_start_index (case implicit global index mapping)
   * \pre not_positive_global_index (case explicit global index mapping)
   */
  virtual vtkIdType GetGlobalIndexFromLocal(vtkIdType index) const = 0;

  /**
   * Return the maximum value reached by global index mapping
   * (implicit or explicit).
   */
  virtual vtkIdType GetGlobalNodeIndexMax() const = 0;

  /**
   * Return if a vertice identified by index in tree as being leaf.
   * \pre not_valid_index
   */
  virtual bool IsLeaf(vtkIdType index) const = 0;

  /**
   * Subdivide a vertice, only if its a leaf.
   * \pre not_valide_index
   * \pre not_leaf
   */
  virtual void SubdivideLeaf(vtkIdType index, unsigned int level) = 0;

  /**
   * Return if a vertice identified by index in tree as a terminal node.
   * For this, all childrens mus be all leaves.
   * \pre not_valid_index
   * \pre not_valid_child_index
   */
  virtual bool IsTerminalNode(vtkIdType index) const = 0;

  /**
   * Return the elder child index, local index node of first child, of
   * node, coarse cell, identified by index_parent.
   * \pre not_valid_index_parent
   * Public only for entry: vtkHyperTreeGridEntry,
   * vtkHyperTreeGridGeometryEntry, vtkHyperTreeGridGeometryLevelEntry
   */
  virtual vtkIdType GetElderChildIndex(unsigned int index_parent) const = 0;

  //@{
  /**
   * In an hypertree, all cells are the same size by level. This
   * fonction initializes this cache system is particulary used by
   * the symetric filter.
   * Here, you set a scales since extern description (sharing).
   */
  void SetScales(std::shared_ptr<vtkHyperTreeGridScales> scales) const { this->Scales = scales; }
  //@}

  //@{
  /**
   * Return the existence scales.
   */
  bool HasScales() const { return (this->Scales != nullptr); }
  //@}

  //@{
  /**
   * Return all scales.
   */
  std::shared_ptr<vtkHyperTreeGridScales> GetScales() const
  {
    assert(this->Scales != nullptr);
    return this->Scales;
  }
  //@}

protected:
  vtkHyperTree()
    : BranchFactor(2)
    , Dimension(3)
    , NumberOfChildren(8)
  {
  }

  virtual ~vtkHyperTree() override {}

  virtual void InitializePrivate() = 0;
  virtual void PrintSelfPrivate(ostream& os, vtkIndent indent) = 0;
  virtual void CopyStructurePrivate(vtkHyperTree* ht) = 0;

  //-- Global information

  // Branching factor of tree (2 or 3)
  unsigned char BranchFactor;

  // Dimension of tree (1, 2, or 3)
  unsigned char Dimension;

  // Number of children for coarse cell
  unsigned char NumberOfChildren;

  //-- Local information
  std::shared_ptr<vtkHyperTreeData> Datas;

  // Storage of pre-computed per-level cell scales
  // In hypertree grid, one description by hypertree.
  // In Uniform hypertree grid, one description by hypertree grid
  // (all cells, differents hypertree, are identicals by level).
  mutable std::shared_ptr<vtkHyperTreeGridScales> Scales;

private:
  vtkHyperTree(const vtkHyperTree&) = delete;
  void operator=(const vtkHyperTree&) = delete;
};

#endif
