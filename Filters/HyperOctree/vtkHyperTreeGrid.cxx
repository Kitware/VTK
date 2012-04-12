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
#include "vtkHyperTreeGrid.h"

#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkHyperTreeCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTimerLog.h"
#include "vtkVoxel.h"

#include <deque>
#include <vector>

#include <assert.h>


// Issues:
// 1: Order of leaf id's due to refining nodes.  Reader could order leaves base on its own needs.
// 2: Default cell interface creates connectivity arrays (effectively unstructured grid) to support random
//    access to cells.  A serial iterator would be much more efficient.

vtkInformationKeyMacro(vtkHyperTreeGrid, LEVELS, Integer);
vtkInformationKeyMacro(vtkHyperTreeGrid, DIMENSION, Integer);
vtkInformationKeyRestrictedMacro(vtkHyperTreeGrid, SIZES, DoubleVector, 3 );

//===================================================
// This internal class is used as a supperclass
// by the templated Compact class. All methods are pure virtual.
// I assume this is done to hide templates.
class vtkHyperTreeInternal
  : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeInternal,vtkObject);
  virtual void Initialize()=0;
  virtual vtkHyperTreeCursor *NewCursor()=0;
  virtual vtkIdType GetNumberOfLeaves()=0;
  virtual int GetNumberOfNodes()=0;
  virtual int GetBranchFactor()=0;
  virtual int GetDimension()=0;

  // Description:
  // Return the number of levels.
  // \post result_greater_or_equal_to_one: result>=1
  virtual vtkIdType GetNumberOfLevels()=0;

  // Description:
  // Subdivide node pointed by cursor, only if its a leaf.
  // At the end, cursor points on the node that used to be leaf.
  // \pre leaf_exists: leaf!=0
  // \pre is_a_leaf: leaf->CurrentIsLeaf()
  virtual void SubdivideLeaf(vtkHyperTreeCursor *leaf)=0;

  // Description:
  // Returns the actual memory size in kilobytes.
  // Ignores the attribute array.
  virtual unsigned int GetActualMemorySize() = 0;

protected:
  vtkHyperTreeInternal()
    {
    }

private:
  vtkHyperTreeInternal(const vtkHyperTreeInternal &);  // Not implemented.
  void operator=(const vtkHyperTreeInternal &);    // Not implemented.
};

