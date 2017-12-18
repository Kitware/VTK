/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGrid.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTree.h"

#include "vtkHyperTreeCursor.h"
#include "vtkObjectFactory.h"

#include <deque>
#include <vector>

#include <cassert>

// The template value N describes the number of children for binary and
// ternary trees in 1, 2, or 3 dimensions.
// N thus has the following valid values:
//   2 for a binary tree in 1D (bin-tree)
//   3 for a ternary tree in 1D (tri-tree)
//   4 for a binary tree in 2D (quad-tree or quadtree)
//   8 for a binary tree in 3D (oct-tree or octree)
//   9 for a ternary tree in 2D (9-tree)
//  27 for a ternary tree in 3D (27-tree)
template<int N> class vtkCompactHyperTree;

// A class to represent true nodes (i.e., non-leaf vertices) in a compact
// hyper tree templated by N.
template<int N> class vtkCompactHyperTreeNode;

//=============================================================================
template<int N> class vtkCompactHyperTreeCursor : public vtkHyperTreeCursor
{
public:
  //---------------------------------------------------------------------------
  vtkTemplateTypeMacro(vtkCompactHyperTreeCursor<N>, vtkHyperTreeCursor);

  //---------------------------------------------------------------------------
  static vtkCompactHyperTreeCursor<N>* New();

  //---------------------------------------------------------------------------
  void SetTree( vtkHyperTree* tree ) override
  {
    this->Tree = vtkCompactHyperTree<N>::SafeDownCast( tree );
  }

  //---------------------------------------------------------------------------
  vtkHyperTree* GetTree() override
  {
    return this->Tree;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetVertexId() override
  {
    return this->Index;
  }

  //---------------------------------------------------------------------------
  bool IsLeaf() override
  {
    return this->Leaf;
  }

  //---------------------------------------------------------------------------
  bool IsRoot() override
  {
    // No special null cursor exists with this object
    return ( this->Index == 0 );
  }

  //---------------------------------------------------------------------------
  unsigned int GetLevel() override
  {
    return static_cast<unsigned int>( this->ChildHistory.size() );
  }

  //---------------------------------------------------------------------------
  int GetChildIndex() override
  {
    assert( "post: valid_range" && this->ChildIndex >= 0 &&
      this->ChildIndex < GetNumberOfChildren() );
    return this->ChildIndex;
  }

  //---------------------------------------------------------------------------
  void ToRoot() override
  {
    // No special null cursor exists with this object
    this->Index = 0;

    // Clear child history
    this->ChildHistory.clear();
    this->Leaf = ( this->Tree->GetNumberOfVertices() == 1 );
    this->ChildIndex = 0;
    memset( this->Indices, 0, 3 * sizeof(int) );
  }

  //---------------------------------------------------------------------------
  void ToParent() override
  {
    assert( "pre: not_root" && ! IsRoot() );

    // Move one level up
    this->Index = this->Tree->GetParentIndex( this->Index );
    this->ChildIndex = this->ChildHistory.back();
    this->ChildHistory.pop_back();

    // Cursor can no longer be at a leaf
    this->Leaf = false;

    for ( unsigned int i = 0; i < this->Dimension;  ++ i )
    {
      this->Indices[i] = this->Indices[i] / this->Tree->GetBranchFactor();
    }
  }

  //---------------------------------------------------------------------------
  void ToChild( int child ) override
  {
    assert( "pre: not_leaf" && ! this->IsLeaf() );
    assert( "pre: valid_child" && child >= 0
      && child < this->GetNumberOfChildren() );

    vtkCompactHyperTreeNode<N>* node = this->Tree->GetNode( this->Index );
    this->ChildHistory.push_back( this->ChildIndex );
    this->ChildIndex = child;
    this->Index = node->GetChild( child );
    this->Leaf = node->IsChildLeaf( child );

    int tmpChild = child;
    int branchFactor = this->Tree->GetBranchFactor();
    for ( unsigned int i = 0; i < this->Dimension; ++ i )
    {
      // Effectively convert child to base 2/3 (branch factor)
      int tmp = tmpChild;
      tmpChild /= branchFactor;
      int index = tmp - ( branchFactor * tmpChild ); // Remainder (mod)
      assert( "check: mod 3 value" && index >= 0 && index < branchFactor );
      this->Indices[i] = ( this->Indices[i] * branchFactor ) + index;
    }
  }

  //---------------------------------------------------------------------------
  void ToSameVertex( vtkHyperTreeCursor* other ) override
  {
    assert( "pre: other_exists" && other != nullptr );
    assert( "pre: same_hyperTree" && this->SameTree( other ) );

    vtkCompactHyperTreeCursor<N>* o
      = static_cast<vtkCompactHyperTreeCursor<N> *>( other );

    this->Index = o->Index;
    this->ChildIndex = o->ChildIndex;
    this->Leaf = o->Leaf;
    this->ChildHistory = o->ChildHistory; // use assignment operator
    memcpy( this->Indices, o->Indices, 3 * sizeof( int ) );

    assert( "post: equal" && this->IsEqual( other ) );
  }

  //--------------------------------------------------------------------------
  bool IsEqual( vtkHyperTreeCursor* other ) override
  {
    assert( "pre: other_exists" && other != nullptr );
    assert( "pre: same_hyperTree" && this->SameTree(other) );

    vtkCompactHyperTreeCursor<N>* o
      = static_cast<vtkCompactHyperTreeCursor<N> *>( other );

    bool result = this->Index == o->Index
      && this->ChildIndex == o->ChildIndex
      && this->Leaf == o->Leaf
      && this->ChildHistory == o->ChildHistory;

    for ( unsigned int i = 0; result && i < this->Dimension; ++ i )
    {
      result = ( this->Indices[i] == o->Indices[i] );
    }
    return result;
  }

  //--------------------------------------------------------------------------
  vtkHyperTreeCursor* Clone() override
  {
    vtkCompactHyperTreeCursor<N>* result = this->NewInstance();
    assert( "post: results_exists" && result != nullptr );
    result->Tree = this->Tree;
    assert( "post: same_tree" && result->SameTree( this ) );
    return result;
  }

  //---------------------------------------------------------------------------
  int SameTree( vtkHyperTreeCursor* other ) override
  {
    assert( "pre: other_exists" && other != nullptr );
    vtkCompactHyperTreeCursor<N> *o =
      vtkCompactHyperTreeCursor<N>::SafeDownCast( other );
    return o != nullptr && this->Tree == o->Tree;
  }

  //---------------------------------------------------------------------------
  int GetNumberOfChildren() override
  {
    return N;
  }

  //---------------------------------------------------------------------------
  int GetDimension() override
  {
    assert( "post: positive_result " && this->Dimension > 0 );
    assert( "post: up_to_3 " && this->Dimension <= 3 ); // and then
    return this->Dimension;
  }

  //---------------------------------------------------------------------------
  // NB: Public only for the vtkCompactHyperTreeCursor.
  void SetIsLeaf( bool value )
  {
    this->Leaf = value;
  }

  //---------------------------------------------------------------------------
  // NB: Public only for the vtkCompactHyperTreeCursor.
  void SetChildIndex( int childIndex )
  {
    assert( "pre: valid_range" && childIndex >= 0
      && childIndex < GetNumberOfChildren() );
    this->ChildIndex = childIndex;
    assert( "post: is_set" && childIndex==GetChildIndex() );
  }

  //---------------------------------------------------------------------------
  // NB: Public only for the vtkCompactHyperTreeCursor.
  void SetIndex( vtkIdType index )
  {
    assert( "pre: positive_index" && index >= 0 );
    this->Index = index;
  }

  //---------------------------------------------------------------------------
  void PrintSelf( ostream& os, vtkIndent indent ) override
  {
    this->Superclass::PrintSelf( os, indent );

    os << indent << "Index=" << this->Index << endl;
    os << indent << "Leaf: " << ( this->Leaf ? "true" : "false" ) << endl;
    os << indent << "ChildIndex=" << this->ChildIndex << endl;

    os << indent << "Indices:"
       << this->Indices[0] <<","
       << this->Indices[1] <<","
       << this->Indices[2] << endl;

    os << indent << "ChildHistory:" << endl;
    for ( unsigned int i = 0; i < this->ChildHistory.size(); ++ i )
    {
      os << this->ChildHistory[i] << " ";
    }
    os << endl;
  }

  //---------------------------------------------------------------------------
protected:
  vtkCompactHyperTreeCursor()
  {
    switch ( N )
    {
      case 2:
      case 3:
        this->Dimension = 1;
        break;
      case 4:
      case 9:
        this->Dimension = 2;
        break;
      case 8:
      case 27:
        this->Dimension = 3;
        break;
      default:
        this->Dimension = 0;
        assert( "Bad number of children" && this->Dimension == 0 );
    }
    this->Tree = nullptr;
    this->Index = 0;
    this->Leaf  = false;
    this->ChildIndex = 0;
    memset( this->Indices, 0, 3 * sizeof( int ) );
  }

  // Tree to which the cursor is attached
  vtkCompactHyperTree<N>* Tree;

  // Dimension of tree to which the cursor is attached
  unsigned char Dimension;

  // Index either in the nodes or parent (if leaf)
  vtkIdType Index;

  // Number of current node as a child
  int ChildIndex;

  // Is center of cursor at a leaf?
  bool Leaf;

  // A stack, but stack does not have clear()
  std::deque<int> ChildHistory;

  // Index in each dimension of the current node, as if the tree at the current
  // level were a uniform grid.
  // NB: Default to 3 dimensions, use only those needed
  int Indices[3];

private:
  vtkCompactHyperTreeCursor(const vtkCompactHyperTreeCursor<N> &) = delete;
  void operator=(const vtkCompactHyperTreeCursor<N> &) = delete;
};
//-----------------------------------------------------------------------------
template<int N>
vtkStandardNewMacro(vtkCompactHyperTreeCursor<N>);
//=============================================================================

//=============================================================================
// Description:
// A class to hide the specifics of leaf flags encoding
class vtkHyperTreeLeafFlags
{
public:
  vtkHyperTreeLeafFlags()
  {
    // Unused bits are set to 1 (by default all children are leaves).
    for ( int i = 0; i < 4; ++ i )
    {
      this->Flags[i] = 0xFF;
    }
  }

  //---------------------------------------------------------------------------
  void SetLeafFlag( int idx, bool val )
  {
    assert( "Valid child idx" && idx >= 0 && idx < 32 );
    int i = 0;
    while ( idx >= 8 )
    {
      ++ i;
      idx -= 8;
    }
    unsigned char mask = 1 << idx;
    this->Flags[i] = val ?
      ( this->Flags[i] | mask ) : ( this->Flags[i] & (mask ^ 0xFF) );
  }

  //---------------------------------------------------------------------------
  bool GetLeafFlag( int idx )
  {
    assert( "Valid child idx" && idx >= 0 && idx < 32 );

    // Retrieve to which flag corresponds the index
    div_t d = div( idx, 8 );

    // Place mask at index position in flag
    unsigned char mask = 1 << d.rem;

    // Decide whether child bit is present
    return ( mask & this->Flags[d.quot] ) == mask;
  }

  //---------------------------------------------------------------------------
  void PrintSelf( ostream& os, int numChildren )
  {
    assert( "Number of children" && numChildren >= 0 && numChildren < 32 );
    int childIdx = 0;
    int byteIdx = 0;
    unsigned char mask = 1;
    while ( childIdx < numChildren )
    {
      os << ( ( this->Flags[byteIdx] & mask ) == mask );
      ++ childIdx;
      if ( mask == 128 )
      {
        mask = 1;
        ++ byteIdx;
      }
      else
      {
        mask <<= 1;
      }
    }
    os << endl;
  }

private:
  unsigned char Flags[4];
};

//=============================================================================
// Description:
// A node is a vertex of a tree which is not a leaf.
// N thus has the following valid values:
//   2 for a binary tree in 1D (bin-tree)
//   3 for a ternary tree in 1D (tri-tree)
//   4 for a binary tree in 2D (quad-tree or quadtree)
//   8 for a binary tree in 3D (oct-tree or octree)
//   9 for a ternary tree in 2D (9-tree)
//  27 for a ternary tree in 3D (27-tree)
template<int N> class vtkCompactHyperTreeNode
{
public:
  //---------------------------------------------------------------------------
  // Description:
  // See GetParent().
  void SetParent( vtkIdType parent )
  {
    assert( "pre: positive_parent" && parent >= 0 );
    this->Parent = parent;
    assert( "post: is_set" && parent == this->GetParent() );
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index of the parent node of the current node in the
  // nodes array of the hyperTree.
  vtkIdType GetParent()
  {
    assert( "post: positive_result" && this->Parent >= 0 );
    return this->Parent;
  }

  //---------------------------------------------------------------------------
  // Description:
  // See GetLeafFlags()
  void SetLeafFlag( vtkIdType childIdx, bool flag )
  {
    this->LeafFlags.SetLeafFlag( childIdx, flag );
  }

  //---------------------------------------------------------------------------
  // Description:
  // Is the `i'-th child of the node a leaf ?
  bool IsChildLeaf( int i )
  {
    assert( "pre: valid_range" && i >= 0 && i < N );
    return this->LeafFlags.GetLeafFlag( i );
  }

  //---------------------------------------------------------------------------
  // Description:
  // See GetChild().
  void SetChild( int i, int child )
  {
    assert( "pre: valid_range" && i >= 0 && i < N);
    assert( "pre: positive_child" && child >= 0 );
    this->Children[i] = child;
    assert( "post: is_set" && child == this->GetChild( i ) );
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index of of the 'i'-th child. If the result of
  // IsChildLeaf( i ) is true, the index points to an element in the ParentIndex
  // and Attribute arrays of the HyperTree class. If not, the index points to
  // an element in the Nodes array of the HyperTree class.
  int GetChild( int i )
  {
    assert( "pre: valid_range" && i >= 0 && i < N);
    assert( "post: positive_result" && this->Children[i] >= 0 );
    return this->Children[i];
  }

  //---------------------------------------------------------------------------
  void PrintSelf( ostream& os, vtkIndent indent )
  {
    os << indent << "Parent=" << this->Parent << endl;
    os << indent << "LeafFlags:";
    this->LeafFlags.PrintSelf( os, N );

    os << indent << "Children:";
    for( int i = 0; i < N; ++ i )
    {
      os << " " << this->Children[i];
    }
    os << indent << endl;
  }

protected:
  //---------------------------------------------------------------------------
  vtkIdType Parent; // index
  vtkHyperTreeLeafFlags LeafFlags;
  vtkIdType Children[N];
};

//=============================================================================
template<int N> class vtkCompactHyperTree : public vtkHyperTree
{
public:
  vtkTemplateTypeMacro(vtkCompactHyperTree<N>,vtkHyperTree);

  //---------------------------------------------------------------------------
  static vtkCompactHyperTree<N>* New();

  //---------------------------------------------------------------------------
  void PrintSelf( ostream& os, vtkIndent indent ) override
  {
    this->Superclass::PrintSelf( os, indent );

    os << indent << "Dimension=" << this->Dimension << endl;
    os << indent << "BranchFactor=" << this->BranchFactor << endl;

    os << indent << "Scale: "
       << this->Scale[0] <<","
       << this->Scale[1] <<","
       << this->Scale[2] << endl;

    os << indent << "NumberOfLevels=" << this->NumberOfLevels << endl;
    os << indent << "NumberOfNodes=" << this->NumberOfNodes << endl;

    os << indent << "Nodes (size=" << this->Nodes.size() << "):" << endl;
    for ( unsigned int i = 0; i < this->Nodes.size(); ++ i )
    {
      this->Nodes[i].PrintSelf( os, indent.GetNextIndent() );
    }

    os << indent << "ParentIndex (size=" << this->ParentIndex.size() << "):" << endl;
    for ( unsigned int i = 0; i < this->ParentIndex.size(); ++ i )
    {
      os << " " << this->ParentIndex[i];
    }
    os << endl;

    os << indent << "GlobalIndexStart=" << this->GlobalIndexStart << endl;
    os << indent << "GlobalIndexTable:";
    for ( unsigned int i = 0; i < this->GlobalIndexTable.size(); ++ i )
    {
      os << " " << this->GlobalIndexTable[i];
    }
    os << endl;
  }

  //---------------------------------------------------------------------------
  void Initialize() override
  {
    this->Nodes.resize( 1 );
    this->Nodes[0].SetParent( 0 );
    for ( unsigned int i = 0; i < N; ++ i )
    {
      // It is assumed that the root is a special node with only one child.
      // The other children flags are irrelevant, but set them as nodes for
      // no good reason.
      this->Nodes[0].SetLeafFlag( i, i == 0 ); // First child is a leaf
      this->Nodes[0].SetChild( i, 0 );
    }
    this->ParentIndex.resize( 1 );
    this->ParentIndex[0] = 0;
    this->NumberOfLevels = 1;
    this->NumberOfNodes = 0;
    this->GlobalIndexTable.clear();
    this->GlobalIndexStart = 0;
  }

  //---------------------------------------------------------------------------
  vtkHyperTreeCursor* NewCursor() override
  {
    vtkCompactHyperTreeCursor<N>* result = vtkCompactHyperTreeCursor<N>::New();
    result->SetTree( this );

    return result;
  }

  //---------------------------------------------------------------------------
  ~vtkCompactHyperTree() override
  {
  }

  //---------------------------------------------------------------------------
  int GetBranchFactor() override
  {
    return this->BranchFactor;
  }

  //---------------------------------------------------------------------------
  int GetDimension() override
  {
    return this->Dimension;
  }

  //---------------------------------------------------------------------------
  void SetScale( double s[3] ) override
  {
    memcpy( this->Scale, s, 3 * sizeof( double ) );
  }

  //---------------------------------------------------------------------------
  void GetScale( double s[3] ) override
  {
    memcpy( s, this->Scale, 3 * sizeof( double ) );
  }

  //---------------------------------------------------------------------------
  double GetScale( unsigned int d ) override
  {
    return this->Scale[d];
  }

  //---------------------------------------------------------------------------
  vtkIdType GetNumberOfLevels() override
  {
    return this->NumberOfLevels;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetNumberOfChildren() override
  {
    return N;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetNumberOfVertices() override
  {
    return static_cast<vtkIdType>( this->ParentIndex.size() );
  }

  //---------------------------------------------------------------------------
  vtkIdType GetNumberOfNodes() override
  {
    return this->NumberOfNodes;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetNumberOfLeaves() override
  {
    return static_cast<vtkIdType>( this->ParentIndex.size() ) - this->NumberOfNodes;
  }

  //---------------------------------------------------------------------------
  void SetGlobalIndexStart( vtkIdType start ) override
  {
    this->GlobalIndexStart = start;
  }

  //---------------------------------------------------------------------------
  void SetGlobalIndexFromLocal( vtkIdType local, vtkIdType global ) override
  {
    // If local index outside map range, resize the latter
    if ( static_cast<vtkIdType>( this->GlobalIndexTable.size() ) <= local )
    {
      this->GlobalIndexTable.resize( local + 1 );
    }

    // Assign map value at local key with given global index
    this->GlobalIndexTable[local] = global;

    // Root node is special and has only one child
    if ( ! local && this->ParentIndex.size() == 1 )
    {
      // Assign global index to node with local index 1
      this->SetGlobalIndexFromLocal( 1, global );
    }
  }

  //---------------------------------------------------------------------------
  vtkIdType GetGlobalIndexFromLocal( vtkIdType local ) override
  {
    // If local index outside map range, return global index start + local
    return ( local < static_cast<vtkIdType>( this->GlobalIndexTable.size() ) ) ?
      this->GlobalIndexTable[local] : ( this->GlobalIndexStart + local );
  }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperTreeCursor
  vtkCompactHyperTreeNode<N>* GetNode( int nodeIdx )
  {
    assert( "pre: valid_range" &&
            nodeIdx >= 0 &&
            static_cast<size_t>(nodeIdx) < this->Nodes.size() );

    return &this->Nodes[nodeIdx];
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index of the parent vertex
  int GetParentIndex( int leafIdx )
  {
    assert( "pre: valid_range" &&
            leafIdx >= 0 &&
            leafIdx < this->GetNumberOfVertices() );
    assert( "post: valid_result" &&
            this->ParentIndex[leafIdx] >= 0 &&
            static_cast<size_t>(this->ParentIndex[leafIdx]) < this->Nodes.size() );

    return this->ParentIndex[leafIdx];
  }

  //---------------------------------------------------------------------------
  void SubdivideLeaf( vtkHyperTreeCursor* leafCursor ) override
  {
    assert( "pre: leaf_exists" && leafCursor != nullptr );
    assert( "pre: is_a_leaf" && leafCursor->IsLeaf() );

    // Instantiate a vtkCompactHyperTreeCursor
    vtkCompactHyperTreeCursor<N>* cursor
      = static_cast<vtkCompactHyperTreeCursor<N> *>( leafCursor );

    // The leaf becomes a node and is not anymore a leaf
    cursor->SetIsLeaf( false );

    // Retrieve node index
    vtkIdType nodeIndex = cursor->GetVertexId();

    // Increase storage size if needed for new node
    if ( this->Nodes.size() <= static_cast<size_t>(nodeIndex) )
    {
      this->Nodes.resize( nodeIndex + 1 );
    }

    // Set parent index of new node
    vtkIdType parentNodeIdx = this->ParentIndex[nodeIndex];
    this->Nodes[nodeIndex].SetParent( parentNodeIdx );

    // Change the parent leaf flags: it has one less child as a leaf
    vtkCompactHyperTreeNode<N>& parent = this->Nodes[parentNodeIdx];
    int idx = cursor->GetChildIndex();
    parent.SetLeafFlag( idx, false );
    parent.SetChild( idx, static_cast<int>( nodeIndex ) );

    // Recycle the index of deleted leaf as it becomes a node.
    ++ this->NumberOfNodes;
    size_t nextLeaf = this->ParentIndex.size();
    this->ParentIndex.resize( nextLeaf + N );
    for ( int i = 0; i < N; ++ i, ++ nextLeaf )
    {
      this->Nodes[nodeIndex].SetChild( i, static_cast<int>( nextLeaf ) );
      this->ParentIndex[nextLeaf] = nodeIndex;

      // Nodes get constructed with leaf flags set to 1.
      this->Nodes[nodeIndex].SetLeafFlag( i, true );
    } // i

    // Update the number of leaves per level.
    vtkIdType level = cursor->GetLevel();

    // Add the new leaves to the number of leaves at the next level.
    if ( level + 1 == this->NumberOfLevels )
    {
      // We have a new level.
      ++ this->NumberOfLevels;
    }
  }

  //---------------------------------------------------------------------------
  unsigned int GetActualMemorySize() override
  {
    size_t size = sizeof( vtkIdType ) * this->GetNumberOfVertices()
      + sizeof( vtkCompactHyperTreeNode<N> ) * this->GetNumberOfNodes()
      + sizeof( vtkIdType ) * this->GlobalIndexTable.size();

    // Compute memory used in kibibytes (1024 bytes)
    unsigned int mem = static_cast<unsigned int>( size / 1024 );

    // Return upper bound of 1 kiB if footprint is smaller
    return ( mem ? mem : 1 );
  }

protected:
  //---------------------------------------------------------------------------
  vtkCompactHyperTree()
  {
    // Set tree parameters depending on template parameter value
    switch ( N )
    {
      case 2:
        this->BranchFactor = 2;
        this->Dimension = 1;
        break;
      case 3:
        this->BranchFactor = 3;
        this->Dimension = 1;
        break;
      case 4:
        this->BranchFactor = 2;
        this->Dimension = 2;
        break;
      case 8:
        this->BranchFactor = 2;
        this->Dimension = 3;
        break;
      case 9:
        this->BranchFactor = 3;
        this->Dimension = 2;
        break;
      case 27:
        this->BranchFactor = 3;
        this->Dimension = 3;
        break;
    } // switch ( N )

    // Set default X, Y, and Z scales
    for ( unsigned int i = 0; i < 3; ++ i )
    {
      this->Scale[i] = 1.;
    }

    // Initialize: the tree has only one node and one leaf: the root.
    this->Initialize();

  }

  // Branching factor of tree (2 or 3)
  int BranchFactor;

  // Dimension of tree (1, 2, or 3)
  int Dimension;

  // X, Y, and Z scales of tree
  double Scale[3];

  // Number of levels in tree
  vtkIdType NumberOfLevels;

  // Number of nodes (non-leaf vertices) in tree
  vtkIdType NumberOfNodes;

  // Offset for the global id mapping
  vtkIdType GlobalIndexStart;

  // Storage for non-leaf tree nodes
  std::vector<vtkCompactHyperTreeNode<N> > Nodes;

  // Storage to record the parent of each tree vertex
  std::vector<vtkIdType> ParentIndex;

  // Storage to record the local to global id mapping
  std::vector<vtkIdType> GlobalIndexTable;

private:
  vtkCompactHyperTree(const vtkCompactHyperTree<N> &) = delete;
  void operator=(const vtkCompactHyperTree<N> &) = delete;
};
//-----------------------------------------------------------------------------
template<int N>
vtkStandardNewMacro(vtkCompactHyperTree<N>);
//=============================================================================

//-----------------------------------------------------------------------------
void vtkHyperTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTree::CreateInstance( unsigned int factor,
                                            unsigned int dimension )
{
  switch ( factor )
  {
    case 2:
      switch ( dimension )
      {
        case 3:
          return vtkCompactHyperTree<8>::New();
        case 2:
          return vtkCompactHyperTree<4>::New();
        case 1:
          return vtkCompactHyperTree<2>::New();
        default:
          vtkGenericWarningMacro( "Bad dimension " << dimension );
      }
      break;
    case 3:
      switch ( dimension )
      {
        case 3:
          return vtkCompactHyperTree<27>::New();
        case 2:
          return vtkCompactHyperTree<9>::New();
        case 1:
          return vtkCompactHyperTree<3>::New();
        default:
          vtkGenericWarningMacro( "Bad dimension " << dimension );
      }
      break;
    default:
      vtkGenericWarningMacro( "Bad branching factor " << factor );
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void vtkHyperTree::FindChildParameters( int child, vtkIdType& index, bool& isLeaf )
{
#define vtkHyperTreeGetNodeParameterMacro( _N_ )                                   \
  {                                                                                \
  vtkCompactHyperTree<_N_>* tree = static_cast<vtkCompactHyperTree<_N_>*>( this ); \
  vtkCompactHyperTreeNode<_N_>* node = tree->GetNode( static_cast<int>( index ) ); \
  index = node->GetChild( child );                                                 \
  isLeaf = node->IsChildLeaf( child );                                             \
  return;                                                                          \
  }

  switch ( this->GetDimension() )
  {
    case 3:
      switch ( this->GetBranchFactor() )
      {
        case 2: vtkHyperTreeGetNodeParameterMacro( 8 );
        case 3: vtkHyperTreeGetNodeParameterMacro( 27 );
      } // case 3
      break;
    case 2:
      switch ( this->GetBranchFactor() )
      {
        case 2: vtkHyperTreeGetNodeParameterMacro( 4 );
        case 3: vtkHyperTreeGetNodeParameterMacro( 9 );
      } // case 2
      break;
    case 1:
      switch ( this->GetBranchFactor() )
      {
        case 2: vtkHyperTreeGetNodeParameterMacro( 2 );
        case 3: vtkHyperTreeGetNodeParameterMacro( 3 );
      } // case 1
      break;
  }

  vtkGenericWarningMacro( "Bad branching factor " << this->GetBranchFactor() );

#undef vtkHyperTreeGetNodeParameterMacro
}

//-----------------------------------------------------------------------------
void vtkHyperTree::FindParentIndex( vtkIdType& index )
{
#define vtkHyperTreeGetParentIndexMacro( _N_ )                                     \
  {                                                                                \
  vtkCompactHyperTree<_N_>* tree = static_cast<vtkCompactHyperTree<_N_>*>( this ); \
  vtkCompactHyperTreeNode<_N_>* node = tree->GetNode( static_cast<int>( index));   \
  index = node->GetParent();                                                       \
  return;                                                                          \
  }

  switch ( this->GetDimension() )
  {
    case 3:
      switch ( this->GetBranchFactor() )
      {
        case 2: vtkHyperTreeGetParentIndexMacro( 8 );
        case 3: vtkHyperTreeGetParentIndexMacro( 27 );
      } // case 3
      break;
    case 2:
      switch ( this->GetBranchFactor() )
      {
        case 2: vtkHyperTreeGetParentIndexMacro( 4 );
        case 3: vtkHyperTreeGetParentIndexMacro( 9 );
      } // case 2
      break;
    case 1:
      switch ( this->GetBranchFactor() )
      {
        case 2: vtkHyperTreeGetParentIndexMacro( 2 );
        case 3: vtkHyperTreeGetParentIndexMacro( 3 );
      } // case 1
      break;
  }

  vtkGenericWarningMacro( "Bad branching factor " << this->GetBranchFactor() );

#undef vtkHyperTreeGetParentIndexMacro
}
