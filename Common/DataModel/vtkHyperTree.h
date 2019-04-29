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
 * An hypertree is a dataset where each node has either exactly f^d
 * children or no child at all if the node is a leaf, where f in {2,3}
 * is the branching factor of the tree and d in {1,2,3} is the
 * dimension of the dataset.
 * Such trees have particular names when f=2: bintree (d=1), quadtree
 * (d=2), and octree (d=2). When f=3, we respectively call them
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
 * position and orientation). The geometry is then not limited to an hybercube
 * but can have a rectangular shape.
 * Attributes are associated with leaves. For LOD (Level-Of-Detail) purpose,
 * attributes can be computed on none-leaf nodes by computing the average
 * values from its children (which can be leaves or not).
 *
 * By construction, an hypertree is efficient in memory usage when the
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
 * JB This class was simplified and optimized (memory) by Jacques-Bernard Lekien 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
*/

#ifndef vtkHyperTree_h
#define vtkHyperTree_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include <cassert> // Used internally
#include <memory> // std::shared_ptr

class vtkHyperTreeGridScales;
//=============================================================================
struct vtkHyperTreeData
{
  // JB Index de Tree dans l'HyperTreeGrid
  vtkIdType TreeIndex;

  // Number of levels in tree
  unsigned int NumberOfLevels;

  // Number of nodes in tree
  vtkIdType NumberOfVertices;

  // Number of nodes (non-leaf vertices) in tree
  vtkIdType NumberOfNodes;

  // Offset for the global id mapping
  vtkIdType GlobalIndexStart;
};

