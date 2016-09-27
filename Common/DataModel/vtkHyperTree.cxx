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
#include <map>

#include <cassert>

// Description:
// The template value N describes the number of children to binary and
// ternary trees.
template<int N> class vtkCompactHyperTree;
template<int N> class vtkCompactHyperTreeNode;

template<int N> class vtkCompactHyperTreeCursor : public vtkHyperTreeCursor
{
public:
  //---------------------------------------------------------------------------
  vtkTypeMacro(vtkCompactHyperTreeCursor<N>, vtkHyperTreeCursor);

  static vtkCompactHyperTreeCursor<N>* New()
  {
    vtkCompactHyperTreeCursor<N> *ret = new vtkCompactHyperTreeCursor<N>;
    ret->InitializeObjectBase();
    return ret;
  }

  //---------------------------------------------------------------------------
  // Initialization
  virtual void SetTree( vtkCompactHyperTree<N>* tree )
  {
    this->Tree = tree;
  }

   //---------------------------------------------------------------------------
  // Initialization
  vtkHyperTree* GetTree() VTK_OVERRIDE
  {
    return this->Tree;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetLeafId() VTK_OVERRIDE
  {
    assert( "pre: is_leaf" && IsLeaf() );
    return this->Index;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetNodeId() VTK_OVERRIDE
  {
    return this->Index;
  }

  //---------------------------------------------------------------------------
  bool IsLeaf() VTK_OVERRIDE
  {
    return this->Leaf;
  }

  //---------------------------------------------------------------------------
  bool IsTerminalNode() VTK_OVERRIDE
  {
    bool result = ! this->Leaf;
    if( result )
    {
      vtkCompactHyperTreeNode<N>* node = this->Tree->GetNode( this->Index );
      result = node->IsTerminalNode();
    }
    // A=>B: notA or B
    assert( "post: compatible" && ( ! result || ! this->Leaf) );
    return result;
  }

  //---------------------------------------------------------------------------
  bool IsRoot() VTK_OVERRIDE
  {
    return ( ! this->Leaf && this->Index == 0 )
      || ( this->Leaf && ! this->Index && this->Tree->GetLeafParentSize() == 1 );
  }

  //---------------------------------------------------------------------------
  int GetCurrentLevel() VTK_OVERRIDE
  {
    int result = this->GetChildHistorySize();
    assert( "post: positive_result" && result >= 0 );
    return result;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the child number of the current node relative to its parent.
  // \pre not_root: !IsRoot().
  // \post valid_range: result >= 0 && result<GetNumberOfChildren()
  int GetChildIndex() VTK_OVERRIDE
  {
    assert( "post: valid_range" && this->ChildIndex >= 0 &&
      this->ChildIndex < GetNumberOfChildren() );
    return this->ChildIndex;
  }

  //---------------------------------------------------------------------------
  // Cursor movement.
  // \pre can be root
  // \post is_root: IsRoot()
  void ToRoot() VTK_OVERRIDE
  {
    this->ChildHistory.clear();
    this->Leaf = ( this->Tree->GetLeafParentSize() == 1 );
    this->Index = this->Leaf ? 0 : 1;
    this->ChildIndex = 0;
    memset( this->Indices, 0, 3 * sizeof(int) );
  }

  //---------------------------------------------------------------------------
  // \pre not_root: !IsRoot()
  void ToParent() VTK_OVERRIDE
  {
    assert( "pre: not_root" && !IsRoot() );
    this->Index = this->Leaf ?
      this->Tree->GetLeafParent( this->Index ) :
      this->Tree->GetNode( this->Index )->GetParent();

    this->Leaf = false;
    this->ChildIndex = this->ChildHistory.back(); // top()
    this->ChildHistory.pop_back();

    for ( unsigned int i = 0; i < this->Dimension;  ++ i )
    {
      this->Indices[i] = this->Indices[i] / this->Tree->GetBranchFactor();
    }
  }

  //---------------------------------------------------------------------------
  // \pre not_leaf: !IsLeaf()
  // \pre valid_child: child >= 0 && child<this->GetNumberOfChildren()
  void ToChild( int child ) VTK_OVERRIDE
  {
    assert( "pre: not_leaf" && !IsLeaf() );
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
      assert( "check: mod 3 value" && index >= 0 && index<branchFactor);
      this->Indices[i] = ( this->Indices[i] * branchFactor ) + index;
    }
  }

  //---------------------------------------------------------------------------
  // Description:
  // Move the cursor to the same node pointed by `other'.
  // \pre other_exists: other != 0
  // \pre same_hyperTree: this->SameTree( other )
  // \post equal: this->IsEqual( other )
  void ToSameNode( vtkHyperTreeCursor* other ) VTK_OVERRIDE
  {
    assert( "pre: other_exists" && other != 0 );
    assert( "pre: same_hyperTree" && this->SameTree( other ) );

    vtkCompactHyperTreeCursor<N> *o =
      static_cast<vtkCompactHyperTreeCursor<N> *>( other );

    this->Index = o->Index;
    this->ChildIndex = o->ChildIndex;
    this->Leaf = o->Leaf;
    this->ChildHistory = o->ChildHistory; // use assignment operator
    memcpy( this->Indices, o->Indices, 3 * sizeof(int) );

    assert( "post: equal" && this->IsEqual(other) );
  }

  //--------------------------------------------------------------------------
  // Description:
  // Is `this' equal to `other'?
  // \pre other_exists: other != 0
  // \pre same_hyperTree: this->SameTree(other);
  bool IsEqual( vtkHyperTreeCursor* other ) VTK_OVERRIDE
  {
    assert( "pre: other_exists" && other != 0 );
    assert( "pre: same_hyperTree" && this->SameTree(other) );

    vtkCompactHyperTreeCursor<N>* o =
      static_cast<vtkCompactHyperTreeCursor<N> *>( other );

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
  vtkHyperTreeCursor* Clone() VTK_OVERRIDE
  {
    vtkCompactHyperTreeCursor<N>* result = this->NewInstance();
    assert( "post: results_exists" && result != 0 );
    result->Tree = this->Tree;
    assert( "post: same_tree" && result->SameTree( this ) );
    return result;
  }

  //---------------------------------------------------------------------------
  int SameTree( vtkHyperTreeCursor* other ) VTK_OVERRIDE
  {
    assert( "pre: other_exists" && other != 0 );
    vtkCompactHyperTreeCursor<N> *o =
      vtkCompactHyperTreeCursor<N>::SafeDownCast( other );
    return o != 0 && this->Tree == o->Tree;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index in dimension `d', as if the node was a cell of a
  // uniform grid of 1<<GetCurrentLevel() cells in each dimension.
  // \pre valid_range: d >= 0 && d<GetDimension()
  // \post valid_result: result >= 0 && result<(1<<GetCurrentLevel() )
  int GetIndex(int d) VTK_OVERRIDE
  {
    assert( "pre: valid_range" &&  d >= 0 && d < this->Dimension );
    return this->Indices[d];
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the number of children for each node of the tree.
  // \post positive_number: result>0
  int GetNumberOfChildren() VTK_OVERRIDE
  {
    return N;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the dimension of the tree.
  // \post positive_result: result >= 0
  int GetDimension() VTK_OVERRIDE
  {
    assert( "post: positive_result " && this->Dimension > 0 );
    assert( "post: up_to_3 " && this->Dimension <= 3 ); // and then
    return this->Dimension;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Move to the node described by its indices in each dimension and
  // at a given level. If there is actually a node or a leaf at this
  // location, Found() returns true. Otherwise, Found() returns false and the
  // cursor moves to the closest parent of the query. It can be the root in the
  // worst case.
  // \pre indices_exists: indices != 0
  // \pre valid_size: sizeof(indices)==GetDimension()
  // \pre valid_level: level >= 0
  void MoveToNode(int* indices,
                          int level) VTK_OVERRIDE
  {
    assert( "pre: indices_exists" && indices != 0 );
    assert( "pre: valid_level" && level >= 0 );

    this->ToRoot();

    int tmpIndices[3];
    memcpy( tmpIndices, indices, this->Dimension * sizeof(int) );
    // Convert to base 2 / 3 starting with most significant digit.

    int i = 0;
    int mask = 1;
    while ( ++ i < level )
    {
      mask *= this->Tree->GetBranchFactor();
    }

    int currentLevel = 0;
    while( !this->IsLeaf() && currentLevel < level )
    {
      // Compute the child index
      i = this->Dimension - 1;
      int child = 0;
      while ( i >= 0 )
      {
        int digit = tmpIndices[i] / mask;
        tmpIndices[i] -= digit*mask;
        child *= child * this->Tree->GetBranchFactor() + digit;
        -- i;
      }
      this->ToChild( child );
      ++ currentLevel;
      mask /= this->Tree->GetBranchFactor();
    }
    this->IsFound = ( currentLevel == level );
  }

  //---------------------------------------------------------------------------
  bool Found() VTK_OVERRIDE
  {
    return this->IsFound;
  }

  //---------------------------------------------------------------------------
  // NB: Public only for the vtkCompactHyperTreeCursor.
  void SetIsLeaf( bool value )
  {
    this->Leaf = value;
  }

  //---------------------------------------------------------------------------
  void SetChildIndex(int childIndex )
  {
    assert( "pre: valid_range" && childIndex >= 0
      && childIndex<GetNumberOfChildren() );
    this->ChildIndex = childIndex;
    assert( "post: is_set" && childIndex==GetChildIndex() );
  }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperTree.
  void SetIndex( vtkIdType index )
  {
    assert( "pre: positive_index" && index >= 0 );
    this->Index = index;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperTree.
  vtkIdType GetChildHistorySize()
  {
    return static_cast<vtkIdType>( this->ChildHistory.size() );
  }

protected:
  //---------------------------------------------------------------------------
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
    this->Tree = 0;
    this->Index = 0;
    this->Leaf = false;
    this->ChildIndex = 0;
    memset( this->Indices, 0, 3 * sizeof(int) );
  }

  vtkCompactHyperTree<N> *Tree;
  unsigned char Dimension;

  // Index either in the Nodes or Parents (if leaf)
  vtkIdType Index;

  // Number of current node as a child
  int ChildIndex;

  bool IsFound;
  bool Leaf;

  // A stack, but stack does not have clear()
  std::deque<int> ChildHistory;

  // Index in each dimension of the current node, as if the tree at the current
  // level were a uniform grid. Default to 3 dimensions, use only those needed
  int Indices[3];

private:
  vtkCompactHyperTreeCursor(const vtkCompactHyperTreeCursor<N> &) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompactHyperTreeCursor<N> &) VTK_DELETE_FUNCTION;
};

// We could use a 4 byte int, but the internals are completely hidden.
class vtkHyperTreeLeafFlags
{
public:
  vtkHyperTreeLeafFlags()
  { // Unused bits are set to 1.
    for ( int i = 0; i < 4; ++ i )
    {
      this->Flags[i] = 0xFF;
    }
  }

  // True if all chilren are leaves.
  bool IsTerminal()
  {
    // Unused bits are set to 1.
    return this->Flags[0] == 0xFF
      && this->Flags[1] == 0xFF
      && this->Flags[2] == 0xFF;
  }

  void SetLeafFlag(int idx, bool val)
  {
    assert( "Valid child idx" && idx >= 0 && idx < 32);
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

  bool GetLeafFlag(int idx)
  {
    assert( "Valid child idx" && idx >= 0 && idx < 32);
    int i = 0;
    while ( idx >= 8 )
    {
      ++ i;
      idx -= 8;
    }
    unsigned char mask = 1 << idx;
    return ( mask & this->Flags[i] ) == mask;
  }

  void PrintSelf(ostream& os, int numChildren)
  {
    assert( "Number of children" && numChildren >= 0 && numChildren < 32);
    int childIdx = 0;
    int byteIdx = 0;
    unsigned char mask = 1;
    while ( childIdx < numChildren )
    {
      os << ( ( this->Flags[byteIdx] & mask ) == mask );
      ++childIdx;
      if ( mask == 128 )
      {
        mask = 1;
        ++byteIdx;
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

// Description:
// A node of the Tree which is not a leaf.
// Expected template values: 4, 8, 9, 27.
template<int N> class vtkCompactHyperTreeNode
{
public:
  //---------------------------------------------------------------------------
  // Description:
  // See GetParent().
  void SetParent(vtkIdType parent)
  {
    assert( "pre: positive_parent" && parent >= 0 );
    this->Parent = parent;
    assert( "post: is_set" && parent == this->GetParent() );
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index of the parent node of the current node in the
  // nodes array of the hyperTree.
  int GetParent()
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
  // Description
  // Are children all leaves?
  bool IsTerminalNode()
  {
    return this->LeafFlags.IsTerminal();
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
  // IsChildLeaf( i ) is true, the index points to an element in the LeafParent
  // and Attribute arrays of the hyperTree class. If not, the index points to
  // an element in the Nodes array of the hyperTree class.
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
    os << indent << "LeafFlags= ";
    this->LeafFlags.PrintSelf( os, N );

    for( int i = 0; i < N; ++ i )
    {
      os << indent << this->Children[i] << endl;
    }
  }

protected:
  //---------------------------------------------------------------------------
  int Parent; // index
  vtkHyperTreeLeafFlags LeafFlags;
  int Children[N];
};

template<int N> class vtkCompactHyperTree : public vtkHyperTree
{
public:
  vtkTypeMacro(vtkCompactHyperTree<N>,vtkHyperTree);

  //---------------------------------------------------------------------------
  static vtkCompactHyperTree<N>* New()
  {
    vtkCompactHyperTree<N> *ret = new vtkCompactHyperTree<N>;
    ret->InitializeObjectBase();
    return ret;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Restore the initial state: only one node and one leaf: the root.
  void Initialize() VTK_OVERRIDE
  {
    this->Nodes.resize( 1 );
    this->Nodes[0].SetParent( 0 );
    for ( int i = 0; i < N; ++ i )
    {
      // It is assumed that the root is a special node with only one child.
      // The other children flags are irrelevant, but set them as nodes for
      // no good reason.
      this->Nodes[0].SetLeafFlag( i, i == 0 ); // First child is a leaf
      this->Nodes[0].SetChild( i, 0 );
    }
    this->LeafParent.resize( 1 );
    this->LeafParent[0] = 0;
    this->NumberOfLevels = 1;
    this->NumberOfNodes = 1;
    this->GlobalIndexTable.clear();
    this->GlobalIndexStart = 0;
  }

  //---------------------------------------------------------------------------
  vtkHyperTreeCursor* NewCursor() VTK_OVERRIDE
  {
    vtkCompactHyperTreeCursor<N>* result = vtkCompactHyperTreeCursor<N>::New();
    result->SetTree( this );
    return result;
  }

  //---------------------------------------------------------------------------
  ~vtkCompactHyperTree() VTK_OVERRIDE
  {
  }

  //---------------------------------------------------------------------------
  vtkIdType GetNumberOfLeaves() VTK_OVERRIDE
  {
    return this->NumberOfNodes;
  }

  //---------------------------------------------------------------------------
  vtkIdType GetNumberOfIndex() VTK_OVERRIDE
  {
    return static_cast<vtkIdType>( this->LeafParent.size() );
  }

  //---------------------------------------------------------------------------
  void SetGlobalIndexStart( vtkIdType start ) VTK_OVERRIDE
  {
    this->GlobalIndexStart = start;
  }

  //---------------------------------------------------------------------------
  void SetGlobalIndexFromLocal( vtkIdType local, vtkIdType global ) VTK_OVERRIDE
  {
    if ( static_cast<vtkIdType>( this->GlobalIndexTable.size() ) <= local )
    {
      this->GlobalIndexTable.resize( local + 1 );
    }
    this->GlobalIndexTable[ local ] = global;
    if ( local == 0 && this->LeafParent.size() == 1 )
    {
      SetGlobalIndexFromLocal( 1, global );
    }
  }

  //---------------------------------------------------------------------------
  vtkIdType GetGlobalIndexFromLocal( vtkIdType local ) VTK_OVERRIDE
  {
    return ( local < static_cast<vtkIdType>( this->GlobalIndexTable.size() ) ) ?
      this->GlobalIndexTable[ local ] : ( this->GlobalIndexStart + local );
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return the erenumber of levels.
  // \post result_greater_or_equal_to_one: result>=1
  vtkIdType GetNumberOfLevels() VTK_OVERRIDE
  {
    assert( "post: result_greater_or_equal_to_one"
      && this->NumberOfLevels >= 1 );
    return this->NumberOfLevels;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperTreeCursor.
  vtkCompactHyperTreeNode<N>* GetNode( int nodeIdx )
  {
    assert( "pre: valid_range" && nodeIdx >= 0
      && nodeIdx < this->GetNumberOfNodes() );
    return &this->Nodes[nodeIdx];
  }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperTreeCursor.
  // NB: Cursor (index ) appears to be different between nodes and leaves.
  // Different arrays => overlapping indexes.
  // I am changing the name for clarity.
  // This really returns the nodeIdx of the leafs parent.
  int GetLeafParent( int leafIdx )
  {
    assert( "pre: valid_range" && leafIdx >= 0
      && leafIdx < this->GetNumberOfIndex() );
    assert( "post: valid_result" && this->LeafParent[leafIdx] >= 0
      && this->LeafParent[leafIdx] < this->GetNumberOfNodes() );
    return this->LeafParent[leafIdx];
  }

  //---------------------------------------------------------------------------
  // NB: Public only for the vtkCompactHyperTreeCursor.
  vtkIdType GetNumberOfNodes() VTK_OVERRIDE
  {
    assert( "post: not_empty" && this->Nodes.size() > 0 );
    return static_cast<vtkIdType>( this->Nodes.size() );
  }

  //---------------------------------------------------------------------------
  void SubdivideLeaf( vtkHyperTreeCursor* leafCursor ) VTK_OVERRIDE
  {
    assert( "pre: leaf_exists" && leafCursor != 0 );
    assert( "pre: is_a_leaf" && leafCursor->IsLeaf() );

    // We are using a vtkCompactHyperTreeCursor.
    vtkCompactHyperTreeCursor<N>* cursor =
      static_cast<vtkCompactHyperTreeCursor<N> *>(leafCursor);

    // The leaf becomes a node and is not anymore a leaf
    cursor->SetIsLeaf( false ); // let the cursor know about that change.
    vtkIdType nodeIndex = cursor->GetNodeId();

    // Nodes get constructed with leaf flags set to 1.
    if ( this->GetNumberOfNodes() <= nodeIndex )
    {
      this->Nodes.resize( nodeIndex + 1 );
    }
    vtkIdType parentNodeIdx = this->LeafParent[nodeIndex];
    this->Nodes[nodeIndex].SetParent( parentNodeIdx );

    // Change the parent: it has one less child as a leaf
    vtkCompactHyperTreeNode<N>& parent = this->Nodes[parentNodeIdx];

    // New nodes index in parents children array.
    int idx = cursor->GetChildIndex();
    parent.SetLeafFlag( idx, false );
    parent.SetChild( idx, static_cast<int>( nodeIndex ) );

    // The first new child
    // Recycle the leaf index we are deleting because it became a node.
    // This avoids messy leaf parent array issues.
    this->NumberOfNodes += N;
    // The other (N) new children.
    size_t nextLeaf = this->LeafParent.size();
    this->LeafParent.resize( nextLeaf + N );
    for ( int i = 0; i < N; ++ i, ++ nextLeaf )
    {
      this->Nodes[nodeIndex].SetChild( i, static_cast<int>( nextLeaf ) );
      this->LeafParent[nextLeaf] = nodeIndex;
      this->Nodes[nodeIndex].SetLeafFlag( i, true );
    }

    // Update the number of leaves per level.
    vtkIdType level = cursor->GetChildHistorySize();

    // Add the new leaves to the number of leaves at the next level.
    if ( level + 1 == this->NumberOfLevels ) // >=
    {
      // We have a new level.
      ++ this->NumberOfLevels;
    }
  }

  //---------------------------------------------------------------------------
  // NB: Bad interface: This is really GetNumberOfLeaves.
  vtkIdType GetLeafParentSize()
  {
    return static_cast<int>( this->LeafParent.size() );
  }

  //---------------------------------------------------------------------------
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE
  {
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Dimension=" << this->Dimension << endl;
    os << indent << "BranchFactor=" << this->BranchFactor << endl;

    os << indent << "Nodes=" << this->Nodes.size() << endl;
    for ( unsigned int i = 0; i < this->Nodes.size(); ++ i )
    {
      this->Nodes[i].PrintSelf( os, indent );
    }
    os << endl;

    os << indent << "LeafParent="<<this->LeafParent.size() << endl;
    for ( unsigned int i = 0; i < this->LeafParent.size(); ++ i )
    {
      os << this->LeafParent[i] << " ";
    }
    os << endl;
  }

  //---------------------------------------------------------------------------
  // Description:
  // Return memory used in kibibytes (1024 bytes).
  // Ignore the attribute array because its size is added by the data set.
  unsigned int GetActualMemorySize() VTK_OVERRIDE
  {
    size_t size = sizeof(vtkIdType) * this->LeafParent.size() +
      sizeof(vtkCompactHyperTreeNode<N>) * this->Nodes.size() +
      sizeof(vtkIdType) * this->GlobalIndexTable.size();
    return static_cast<unsigned int>( size / 1024 );
  }

  int GetBranchFactor() VTK_OVERRIDE
  {
    return this->BranchFactor;
  }

  int GetDimension() VTK_OVERRIDE
  {
    return this->Dimension;
  }

  void SetScale( double s[3] ) VTK_OVERRIDE
  {
    memcpy( this->Scale, s, 3 * sizeof( double ) );
  }
  void GetScale( double s[3] ) VTK_OVERRIDE
  {
    memcpy( s, this->Scale, 3 * sizeof( double ) );
  }
  double GetScale( unsigned int d ) VTK_OVERRIDE
  {
    return this->Scale[d];
  }

protected:
  //---------------------------------------------------------------------------
  // Description:
  // Default constructor.
  // The tree as only one node and one leaf: the root.
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

    // Set default scale
    for ( int i = 0; i < 3; ++ i )
    {
      this->Scale[i] = 1.;
    }

    this->Initialize();
  }

  int BranchFactor;
  int Dimension;
  double Scale[3];
  vtkIdType NumberOfLevels;
  vtkIdType NumberOfNodes;

  vtkIdType GlobalIndexStart;

  // Storage for non-leaf tree nodes
  std::vector<vtkCompactHyperTreeNode<N> > Nodes;

  // Storage to record the parent of each leaf
  std::vector<vtkIdType> LeafParent;

  // Storage to record the local to global id mapping
  std::vector<vtkIdType> GlobalIndexTable;

private:
  vtkCompactHyperTree(const vtkCompactHyperTree<N> &) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompactHyperTree<N> &) VTK_DELETE_FUNCTION;
};

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

  return NULL;
}

//-----------------------------------------------------------------------------
void vtkHyperTree::FindChildParameters( int child, vtkIdType& index, bool& isLeaf )
{
#define GetNodeParametersMacro( _N_ )                                              \
  {                                                                                \
  vtkCompactHyperTree<_N_>* tree = static_cast<vtkCompactHyperTree<_N_>*>( this ); \
  vtkCompactHyperTreeNode<_N_>* node = tree->GetNode( static_cast<int>(index) );   \
  index = static_cast<vtkIdType>(node->GetChild( child ));                         \
  isLeaf = node->IsChildLeaf( child );                                             \
  return;                                                                          \
  }

  switch ( this->GetDimension() )
  {
    case 3:
      switch ( this->GetBranchFactor() )
      {
        case 2: GetNodeParametersMacro( 8 );
        case 3: GetNodeParametersMacro( 27 );
      } // case 3
      break;
    case 2:
      switch ( this->GetBranchFactor() )
      {
        case 2: GetNodeParametersMacro( 4 );
        case 3: GetNodeParametersMacro( 9 );
      } // case 2
      break;
    case 1:
      switch ( this->GetBranchFactor() )
      {
        case 2: GetNodeParametersMacro( 2 );
        case 3: GetNodeParametersMacro( 3 );
      } // case 1
      break;
  }

#undef GetNodeParametersMacro

  vtkGenericWarningMacro( "Bad branching factor " << this->GetBranchFactor() );
}