// Description:
// The template value N describes the number of children to binary and
// ternary trees.
template<int N> class vtkCompactHyperTree;
template<int N> class vtkCompactHyperTreeNode;
template<int N> class vtkCompactHyperTreeCursor
  :public vtkHyperTreeCursor
{
public:
  //---------------------------------------------------------------------------
  static vtkCompactHyperTreeCursor<N> *New()
    {
      vtkObject *ret=
        vtkObjectFactory::CreateInstance( "vtkCompactHyperTreeCursor<N>" );

      if(ret!=0)
        {
        return static_cast<vtkCompactHyperTreeCursor<N> *>(ret);
        }
      else
        {
        return new vtkCompactHyperTreeCursor<N>;
        }
    }

  vtkTypeMacro(vtkCompactHyperTreeCursor<N>,vtkHyperTreeCursor);

  //---------------------------------------------------------------------------
  // Initialization
  virtual void Init(vtkCompactHyperTree<N> *tree)
    {
      this->Tree=tree;
    }

  //---------------------------------------------------------------------------
  // Access
  // Return the id of the current leaf in order to
  // access to the data.
  // \pre is_leaf: CurrentIsLeaf()
  int GetLeafId()
    {
      assert( "pre: is_leaf" && CurrentIsLeaf() );
      return this->Cursor;
    }

  // Status
  virtual int CurrentIsLeaf()
    {
      return this->IsLeaf;
    }

  virtual int CurrentIsRoot()
    {
      return ( this->IsLeaf && this->Cursor==0 && this->Tree->GetLeafParentSize()==1 ) || (!this->IsLeaf && this->Cursor==1);
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the level of the node pointed by the cursor.
  // \post positive_result: result>=0
  virtual int GetCurrentLevel()
    {
      int result = this->GetChildHistorySize();
      assert( "post: positive_result" && result>=0);
      return result;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the child number of the current node relative to its parent.
  // \pre not_root: !CurrentIsRoot().
  // \post valid_range: result>=0 && result<GetNumberOfChildren()
  virtual int GetChildIndex()
    {
      assert( "post: valid_range" && this->ChildIndex>=0 && this->ChildIndex<GetNumberOfChildren() );
      return this->ChildIndex;
    }

  //---------------------------------------------------------------------------
  // Are the children of the current node all leaves?
  // This query can be called also on a leaf node.
  // \post compatible: result implies !CurrentIsLeaf()
  virtual int CurrentIsTerminalNode()
    {
      int result = !this->IsLeaf;
      if(result)
        {
        vtkCompactHyperTreeNode<N> *node=this->Tree->GetNode( this->Cursor);
        result = node->IsTerminalNode();
        }
      // A=>B: notA or B
      assert( "post: compatible" && (!result || !this->IsLeaf) );
      return result;
    }

  //---------------------------------------------------------------------------
  // Cursor movement.
  // \pre can be root
  // \post is_root: CurrentIsRoot()
  virtual void ToRoot()
    {
      this->ChildHistory.clear();
      this->IsLeaf=( this->Tree->GetLeafParentSize() == 1 );
      if( this->IsLeaf )
        {
        this->Cursor=0;
        }
      else
        {
        this->Cursor=1;
        }
      this->ChildIndex=0;
      unsigned int i=0;
      while( i < this->Dimension )
        {
        this->Index[i]=0;
        ++ i;
        }
    }

  //---------------------------------------------------------------------------
  // \pre not_root: !CurrentIsRoot()
  virtual void ToParent()
    {
      assert( "pre: not_root" && !CurrentIsRoot() );
      if( this->IsLeaf)
        {
        this->Cursor=this->Tree->GetLeafParent( this->Cursor);
        }
      else
        {
        this->Cursor=this->Tree->GetNode( this->Cursor)->GetParent();
        }
      this->IsLeaf=0;
      this->ChildIndex=this->ChildHistory.back(); // top()
      this->ChildHistory.pop_back();

      for ( unsigned int i=0; i < this->Dimension;  ++ i )
        {
        this->Index[i]=( this->Index[i] ) / this->Tree->GetBranchFactor();
        }
    }

  //---------------------------------------------------------------------------
  // \pre not_leaf: !CurrentIsLeaf()
  // \pre valid_child: child>=0 && child<this->GetNumberOfChildren()
  virtual void ToChild(int child)
    {
      assert( "pre: not_leaf" && !CurrentIsLeaf() );
      assert( "pre: valid_child" && child>=0 && child<this->GetNumberOfChildren() );

      vtkCompactHyperTreeNode<N> *node=this->Tree->GetNode( this->Cursor);
      this->ChildHistory.push_back( this->ChildIndex);
      this->ChildIndex=child;
      this->Cursor=node->GetChild( child );
      this->IsLeaf=node->IsChildLeaf( child );
      unsigned int i=0;

      int tmpChild = child;
      int tmp;
      int branchFactor = this->Tree->GetBranchFactor();
      while( i < this->Dimension )
        { // Effectively converting child to base 2/3 (branch factor)
        tmp = tmpChild;
        tmpChild /= branchFactor;
        int index=tmp-(branchFactor*tmpChild); // Remainder (mod)
        assert( "check: mod 3 value" && index>=0 && index<branchFactor);
        this->Index[i]=(( this->Index[i])*branchFactor)+index;
        ++ i;
        }
    }

  //---------------------------------------------------------------------------
  // Description:
  // Move the cursor to the same node pointed by `other'.
  // \pre other_exists: other!=0
  // \pre same_hyperTree: this->SameTree(other)
  // \post equal: this->IsEqual(other)
  virtual void ToSameNode(vtkHyperTreeCursor *other)
    {
      assert( "pre: other_exists" && other!=0);
      assert( "pre: same_hyperTree" && this->SameTree(other) );

      vtkCompactHyperTreeCursor<N> *o=static_cast<vtkCompactHyperTreeCursor<N> *>(other);

      this->Cursor=o->Cursor;
      this->ChildIndex=o->ChildIndex;
      this->IsLeaf=o->IsLeaf;
      this->ChildHistory=o->ChildHistory; // use assignment operator
      unsigned int i=0;
      while( i < this->Dimension )
        {
        this->Index[i] = o->Index[i];
        ++ i;
        }
      assert( "post: equal" && this->IsEqual(other) );
    }

  //--------------------------------------------------------------------------
  // Description:
  // Is `this' equal to `other'?
  // \pre other_exists: other!=0
  // \pre same_hyperTree: this->SameTree(other);
  virtual int IsEqual(vtkHyperTreeCursor *other)
    {
      assert( "pre: other_exists" && other!=0);
      assert( "pre: same_hyperTree" && this->SameTree(other) );

      vtkCompactHyperTreeCursor<N> *o=static_cast<vtkCompactHyperTreeCursor<N> *>(other);

      int result = this->Cursor==o->Cursor && this->ChildIndex==o->ChildIndex
        && this->IsLeaf==o->IsLeaf && this->ChildHistory==o->ChildHistory;

      unsigned int i=0;
      while( result && i < this->Dimension )
        {
        result = this->Index[i] == o->Index[i];
        ++ i;
        }
      return result;
    }

  //--------------------------------------------------------------------------
  // Description:
  // Create a copy of `this'.
  // \post results_exists:result!=0
  // \post same_tree: result->SameTree( this )
  virtual vtkHyperTreeCursor *Clone()
    {
      vtkCompactHyperTreeCursor<N> *result = this->NewInstance();
      result->Tree=this->Tree;
      assert( "post: results_exists" && result!=0);
      assert( "post: same_tree" && result->SameTree( this ) );
      return result;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Are `this' and `other' pointing on the same hyperTree?
  // \pre other_exists: other!=0
  virtual int SameTree(vtkHyperTreeCursor *other)
    {
      assert( "pre: other_exists" && other!=0);
      vtkCompactHyperTreeCursor<N> *o=vtkCompactHyperTreeCursor<N>::SafeDownCast(other);
      int result = o!=0;
      if(result)
        {
        result = this->Tree==o->Tree;
        }
      return result;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index in dimension `d', as if the node was a cell of a
  // uniform grid of 1<<GetCurrentLevel() cells in each dimension.
  // \pre valid_range: d>=0 && d<GetDimension()
  // \post valid_result: result>=0 && result<(1<<GetCurrentLevel() )
  virtual int GetIndex(int d)
    {
      assert( "pre: valid_range" &&  d>=0 && d<this->Dimension );
      int result = this->Index[d];
      return result;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the number of children for each node of the tree.
  // \post positive_number: result>0
  virtual int GetNumberOfChildren()
    {
      return N;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the dimension of the tree.
  // \post positive_result: result>=0
  virtual int GetDimension()
    {
      assert( "post: positive_result " && this->Dimension>0);
      assert( "post: up_to_3 " && this->Dimension<=3 ); // and then
      return this->Dimension;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Move to the node described by its indices in each dimension and
  // at a given level. If there is actually a node or a leaf at this
  // location, Found() returns true. Otherwise, Found() returns false and the
  // cursor moves to the closest parent of the query. It can be the root in the
  // worst case.
  // \pre indices_exists: indices!=0
  // \pre valid_size: sizeof(indices)==GetDimension()
  // \pre valid_level: level>=0
  virtual void MoveToNode(int *indices,
                          int level)
    {
      assert( "pre: indices_exists" && indices!=0);
      assert( "pre: valid_level" && level>=0);

      this->ToRoot();
      int currentLevel=0;

      int child;
      int tmpIndices[3];

      // Convert to base 2 / 3 starting with most significant digit.
      int mask;
      tmpIndices[0] = indices[0];
      tmpIndices[1] = indices[1];
      tmpIndices[2] = indices[2];
      int i = 0;
      mask = 1;
      while (++ i<level)
        {
        mask *= this->Tree->GetBranchFactor();
        }

      while(!this->CurrentIsLeaf() && currentLevel<level)
        {
        // compute the child index.
        i=this->Dimension-1;
        child=0;
        while(i>=0)
          {
          int digit = tmpIndices[i] / mask;
          tmpIndices[i] -= digit*mask;
          child *= child*this->Tree->GetBranchFactor() + digit;
          --i;
          }
        this->ToChild( child );
        ++currentLevel;
        mask /= this->Tree->GetBranchFactor();
        }
      this->IsFound=currentLevel==level;
    }

  //---------------------------------------------------------------------------
  // Description
  // Did the last call to MoveToNode succeed?
  virtual int Found()
    {
      return this->IsFound;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperTree.
  void SetIsLeaf(int value)
    {
      this->IsLeaf=value;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperTree.
  void SetChildIndex(int childIndex)
    {
      assert( "pre: valid_range" && childIndex>=0 && childIndex<GetNumberOfChildren() );
      this->ChildIndex=childIndex;
      assert( "post: is_set" && childIndex==GetChildIndex() );
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperTree.
  void SetCursor(int cursor)
    {
      assert( "pre: positive_cursor" && cursor>=0);
      this->Cursor=cursor;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperTree.
  vtkIdType GetChildHistorySize()
    {
      return this->ChildHistory.size();
    }

protected:
  //---------------------------------------------------------------------------
  vtkCompactHyperTreeCursor()
    {
      this->Dimension = 0;
      switch (N)
        {
        case 2:
          this->Dimension = 1;
          break;
        case 3:
          this->Dimension = 1;
          break;
        case 4:
          this->Dimension = 2;
          break;
        case 9:
          this->Dimension = 2;
          break;
        case 8:
          this->Dimension = 3;
          break;
        case 27:
          this->Dimension = 3;
          break;
        default:
          assert( "Bad number of children" && this->Dimension == 0);
        }
      this->Tree=0;
      this->Cursor=0;
      this->IsLeaf=0;
      this->ChildIndex=0;
      unsigned int i=0;
      while(i<this->Dimension )
        {
        this->Index[i]=0;
        ++ i;
        }
    }

  vtkCompactHyperTree<N> *Tree;
  unsigned char Dimension;
  int Cursor; // index either in the Nodes or Parents (if leaf)
  int ChildIndex; // the current node is
  // child number ChildIndex (in [0,1<<D-1]) for its parent node (comment specific for oct/quad trees)

  int IsFound;
  int IsLeaf;

  std::deque<int> ChildHistory; // a stack, but stack does not have clear()
  // I have to default to three dimensions and not use the third for quad/9 trees
  int Index[3]; // index in each dimension of the current node, as if the
  // tree at the current level was a uniform grid.
private:
  vtkCompactHyperTreeCursor(const vtkCompactHyperTreeCursor<N> &);  // Not implemented.
  void operator=(const vtkCompactHyperTreeCursor<N> &);    // Not implemented.
};

// We could use a 4 byte int, but the internals are completely hidden.
class vtkHyperTreeLeafFlags
{
public:
  vtkHyperTreeLeafFlags()
    { // Unused bits are set to 1.
    this->Flags[0] = this->Flags[1] = this->Flags[2] = this->Flags[3] = 255;
    }
  // True if all chilren are leaves.
  bool IsTerminal()
    {
    // Unused bits are set to 1.
    return ( this->Flags[0] == 255) && ( this->Flags[1] == 255) && ( this->Flags[2] == 255);
    }
  void SetLeafFlag(int idx, bool val)
    {
    assert( "Valid child idx" && idx >= 0 && idx < 32);
    int i = 0;
    while (idx >= 8)
      {
      ++ i;
      idx-=8;
      }
    unsigned char mask = 1<<idx;
    if (val)
      {
      this->Flags[i] = this->Flags[i] | mask;
      }
    else
      {
      this->Flags[i] = this->Flags[i] & (mask^255);
      }
    }
  bool GetLeafFlag(int idx)
    {
    assert( "Valid child idx" && idx >= 0 && idx < 32);
    int i = 0;
    while (idx >= 8)
      {
      ++ i;
      idx-=8;
      }
    unsigned char mask = 1<<idx;
    return (mask & this->Flags[i]) == mask;
    }
  void PrintSelf(ostream& os, int numChildren)
    {
    assert( "Number of children" && numChildren >= 0 && numChildren < 32);
    int childIdx=0;
    int byteIdx = 0;
    unsigned char mask = 1;
    while (childIdx < numChildren)
      {
      os << ((( this->Flags[byteIdx])&mask)==mask);
      ++childIdx;
      if (mask == 128)
        {
        mask = 1;
        ++byteIdx;
        }
      else
        {
        mask<<=1;
        }
      }
    os<<endl;
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
  void SetParent(int parent)
    {
      assert( "pre: positive_parent" && parent>=0);
      this->Parent=parent;
      assert( "post: is_set" && parent==this->GetParent() );
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index of the parent node of the current node in the
  // nodes array of the hyperTree.
  int GetParent()
    {
      assert( "post: positive_result" && this->Parent>=0);
      return this->Parent;
    }

  //---------------------------------------------------------------------------
  // Description:
  // See GetLeafFlags()
  void SetLeafFlag(int childIdx, bool flag)
    {
      this->LeafFlags.SetLeafFlag(childIdx, flag);
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
  bool IsChildLeaf(int i)
    {
    assert( "pre: valid_range" && i>=0 && i < N);
    return this->LeafFlags.GetLeafFlag( i );
    }

  //---------------------------------------------------------------------------
  // Description:
  // See GetChild().
  void SetChild(int i,
                int child)
    {
      assert( "pre: valid_range" && i>=0 && i < N);
      assert( "pre: positive_child" && child>=0);
      this->Children[i]=child;
      assert( "post: is_set" && child==this->GetChild( i ) );
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index of of the 'i'-th child. If the result of
  // IsChildLeaf( i ) is true, the index points to an element in the LeafParent
  // and Attribute arrays of the hyperTree class. If not, the index points to
  // an element in the Nodes array of the hyperTree class.
  int GetChild(int i)
    {
      assert( "pre: valid_range" && i>=0 && i < N);
      assert( "post: positive_result" && this->Children[i]>=0);
      return this->Children[i];
    }

  //---------------------------------------------------------------------------
  void PrintSelf(ostream& os, vtkIndent indent)
    {
      os << indent << "Parent=" << this->Parent<<endl;
      os << indent << "LeafFlags= ";
      this->LeafFlags.PrintSelf(os, N);
      int i=0;
      while(i < N)
        {
        os<<indent<<this->Children[i]<<endl;
        ++ i;
        }
    }

protected:
  //---------------------------------------------------------------------------
  int Parent; // index
  vtkHyperTreeLeafFlags LeafFlags;
  int Children[N];
};

template<int N> class vtkCompactHyperTree
  : public vtkHyperTreeInternal
{
public:
  //---------------------------------------------------------------------------
  static vtkCompactHyperTree<N> *New()
    {
      vtkObject *ret=
        vtkObjectFactory::CreateInstance( "vtkCompactHyperTree<N>" );

      if(ret!=0)
        {
        return static_cast<vtkCompactHyperTree<N> *>(ret);
        }
      else
        {
        return new vtkCompactHyperTree<N>;
        }
    }

  vtkTypeMacro(vtkCompactHyperTree<N>,vtkHyperTreeInternal);

  //---------------------------------------------------------------------------
  // Description:
  // Restore the initial state: only one node and one leaf: the root.
  virtual void Initialize()
    {
      // Law: I believe that leaves are implicit (not node objects)
      // so why initialize a root node with one leaf?
      // Does the root always have one child?
      this->Nodes.resize(1);
      this->Nodes[0].SetParent( 0 );
      int i=0;
      while (i < N)
        {
        // Law: I assume that the root is a special node with only one child.
        // The other children flags are irrelavent, but set them as nodes for no good reason.
        this->Nodes[0].SetLeafFlag(i, i==0); // First child is a leaf
        this->Nodes[0].SetChild(i,0);
        }
      this->LeafParent.resize(1);
      this->LeafParent[0]=0;
      this->NumberOfLevels=1;
      this->NumberOfLeavesPerLevel.resize(1);
      this->NumberOfLeavesPerLevel[0]=1;
    }

  //---------------------------------------------------------------------------
  virtual vtkHyperTreeCursor *NewCursor()
    {
      vtkCompactHyperTreeCursor<N> *result = vtkCompactHyperTreeCursor<N>::New();
      result->Init( this );
      return result;
    }

  //---------------------------------------------------------------------------
  virtual ~vtkCompactHyperTree()
    {
    }

  //---------------------------------------------------------------------------
  virtual vtkIdType GetNumberOfLeaves()
    {
      return this->LeafParent.size();
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the number of levels.
  // \post result_greater_or_equal_to_one: result>=1
  virtual vtkIdType GetNumberOfLevels()
    {
      assert( "post: result_greater_or_equal_to_one" && this->NumberOfLevels>=1);
      return this->NumberOfLevels;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperTreeCursor.
  vtkCompactHyperTreeNode<N> *GetNode(int nodeIdx)
    {
      assert( "pre: valid_range" && nodeIdx>=0 && nodeIdx<GetNumberOfNodes() );
      return &this->Nodes[nodeIdx];
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperTreeCursor.
  // Law:
  // cursor (index) appears to be different between nodes and leaves.
  // Different arrays => overlapping indexes.
  // I am changing the name for clarity.
  // This really returns the nodeIdx of the leafs parent.
  int GetLeafParent(int leafIdx)
    {
      assert( "pre: valid_range" && leafIdx>=0 && leafIdx<this->GetNumberOfLeaves() );
      assert( "post: valid_result" && this->LeafParent[leafIdx]>=0 && this->LeafParent[leafIdx]<this->GetNumberOfNodes() );
      return this->LeafParent[leafIdx];
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperTreeCursor.
  virtual int GetNumberOfNodes()
    {
      assert( "post: not_empty" && this->Nodes.size()>0);
      return static_cast<int>( this->Nodes.size() );
    }

  //---------------------------------------------------------------------------
  // Description:
  // Subdivide node pointed by cursor, only if its a leaf.
  // At the end, cursor points on the node that used to be leaf.
  // \pre leaf_exists: leaf!=0
  // \pre is_a_leaf: leaf->CurrentIsLeaf()
  void SubdivideLeaf(vtkHyperTreeCursor *leafCursor)
    {
      assert( "pre: leaf_exists" && leafCursor!=0);
      assert( "pre: is_a_leaf" && leafCursor->CurrentIsLeaf() );

      // We are using a vtkCompactHyperTreeCursor.
      // We know that GetLeafId() return Cursor.
      int leafIndex = leafCursor->GetLeafId();
      vtkCompactHyperTreeCursor<N> *cursor=static_cast<vtkCompactHyperTreeCursor<N> *>(leafCursor);

      // the leaf becomes a node and is not anymore a leaf.
      cursor->SetIsLeaf( 0 ); // let the cursor knows about that change.
      size_t nodeIndex=this->Nodes.size();
      // Law: I believe that the node array does not include leaves (which are implicit).
      // Bad interface "SetCursor"  I would rather SetIndex.
      cursor->SetCursor(static_cast<int>(nodeIndex) );
      // Law: Add a node
      // Nodes get constructed with leaf flags set to 1.
      this->Nodes.resize(nodeIndex+1);
      int parentNodeIdx = this->LeafParent[leafIndex];
      this->Nodes[nodeIndex].SetParent(parentNodeIdx);

      // Change the parent: it has one less child as a leaf
      vtkCompactHyperTreeNode<N> *parent=&( this->Nodes[parentNodeIdx]);
      // Law: New nodes index in parents children array.
      int i = cursor->GetChildIndex();
      assert( "check matching_child" && parent->GetChild( i ) == leafIndex );
      parent->SetLeafFlag(i, false);
      parent->SetChild(i,static_cast<int>( nodeIndex ) );

      // The first new child
      // Law: Recycle the leaf index we are deleting because it became a node.
      // This avoids messy leaf parent array issues.
      this->Nodes[nodeIndex].SetChild( 0, leafIndex );
      this->LeafParent[leafIndex]=static_cast<int>( nodeIndex );

      // The other (N-1) new children.
      size_t nextLeaf=this->LeafParent.size();
      this->LeafParent.resize( nextLeaf + ( N - 1 ) );
      i = 1;
      while( i < N )
        {
        this->Nodes[nodeIndex].SetChild(i,static_cast<int>( nextLeaf ) );
        this->LeafParent[nextLeaf]=static_cast<int>( nodeIndex );
        ++ nextLeaf;
        ++ i;
        }


      // Update the number of leaves per level.

      int level=cursor->GetChildHistorySize();

      // remove the subdivided leaf from the number of leaves at its level.
      --this->NumberOfLeavesPerLevel[level];

      // add the new leaves to the number of leaves at the next level.
      if(level+1==this->NumberOfLevels) // >=
        {
        // we have a new level.
        ++this->NumberOfLevels;
        this->NumberOfLeavesPerLevel.resize( this->NumberOfLevels);
        }
      this->NumberOfLeavesPerLevel[level+1]+=N;
    }

  //---------------------------------------------------------------------------
  // Law: Bad interface: This is really GetNumberOfLeaves.
  int GetLeafParentSize()
    {
      return static_cast<int>( this->LeafParent.size() );
    }

  //---------------------------------------------------------------------------
  void PrintSelf(ostream& os, vtkIndent indent)
    {
      this->Superclass::PrintSelf(os,indent);

      os << indent << "Nodes="<<this->Nodes.size()<<endl;
      os << indent << "LeafParent="<<this->LeafParent.size()<<endl;

      os << indent << "Nodes="<<this->Nodes.size()<<endl;
      size_t i;
      os << indent;
      i=0;
      size_t c=this->Nodes.size();
      while(i<c)
        {
        this->Nodes[i].PrintSelf(os,indent);
        ++ i;
        }
      os<<endl;

      os << indent << "LeafParent="<<this->LeafParent.size()<<endl;
      i=0;
      c=this->LeafParent.size();
      while(i<c)
        {
        os << this->LeafParent[i]<<" ";
        ++ i;
        }
      os<<endl;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return memory used in kilobytes.
  // Ignore the attribute array because its size is added by the data set.
  unsigned int GetActualMemorySize()
  {
    size_t size;
    size = sizeof(int) * this->GetNumberOfLeaves();
    size += sizeof(vtkCompactHyperTreeNode<N>) * this->Nodes.size();
    return static_cast<unsigned int>(size / 1024);
  }

  int GetBranchFactor()
    {
    return this->BranchFactor;
    }

  int GetDimension()
    {
    return this->Dimension;
    }

protected:
  //---------------------------------------------------------------------------
  // Description:
  // Default constructor.
  // The tree as only one node and one leaf: the root.
  vtkCompactHyperTree()
    {
      if ( N == 2 || N == 4 || N == 8 )
        {
        this->BranchFactor = 2;
        }
      if ( N == 3 || N == 9 || N == 27 )
        {
        this->BranchFactor = 3;
        }

      if ( N == 2 || N == 3 )
        {
        this->Dimension = 1;
        }
      if ( N == 4 || N == 9 )
        {
        this->Dimension = 2;
        }
      if ( N == 8 || N == 27 )
        {
        this->Dimension = 3;
        }

      // Law: The root.
      this->Nodes.resize(1);
      this->Nodes[0].SetParent( 0 );
      // Law: Nodes default to have all children leaf flags equal true.
      int i=0;
      while( i < N )
        {
        this->Nodes[0].SetChild(i,0);
        ++ i;
        }
      this->LeafParent.resize(1);
      this->LeafParent[0]=0;
      this->NumberOfLevels=1;
      this->NumberOfLeavesPerLevel.resize(1);
      this->NumberOfLeavesPerLevel[0]=1;
    }

  std::vector<int> NumberOfLeavesPerLevel; // number of leaves in each level
  // its size is NumberOfLevels;

  vtkIdType NumberOfLevels;
  int BranchFactor;
  int Dimension;
  std::vector<vtkCompactHyperTreeNode<N> > Nodes;
  std::vector<int> LeafParent; // record the parent of each leaf
private:
  vtkCompactHyperTree(const vtkCompactHyperTree<N> &);  // Not implemented.
  void operator=(const vtkCompactHyperTree<N> &);    // Not implemented.
};


vtkStandardNewMacro(vtkHyperTreeGrid);

vtkCxxSetObjectMacro(vtkHyperTreeGrid,XCoordinates,vtkDataArray);
vtkCxxSetObjectMacro(vtkHyperTreeGrid,YCoordinates,vtkDataArray);
vtkCxxSetObjectMacro(vtkHyperTreeGrid,ZCoordinates,vtkDataArray);
//-----------------------------------------------------------------------------
// Default constructor.
vtkHyperTreeGrid::vtkHyperTreeGrid()
{
  // Grid of hyper trees
  this->CellTree = 0;
  
  // Primal grid
  this->CornerPoints = 0;
  this->LeafCornerIds = 0;

  // Dual grid
  this->LeafCenters = 0;
  this->CornerLeafIds = 0;

  // Internal links
  this->Links = 0;

  // Grid topology
  this->GridSize[0] = 0;
  this->GridSize[1] = 0;
  this->GridSize[2] = 0;

  // Grid parameters
  this->DualGridFlag = 1;
  this->Dimension = 1; // invalid
  this->NumberOfChildren = 1; // invalid set by SetDimensions
  this->AxisBranchFactor = 2;
  this->Dimension =  3;

  // Grid geometry
  this->XCoordinates=vtkDoubleArray::New();
  this->XCoordinates->SetNumberOfTuples( 1 );
  this->XCoordinates->SetComponent( 0, 0, 0. );
  this->YCoordinates=vtkDoubleArray::New();
  this->YCoordinates->SetNumberOfTuples( 1 );
  this->YCoordinates->SetComponent( 0, 0, 0. );
  this->ZCoordinates=vtkDoubleArray::New();
  this->ZCoordinates->SetNumberOfTuples( 1 );
  this->ZCoordinates->SetComponent( 0, 0, 0. );

  // For data set API
  this->Voxel = vtkVoxel::New();
  this->Pixel = vtkPixel::New();
  this->Line = vtkLine::New();
}

//-----------------------------------------------------------------------------
// Destructor.
vtkHyperTreeGrid::~vtkHyperTreeGrid()
{
  if ( this->CellTree )
    {
    int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
    for ( int i = 0; i < nCells; ++ i )
      {
      if ( this->CellTree[i] )
        {
        this->CellTree[i]->UnRegister( this );
        }
      }
    delete [] this->CellTree;
    }

  if ( this->XCoordinates ) 
    {
    this->XCoordinates->UnRegister( this );
    this->XCoordinates = NULL;
    }

  if ( this->YCoordinates ) 
    {
    this->YCoordinates->UnRegister( this );
    this->YCoordinates = NULL;
    }

  if ( this->ZCoordinates ) 
    {
    this->ZCoordinates->UnRegister( this );
    this->ZCoordinates = NULL;
    }

  this->DeleteInternalArrays();
  this->Voxel->Delete();
  this->Voxel = 0;
  this->Pixel->Delete();
  this->Pixel = 0;
  this->Line->Delete();
  this->Line = 0;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Dimension: " << this->Dimension<< endl;
  os << indent << "GridSize: " 
     << this->GridSize[0] <<","
     << this->GridSize[1] <<","
     << this->GridSize[2] << endl;
  if ( this->XCoordinates )
    {
    this->XCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->YCoordinates )
    {
    this->YCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->ZCoordinates )
    {
    this->ZCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  os << indent << "DualGridFlag: " << this->DualGridFlag << endl;
}

//-----------------------------------------------------------------------------
// Description:
// Return what type of dataset this is.
int vtkHyperTreeGrid::GetDataObjectType()
{
  return VTK_DATA_SET;
}

//-----------------------------------------------------------------------------
// Description:
// Copy the geometric and topological structure of an input rectilinear grid
// object.
void vtkHyperTreeGrid::CopyStructure(vtkDataSet *ds)
{
  assert( "pre: ds_exists" && ds!=0);
  assert( "pre: same_type" && vtkHyperTreeGrid::SafeDownCast(ds)!=0);

  vtkHyperTreeGrid *ho=vtkHyperTreeGrid::SafeDownCast(ds);

//  this->Superclass::CopyStructure(ho);

  // What about copying celldata???
  if ( this->CellTree )
    {
    int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
    for ( int i = 0; i < nCells; ++ i )
      {
      if ( this->CellTree[i] )
        {
        this->CellTree[i]->UnRegister( this );
        }
      }
    delete [] this->CellTree;
    }

  this->CellTree=ho->CellTree;
  if ( this->CellTree )
    {
    int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
    for ( int i = 0; i < nCells; ++ i )
      {
      if ( this->CellTree[i] )
        {
        this->CellTree[i]->Register( this );
        }
      }
    }

  this->Dimension = ho->Dimension;

  for ( int i = 0; i < 3; ++ i )
    {
    this->GridSize[i] = ho->GridSize[i];
    }

  this->SetXCoordinates(ho->XCoordinates);
  this->SetYCoordinates(ho->YCoordinates);
  this->SetZCoordinates(ho->ZCoordinates);

  this->Modified();
}

//-----------------------------------------------------------------------------
// Set the number of root cells of the tree.
void vtkHyperTreeGrid::SetGridSize( int n[3] )
{
  if( this->GridSize[0] == n[0] && this->GridSize[1] == n[1] && this->GridSize[2] == n[2] )
    {
    return;
    }
  this->GridSize[0] = n[0];
  this->GridSize[1] = n[1];
  this->GridSize[2] = n[2];
  this->Modified();
  this->UpdateTree();
}

//-----------------------------------------------------------------------------
// Description:
// Return the dimension of the tree (1D:binary tree(2 children), 2D:quadtree
// (4 children), 3D:Tree (8 children) )
// \post valid_result: result>=1 && result<=3
int vtkHyperTreeGrid::GetDimension()
{
  assert( "post: valid_result" && this->Dimension >= 1 && this->Dimension <= 3 );
  return this->Dimension;
}

//-----------------------------------------------------------------------------
// Set the dimension of the tree with `dim'. See GetDimension() for details.
// \pre valid_dim: dim>=1 && dim<=3
// \post dimension_is_set: GetDimension()==dim
void vtkHyperTreeGrid::SetDimension( int dim )
{
  assert( "pre: valid_dim" && dim >= 1 && dim <= 3 );
  if( this->Dimension == dim)
    {
    return;
    }
  this->Dimension = dim;
  this->Modified();
  this->UpdateTree();
}

//-----------------------------------------------------------------------------
// \pre valid_dim: factor == 2 or factor == 3;
// \post dimension_is_set: GetAxisBranchFactor()==dim
void vtkHyperTreeGrid::SetAxisBranchFactor( int factor )
{
  assert( "pre: valid_factor" && factor>=2 && factor<=3 );
  if( this->AxisBranchFactor==factor)
    {
    return;
    }
  this->AxisBranchFactor=factor;
  this->Modified();
  this->UpdateTree();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::UpdateTree()
{
  int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
  
  if ( this->CellTree )
    {
    for ( int i = 0; i < nCells; ++ i )
      {
      if ( this->CellTree[i] )
        {
        this->CellTree[i]->UnRegister( this );
        }
      }
    delete [] this->CellTree;
    }

  this->CellTree = new vtkHyperTreeInternal*[nCells];

  if ( this->AxisBranchFactor == 2 )
    {
    switch( this->Dimension )
      {
      case 3:
        this->NumberOfChildren = 8;
        for ( int i = 0; i < nCells; ++ i )
          {
          this->CellTree[i]=vtkCompactHyperTree<8>::New();
          }
        break;
      case 2:
        this->NumberOfChildren = 4;
         for ( int i = 0; i < nCells; ++ i )
          {
          this->CellTree[i]=vtkCompactHyperTree<4>::New();
          }
       break;
      case 1:
        this->NumberOfChildren = 2;
        for ( int i = 0; i < nCells; ++ i )
          {
          this->CellTree[i]=vtkCompactHyperTree<2>::New();
          }
        break;
      default:
        assert( "check: impossible case" && 0);
        break;
      }
    }
  else if ( this->AxisBranchFactor == 3 )
    {
    switch( this->Dimension )
      {
      case 3:
        this->NumberOfChildren = 27;
         for ( int i = 0; i < nCells; ++ i )
          {
          this->CellTree[i]=vtkCompactHyperTree<27>::New();
          }
       break;
      case 2:
        this->NumberOfChildren = 9;
        for ( int i = 0; i < nCells; ++ i )
          {
          this->CellTree[i]=vtkCompactHyperTree<9>::New();
          }
        break;
      case 1:
        this->NumberOfChildren = 3;
        for ( int i = 0; i < nCells; ++ i )
          {
          this->CellTree[i]=vtkCompactHyperTree<3>::New();
          }
        break;
      default:
        assert( "check: impossible case" && 0);
        break;
      }
    }
  else
    {
    vtkErrorMacro( "Bad branching factor " << this->AxisBranchFactor);
    }
  this->Modified();
  this->DeleteInternalArrays();
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::ComputeBounds()
{
  double tmp;

  if (this->XCoordinates == NULL || this->YCoordinates == NULL || 
      this->ZCoordinates == NULL)
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
    }

  if ( this->XCoordinates->GetNumberOfTuples() == 0 || 
       this->YCoordinates->GetNumberOfTuples() == 0 || 
       this->ZCoordinates->GetNumberOfTuples() == 0 )
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
    }

  this->Bounds[0] = this->XCoordinates->GetComponent(0, 0);
  this->Bounds[2] = this->YCoordinates->GetComponent(0, 0);
  this->Bounds[4] = this->ZCoordinates->GetComponent(0, 0);

  this->Bounds[1] = this->XCoordinates->GetComponent(
                        this->XCoordinates->GetNumberOfTuples()-1, 0);
  this->Bounds[3] = this->YCoordinates->GetComponent(
                        this->YCoordinates->GetNumberOfTuples()-1, 0);
  this->Bounds[5] = this->ZCoordinates->GetComponent(
                        this->ZCoordinates->GetNumberOfTuples()-1, 0);
  // ensure that the bounds are increasing
  for (int i = 0; i < 5; i += 2)
    {
    if (this->Bounds[i + 1] < this->Bounds[i])
      {
      tmp = this->Bounds[i + 1];
      this->Bounds[i + 1] = this->Bounds[i];
      this->Bounds[i] = tmp;
      }
    }
}

//-----------------------------------------------------------------------------
// Description:
// Return the number of levels.
// \post result_greater_or_equal_to_one: result>=1
int vtkHyperTreeGrid::GetNumberOfLevels( int i )
{
  int result = this->CellTree[i]->GetNumberOfLevels();
  assert( "post: result_greater_or_equal_to_one" && result>=1);
  return result;
}


//-----------------------------------------------------------------------------
// Description:
// Create a new cursor: an object that can traverse
// hyperTree cells.
vtkHyperTreeCursor *vtkHyperTreeGrid::NewCellCursor( int i, int j, int k )
{
  int index = ( k * this->GridSize[1] + j ) * this->GridSize[0] + i;
  vtkHyperTreeCursor *result = this->CellTree[index]->NewCursor();

  assert( "post: result_exists" && result!=0);

  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Subdivide node pointed by cursor, only if its a leaf.
// At the end, cursor points on the node that used to be leaf.
// \pre leaf_exists: leaf!=0
// \pre is_a_leaf: leaf->CurrentIsLeaf()
void vtkHyperTreeGrid::SubdivideLeaf(vtkHyperTreeCursor *leaf,vtkIdType i)
{
  assert( "pre: leaf_exists" && leaf!=0);
  assert( "pre: is_a_leaf" && leaf->CurrentIsLeaf() );
  this->CellTree[i]->SubdivideLeaf(leaf);
  this->DeleteInternalArrays();
}

//-----------------------------------------------------------------------------
// Description:
// Restore data object to initial state,
// THIS METHOD IS NOT THREAD SAFE.
void vtkHyperTreeGrid::Initialize()
{
  int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
  
  for ( int i = 0; i < nCells; ++ i )
    {
    this->CellTree[i]->Initialize();
    }

  this->DeleteInternalArrays();
}

//-----------------------------------------------------------------------------
// Description:
// Convenience method returns largest cell size in dataset. This is generally
// used to allocate memory for supporting data structures.
// This is the number of points of a cell.
// THIS METHOD IS THREAD SAFE
int vtkHyperTreeGrid::GetMaxCellSize()
{
  int result;
  switch( this->Dimension )
    {
    case 3:
      result = 8; // hexahedron=8 points
      break;
    case 2:
      result = 4; // quad=4 points
      break;
    case 1:
      result = 2; // line=2 points
      break;
    default:
      result = 0; // useless, just to avoid a warning
      assert( "check: impossible_case" && 0);
      break;
    }
  assert( "post: positive_result" && result>0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Shallow and Deep copy.
void vtkHyperTreeGrid::ShallowCopy(vtkDataObject *src)
{
  assert( "src_same_type" && vtkHyperTreeGrid::SafeDownCast(src)!=0);
  this->Superclass::ShallowCopy(src);
  this->CopyStructure(vtkHyperTreeGrid::SafeDownCast(src) );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeepCopy(vtkDataObject *src)
{
  assert( "src_same_type" && vtkHyperTreeGrid::SafeDownCast(src)!=0);
  this->Superclass::DeepCopy(src);
  this->CopyStructure(vtkHyperTreeGrid::SafeDownCast(src) );
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGrid::GetNumberOfLeaves()
{
  int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];

  int nLeaves = 0;
  for ( int i = 0; i < nCells; ++ i )
    {
    nLeaves += this->CellTree[i]->GetNumberOfLeaves();
    }

  return nLeaves;
}

//=============================================================================
// DataSet API that returns dual grid.

//-----------------------------------------------------------------------------
// Description:
// Return the number of leaves.
// \post positive_result: result>=0
vtkIdType vtkHyperTreeGrid::GetNumberOfCells()
{
  if ( this->DualGridFlag )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    return cornerLeafIds->GetNumberOfTuples();
    }
  else
    {
    int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
    
    int nLeaves = 0;
    for ( int i = 0; i < nCells; ++ i )
      {
      nLeaves += this->CellTree[i]->GetNumberOfLeaves();
      }

    return nLeaves;
    }
}

//-----------------------------------------------------------------------------
// Description:
// Return the number of points.
// \post positive_result: result>=0
vtkIdType vtkHyperTreeGrid::GetNumberOfPoints()
{
  if ( this->DualGridFlag )
    {
    vtkIdType nCells = this->GridSize[0]
      * this->GridSize[1]
      * this->GridSize[2];
    
    vtkIdType nLeaves = 0;
    for ( vtkIdType i = 0; i < nCells; ++ i )
      {
      nLeaves += this->CellTree[i]->GetNumberOfLeaves();
      }

    return nLeaves;
    }
  else
    {
    this->UpdateGridArrays();
    vtkPoints* cornerPoints = this->GetCornerPoints();
    return cornerPoints->GetNumberOfPoints();
    }
}


//-----------------------------------------------------------------------------
// Description:
// Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
// THIS METHOD IS NOT THREAD SAFE.
double *vtkHyperTreeGrid::GetPoint(vtkIdType ptId)
{
  if ( this->DualGridFlag )
    {
    this->UpdateDualArrays();
    vtkPoints* leafCenters = this->GetLeafCenters();
    assert( "Index out of bounds." &&
           ptId >= 0 && ptId < leafCenters->GetNumberOfPoints() );
    return leafCenters->GetPoint(ptId);
    }
  else
    {
    this->UpdateGridArrays();
    vtkPoints* cornerPoints = this->GetCornerPoints();
    assert( "Index out of bounds." &&
           ptId >= 0 && ptId < cornerPoints->GetNumberOfPoints() );
    return cornerPoints->GetPoint(ptId);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Copy point coordinates into user provided array x[3] for specified
// point id.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperTreeGrid::GetPoint(vtkIdType id, double x[3])
{
  if ( this->DualGridFlag )
    {
    this->UpdateDualArrays();
    vtkPoints* leafCenters = this->GetLeafCenters();
    assert( "Index out of bounds." &&
           id >= 0 && id < leafCenters->GetNumberOfPoints() );
    leafCenters->GetPoint(id, x);
    }
  else
    {
    this->UpdateGridArrays();
    vtkPoints* cornerPoints = this->GetCornerPoints();
    assert( "Index out of bounds." &&
           id >= 0 && id < cornerPoints->GetNumberOfPoints() );
    cornerPoints->GetPoint(id, x);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Get cell with cellId such that: 0 <= cellId < NumberOfCells.
// THIS METHOD IS NOT THREAD SAFE.
vtkCell *vtkHyperTreeGrid::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  int numPts = 1<<this->GetDimension();
  int ptIdx;
  double x[3];

  switch ( this->GetDimension() )
    {
    case 1:
      cell = this->Line;
      //return NULL; ???
      break;
    case 2:
      cell = this->Pixel;
      break;
    case 3:
      cell = this->Voxel;
      break;
    }

  if ( this->DualGridFlag )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    assert( "Index out of bounds." &&
           cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
    vtkPoints* leafCenters = this->GetLeafCenters();
    vtkIdType* ptr = cornerLeafIds->GetPointer( 0 ) + cellId*numPts;
    for (ptIdx = 0; ptIdx < numPts; ++ptIdx)
      {
      cell->PointIds->SetId(ptIdx, *ptr);
      leafCenters->GetPoint(*ptr, x);
      cell->Points->SetPoint(ptIdx,x);
      ++ptr;
      }
    }
  else
    {
    this->UpdateGridArrays();
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert( "Index out of bounds." &&
           cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples() );
    vtkPoints* cornerPoints = this->GetCornerPoints();
    vtkIdType* ptr = leafCornerIds->GetPointer( 0 ) + cellId*numPts;
    for (ptIdx = 0; ptIdx < numPts; ++ptIdx)
      {
      cell->PointIds->SetId(ptIdx, *ptr);
      cornerPoints->GetPoint(*ptr, x);
      cell->Points->SetPoint(ptIdx,x);
      ++ptr;
      }
    }

  return cell;
}

//-----------------------------------------------------------------------------
// Description:
// Get cell with cellId such that: 0 <= cellId < NumberOfCells.
// This is a thread-safe alternative to the previous GetCell()
// method.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperTreeGrid::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  int numPts = 1<<this->GetDimension();
  int ptIdx;
  double x[3];

  switch ( this->GetDimension() )
    {
    case 1:
      cell->SetCellTypeToLine();
      break;
    case 2:
      cell->SetCellTypeToPixel();
      break;
    case 3:
      cell->SetCellTypeToVoxel();
      break;
    }

  if ( this->DualGridFlag )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    assert( "Index out of bounds." &&
           cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
    vtkPoints* leafCenters = this->GetLeafCenters();
    vtkIdType* ptr = cornerLeafIds->GetPointer( 0 ) + cellId*numPts;
    for (ptIdx = 0; ptIdx < numPts; ++ptIdx)
      {
      cell->PointIds->SetId(ptIdx, *ptr);
      leafCenters->GetPoint(*ptr, x);
      cell->Points->SetPoint(ptIdx,x);
      ++ptr;
      }
    }
  else
    {
    this->UpdateGridArrays();
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert( "Index out of bounds." &&
           cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples() );
    vtkPoints* cornerPoints = this->GetCornerPoints();
    vtkIdType* ptr = leafCornerIds->GetPointer( 0 ) + cellId*numPts;
    for (ptIdx = 0; ptIdx < numPts; ++ptIdx)
      {
      cell->PointIds->SetId(ptIdx, *ptr);
      cornerPoints->GetPoint(*ptr, x);
      cell->Points->SetPoint(ptIdx,x);
      ++ptr;
      }
    }
}


//-----------------------------------------------------------------------------
// Description:
// Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
int vtkHyperTreeGrid::GetCellType(vtkIdType vtkNotUsed(cellId) )
{
  int result;
  switch( this->Dimension )
    {
    case 3:
      result = VTK_VOXEL; // hexahedron=8 points
      break;
    case 2:
      result = VTK_PIXEL; // quad=4 points
      break;
    case 1:
      result = VTK_LINE; // line=2 points
      break;
    default:
      result = 0; // useless, just to avoid a warning
      assert( "check: impossible_case" && 0);
      break;
    }
  assert( "post: positive_result" && result>0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Topological inquiry to get points defining cell.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperTreeGrid::GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
{
  int ii;
  int numPts = 1 << this->GetDimension();
  ptIds->Initialize();

  if ( this->DualGridFlag )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    assert( "Index out of bounds." &&
           cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
    vtkIdType* ptr = cornerLeafIds->GetPointer( 0 ) + cellId*numPts;
    for (ii = 0; ii < numPts; ++ ii)
      {
      ptIds->InsertId(ii, *ptr++);
      }
    }
  else
    {
    this->UpdateGridArrays();
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert( "Index out of bounds." &&
           cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples() );
    vtkIdType* ptr = leafCornerIds->GetPointer( 0 ) + cellId*numPts;
    for (ii = 0; ii < numPts; ++ ii)
      {
      ptIds->InsertId(ii, *ptr++);
      }
    }
}

//----------------------------------------------------------------------------
// Return a pointer to a list of point ids defining cell. (More efficient than alternative
// method.)
void vtkHyperTreeGrid::GetCellPoints( vtkIdType cellId,
                                      vtkIdType& npts,
                                      vtkIdType* &pts )
{
  if ( this->DualGridFlag )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    assert( "Index out of bounds." &&
           cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
    // Casting of 1 is necessary to remove 64bit Compiler warning C4334 on
    // Visual Studio 2005.
    npts = static_cast<vtkIdType>( 1 ) << this->GetDimension();
    pts = cornerLeafIds->GetPointer( 0 ) + cellId*npts;
    }
  else
    {
    this->UpdateGridArrays();
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert( "Index out of bounds." &&
           cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples() );
    // Casting of 1 is necessary to remove 64bit Compiler warning C4334 on
    // Visual Studio 2005.
    npts = static_cast<vtkIdType>( 1 ) << this->GetDimension();
    pts = leafCornerIds->GetPointer( 0 ) + cellId*npts;
    }
}


//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetPointCells( vtkIdType ptId, vtkIdList *cellIds )
{
  vtkIdType *cells;
  int numCells;
  int i;

  if ( ! this->Links )
    {
    this->BuildLinks();
    }
  cellIds->Reset();

  numCells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);

  cellIds->SetNumberOfIds(numCells);
  for (i=0; i < numCells; i++)
    {
    cellIds->SetId(i,cells[i]);
    }
}

//-----------------------------------------------------------------------------
// This is unecessary because we have the info already.
// Is it really a part of the vtkDataSet API?
// It would be better to build the links of both dual and grid.
void vtkHyperTreeGrid::BuildLinks()
{
  assert( "Not tested for 27 trees" && 0);
  this->Links = vtkCellLinks::New();
  this->Links->Allocate( this->GetNumberOfPoints() );
  this->Links->Register( this );
  this->Links->BuildLinks( this );
  this->Links->Delete();
}


//-----------------------------------------------------------------------------
// This is exactly the same as GetCellNeighbors in unstructured grid.
void vtkHyperTreeGrid::GetCellNeighbors( vtkIdType cellId, 
                                         vtkIdList* ptIds,
                                         vtkIdList* cellIds )
{
  int i, j, k;
  int numPts, minNumCells, numCells;
  vtkIdType *pts, ptId, *cellPts, *cells;
  vtkIdType *minCells = NULL;
  int match;
  vtkIdType minPtId = 0, npts;

  if ( ! this->Links )
    {
    this->BuildLinks();
    }

  cellIds->Reset();

  //Find the point used by the fewest number of cells
  //
  numPts = ptIds->GetNumberOfIds();
  pts = ptIds->GetPointer( 0 );
  for (minNumCells=VTK_LARGE_INTEGER,i=0; i<numPts; i++)
    {
    ptId = pts[i];
    numCells = this->Links->GetNcells(ptId);
    cells = this->Links->GetCells(ptId);
    if ( numCells < minNumCells )
      {
      minNumCells = numCells;
      minCells = cells;
      minPtId = ptId;
      }
    }

  if (minNumCells == VTK_LARGE_INTEGER && numPts == 0) {
    vtkErrorMacro( "input point ids empty." );
    minNumCells = 0;
  }
  //Now for each cell, see if it contains all the points
  //in the ptIds list.
  for (i=0; i<minNumCells; i++)
    {
    if ( minCells[i] != cellId ) //don't include current cell
      {
      this->GetCellPoints(minCells[i],npts,cellPts);
      for (match=1, j=0; j<numPts && match; j++) //for all pts in input cell
        {
        if ( pts[j] != minPtId ) //of course minPtId is contained by cell
          {
          for (match=k=0; k<npts; k++) //for all points in candidate cell
            {
            if ( pts[j] == cellPts[k] )
              {
              match = 1; //a match was found
              break;
              }
            }//for all points in current cell
          }//if not guaranteed match
        }//for all points in input cell
      if ( match )
        {
        cellIds->InsertNextId(minCells[i]);
        }
      }//if not the reference cell
    }//for all candidate cells attached to point
}


//----------------------------------------------------------------------------
// Note: This always returns the closest point, even if the point is outside
// tree.
// Since dual points are leaves, use the structure of the Tree instead
// of a point locator.
vtkIdType vtkHyperTreeGrid::FindPoint( double x[3] )
{
  assert( "Not tested for 27 trees, or normal grid" && 0);

  // Find cell to which this point belongs, or at least closest one
  vtkIdType ix = 0;
  vtkIdType nx = this->XCoordinates->GetNumberOfTuples();
  while ( ix < nx && x[0] > this->XCoordinates->GetTuple1( ix ) )
    {
    ++ ix;
    }
  if ( ix )
    {
    -- ix;
    }

  vtkIdType iy = 0;
  vtkIdType ny = this->YCoordinates->GetNumberOfTuples();
  while ( iy < ny && x[0] > this->YCoordinates->GetTuple1( iy ) )
    {
    ++ iy;
    }
  if ( iy )
    {
    -- iy;
    }

  vtkIdType iz = 0;
  vtkIdType nz = this->ZCoordinates->GetNumberOfTuples();
  while ( iz < nz && x[0] > this->ZCoordinates->GetTuple1( iz ) )
    {
    ++ iz;
    }
  if ( iz )
    {
    -- iz;
    }

  cerr << "Point " << x[0] << " " << x[1] << " " << x[2] << ": "
       << ix << " " << iy << " " << iz << endl;

  int index = ( iz * this->GridSize[1] + iy ) * this->GridSize[0] + ix;
  vtkHyperTreeLightWeightCursor cursor;
  cursor.Initialize( this->CellTree[index] );

  // Geometry of the cell
  double origin[3];
  this->XCoordinates->GetTuple( ix, origin );
  this->YCoordinates->GetTuple( iy, origin + 1);
  this->ZCoordinates->GetTuple( iz, origin + 2);

  double extreme[3];
  this->XCoordinates->GetTuple( ix + 1, extreme );
  this->YCoordinates->GetTuple( iy + 1, extreme + 1);
  this->ZCoordinates->GetTuple( iz + 1, extreme + 2);

  double size[3];
  size[0] = extreme[0] - origin[0];
  size[1] = extreme[1] - origin[1];
  size[2] = extreme[2] - origin[2];
        
  return this->RecursiveFindPoint(x, &cursor, origin, size );
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::RecursiveFindPoint( double x[3],
                                                vtkHyperTreeLightWeightCursor* cursor,
                                                double* origin, 
                                                double* size )
{
  if ( cursor->GetIsLeaf() )
    {
    return cursor->GetLeafIndex();
    }

  vtkHyperTreeLightWeightCursor newCursor;
  newCursor = *cursor;
  double newSize[3];
  double newOrigin[3];
  unsigned char child = 0;
  for ( int i = 0; i < 3; ++ i)
    {
    newSize[i] = size[i] * 0.5;
    newOrigin[i] = origin[i];
    if ( x[i] >= origin[i] + newSize[i] )
      {
      child = child | ( 1 << i );
      newOrigin[i] += newSize[i];
      }
    }
  newCursor.ToChild( child );

  return this->RecursiveFindPoint(x, &newCursor, newOrigin, newSize);
}

//----------------------------------------------------------------------------
// No need for a starting cell.  Just use the point.
// Tree is efficient enough.
vtkIdType vtkHyperTreeGrid::FindCell(double x[3], vtkCell* cell,
                                vtkGenericCell *gencell, vtkIdType cellId,
                                double tol2, int& subId, double pcoords[3],
                                double *weights)
{
  vtkIdType       ptId;
  double          closestPoint[3];
  double          dist2;
  vtkIdList       *cellIds;

  assert( "Not tested for 27 trees" && 0);

  ptId = this->FindPoint(x);
  if ( ptId < 0 )
    {
    return (-1); //if point completely outside of data
    }

  cellIds = vtkIdList::New();
  cellIds->Allocate( 8, 100 );
  this->GetPointCells(ptId, cellIds);
  if ( cellIds->GetNumberOfIds() <= 0 )
    {
    cellIds->Delete();
    return -1;
    }

  int num = cellIds->GetNumberOfIds();
  for ( int ii = 0; ii < num; ++ ii )
    {
    cellId = cellIds->GetId(ii);
    if ( gencell )
      {
      this->GetCell(cellId, gencell);
      }
    else
      {
      cell = this->GetCell(cellId);
      }

    // See whether this cell contains the point
    double dx[3];
    dx[0] = x[0];
    dx[1] = x[1];
    dx[2] = x[2];
    if ( ( gencell &&
           gencell->EvaluatePosition(dx,closestPoint,subId,
                                     pcoords, dist2,weights) == 1
           && dist2 <= tol2 )  ||
         ( !gencell &&
           cell->EvaluatePosition(dx,closestPoint,subId,
                                  pcoords, dist2,weights) == 1
           && dist2 <= tol2 ) )
      {
      cellIds->Delete();
      return cellId;
      }
    }

  // This should never happen.
  vtkErrorMacro( "Could not find cell." );
  return -1;
}


//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::FindCell(double x[3], vtkCell *cell, vtkIdType cellId,
                               double tol2, int& subId,double pcoords[3],
                               double *weights)
{
  assert( "Not tested for 27 trees" && 0);

  return
    this->FindCell( x, cell, NULL, cellId, tol2, subId, pcoords, weights );
}



//-----------------------------------------------------------------------------
// Generic way to set the leaf data attributes.
vtkDataSetAttributes* vtkHyperTreeGrid::GetLeafData()
{
  if ( this->DualGridFlag )
    {
    return this->PointData;
    }
  else
    {
    return this->CellData;
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetDualGridFlag( int flag )
{
  if ( flag )
    {
    flag = 1;
    }
  if ( ( this->DualGridFlag && ! flag ) || ( ! this->DualGridFlag && flag ) )
    { // Swap point and cell data.
    vtkDataSetAttributes* attr = vtkDataSetAttributes::New();
    attr->ShallowCopy( this->CellData );
    this->CellData->ShallowCopy( this->PointData );
    this->PointData->ShallowCopy( attr );
    attr->Delete();
    }
  this->DeleteInternalArrays();
  this->DualGridFlag = flag;
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned long vtkHyperTreeGrid::GetActualMemorySize()
{
  unsigned long size=this->vtkDataSet::GetActualMemorySize();
  int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
  
  for ( int i = 0; i < nCells; ++ i )
    {
    size += this->CellTree[i]->GetActualMemorySize();
    }

  if ( this->XCoordinates ) 
    {
    size += this->XCoordinates->GetActualMemorySize();
    }

  if ( this->YCoordinates ) 
    {
    size += this->YCoordinates->GetActualMemorySize();
    }

  if ( this->ZCoordinates ) 
    {
    size += this->ZCoordinates->GetActualMemorySize();
    }

  if ( this->LeafCenters)
    {
    size += this->LeafCenters->GetActualMemorySize();
    }
  if ( this->CornerLeafIds)
    {
    size += this->CornerLeafIds->GetActualMemorySize();
    }
  if ( this->CornerPoints)
    {
    size += this->CornerPoints->GetActualMemorySize();
    }
  if ( this->CornerLeafIds)
    {
    size += this->CornerLeafIds->GetActualMemorySize();
    }

  return size;
}

//=============================================================================
// Internal arrays used to generate dual grid.  Random access to cells
// requires the cell leaves connectively array which costs memory.


//-----------------------------------------------------------------------------
vtkPoints* vtkHyperTreeGrid::GetLeafCenters()
{
  this->UpdateDualArrays();
  return this->LeafCenters;
}

//-----------------------------------------------------------------------------
vtkIdTypeArray* vtkHyperTreeGrid::GetCornerLeafIds()
{
  this->UpdateDualArrays();
  return this->CornerLeafIds;
}

//-----------------------------------------------------------------------------
// Traverse tree with 3x3x3 super cursor.  Center cursor generates dual point
// Smallest leaf (highest level) owns corners/dual cell.  Ties are given to
// smallest index (z,y,x order)
// post: Generate LeafCenters and CornerLeafIds.
void vtkHyperTreeGrid::UpdateDualArrays()
{
  int numLeaves = 0;
  int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
  for ( int i = 0; i < nCells; ++ i )
    {
    numLeaves += this->CellTree[i]->GetNumberOfLeaves();
    }

  if ( this->LeafCenters )
    {
    if ( this->LeafCenters->GetNumberOfPoints() == numLeaves )
      {
      return;
      }
    this->LeafCenters->Delete();
    this->LeafCenters = 0;
    this->CornerLeafIds->Delete();
    this->CornerLeafIds = 0;
    }

  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();

  // Primal cell centers are dual points
  this->LeafCenters = vtkPoints::New();
  this->LeafCenters->Allocate( numLeaves );

  this->CornerLeafIds = vtkIdTypeArray::New();
  int dim = this->GetDimension();
  int numComps = 1 << dim;
  this->CornerLeafIds->SetNumberOfComponents( numComps );
  this->CornerLeafIds->Allocate( numLeaves * numComps );

  // Create an array of cursors that occupy 1 3x3x3 neighborhhod.  This
  // will traverse the tree as one.
  // Lower dimensions will not use them all.
  this->GenerateSuperCursorTraversalTable();

  // 3x3x3 has nothing to do with octree or 27tree
  int midCursorId = 0;
  if (dim == 1)
    {
    midCursorId = 1;
    }
  if (dim == 2)
    {
    midCursorId = 4;
    }
  if (dim == 3 )
    {
    midCursorId = 13;
    }

  // Iterate over all hyper trees
  for ( int i = 0; i < this->GridSize[0]; ++ i )
    {
    for ( int j = 0; j < this->GridSize[1]; ++ j )
      {
      for ( int k = 0; k < this->GridSize[2]; ++ k )
        {
        int index = ( k * this->GridSize[1] + j ) * this->GridSize[0] + i;
        vtkHyperTreeLightWeightCursor superCursor[27];
        superCursor[midCursorId].Initialize( this->CellTree[index] );

        // Location and size of the middle cursor/node
        double origin[3];
        this->XCoordinates->GetTuple( i, origin );
        this->YCoordinates->GetTuple( j, origin + 1);
        this->ZCoordinates->GetTuple( k, origin + 2);

        double extreme[3];
        this->XCoordinates->GetTuple( i + 1, extreme );
        this->YCoordinates->GetTuple( j + 1, extreme + 1);
        this->ZCoordinates->GetTuple( k + 1, extreme + 2);

        double size[3];
        size[0] = extreme[0] - origin[0];
        size[1] = extreme[1] - origin[1];
        size[2] = extreme[2] - origin[2];
        
        // Figure out necessary point insertion offset
        int ptOffset =  this->LeafCenters->GetNumberOfPoints();

        // Traverse and populate dual recursively
        this->TraverseDualRecursively( superCursor, ptOffset, origin, size, 0 );
        } // k
      } // j
    } // i

  timer->StopTimer();
  cerr << "Internal dual update : " << timer->GetElapsedTime() << endl;
  timer->Delete();
}

//-----------------------------------------------------------------------------
// Iterate over leaves.  Generate dual point.  Highest level (smallest leaf)
// owns the corner and generates that dual cell.
// Note: The recursion here is the same as TraverseGridRecursively.  The only
// difference is which arrays are generated.  We should merge the two.
void vtkHyperTreeGrid::TraverseDualRecursively( vtkHyperTreeLightWeightCursor* superCursor,
                                                int ptOffset,
                                                double* origin,
                                                double* size,
                                                int level )
{
  int midCursorId = 0;
  int numCursors = 1;
  switch ( this->GetDimension() )
    {
    case 1:
      midCursorId = 1;
      numCursors = 3;
      break;
    case 2:
      midCursorId = 4;
      numCursors = 9;
      break;
    case 3:
      midCursorId = 13;
      numCursors = 27;
      break;
    }
  // Level of the middle cursor.
  int midLevel = superCursor[midCursorId].GetLevel();
  if ( superCursor[midCursorId].GetIsLeaf() )
    { 
    // Center is a leaf. Make a dual point.
    double pt[3];

    // Start at minimum boundary of cell.
    pt[0] = origin[0];
    pt[1] = origin[1];
    pt[2] = origin[2];

    // Adjust point so the boundary of the dataset does not shrink.
    if ( superCursor[midCursorId-1].GetTree() && superCursor[midCursorId+1].GetTree() )
      {
      // Middle of cell
      pt[0] += size[0] * 0.5;
      }
    else if ( superCursor[midCursorId+1].GetTree() == 0)
      {
      // Move to maximum boundary of cell
      pt[0] += size[0];
      }
    if ( this->Dimension > 1 && superCursor[midCursorId-3].GetTree() && superCursor[midCursorId+3].GetTree() )
      {
      // Middle of cell
      pt[1] += size[1] * 0.5;
      }
    else if ( this->Dimension > 1 && superCursor[midCursorId+3].GetTree() == 0)
      {
      // Move to maximum boundary of cell
      pt[1] += size[1];
      }
    if ( this->Dimension > 2 && superCursor[midCursorId-9].GetTree() && superCursor[midCursorId+9].GetTree() )
      {
      // Middle of cell
      pt[2] += size[2] * 0.5;
      }
    else if ( this->Dimension > 2 && superCursor[midCursorId+9].GetTree() == 0)
      {
      // Move to maximum boundary of cell
      pt[2] += size[2];
      }

    // Insert point with given offset into leaf centers array
    int index =  ptOffset + superCursor[midCursorId].GetLeafIndex();
    this->LeafCenters->InsertPoint( index, pt );

    // Now see if the center leaf owns any of the corners.
    // If it does, create the dual cell.
    // Iterate over the corners around the middle leaf.
    int numLeavesCorners = 1 << this->Dimension;
    for ( int cornerIdx = 0; cornerIdx < numLeavesCorners; ++ cornerIdx )
      {
      bool owner = true;
      vtkIdType leaves[8];
      // Iterate over every leaf touching the corner.
      for ( int leafIdx = 0; leafIdx < numLeavesCorners && owner; ++ leafIdx )
        {
        // Compute the cursor index into the superCursor.
        int cursorIdx = 0;
        switch ( this->Dimension )
          {
          // Run through is intended
          case 3:
            cursorIdx += 9 * (((cornerIdx>>2)&1) + ((leafIdx>>2)&1) );
          case 2:
            cursorIdx += 3 * (((cornerIdx>>1)&1) + ((leafIdx>>1)&1) );
          case 1:
            cursorIdx += (cornerIdx&1) + (leafIdx&1);
          }
        // Collect the leaf indexes for the dual cell.
        leaves[leafIdx] = ptOffset + superCursor[cursorIdx].GetLeafIndex();

        // Compute if the mid leaf owns the corner.
        if ( cursorIdx != midCursorId )
          {
          vtkHyperTreeLightWeightCursor* cursor = superCursor+cursorIdx;
          if ( cursor->GetTree() == 0 || ! cursor->GetIsLeaf() )
            {
            // If neighbor leaf is out of bounds or has not been
            // refined to a leaf, this leaf does not own the corner.
            owner = false;
            }
          else if ( cursor->GetLevel() == midLevel && midCursorId < cursorIdx )
            {
            // A level tie is broken by index
            // The larger index wins.  We want all points set before
            // defining the cell.
            owner = false;
            }
          }
        }
      if ( owner )
        {
        this->CornerLeafIds->InsertNextTupleValue( leaves );
        }
      }
    // Middle cursor was a leaf,  terminate recursion
    return;
    }

  // Middle cursor is not leaf, must recurse deeper
  double childOrigin[3];
  double childSize[3];
  childSize[0] = size[0] / double( this->AxisBranchFactor );
  childSize[1] = size[1] / double( this->AxisBranchFactor );
  childSize[2] = size[2] / double( this->AxisBranchFactor );

  // We will not use all of these if dim < 3
  vtkHyperTreeLightWeightCursor newSuperCursor[27];
  int child;
  vtkSuperCursorEntry* cursorPtr = this->SuperCursorTraversalTable;
  for ( child = 0; child < this->NumberOfChildren; ++ child, cursorPtr += 27 )
    {
    int x,y,z;
    if ( this->AxisBranchFactor == 2 )
      {
      x = child&1;
      y = (child&2)>>1;
      z = (child&4)>>2;
      }
    else
      {
      z = child / 9;
      y = (child-z*9) / 3;
      x = child%3;
      }
    // Compute origin for child
    childOrigin[0] = origin[0] + ( x * childSize[0] );
    childOrigin[1] = origin[1] + ( y * childSize[1] );
    childOrigin[2] = origin[2] + ( z * childSize[2] );
    // Move each cursor in the superCursor down to a child.
    for ( int cursorIdx = 0; cursorIdx < numCursors; ++ cursorIdx )
      {
      // Extract the parent and child of the new node from the traversal table.
      // Child is encoded in the first three bits for all dimensions.
      int tChild = cursorPtr[cursorIdx].Child;
      int tParent = cursorPtr[cursorIdx].Parent;
      if ( ! superCursor[tParent].GetTree() )
        {
        // No node for this cursor.
        newSuperCursor[cursorIdx] = superCursor[tParent];
        }
      else if ( superCursor[tParent].GetIsLeaf() )
        {
        // Parent is a leaf.  Can't traverse anymore.
        // equal operator should work for this class.
        newSuperCursor[cursorIdx] = superCursor[tParent];
        }
      else
        {
        // Move to child.
        // equal operator should work for this class.
        newSuperCursor[cursorIdx] = superCursor[tParent];
        newSuperCursor[cursorIdx].ToChild(tChild);
        }
      }
    this->TraverseDualRecursively( newSuperCursor, ptOffset, childOrigin, childSize, level + 1 );
    }
}

//----------------------------------------------------------------------------
// Returns id if a new corner was created, -1 otherwise.
vtkIdType vtkHyperTreeGrid::EvaluateGridCorner( int level,
                                                vtkHyperTreeLightWeightCursor* superCursor,
                                                int lfOffset,
                                                unsigned char* visited,
                                                int* cornerCursorIds )
{
  // This is correct for 27trees too because it is the number of cells around a point.
  int numLeaves = 1 << this->GetDimension();
  int leaf;
  vtkIdType cornerId;

  for ( leaf = 0; leaf < numLeaves; ++ leaf )
    {
    // All corners must be leaves
    // Note: this test also makes sure all are initialized.
    if ( superCursor[cornerCursorIds[leaf]].GetTree() &&
        !superCursor[cornerCursorIds[leaf]].GetIsLeaf() )
      {
      return -1;
      }
    // If any cursor on the same level has already generated this point ...
    if ( superCursor[cornerCursorIds[leaf]].GetLevel() == level &&
        visited[superCursor[cornerCursorIds[leaf]].GetLeafIndex()])
      {
      return -1;
      }
    }

  // Point is actually inserted in the Traverse method that calls this method.
  cornerId = this->CornerPoints->GetNumberOfPoints();

  // Loop through the leaves to determine which use this point.
  for ( leaf = 0; leaf < numLeaves; ++ leaf )
    {
    if ( superCursor[cornerCursorIds[leaf]].GetTree() )
      {
      // We know it is a leaf from the previous check.
      // use bitwise exculsive or to find cursors of leaf.
      int leafId = superCursor[cornerCursorIds[leaf]].GetLeafIndex();
      int sideLeaf = leaf^1;
      if ( superCursor[cornerCursorIds[sideLeaf]].GetTree() &&
          leafId == superCursor[cornerCursorIds[sideLeaf]].GetLeafIndex() )
        {
        // Two cursors are the same.
        // We are not inserting face or edge points.
        continue;
        }
      if ( this->Dimension > 1)
        {
        sideLeaf = leaf^2;
        if ( superCursor[cornerCursorIds[sideLeaf]].GetTree() &&
            leafId == superCursor[cornerCursorIds[sideLeaf]].GetLeafIndex() )
          {
          // Two cursors are the same.
          // We are not inserting face or edge points.
          continue;
          }
        }
      if ( this->Dimension > 2)
        {
        sideLeaf = leaf^4;
        if ( superCursor[cornerCursorIds[sideLeaf]].GetTree() &&
            leafId == superCursor[cornerCursorIds[sideLeaf]].GetLeafIndex() )
          {
          // Two cursors are the same.
          // We are not inserting face or edge points.
          continue;
          }
        }
      // Center point is opposite to the leaf position in supercursor.
      leafId += lfOffset;
      this->LeafCornerIds->InsertComponent( leafId, 
                                            numLeaves-leaf-1,
                                            static_cast<double>( cornerId ) );
      }
    }

  return cornerId;
}



//-----------------------------------------------------------------------------
vtkPoints* vtkHyperTreeGrid::GetCornerPoints()
{
  this->UpdateGridArrays();
  return this->CornerPoints;
}

//-----------------------------------------------------------------------------
vtkIdTypeArray* vtkHyperTreeGrid::GetLeafCornerIds()
{
  this->UpdateGridArrays();
  return this->LeafCornerIds;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::UpdateGridArrays()
{
  int numLeaves = 0;
  int nCells = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
  
  for ( int i = 0; i < nCells; ++ i )
    {
    numLeaves += this->CellTree[i]->GetNumberOfLeaves();
    }

  if ( this->LeafCornerIds )
    {
    if ( this->LeafCornerIds->GetNumberOfTuples() == numLeaves )
      {
      return;
      }
    this->LeafCornerIds->Delete();
    this->LeafCornerIds = 0;
    this->CornerPoints->Delete();
    this->CornerPoints = 0;
    }

  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();

  // Primal corner points
  this->CornerPoints = vtkPoints::New();
  this->CornerPoints->Allocate( numLeaves );

  this->LeafCornerIds = vtkIdTypeArray::New();
  int dim = this->GetDimension();
  int numComps = 1 << dim;
  this->LeafCornerIds->SetNumberOfComponents( numComps );
  //this->LeafCornerIds->SetNumberOfTuples( numLeaves );

  // Create an array of cursors that occupy 1 3x3x3 neighborhhood.  This
  // will traverse the tree as one.
  // Lower dimensions will not use them all.
  this->GenerateSuperCursorTraversalTable();

  // 3x3x3 has nothing to do with octree or 27tree
  int midCursorId = 0;
  if (dim == 1)
    {
    midCursorId = 1;
    }
  if (dim == 2)
    {
    midCursorId = 4;
    }
  if (dim == 3 )
    {
    midCursorId = 13;
    }

  // Iterate over all hyper trees
  for ( int i = 0; i < this->GridSize[0]; ++ i )
    {
    for ( int j = 0; j < this->GridSize[1]; ++ j )
      {
      for ( int k = 0; k < this->GridSize[2]; ++ k )
        {
        int index = ( k * this->GridSize[1] + j ) * this->GridSize[0] + i;
        vtkHyperTreeLightWeightCursor superCursor[27];
        superCursor[midCursorId].Initialize( this->CellTree[index] );

        // Location and size for primal dataset API.
        double origin[3];
        this->XCoordinates->GetTuple( i, origin );
        this->YCoordinates->GetTuple( j, origin + 1);
        this->ZCoordinates->GetTuple( k, origin + 2);
        
        double extreme[3];
        this->XCoordinates->GetTuple( i + 1, extreme );
        this->YCoordinates->GetTuple( j + 1, extreme + 1);
        this->ZCoordinates->GetTuple( k + 1, extreme + 2);
        
        double size[3];
        size[0] = extreme[0] - origin[0];
        size[1] = extreme[1] - origin[1];
        size[2] = extreme[2] - origin[2];
        
        // Create a mask array to keep a record of which leaves have already
        // generated their corner cell entries
        unsigned char* leafMask = new unsigned char[numLeaves];

        // Initialize to 0
        memset(leafMask, 0, numLeaves);
        
        // Figure out necessary leaf corner insertion offset
        int lfOffset =  this->LeafCornerIds->GetNumberOfTuples();
        
        // Traverse and populate dual recursively
        this->TraverseGridRecursively( superCursor, lfOffset, leafMask, origin, size );

        delete [] leafMask;
        } // k
      } // j
    } // i

  timer->StopTimer();
  cerr << "Internal grid update : " << timer->GetElapsedTime() << endl;
  timer->Delete();
}

// I may be able to merge the two methods.
// Non dual create the corner points on the boundaries of the tree.
// Dual does not.
//----------------------------------------------------------------------------
// The purpose of traversing the supercursor / cells is to visit
// every corner and have the leaves connected to that corner.
void vtkHyperTreeGrid::TraverseGridRecursively( vtkHyperTreeLightWeightCursor* superCursor,
                                                int lfOffset,
                                                unsigned char* visited,
                                                double* origin,
                                                double* size )
{
  // This is the number of corners that a leaf has (valid for octrees and 27 trees)
  int numCorners = 1 << this->Dimension;
  int midCursorId = 0;
  int numCursors = 1;
  switch ( this->GetDimension() )
    {
    case 1:
      midCursorId = 1;
      numCursors = 3;
      break;
    case 2:
      midCursorId = 4;
      numCursors = 9;
      break;
    case 3:
      midCursorId = 13;
      numCursors = 27;
      break;
    }

  int cornerId;
  int cornerIds[8];
  int level = superCursor[midCursorId].GetLevel();
  if ( superCursor[midCursorId].GetIsLeaf() )
    {
    // Center is a leaf.
    // Evaluate each corner to see if we should process it now.
    // This is looping over the 8 corner points of the center cursor leaf.
    for ( int corner = 0; corner < numCorners; ++ corner )
      {
      // We will not use all of these if dim < 3, but generate anyway.
      // These are the cursor index (into the supercursor) of the eight
      // cursors (nodes) surrounding the corner.
      cornerIds[0] = (corner&1) + 3*((corner>>1)&1) + 9*((corner>>2)&1);
      cornerIds[1] = cornerIds[0] + 1;
      cornerIds[2] = cornerIds[0] + 3;
      cornerIds[3] = cornerIds[1] + 3;
      cornerIds[4] = cornerIds[0] + 9;
      cornerIds[5] = cornerIds[1] + 9;
      cornerIds[6] = cornerIds[2] + 9;
      cornerIds[7] = cornerIds[3] + 9;
      cornerId = this->EvaluateGridCorner( level,
                                           superCursor,
                                           lfOffset,
                                           visited,
                                           cornerIds );
      if ( cornerId >= 0 )
        {
        // A bit funny inserting the point here, but we need the
        // to determine the id for the corner leaves in EvaluateGridCorner,
        // and I do not want to compute the point unless absolutely necessary.
        double pt[3];

        // Create the corner point.
        pt[0] = origin[0];
        if ( corner&1 )
          {
          pt[0] += size[0];
          }
        pt[1] = origin[1];
        if ( (corner>>1)&1 )
          {
          pt[1] += size[1];
          }
        pt[2] = origin[2];
        if ( (corner>>2)&1 )
          {
          pt[2] += size[2];
          }
        this->CornerPoints->InsertPoint( cornerId, pt );
        }
      }
    // Mark this leaf as visited.
    // Neighbor value is leafId for leaves, nodeId for nodes.
    visited[superCursor[midCursorId].GetLeafIndex()] = 1;
    return;
    }

  // Now recurse.
  double childOrigin[3];
  double childSize[3];
  childSize[0] = size[0]/double( this->AxisBranchFactor);
  childSize[1] = size[1]/double( this->AxisBranchFactor);
  childSize[2] = size[2]/double( this->AxisBranchFactor);
  // We will not use all of these if dim < 3.
  vtkHyperTreeLightWeightCursor newSuperCursor[27];
  int child;
  vtkSuperCursorEntry* cursorPtr = this->SuperCursorTraversalTable;
  for ( child = 0; child < this->NumberOfChildren; ++ child, cursorPtr += 27 )
    {
    int x,y,z;
    if ( this->AxisBranchFactor == 2 )
      {
      x = child & 1;
      y = ( child & 2 ) >> 1;
      z = ( child & 4 ) >> 2;
      }
    else
      {
      z = child / 9;
      y = ( child - z * 9 ) / 3;
      x = child % 3;
      }

    // Compute origin for child
    childOrigin[0] = origin[0] + ( x * childSize[0] );
    childOrigin[1] = origin[1] + ( y * childSize[1] );
    childOrigin[2] = origin[2] + ( z * childSize[2] );

    // Move each cursor in the superCursor down to a child.
    for ( int cursorIdx = 0; cursorIdx < numCursors; ++ cursorIdx )
      {
      // Extract the parent and child of the new node from the traversal table.
      // Child is encoded in the first three bits for all dimensions.
      int tChild = cursorPtr[cursorIdx].Child;
      int tParent = cursorPtr[cursorIdx].Parent;
      if ( superCursor[tParent].GetTree() == 0)
        {
        // No node for this cursor.
        newSuperCursor[cursorIdx] = superCursor[tParent];
        }
      else if ( superCursor[tParent].GetIsLeaf() )
        {
        // Parent is a leaf.  Can't traverse anymore.
        // equal operator should work for this class.
        newSuperCursor[cursorIdx] = superCursor[tParent];
        }
      else
        {
        // Move to child.
        // equal operator should work for this class.
        newSuperCursor[cursorIdx] = superCursor[tParent];
        newSuperCursor[cursorIdx].ToChild(tChild);
        }
      }
    this->TraverseGridRecursively( newSuperCursor, 
                                   lfOffset,
                                   visited,
                                   childOrigin, 
                                   childSize);
    }
}

//----------------------------------------------------------------------------
// This table is used to move a 3x3x3 neighborhood of cursors through the tree.
void vtkHyperTreeGrid::GenerateSuperCursorTraversalTable()
{
  int xChildDim = 1;
  int yChildDim = 1;
  int zChildDim = 1;
  int xCursorDim = 1;
  int yCursorDim = 1;
  int zCursorDim = 1;

  assert( "Dimension cannot be 0." && this->GetDimension() );

  switch ( this->GetDimension() )
    {
    case 1:
      xChildDim = this->AxisBranchFactor;
      xCursorDim = 3;
      break;
    case 2:
      xChildDim = yChildDim = this->AxisBranchFactor;
      xCursorDim = yCursorDim = 3;
      break;
    case 3:
      xChildDim = yChildDim = zChildDim = this->AxisBranchFactor;
      xCursorDim = yCursorDim = zCursorDim = 3;
      break;
    }

  int fac = this->AxisBranchFactor;
  int childIdx = 0;
  for ( int zChild = 0; zChild < zChildDim; ++ zChild )
    {
    for ( int yChild = 0; yChild < yChildDim; ++ yChild )
      {
      for ( int xChild = 0; xChild < xChildDim; ++ xChild )
        {
        int cursorIdx = 0;
        for ( int zCursor = 0; zCursor < zCursorDim; ++ zCursor )
          {
          for ( int yCursor = 0; yCursor < yCursorDim; ++ yCursor )
            {
            for ( int xCursor = 0; xCursor < xCursorDim; ++ xCursor )
              {
              // Compute the x, y, z index into the
              // 6x6x6 (9x9x9) neighborhood of children.
              int xNeighbor = xCursor + xChild + xChildDim - 1;
              int yNeighbor = yCursor + yChild + yChildDim - 1;
              int zNeighbor = zCursor + zChild + zChildDim - 1;

              // Separate neighbor index into Cursor/Child index.
              int xNewCursor = xNeighbor / fac;
              int yNewCursor = yNeighbor / fac;
              int zNewCursor = zNeighbor / fac;
              int xNewChild = xNeighbor - xNewCursor*fac;
              int yNewChild = yNeighbor - yNewCursor*fac;
              int zNewChild = zNeighbor - zNewCursor*fac;
              int tableIdx = childIdx * 27 + cursorIdx;
              this->SuperCursorTraversalTable[tableIdx].Parent
                = xNewCursor + 3 * ( yNewCursor + 3 * zNewCursor );
              this->SuperCursorTraversalTable[tableIdx].Child
                = xNewChild + fac * ( yNewChild + fac * zNewChild );
              ++ cursorIdx;
              }
            }
          }
        ++ childIdx;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeleteInternalArrays()
{
  if ( this->LeafCenters )
    {
    this->LeafCenters->Delete();
    this->LeafCenters = 0;
    }
  if ( this->CornerLeafIds )
    {
    this->CornerLeafIds->Delete();
    this->CornerLeafIds = 0;
    }
  if ( this->CornerPoints )
    {
    this->CornerPoints->Delete();
    this->CornerPoints = 0;
    }
  if ( this->LeafCornerIds )
    {
    this->LeafCornerIds->Delete();
    this->LeafCornerIds = 0;
    }

  if ( this->Links )
    {
    this->Links->Delete();
    this->Links = 0;
    }
}

//=============================================================================
// Lightweight cursor
// Implemented here because it needs access to the internal classes.

//-----------------------------------------------------------------------------
// Constructor.
vtkHyperTreeLightWeightCursor::vtkHyperTreeLightWeightCursor()
{
  this->Level = 0;
  this->IsLeaf = 0;
  this->Index = 0;
  this->Tree = 0;
}

//-----------------------------------------------------------------------------
// Destructor.
vtkHyperTreeLightWeightCursor::~vtkHyperTreeLightWeightCursor()
{
  this->Level = 0;
  this->IsLeaf = 1;
  this->Index = 0;
  // I can't reference count because of the default copy constructor.
  //if ( this->Tree)
  //  {
  //  this->Tree->UnRegister( 0 );
  //  }
  this->Tree = 0;
}


//-----------------------------------------------------------------------------
void vtkHyperTreeLightWeightCursor::Initialize( vtkHyperTreeInternal* tree )
{
  //if ( this->Tree)
  //  {
  //  this->Tree->UnRegister( 0 );
  //  }
  this->Tree = tree;
  if (tree == 0)
    {
    return;
    }
  //this->Tree->Register( 0 );

  this->ToRoot();
}

//-----------------------------------------------------------------------------
unsigned short vtkHyperTreeLightWeightCursor::GetIsLeaf()
{
  // I want enpty cursors to appear like a leaf so recursion stops.
  if ( this->Tree == 0)
    {
    return 1;
    }
  return this->IsLeaf;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeLightWeightCursor::ToRoot()
{
  if ( this->Tree == 0)
    {
    return;
    }
  this->Level = 0;
  if ( this->Tree->GetNumberOfLeaves() == 1)
    { // Root is a leaf.
    this->Index = 0;
    this->IsLeaf = 1;
    }
  else
    { // Root is a node.
    this->Index = 1; // First node ( 0 ) is a special empty node.
    this->IsLeaf = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeLightWeightCursor::ToChild(int child)
{
  if ( this->Tree == 0 )
    {
    return;
    }
  if ( this->IsLeaf )
    {
    // Leaves do not have children.
    return;
    }

  if ( this->Tree->GetDimension() == 3 )
    {
    switch ( this->Tree->GetBranchFactor() )
      {
      case 2:
        {
        vtkCompactHyperTree<8>* tree3;
        tree3 = static_cast<vtkCompactHyperTree<8>*>( this->Tree );
        vtkCompactHyperTreeNode<8> *node=tree3->GetNode( this->Index );
        this->Index=node->GetChild( child );
        this->IsLeaf=node->IsChildLeaf( child );
        break;
        }
      case 3:
        {
        vtkCompactHyperTree<27>* tree3;
        tree3 = static_cast<vtkCompactHyperTree<27>*>( this->Tree );
        vtkCompactHyperTreeNode<27> *node=tree3->GetNode( this->Index );
        this->Index=node->GetChild( child );
        this->IsLeaf=node->IsChildLeaf( child );
        break;
        }
      default:
        assert( "Bad branch factor " && 0);
      }

    this->Level += 1;
    assert( "Bad index" && this->Index >= 0);
    if ( this->IsLeaf)
      {
      assert( "Bad leaf index" && this->Index < this->Tree->GetNumberOfLeaves() );
      }
    else
      {
      assert( "Bad node index" && this->Index < this->Tree->GetNumberOfNodes() );
      }
    }
  else if ( this->Tree->GetDimension() == 2 )
    {
      switch ( this->Tree->GetBranchFactor() )
      {
      case 2:
        {
        vtkCompactHyperTree<4>* tree2;
        tree2 = static_cast<vtkCompactHyperTree<4>*>( this->Tree );
        vtkCompactHyperTreeNode<4> *node=tree2->GetNode( this->Index);
        this->Index=node->GetChild( child );
        this->IsLeaf=node->IsChildLeaf( child );
        break;
        }
      case 3:
        {
        vtkCompactHyperTree<9>* tree2;
        tree2 = static_cast<vtkCompactHyperTree<9>*>( this->Tree );
        vtkCompactHyperTreeNode<9> *node=tree2->GetNode( this->Index);
        this->Index=node->GetChild( child );
        this->IsLeaf=node->IsChildLeaf( child );
        break;
        }
      default:
        vtkGenericWarningMacro( "Bad branch factor" );
      }
    this->Level += 1;
    }
  else if ( this->Tree->GetDimension() == 1 )
    {
      switch ( this->Tree->GetBranchFactor() )
      {
      case 2:
        {
        vtkCompactHyperTree<2>* tree1;
        tree1 = static_cast<vtkCompactHyperTree<2>*>( this->Tree );
        vtkCompactHyperTreeNode<2> *node=tree1->GetNode( this->Index);
        this->Index=node->GetChild( child );
        this->IsLeaf=node->IsChildLeaf( child );
        break;
        }
      case 3:
        {
        vtkCompactHyperTree<3>* tree1;
        tree1 = static_cast<vtkCompactHyperTree<3>*>( this->Tree );
        vtkCompactHyperTreeNode<3> *node=tree1->GetNode( this->Index);
        this->Index=node->GetChild( child );
        this->IsLeaf=node->IsChildLeaf( child );
        break;
        }
      default:
        vtkGenericWarningMacro( "Bad branch factor" );
      }
    this->Level += 1;
    }
  return;
}