//=============================================================================
class VTKCOMMONDATAMODEL_EXPORT vtkHyperTree : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTree, vtkObject);

  void PrintSelf( ostream& os, vtkIndent indent ) override;

  /**
   * Restore the initial state: only one node and one leaf: the root.
   */
  void Initialize( unsigned char branchFactor, unsigned char dimension, unsigned char numberOfChildren );

  /*
   * JB CopyStructure presque un shallow copy
   */
  void CopyStructure( vtkHyperTree* ht );

  /**
   * JB Return a freeze instance (a priori compact but potentially not changeable).
   * Le mode pourra permettre de proposer plusieurs instances freeze possible.
   */
  virtual vtkHyperTree* Freeze( const char* mode ) = 0;

  //@{
  /**
   * Set/Get tree index in HTG.
   * JB Ces services sont a usage interne.
   */
  void SetTreeIndex( vtkIdType treeIndex ) {
    this->Datas->TreeIndex = treeIndex;
  }
  vtkIdType GetTreeIndex() const {
    return this->Datas->TreeIndex;
  }
  //@}

  /**
   * Return the number of levels.
   */
  unsigned int GetNumberOfLevels() const
  {
    assert( "post: result_greater_or_equal_to_one" &&
            this->Datas->NumberOfLevels >= 1 );
    return this->Datas->NumberOfLevels;
  }

  /**
   * Return the number of all vertices in the tree.
   */
  vtkIdType GetNumberOfVertices() const
  {
    return this->Datas->NumberOfVertices;
  }

  /**
   * Return the number of nodes (non-leaf vertices) in the tree.
   */
  vtkIdType GetNumberOfNodes() const
  {
    return this->Datas->NumberOfNodes;
  }

  /**
   * Return the number of leaf vertices in the tree.
   */
  vtkIdType GetNumberOfLeaves() const
  {
    return this->Datas->NumberOfVertices - this->Datas->NumberOfNodes;
  }

  /**
   * Return the branch factor of the tree.
   */
  int GetBranchFactor() const
  {
    return this->BranchFactor;
  }

  /**
   * Return the dimension of the tree.
   */
  int GetDimension() const
  {
    return this->Dimension;
  }

  /**
   * Return the number of children per node of the tree.
   */
  vtkIdType GetNumberOfChildren() const
  {
    return this->NumberOfChildren;
  }

  //@{
  /**
   * JB Set/Get scale of the tree in each direction for the ground level (0).
   */
  void GetScale( double s[3] ) const;

  double GetScale( unsigned int d ) const;
  //@}

   /**
   * JB mailles par niveau. Il est possible de demander la reinitialisation de
   * JB ce systeme de cache ce qui est utilise dans le filtre de symetrisation.
   */
  std::shared_ptr<vtkHyperTreeGridScales> InitializeScales ( const double *scales, bool reinitialize = false ) const;

  /**
   * JB Return an instance of a implementation of a hypertree for given branch
   * JB factor and dimension.
   * JB This is done for permettre l'utilisation d'implementation differentes
   * JB de l'hypertree.
   * JB Une des possiblites offertes par cela, c'est qu'a partir
   * JB d'une indstance d'une implementation de l'hypertree en definir une autre
   * JB par l'appel a Frozen (par defaut, ce service ne fait rien), la nouvelle
   * JB instance se voulant soit plus compact soit plus rapide, voire les deux.
   */
  VTK_NEWINSTANCE
  static vtkHyperTree* CreateInstance( unsigned char branchFactor,
                                       unsigned char dimension );

  /**
   * JB Return the parent index node and this stat return by IsLeaf()
   * JB for this node the Index, Parent Index and IsLeaf() of the child
   * JB of a node in the hypertree.
   * JB L'existence de ce service est liee a celle de CreateInstance.
   */
  virtual void FindChildParameters(
    unsigned char ichild,
    vtkIdType& index_parent,
    bool &isLeaf ) = 0;

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
    return static_cast<unsigned int>( this->GetActualMemorySizeBytes() / 1024 );
  }

  /**
   * Set the start global index for the current tree.
   * The global index of a node will be this index + the node index.
   */
  void SetGlobalIndexStart( vtkIdType start )
  {
    this->Datas->GlobalIndexStart = start;
  }

  /**
   * Get the start global index for the current tree.
   * The global index of a node will be this index + the node index.
   */
  vtkIdType GetGlobalIndexStart( ) const
  {
    return this->Datas->GlobalIndexStart;
  }

  /**
   * JB Set the mapping between a local node identified by index
   * and a global id qui permet de recuperer la valeur du node correspondant
   * car ce global id est l'indice dans ce tableau qui decrit une grandeur.
   */
  virtual void SetGlobalIndexFromLocal( vtkIdType index, vtkIdType global ) = 0;

  /**
   * JB Get the global id of a local node identified by index.
   * Use the mapping function if available or the start global index.
   */
  virtual vtkIdType GetGlobalIndexFromLocal( vtkIdType index ) const = 0;

  /**
   * JB Returne la valeur maximale atteinte par l'index global.
   */
  virtual vtkIdType GetGlobalNodeIndexMax() const = 0;

  /**
   * JB Return if a local node identified by index is leaf.
   */
  virtual bool IsLeaf( vtkIdType index ) const = 0;

  /**
   * Subdivide node pointed by cursor, only if its a leaf.
   * At the end, cursor points on the node that used to be leaf.
   * \pre leaf_exists: leaf!=0
   * \pre is_a_leaf: leaf->CurrentIsLeaf()
   */
  virtual void SubdivideLeaf( vtkIdType index, unsigned int level ) = 0;

  /**
   * JB Return if a local node identified by index is a terminal node,
   * c'est-a-dire que this coarse node a toutes ses filles qui sont
   * des feuilles.
   */
  virtual bool IsTerminalNode( vtkIdType index ) const = 0;

  /**
   * JB Return the local index node du premier noeud fille d'un
   * parent node identified by index_parent.
   */
  virtual vtkIdType GetElderChildIndex( unsigned int index_parent ) const = 0;

  //@{
  /**
   * JB
   */
  void SetScales ( std::shared_ptr<vtkHyperTreeGridScales> scales ) const {
    this->Scales = scales;
  }
  //@}

  //@{
  /**
   * JB
   */
  bool HasScales ( ) const {
    return ( this->Scales != nullptr );
  }
  //@}

  //@{
  /**
   * JB
   */
  std::shared_ptr<vtkHyperTreeGridScales> GetScales ( ) const {
    assert( this->Scales != nullptr );
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

  virtual void InitializePrivate() = 0;
  virtual void PrintSelfPrivate( ostream& os, vtkIndent indent ) = 0;
  virtual void CopyStructurePrivate( vtkHyperTree* ht ) = 0;

  //-- Global information

  // Branching factor of tree (2 or 3)
  unsigned char BranchFactor;

  // Dimension of tree (1, 2, or 3)
  unsigned char Dimension;

  // Number of children for coarse cell
  unsigned char NumberOfChildren;

  //-- Local information
  std::shared_ptr< vtkHyperTreeData > Datas;

  // JB Storage of pre-computed per-level cell scales
  mutable std::shared_ptr<vtkHyperTreeGridScales> Scales;

private:
  vtkHyperTree(const vtkHyperTree&) = delete;
  void operator=(const vtkHyperTree&) = delete;
};

#endif
