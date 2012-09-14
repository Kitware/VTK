/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctree.h"

#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkCellType.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkHyperOctreePointsGrabber.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkTimerLog.h"
#include "vtkVoxel.h"

#include <deque>
//#include <set>
#include <vector>

#include <assert.h>

vtkInformationKeyMacro(vtkHyperOctree, LEVELS, Integer);
vtkInformationKeyMacro(vtkHyperOctree, DIMENSION, Integer);
vtkInformationKeyRestrictedMacro(vtkHyperOctree, SIZES, DoubleVector, 3);

class vtkHyperOctreeInternal
  : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperOctreeInternal,vtkObject);
  virtual void Initialize()=0;
  virtual vtkHyperOctreeCursor *NewCursor()=0;
  virtual vtkIdType GetNumberOfLeaves()=0;
  virtual int GetNumberOfNodes()=0;

  // Description:
  // Return the number of levels.
  // \post result_greater_or_equal_to_one: result>=1
  virtual vtkIdType GetNumberOfLevels()=0;

  // Description:
  // Subdivide node pointed by cursor, only if its a leaf.
  // At the end, cursor points on the node that used to be leaf.
  // \pre leaf_exists: leaf!=0
  // \pre is_a_leaf: leaf->CurrentIsLeaf()
  virtual void SubdivideLeaf(vtkHyperOctreeCursor *leaf)=0;

  // Description:
  // Collapse a node for which all children are leaves.
  // At the end, cursor points on the leaf that used to be a node.
  // \pre node_exists: node!=0
  // \pre node_is_node: !node->CurrentIsLeaf()
  // \pre children_are_leaves: node->CurrentIsTerminalNode()
  virtual void CollapseTerminalNode(vtkHyperOctreeCursor *node)=0;

  // Description:
  // Set the internal attributes.
  // \pre attributes_exist: attributes!=0
  virtual void SetAttributes(vtkDataSetAttributes *attributes)=0;

  // Description:
  // Returns the actual memory size in kilobytes.
  // Ignores the attribute array.
  virtual unsigned int GetActualMemorySize() = 0;

protected:
  vtkHyperOctreeInternal()
    {
    }

private:
  vtkHyperOctreeInternal(const vtkHyperOctreeInternal &);  // Not implemented.
  void operator=(const vtkHyperOctreeInternal &);    // Not implemented.
};

//-----------------------------------------------------------------------------
// because the PrintSelf test is not smart, PrintSelf has to be here.
void vtkHyperOctree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dimension: "<<this->Dimension<<endl;
  os << indent << "Size: "<<this->Size[0]<<","<<this->Size[1]<<",";
  os <<this->Size[2]<<endl;
  os << indent << "origin: "<<this->Origin[0]<<","<<this->Origin[1]<<",";
  os <<this->Origin[2]<<endl;

  os << indent << "DualGridFlag: " << this->DualGridFlag << endl;

  this->CellTree->PrintSelf(os,indent);
}



template<unsigned int D> class vtkCompactHyperOctree;
template<unsigned int D> class vtkCompactHyperOctreeNode;

template<unsigned int D> class vtkCompactHyperOctreeCursor
  :public vtkHyperOctreeCursor
{
public:
  //---------------------------------------------------------------------------
  static vtkCompactHyperOctreeCursor<D> *New()
    {
      vtkObject *ret=
        vtkObjectFactory::CreateInstance("vtkCompactHyperOctreeCursor<D>");

      if(ret!=0)
        {
        return static_cast<vtkCompactHyperOctreeCursor<D> *>(ret);
        }
      else
        {
        return new vtkCompactHyperOctreeCursor<D>;
        }
    }

  vtkTypeMacro(vtkCompactHyperOctreeCursor<D>,vtkHyperOctreeCursor);

  //---------------------------------------------------------------------------
  // Initialization
  virtual void Init(vtkCompactHyperOctree<D> *tree)
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
      assert("pre: is_leaf" && CurrentIsLeaf());
      return this->Cursor;
    }

  // Status
  virtual int CurrentIsLeaf()
    {
      return this->IsLeaf;
    }

  virtual int CurrentIsRoot()
    {
      return (this->IsLeaf && this->Cursor==0 && this->Tree->GetLeafParentSize()==1) || (!this->IsLeaf && this->Cursor==1);
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the level of the node pointed by the cursor.
  // \post positive_result: result>=0
  virtual int GetCurrentLevel()
    {
      int result=this->GetChildHistorySize();
      assert("post: positive_result" && result>=0);
      return result;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the child number of the current node relative to its parent.
  // \pre not_root: !CurrentIsRoot().
  // \post valid_range: result>=0 && result<GetNumberOfChildren()
  virtual int GetChildIndex()
    {
      assert("post: valid_range" && this->ChildIndex>=0 && this->ChildIndex<GetNumberOfChildren());
      return this->ChildIndex;
    }

  //---------------------------------------------------------------------------
  // Are the children of the current node all leaves?
  // This query can be called also on a leaf node.
  // \post compatible: result implies !CurrentIsLeaf()
  virtual int CurrentIsTerminalNode()
    {
      int result=!this->IsLeaf;
      if(result)
        {
        vtkCompactHyperOctreeNode<D> *node=this->Tree->GetNode(this->Cursor);
        result=node->IsTerminalNode();
        }
      // A=>B: notA or B
      assert("post: compatible" && (!result || !this->IsLeaf));
      return result;
    }

  //---------------------------------------------------------------------------
  // Cursor movement.
  // \pre can be root
  // \post is_root: CurrentIsRoot()
  virtual void ToRoot()
    {
      this->ChildHistory.clear();
      this->IsLeaf=this->Tree->GetLeafParentSize()==1;
      if(this->IsLeaf)
        {
        this->Cursor=0;
        }
      else
        {
        this->Cursor=1;
        }
      this->ChildIndex=0;
      unsigned int i=0;
      while(i<D)
        {
        this->Index[i]=0;
        ++i;
        }
    }

  //---------------------------------------------------------------------------
  // \pre not_root: !CurrentIsRoot()
  virtual void ToParent()
    {
      assert("pre: not_root" && !CurrentIsRoot());
      if(this->IsLeaf)
        {
        this->Cursor=this->Tree->GetLeafParent(this->Cursor);
        }
      else
        {
        this->Cursor=this->Tree->GetNode(this->Cursor)->GetParent();
        }
      this->IsLeaf=0;
      this->ChildIndex=this->ChildHistory.back(); // top()
      this->ChildHistory.pop_back();

      unsigned int i=0;
      while(i<D)
        {
        this->Index[i]=(this->Index[i])>>1;
        ++i;
        }
    }

  //---------------------------------------------------------------------------
  // \pre not_leaf: !CurrentIsLeaf()
  // \pre valid_child: child>=0 && child<this->GetNumberOfChildren()
  virtual void ToChild(int child)
    {
      assert("pre: not_leaf" && !CurrentIsLeaf());
      assert("pre: valid_child" && child>=0 && child<this->GetNumberOfChildren());

      vtkCompactHyperOctreeNode<D> *node=this->Tree->GetNode(this->Cursor);
      this->ChildHistory.push_back(this->ChildIndex);
      this->ChildIndex=child;
      this->Cursor=node->GetChild(child);
      this->IsLeaf=node->IsChildLeaf(child);
      unsigned int i=0;
      int mask=1;
      while(i<D)
        {
        int index=(child & mask )>>i;
        assert("check: binary_value" && index>=0 && index<=1);
        this->Index[i]=((this->Index[i])<<1)+index;
        ++i;
        mask<<=1;
        }
    }

  //---------------------------------------------------------------------------
  // Description:
  // Move the cursor to the same node pointed by `other'.
  // \pre other_exists: other!=0
  // \pre same_hyperoctree: this->SameTree(other)
  // \post equal: this->IsEqual(other)
  virtual void ToSameNode(vtkHyperOctreeCursor *other)
    {
      assert("pre: other_exists" && other!=0);
      assert("pre: same_hyperoctree" && this->SameTree(other));

      vtkCompactHyperOctreeCursor<D> *o=static_cast<vtkCompactHyperOctreeCursor<D> *>(other);

      this->Cursor=o->Cursor;
      this->ChildIndex=o->ChildIndex;
      this->IsLeaf=o->IsLeaf;
      this->ChildHistory=o->ChildHistory; // use assignment operator
      unsigned int i=0;
      while(i<D)
        {
        this->Index[i]=o->Index[i];
        ++i;
        }
      assert("post: equal" && this->IsEqual(other));
    }
  //--------------------------------------------------------------------------
   // Description:
  // Is `this' equal to `other'?
  // \pre other_exists: other!=0
  // \pre same_hyperoctree: this->SameTree(other);
  virtual int IsEqual(vtkHyperOctreeCursor *other)
    {
      assert("pre: other_exists" && other!=0);
      assert("pre: same_hyperoctree" && this->SameTree(other));

      vtkCompactHyperOctreeCursor<D> *o=static_cast<vtkCompactHyperOctreeCursor<D> *>(other);

      int result=this->Cursor==o->Cursor && this->ChildIndex==o->ChildIndex
        && this->IsLeaf==o->IsLeaf && this->ChildHistory==o->ChildHistory;

      unsigned int i=0;
      while(result && i<D)
        {
        result=this->Index[i]==o->Index[i];
        ++i;
        }
      return result;
    }

  //--------------------------------------------------------------------------
  // Description:
  // Create a copy of `this'.
  // \post results_exists:result!=0
  // \post same_tree: result->SameTree(this)
  virtual vtkHyperOctreeCursor *Clone()
    {
      vtkCompactHyperOctreeCursor<D> *result=this->NewInstance();
      result->Tree=this->Tree;
      assert("post: results_exists" && result!=0);
      assert("post: same_tree" && result->SameTree(this));
      return result;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Are `this' and `other' pointing on the same hyperoctree?
  // \pre other_exists: other!=0
  virtual int SameTree(vtkHyperOctreeCursor *other)
    {
      assert("pre: other_exists" && other!=0);
      vtkCompactHyperOctreeCursor<D> *o=vtkCompactHyperOctreeCursor<D>::SafeDownCast(other);
      int result=o!=0;
      if(result)
        {
        result=this->Tree==o->Tree;
        }
      return result;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index in dimension `d', as if the node was a cell of a
  // uniform grid of 1<<GetCurrentLevel() cells in each dimension.
  // \pre valid_range: d>=0 && d<GetDimension()
  // \post valid_result: result>=0 && result<(1<<GetCurrentLevel())
  virtual int GetIndex(int d)
    {
      assert("pre: valid_range" &&  d>=0 && d<this->GetDimension());

      int result=this->Index[d];

      assert("post: valid_result" && result>=0 && result<(1<<this->GetCurrentLevel()));
      return result;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the number of children for each node of the tree.
  // \post positive_number: result>0
  virtual int GetNumberOfChildren()
    {
      return 1<<D;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the dimension of the tree.
  // \post positive_result: result>=0
  virtual int GetDimension()
    {
      assert("post: positive_result " && D>0);
      assert("post: up_to_3 " && D<=3); // and then
      return D;
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
      assert("pre: indices_exists" && indices!=0);
      assert("pre: valid_level" && level>=0);

      this->ToRoot();
      int currentLevel=0;

      int i;

      int child;

      int mask=1<<(level-1);

      while(!this->CurrentIsLeaf() && currentLevel<level)
        {
        i=D-1;
        child=0;
        while(i>=0)
          {
          child<<=1;
          child+=((indices[i]&mask)==mask);
          --i;
          }
        this->ToChild(child);
        ++currentLevel;
        mask>>=1;
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
  // Public only for vtkCompactHyperOctree.
  void SetIsLeaf(int value)
    {
      this->IsLeaf=value;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperOctree.
  void SetChildIndex(int childIndex)
    {
      assert("pre: valid_range" && childIndex>=0 && childIndex<GetNumberOfChildren());
      this->ChildIndex=childIndex;
      assert("post: is_set" && childIndex==GetChildIndex());
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperOctree.
  void SetCursor(int cursor)
    {
      assert("pre: positive_cursor" && cursor>=0);
      this->Cursor=cursor;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for vtkCompactHyperOctree.
  vtkIdType GetChildHistorySize()
    {
      return this->ChildHistory.size();
    }

protected:
  //---------------------------------------------------------------------------
  vtkCompactHyperOctreeCursor()
    {
      this->Tree=0;
      this->Cursor=0;
      this->IsLeaf=0;
      this->ChildIndex=0;
      unsigned int i=0;
      while(i<D)
        {
        this->Index[i]=0;
        ++i;
        }
    }

  vtkCompactHyperOctree<D> *Tree;
  int Cursor; // index either in the Nodes or Parents (if leaf)
  int ChildIndex; // the current node is
  // child number ChildIndex (in [0,1<<D-1]) for its parent node

  int IsFound;
  int IsLeaf;

  std::deque<int> ChildHistory; // a stack, but stack does not have clear()
  int Index[D]; // index in each dimension of the current node, as if the
  // tree at the current level was a uniform grid.
private:
  vtkCompactHyperOctreeCursor(const vtkCompactHyperOctreeCursor<D> &);  // Not implemented.
  void operator=(const vtkCompactHyperOctreeCursor<D> &);    // Not implemented.
};

// D is the dimension of the space
// D>=1 && D<=3
// So its not really a generi class because the template argument
// is not a class but a value.

// Description:
// A node of the octree which is not a leaf.
template<unsigned int D> class vtkCompactHyperOctreeNode
{
public:
  //---------------------------------------------------------------------------
  // Description:
  // See GetParent().
  void SetParent(int parent)
    {
      assert("pre: positive_parent" && parent>=0);
      this->Parent=parent;
      assert("post: is_set" && parent==this->GetParent());
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index of the parent node of the current node in the
  // nodes array of the hyperoctree.
  int GetParent()
    {
      assert("post: positive_result" && this->Parent>=0);
      return this->Parent;
    }

  //---------------------------------------------------------------------------
  // Description:
  // See GetLeafFlags()
  void SetLeafFlags(int leafFlags)
    {
      this->LeafFlags=leafFlags;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the flag field. Bit i tells if the i-th child of the current node
  // is a leaf or not. Because the size of the field is limited to 8 bits,
  // the template parameter D is constrained to be 3 or less.
  unsigned char GetLeafFlags()
    {
      return this->LeafFlags;
    }

  //---------------------------------------------------------------------------
  // Description
  // Are children all leaves?
  int IsTerminalNode()
    {
      // this trick set 2^D less significant bits to 1 and the other
      // more significants bits to 0;
      unsigned char mask=(1<<(1<<D))-1;
      return this->LeafFlags & mask;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Is the `i'-th child of the node a leaf ?
  int IsChildLeaf(int i)
    {
      assert("pre: valid_range" && i>=0 && i<(1<<D));
      return this->LeafFlags>>i & 1;
    }

  //---------------------------------------------------------------------------
  // Description:
  // See GetChild().
  void SetChild(int i,
                int child)
    {
      assert("pre: valid_range" && i>=0 && i<(1<<D));
      assert("pre: positive_child" && child>=0);
      this->Children[i]=child;
      assert("post: is_set" && child==this->GetChild(i));
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return the index of of the 'i'-th child. If the result of
  // IsChildLeaf(i) is true, the index points to an element in the LeafParent
  // and Attribute arrays of the hyperoctree class. If not, the index points to
  // an element in the Nodes array of the hyperoctree class.
  int GetChild(int i)
    {
      assert("pre: valid_range" && i>=0 && i<(1<<D));
      assert("post: positive_result" && this->Children[i]>=0);
      return this->Children[i];
    }

  //---------------------------------------------------------------------------
  void PrintSelf(ostream& os, vtkIndent indent)
    {
      os << indent << "Parent=" << this->Parent<<endl;

//      std::bitset<8> b=this->LeafFlags;

      os << indent << "LeafFlags="<<static_cast<int>(this->LeafFlags)<<" ";

      int i=0;
      int c=1<<D;
      int mask=128;
      while(i<c)
        {
        os<<((static_cast<int>(this->LeafFlags)&mask)==mask);
        ++i;
        mask>>=1;
        }
      os<<endl;

      i=0;
      while(i<c)
        {
        os<<indent<<this->Children[i]<<endl;
        ++i;
        }
    }

protected:
  int Parent; // index
  unsigned char LeafFlags; // each bit tells if the related child is a leaf or
  // not. Because this flag as 8 bits, it limits D to be 3 or less.
  int Children[1<<D]; // indices
};

template<unsigned int D> class vtkCompactHyperOctree
  : public vtkHyperOctreeInternal
{
public:
  //---------------------------------------------------------------------------
  static vtkCompactHyperOctree<D> *New()
    {
      vtkObject *ret=
        vtkObjectFactory::CreateInstance("vtkCompactHyperOctree<D>");

      if(ret!=0)
        {
        return static_cast<vtkCompactHyperOctree<D> *>(ret);
        }
      else
        {
        return new vtkCompactHyperOctree<D>;
        }
    }

  vtkTypeMacro(vtkCompactHyperOctree<D>,vtkHyperOctreeInternal);

  //---------------------------------------------------------------------------
  // Description:
  // Restore the initial state: only one node and one leaf: the root.
  // Attributes is empty.
  virtual void Initialize()
    {
      this->Nodes.resize(1);
      this->Nodes[0].SetParent(0);
      this->Nodes[0].SetLeafFlags(1);
      int i=0;
      const int c=1<<D;
      while(i<c)
        {
        this->Nodes[0].SetChild(i,0);
        ++i;
        }
      this->LeafParent.resize(1);
      this->LeafParent[0]=0;
      this->NumberOfLevels=1;
      this->NumberOfLeavesPerLevel.resize(1);
      this->NumberOfLeavesPerLevel[0]=1;
    }

  //---------------------------------------------------------------------------
  virtual vtkHyperOctreeCursor *NewCursor()
    {
      vtkCompactHyperOctreeCursor<D> *result=vtkCompactHyperOctreeCursor<D>::New();
      result->Init(this);
      return result;
    }

  //---------------------------------------------------------------------------
  virtual ~vtkCompactHyperOctree()
    {
      if(this->Attributes!=0)
        {
        this->Attributes->UnRegister(this);
        }
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
      assert("post: result_greater_or_equal_to_one" && this->NumberOfLevels>=1);
      return this->NumberOfLevels;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperOctreeCursor.
  vtkCompactHyperOctreeNode<D> *GetNode(int cursor)
    {
      assert("pre: valid_range" && cursor>=0 && cursor<GetNumberOfNodes());
      return &this->Nodes[cursor];
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperOctreeCursor.
  int GetLeafParent(int cursor)
    {
      assert("pre: valid_range" && cursor>=0 && cursor<this->GetNumberOfLeaves());
      assert("post: valid_result" && this->LeafParent[cursor]>=0 && this->LeafParent[cursor]<this->GetNumberOfNodes());
      return this->LeafParent[cursor];
    }

  //---------------------------------------------------------------------------
  // Description:
  // Public only for the vtkCompactHyperOctreeCursor.
  virtual int GetNumberOfNodes()
    {
      assert("post: not_empty" && this->Nodes.size()>0);
      return static_cast<int>(this->Nodes.size());
    }

  //---------------------------------------------------------------------------
  // Description:
  // Subdivide node pointed by cursor, only if its a leaf.
  // At the end, cursor points on the node that used to be leaf.
  // \pre leaf_exists: leaf!=0
  // \pre is_a_leaf: leaf->CurrentIsLeaf()
  void SubdivideLeaf(vtkHyperOctreeCursor *leaf)
    {
      assert("pre: leaf_exists" && leaf!=0);
      assert("pre: is_a_leaf" && leaf->CurrentIsLeaf());

      // we are using a vtkCompactHyperOctreeCursor.
      // we know that GetLeafId() return Cursor.
      int leafIndex=leaf->GetLeafId();

      vtkCompactHyperOctreeCursor<D> *cursor=static_cast<vtkCompactHyperOctreeCursor<D> *>(leaf);

      int i;
      const int c=1<<D; // number of children;

      // the leaf becomes a node and is not anymore a leaf.
      cursor->SetIsLeaf(0); // let the cursor knows about that change.
      size_t nodeIndex=this->Nodes.size();
      cursor->SetCursor(static_cast<int>(nodeIndex));
      this->Nodes.resize(nodeIndex+1);
      this->Nodes[nodeIndex].SetParent(this->LeafParent[leafIndex]);
      this->Nodes[nodeIndex].SetLeafFlags((1<<(1<<D))-1); // trick: all set a 1.

      // Change the parent: it has one less child as a leaf
      vtkCompactHyperOctreeNode<D> *parent=&(this->Nodes[this->Nodes[nodeIndex].GetParent()]);

      i=cursor->GetChildIndex();
      assert("check matching_child" && parent->GetChild(i)==leafIndex);
      unsigned char mask=1;
      mask<<=i;
      parent->SetLeafFlags(parent->GetLeafFlags()^mask);
      parent->SetChild(i,static_cast<int>(nodeIndex));

      // The first new child
      this->Nodes[nodeIndex].SetChild(0,leafIndex);
      this->LeafParent[leafIndex]=static_cast<int>(nodeIndex);

      // The other (c-1) new children.
      size_t nextLeaf=this->LeafParent.size();
      this->LeafParent.resize(nextLeaf+(c-1));
      i=1;
      while(i<c)
        {
        this->Nodes[nodeIndex].SetChild(i,static_cast<int>(nextLeaf));
        this->LeafParent[nextLeaf]=static_cast<int>(nodeIndex);
        ++nextLeaf;
        ++i;
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
        this->NumberOfLeavesPerLevel.resize(this->NumberOfLevels);
        }
      this->NumberOfLeavesPerLevel[level+1]+=c;
    }

  //---------------------------------------------------------------------------
  // Description:
  // Collapse a node for which all children are leaves.
  // At the end, cursor points on the leaf that used to be a node.
  // \pre node_exists: node!=0
  // \pre node_is_node: !node->CurrentIsLeaf()
  // \pre children_are_leaves: node->CurrentIsTerminalNode()
  void CollapseTerminalNode(vtkHyperOctreeCursor *node)
    {
      assert("pre: node_exists" && node!=0);
      assert("pre: node_is_node" && !node->CurrentIsLeaf());
      assert("pre: children_are_leaves" && node->CurrentIsTerminalNode());
      assert("check: TODO" && 0);

      // can decrease NumberOfLevels, difficult, need to go through each
      // parent node.
      // reference counting per level.
      (void)node; // remove warning in release mode.
    }

  //---------------------------------------------------------------------------
  int GetLeafParentSize()
    {
      return static_cast<int>(this->LeafParent.size());
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
        ++i;
        }
      os<<endl;

      os << indent << "LeafParent="<<this->LeafParent.size()<<endl;
      i=0;
      c=this->LeafParent.size();
      while(i<c)
        {
        os << this->LeafParent[i]<<" ";
        ++i;
        }
      os<<endl;

    }

  //---------------------------------------------------------------------------
  // Description:
  // Set the internal attributes.
  void SetAttributes(vtkDataSetAttributes *attributes)
    {
      assert("pre: attributes_exist" && attributes!=0);
      if(this->Attributes!=attributes)
        {
        if(this->Attributes!=0)
          {
          this->Attributes->UnRegister(this);
          }
        this->Attributes=attributes;
        this->Attributes->Register(this);
        }
    }

  //---------------------------------------------------------------------------
  // Description:
  // Return memory used in kilobytes.
  // Ignore the attribute array because its size is added by the data set.
  unsigned int GetActualMemorySize()
  {
    size_t size;
    size = sizeof(int) * this->GetNumberOfLeaves();
    size += sizeof(vtkCompactHyperOctreeNode<D>) * this->Nodes.size();
    return static_cast<unsigned int>(size / 1024);
  }

protected:
  //---------------------------------------------------------------------------
  // Description:
  // Default constructor.
  // The tree as only one node and one leaf: the root.
  // Attributes is empty.
  vtkCompactHyperOctree()
    {
      this->Nodes.resize(1);
      this->Nodes[0].SetParent(0);
      this->Nodes[0].SetLeafFlags(1);
      int i=0;
      const int c=1<<D;
      while(i<c)
        {
        this->Nodes[0].SetChild(i,0);
        ++i;
        }
      this->LeafParent.resize(1);
      this->LeafParent[0]=0;
      this->Attributes=0;
      this->NumberOfLevels=1;
      this->NumberOfLeavesPerLevel.resize(1);
      this->NumberOfLeavesPerLevel[0]=1;
    }

  std::vector<int> NumberOfLeavesPerLevel; // number of leaves in each level
  // its size is NumberOfLevels;

  vtkIdType NumberOfLevels;
  std::vector<vtkCompactHyperOctreeNode<D> > Nodes;
  std::vector<int> LeafParent; // record the parent of each leaf
  vtkDataSetAttributes *Attributes; // cell data or point data.
private:
  vtkCompactHyperOctree(const vtkCompactHyperOctree<D> &);  // Not implemented.
  void operator=(const vtkCompactHyperOctree<D> &);    // Not implemented.
};

// octree: vtkHyperOctreeInternal<3>
// quadtree: vtkHyperOctreeInternal<2>
// bittree: vtkHyperOctreeInternal<1>

vtkStandardNewMacro(vtkHyperOctree);

//-----------------------------------------------------------------------------
// Default constructor.
vtkHyperOctree::vtkHyperOctree()
{
  this->DualGridFlag = 1;
  this->Dimension=3;

  int i=0;
  while(i<3)
    {
    this->Size[i]=1;
    this->Origin[i]=0;
    ++i;
    }

  this->CellTree=vtkCompactHyperOctree<3>::New();
  this->CellTree->SetAttributes(this->CellData);

  this->TmpChild=this->NewCellCursor();

  // For dual
  this->LeafCenters = 0;
  this->CornerLeafIds = 0;
  // For non dual
  this->CornerPoints = 0;
  this->LeafCornerIds = 0;

  this->Links = 0;

  this->Voxel = vtkVoxel::New();
  this->Pixel = vtkPixel::New();
  this->Line = vtkLine::New();
}

//-----------------------------------------------------------------------------
// Destructor.
vtkHyperOctree::~vtkHyperOctree()
{
  if(this->CellTree!=0)
    {
    this->CellTree->UnRegister(this);
    }
   this->TmpChild->UnRegister(this);

   this->DeleteInternalArrays();

  this->Voxel->Delete();
  this->Voxel = 0;
  this->Pixel->Delete();
  this->Pixel = 0;
  this->Line->Delete();
  this->Line = 0;
}


//-----------------------------------------------------------------------------
// Description:
// Return what type of dataset this is.
int vtkHyperOctree::GetDataObjectType()
{
  return VTK_HYPER_OCTREE;
}

//-----------------------------------------------------------------------------
// Description:
// Copy the geometric and topological structure of an input rectilinear grid
// object.
void vtkHyperOctree::CopyStructure(vtkDataSet *ds)
{
  assert("pre: ds_exists" && ds!=0);
  assert("pre: same_type" && vtkHyperOctree::SafeDownCast(ds)!=0);

  vtkHyperOctree *ho=vtkHyperOctree::SafeDownCast(ds);

//  this->Superclass::CopyStructure(ho);


  // What about copying celldata???
//  this->CellTree->SetAttributes(this->CellData);
  if(this->CellTree!=0)
    {
    this->CellTree->UnRegister(this);
    }
  this->CellTree=ho->CellTree;
  if(this->CellTree!=0)
    {
    this->CellTree->Register(this);
    }

  this->Dimension=ho->Dimension;

  int i=0;
  while(i<3)
    {
    this->Size[i]=ho->Size[i];
    this->Origin[i]=ho->Origin[i];
    ++i;
    }


  this->Modified();
}

//-----------------------------------------------------------------------------
// Description:
// Return the dimension of the tree (1D:binary tree(2 children), 2D:quadtree
// (4 children), 3D:octree (8 children))
// \post valid_result: result>=1 && result<=3
int vtkHyperOctree::GetDimension()
{
  assert("post: valid_result" && this->Dimension>=1 && this->Dimension<=3);
  return this->Dimension;
}

//-----------------------------------------------------------------------------
// Set the dimension of the tree with `dim'. See GetDimension() for details.
// \pre valid_dim: dim>=1 && dim<=3
// \post dimension_is_set: GetDimension()==dim
void vtkHyperOctree::SetDimension(int dim)
{
  assert("pre: valid_dim" && dim>=1 && dim<=3);
  if(this->Dimension!=dim)
    {
    this->Dimension=dim;
    if(this->CellTree!=0)
      {
      this->CellTree->UnRegister(this);
      }
    switch(dim)
      {
      case 3:
        this->CellTree=vtkCompactHyperOctree<3>::New();
        break;
      case 2:
        this->CellTree=vtkCompactHyperOctree<2>::New();
        break;
      case 1:
        this->CellTree=vtkCompactHyperOctree<1>::New();
        break;
      default:
        assert("check: impossible case" && 0);
        break;
      }
    this->CellTree->SetAttributes(this->CellData);
    this->TmpChild->UnRegister(this);
    this->TmpChild=this->NewCellCursor();
    this->Modified();
    }
  assert("post: dimension_is_set" && this->GetDimension()==dim);
  this->DeleteInternalArrays();
  if (this->DualGridFlag)
    {
    this->GenerateDualNeighborhoodTraversalTable();
    }
  else
    {
    this->GenerateGridNeighborhoodTraversalTable();
    }
}

//----------------------------------------------------------------------------
void vtkHyperOctree::ComputeBounds()
{
  this->Bounds[0] = this->Origin[0];
  this->Bounds[2] = this->Origin[1];
  this->Bounds[4] = this->Origin[2];

  this->Bounds[1] = this->Bounds[0]+this->Size[0];
  if(this->Dimension>=2)
    {
    this->Bounds[3] = this->Bounds[2]+this->Size[1];
    }
  else
    {
    this->Bounds[3] = this->Bounds[2];
    }
  if(this->Dimension==3)
    {
    this->Bounds[5] = this->Bounds[4]+this->Size[2];
    }
  else
    {
    this->Bounds[5] = this->Bounds[4];
    }
}

//-----------------------------------------------------------------------------
// Description:
// Return the number of points corresponding to an hyperoctree starting at
// level `level' where all the leaves at at the last level. In this case, the
// hyperoctree is like a uniform grid. So this number is the number of points
// of the uniform grid.
// \pre positive_level: level>=0 && level<this->GetNumberOfLevels()
// \post definition: result==(2^(GetNumberOfLevels()-level-1)+1)^GetDimension()
vtkIdType vtkHyperOctree::GetMaxNumberOfPoints(int level)
{
  assert("pre: positive_level" && level>=0 && level<this->GetNumberOfLevels());

  vtkIdType result=(1<<(this->GetNumberOfLevels()-level-1))+1;
  int c=this->GetDimension();
  int i=1;
  vtkIdType fact=result;
  while(i<c)
    {
    result*=fact;
    ++i;
    }
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Return the number of points corresponding to the boundary of an hyperoctree
// starting at level `level' where all the leaves at at the last level. In
// this case, the hyperoctree is like a uniform grid. So this number is the
// number of points of on the boundary of the uniform grid.
// For an octree, the boundary are the faces. For a quadtree, the boundary
// are the edges.
// \pre 2d_or_3d: this->GetDimension()==2 || this->GetDimension()==3
// \pre positive_level: level>=0 && level<this->GetNumberOfLevels()
// \post min_result: result>=GetMaxNumberOfPoints(this->GetNumberOfLevels()-1)
// \post max_result: result<=GetMaxNumberOfPoints(level)
vtkIdType vtkHyperOctree::GetMaxNumberOfPointsOnBoundary(int level)
{
  assert("pre: 2d_or_3d" &&(this->GetDimension()==2||this->GetDimension()==3));
  assert("pre: positive_level" && level>=0 && level<this->GetNumberOfLevels());

  vtkIdType segment=(1<<(this->GetNumberOfLevels()-level-1))+1;
  vtkIdType result;
  if(this->GetDimension()==3)
    {
    result=(segment*segment)<<1;
    if(segment>2)
      {
      result+=((segment-1)*(segment-2))<<2;
      }
    }
  else // 2D
    {
    result=(segment-1)<<2;
    }

  if(!(result>=GetMaxNumberOfPoints(this->GetNumberOfLevels()-1)))
    {
    cout<<"err1"<<endl;
    }
  if(!(result<=GetMaxNumberOfPoints(level)))
    {
    cout<<"err2"<<endl;
    }

  assert("post: min_result" && result>=GetMaxNumberOfPoints(this->GetNumberOfLevels()-1));
  assert("post: max_result" && result<=GetMaxNumberOfPoints(level));
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Return the number of cells corresponding to the boundary of a cell
// of level `level' where all the leaves at at the last level.
// \pre positive_level: level>=0 && level<this->GetNumberOfLevels()
// \post positive_result: result>=0
vtkIdType vtkHyperOctree::GetMaxNumberOfCellsOnBoundary(int level)
{
  assert("pre: positive_level" && level>=0 && level<this->GetNumberOfLevels());

  vtkIdType result;
  int segment;

  switch(this->GetDimension())
    {
    case 1:
      result=2; // one cell on each side.
      break;
    case 2:
      // 4 corners+ 4faces. on each face 2^(deltalevels) cells.
      result=((1<<(this->GetNumberOfLevels()-1-level))<<2)+4;
      break;
    default: // 3D
      // 8 corners+ 6 faces+12 edges
      segment=1<<(this->GetNumberOfLevels()-1-level);
//      result=8+6*segment*segment+12*segment;
      result=(segment+2)*segment*6+8;
      break;
    }

  assert("post: positive_result" && result>=0);
  return result;
}


//-----------------------------------------------------------------------------
// Description:
// Return the number of levels.
// \post result_greater_or_equal_to_one: result>=1
vtkIdType vtkHyperOctree::GetNumberOfLevels()
{
  vtkIdType result=this->CellTree->GetNumberOfLevels();
  assert("post: result_greater_or_equal_to_one" && result>=1);
  return result;
}


//-----------------------------------------------------------------------------
// Description:
// Create a new cursor: an object that can traverse
// hyperoctree cells.
vtkHyperOctreeCursor *vtkHyperOctree::NewCellCursor()
{
  vtkHyperOctreeCursor *result=this->CellTree->NewCursor();
  assert("post: result_exists" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Subdivide node pointed by cursor, only if its a leaf.
// At the end, cursor points on the node that used to be leaf.
// \pre leaf_exists: leaf!=0
// \pre is_a_leaf: leaf->CurrentIsLeaf()
void vtkHyperOctree::SubdivideLeaf(vtkHyperOctreeCursor *leaf)
{
  assert("pre: leaf_exists" && leaf!=0);
  assert("pre: is_a_leaf" && leaf->CurrentIsLeaf());
  this->CellTree->SubdivideLeaf(leaf);
  this->DeleteInternalArrays();
}

//-----------------------------------------------------------------------------
// Description:
// Collapse a node for which all children are leaves.
// At the end, cursor points on the leaf that used to be a node.
// \pre node_exists: node!=0
// \pre node_is_node: !node->CurrentIsLeaf()
// \pre children_are_leaves: node->CurrentIsTerminalNode()
void vtkHyperOctree::CollapseTerminalNode(vtkHyperOctreeCursor *node)
{
  assert("pre: node_exists" && node!=0);
  assert("pre: node_is_node" && !node->CurrentIsLeaf());
  assert("pre: children_are_leaves" && node->CurrentIsTerminalNode());
  this->CellTree->CollapseTerminalNode(node);
  this->DeleteInternalArrays();
}








//-----------------------------------------------------------------------------
// Description:
// Restore data object to initial state,
// THIS METHOD IS NOT THREAD SAFE.
void vtkHyperOctree::Initialize()
{
  if(this->Dimension!=3)
    {
    this->Dimension=3;
    this->CellTree->UnRegister(this);
    this->CellTree=vtkCompactHyperOctree<3>::New();
    if (this->DualGridFlag)
      {
      this->CellTree->SetAttributes(this->PointData);
      }
    else
      {
      this->CellTree->SetAttributes(this->CellData);
      }
    }
  else
    {
    this->CellTree->Initialize();
    }

  int i=0;
  while(i<3)
    {
    this->Size[i]=1;
    this->Origin[i]=0;
    ++i;
    }

  this->DeleteInternalArrays();

//  this->Width=1;
//  this->Height=1;
//  this->Depth=1;
}

//-----------------------------------------------------------------------------
// Description:
// Convenience method returns largest cell size in dataset. This is generally
// used to allocate memory for supporting data structures.
// This is the number of points of a cell.
// THIS METHOD IS THREAD SAFE
int vtkHyperOctree::GetMaxCellSize()
{
  int result;
  switch(this->Dimension)
    {
    case 3:
      result=8; // hexahedron=8 points
      break;
    case 2:
      result=4; // quad=4 points
      break;
    case 1:
      result=2; // line=2 points
      break;
    default:
      result=0; // impossible case
      break;
    }
  assert("post: positive_result" && result>0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Shallow and Deep copy.
void vtkHyperOctree::ShallowCopy(vtkDataObject *src)
{
  assert("src_same_type" && vtkHyperOctree::SafeDownCast(src)!=0);
  this->Superclass::ShallowCopy(src);
  this->CopyStructure(vtkHyperOctree::SafeDownCast(src));
}

//-----------------------------------------------------------------------------
void vtkHyperOctree::DeepCopy(vtkDataObject *src)
{
  assert("src_same_type" && vtkHyperOctree::SafeDownCast(src)!=0);
  this->Superclass::DeepCopy(src);
  this->CopyStructure(vtkHyperOctree::SafeDownCast(src));
}

//----------------------------------------------------------------------------
// Description:
// Get the points of node `sibling' on its face `face'.
// \pre sibling_exists: sibling!=0
// \pre sibling_not_leaf: !sibling->CurrentIsLeaf()
// \pre sibling_3d: sibling->GetDimension()==3
// \pre valid_face: face>=0 && face<6
// \pre valid_level_not_leaf: level>=0 level<(this->GetNumberOfLevels()-1)
void vtkHyperOctree::GetPointsOnFace(vtkHyperOctreeCursor *sibling,
                                     int face,
                                     int level,
                                     vtkHyperOctreePointsGrabber *grabber)
{
  assert("pre: sibling_exists" && sibling!=0);
  assert("pre: sibling_not_leaf" && !sibling->CurrentIsLeaf());
  assert("pre: sibling_3d" && sibling->GetDimension()==3);
  assert("pre: valid_face" && face>=0 && face<6);
  assert("pre: valid_level_not_leaf" && level>=0
         && level<(this->GetNumberOfLevels()-1));

  // Add the 5 points of the face (merge them).
  // the center point does not need to be merge,
  // it can be added directly to the triangulator

  int i;
  int j;
  int k;
  int kvalue;

  kvalue=(face&1)<<1; //*2
                          k=(face>>1);
  i=(k+1)%3;
  j=(i+1)%3;

  assert("check: valid_kvalue_range" && (kvalue==0 || kvalue==2));
  assert("check: valid_k_range" && k>=0 && k<3);
  assert("check: valid_i_range" && i>=0 && i<3);
  assert("check: valid_j_range" && j>=0 && j<3);

  vtkIdType sijk[3];
  int coord=0;
  while(coord<3)
    {
    sijk[coord]=sibling->GetIndex(coord)<<1;
    ++coord;
    }

  vtkIdType resolution=(1<<(this->GetNumberOfLevels()-1))+1;
  int deltaLevel=this->GetNumberOfLevels()-1-level;

  assert("check positive" && deltaLevel>=0);

  double ratio=1.0/(resolution-1);

  vtkIdType ijk[3];
  double pcoords[3];
  double pt[3];

  ijk[k]=kvalue;
  sijk[k]=sijk[k]+kvalue;

  ijk[j]=0;

  double *size=this->GetSize();
  double *origin=this->GetOrigin();

  int midPoints=0; // 0: on corner point, 1: on edge, 2: on face
  while(ijk[j]<3)
    {
    ijk[i]=0;
    sijk[i]=sibling->GetIndex(i)<<1;
    while(ijk[i]<3)
      {
      if(midPoints>0)
        {
        // build the point
        int ptIndices[3];

        coord=0;
        while(coord<3)
          {
          ptIndices[coord]=sijk[coord]<<(deltaLevel-1);
          pcoords[coord]=ptIndices[coord]*ratio;
          pt[coord]=pcoords[coord]*size[coord]+origin[coord];
          ++coord;
          }
        vtkIdType ptId=((sijk[2]<<(deltaLevel-1))*resolution+(sijk[1]<<(deltaLevel-1)))*resolution+(sijk[0]<<(deltaLevel-1));

        assert("check: in_bounds" && pt[0]>=this->GetBounds()[0] && pt[0]<=this->GetBounds()[1] && pt[1]>=this->GetBounds()[2] && pt[1]<=this->GetBounds()[3] && pt[2]>=this->GetBounds()[4] && pt[2]<=this->GetBounds()[5]);

        if(midPoints==2)
          {
          grabber->InsertPoint(ptId,pt,pcoords,ptIndices);
          }
        else
          {
          // midPoint==1
          // add the point to the merge points
          grabber->InsertPointWithMerge(ptId,pt,pcoords,ptIndices);
          }
        }
      ++ijk[i];
      ++sijk[i];
      if(ijk[i]==1)
        {
        ++midPoints;
        }
      else
        {
        if(ijk[i]==2)
          {
          --midPoints;
          }
        }
      }
    ++ijk[j];
    ++sijk[j];
    if(ijk[j]==1)
      {
      ++midPoints;
      }
    else
      {
      if(ijk[j]==2)
        {
        --midPoints;
        }
      }
    }

  // Go to each child (among 4) that shares this face and that is not a leaf.

  int ainc;
  int binc;
  int childa;

  if(face&1)
    {
    childa=1<<(face>>1);
    }
  else
    {
    childa=0;
    }

  assert("check: valid_childa" &&
         (childa==0 || childa==1 || childa==2 || childa==4) );

  int tmp=((face>>1)+1)%3;

  binc=1<<tmp;
  ainc=1<<((tmp+1)%3);

  assert("check: valid_binc_range" && (binc==1 || binc==2 || binc==4));
  assert("check: valid_ainc_range" && (ainc==1 || ainc==2 || ainc==4));
  assert("check: different" && ainc!=binc);


  int a=0;
  while(a<2)
    {
    int b=0;
    int child=childa;
    while(b<2)
      {
      sibling->ToChild(child);
      if(!sibling->CurrentIsLeaf())
        {
        this->GetPointsOnFace(sibling,face,level+1,grabber);
        }
      sibling->ToParent();
      ++b;
      child=child+binc;
      }
    childa=childa+ainc;
    ++a;
    }
}

// Return the child `child' on edge `edge' of the current node.
// For a quadtree, in counter-clockwise direction.
// [edge][child]

static int childrenOnEdge[4][2]={{0,2},
                                 {3,1},
                                 {1,0},
                                 {2,3}};

//----------------------------------------------------------------------------
// Description:
// Get the points of node `sibling' on its edge `edge'.
// \pre sibling_exists: sibling!=0
// \pre sibling_not_leaf: !sibling->CurrentIsLeaf()
// \pre sibling_2d: sibling->GetDimension()==2
// \pre valid_edge: edge>=0 && edge<4
// \pre valid_level_not_leaf: level>=0 level<(this->GetNumberOfLevels()-1)
void vtkHyperOctree::GetPointsOnEdge2D(vtkHyperOctreeCursor *sibling,
                                       int edge,
                                       int level,
                                       vtkHyperOctreePointsGrabber *grabber)
{
  assert("pre: sibling_exists" && sibling!=0);
  assert("pre: sibling_not_leaf" && !sibling->CurrentIsLeaf());
  assert("pre: sibling_2d" && sibling->GetDimension()==2);
  assert("pre: valid_face" && edge>=0 && edge<4);
  assert("pre: valid_level_not_leaf" && level>=0 &&
         level<(this->GetNumberOfLevels()-1));

  // Add the points of the first child in counter-clockwise direction.
  sibling->ToChild(childrenOnEdge[edge][0]);
  if(!sibling->CurrentIsLeaf())
    {
    this->GetPointsOnEdge2D(sibling,edge,level+1,grabber);
    }
  sibling->ToParent();

  // Add the point of the edge.
  // the point does not need to be merge,
  // it can be added directly to the triangulator
  int k=(edge>>1);
  int kvalue=(edge&1)<<1; //*2
                              int i=(k+1)%2;

  assert("check: valid_kvalue_range" && (kvalue==0 || kvalue==2));
  assert("check: valid_k_range" && k>=0 && k<2);
  assert("check: valid_i_range" && i>=0 && i<2);

  vtkIdType sijk[2];
  int coord=0;
  while(coord<2)
    {
    sijk[coord]=sibling->GetIndex(coord)<<1;
    ++coord;
    }

  vtkIdType resolution=(1<<(this->GetNumberOfLevels()-1))+1;
  int deltaLevel=this->GetNumberOfLevels()-1-level;

  assert("check positive" && deltaLevel>=0);

  double ratio=1.0/(resolution-1);
  double pcoords[2];
  double pt[3];

  sijk[k]=sijk[k]+kvalue;
  sijk[i]=(sibling->GetIndex(i)<<1)+1;

  // build the point

  int ptIndices[3];


  double *size=this->GetSize();
  double *origin=this->GetOrigin();

  coord=0;
  while(coord<2)
    {
    ptIndices[coord]=sijk[coord]<<(deltaLevel-1);
    pcoords[coord]=ptIndices[coord]*ratio;
    pt[coord]=pcoords[coord]*size[coord]+origin[coord];
    ++coord;
    }
  pt[2]=origin[2];

  assert("check: in_bounds" && pt[0]>=this->GetBounds()[0] && pt[0]<=this->GetBounds()[1] && pt[1]>=this->GetBounds()[2] && pt[1]<=this->GetBounds()[3] && pt[2]>=this->GetBounds()[4] && pt[2]<=this->GetBounds()[5]);

  grabber->InsertPoint2D(pt,ptIndices);

  // Add the points of the second child in counter-clockwise direction.
  sibling->ToChild(childrenOnEdge[edge][1]);
  if(!sibling->CurrentIsLeaf())
    {
    this->GetPointsOnEdge2D(sibling,edge,level+1,grabber);
    }
  sibling->ToParent();
}

//----------------------------------------------------------------------------
// Description:
// Get the points of the parent node of `cursor' on its faces `faces' at
// level `level' or deeper.
// \pre cursor_exists: cursor!=0
// \pre cursor_3d: cursor->GetDimension()==3
// \pre valid_level: level>=0
// \pre boolean_faces: (faces[0]==0 || faces[0]==1) && (faces[1]==0 || faces[1]==1) && (faces[2]==0 || faces[2]==1)
void vtkHyperOctree::GetPointsOnParentFaces(
  int faces[3],
  int level,
  vtkHyperOctreeCursor *cursor,
  vtkHyperOctreePointsGrabber *grabber)
{
  assert("pre: cursor_exists" && cursor!=0);
  assert("pre: cursor_3d" && cursor->GetDimension()==3);
  assert("pre: valid_level" && level>=0);
  assert("pre: boolean_faces" && (faces[0]==0 || faces[0]==1)
         && (faces[1]==0 || faces[1]==1) && (faces[2]==0 || faces[2]==1));

  int indices[3];
  indices[0]=cursor->GetIndex(0);
  indices[1]=cursor->GetIndex(1);
  indices[2]=cursor->GetIndex(2);

  int target[3];

  int i;
  int j;
  int skip;
  i=0;
  int faceOffset=0;
  while(i<3)
    {
    j=0;
    skip=0;
    while(j<3 && !skip)
      {
      if(i==j)
        {
        if(faces[j])
          {
          target[j]=indices[j]+1;
          if(target[j]>=(1<<level))
            {
            // on boundary, skip
            skip=1;
            }
          }
        else
          {
          target[j]=indices[j]-1;
          if(target[j]<0)
            {
            // on boundary, skip
            skip=1;
            }
          }
        }
      else
        {
        target[j]=indices[j];
        }
      ++j;
      }
    if(!skip)
      {
      this->TmpChild->MoveToNode(target,level);
      if(this->TmpChild->Found())
        {
        if(!this->TmpChild->CurrentIsLeaf())
          {
          assert("check: requested_level" &&
                 level==this->TmpChild->GetCurrentLevel());
          // There might be some new points.
          int childFace=faceOffset;
          if(!faces[i])
            {
            ++childFace;
            }
          this->GetPointsOnFace(this->TmpChild,childFace,level,grabber);
          }
        }
      }
    ++i;
    faceOffset+=2;
    }
}

//----------------------------------------------------------------------------
// Description:
// Get the points of the parent node of `cursor' on its edge `edge' at
// level `level' or deeper. (edge=0 for -X, 1 for +X, 2 for -Y, 3 for +Y)
// \pre cursor_exists: cursor!=0
// \pre cursor_2d: cursor->GetDimension()==2
// \pre valid_level: level>=0
// \pre valid_edge: edge>=0 && edge<4
void vtkHyperOctree::GetPointsOnParentEdge2D(
  vtkHyperOctreeCursor *cursor,
  int edge,
  int level,
  vtkHyperOctreePointsGrabber *grabber)
{
  assert("pre: cursor_exists" && cursor!=0);
  assert("pre: cursor_2d" && cursor->GetDimension()==2);
  assert("pre: valid_level" && level>=0);
  assert("pre: valid_edge" && edge>=0 && edge<4);

  int target[2];
  int childEdge;

  // visit 2 children in counter-clockwise direction.

  int skip=0;
  if(edge<2) // +x,-x
    {
    target[1]=cursor->GetIndex(1);
    if(edge==1) //+x
      {
      childEdge=0;
      target[0]=cursor->GetIndex(0)+1;
      if(target[0]>=(1<<level))
        {
        skip=1;
        }
      }
    else
      {
      childEdge=1;
      target[0]=cursor->GetIndex(0)-1;
      if(target[0]<0)
        {
        // on boundary, skip
        skip=1;
        }
      }
    }
  else
    {
    // -y,+y
    target[0]=cursor->GetIndex(0);
    if(edge==3) //+y
      {
      childEdge=2;
      target[1]=cursor->GetIndex(1)+1;
      if(target[1]>=(1<<level))
        {
        skip=1;
        }
      }
    else
      {
      childEdge=3;
      target[1]=cursor->GetIndex(1)-1;
      if(target[1]<0)
        {
        // on boundary, skip
        skip=1;
        }
      }
    }

  if(!skip)
    {
    this->TmpChild->MoveToNode(target,level);
    if(this->TmpChild->Found())
      {
      if(!this->TmpChild->CurrentIsLeaf())
        {
        assert("check: requested_level" &&
               level==this->TmpChild->GetCurrentLevel());
        this->GetPointsOnEdge2D(this->TmpChild,childEdge,level,grabber);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Description:
// Get the points of node `sibling' on its edge `axis','k','j'.
// If axis==0, the edge is X-aligned and k gives the z coordinate and j the
// y-coordinate. If axis==1, the edge is Y-aligned and k gives the x coordinate
// and j the z coordinate. If axis==2, the edge is Z-aligned and k gives the
// y coordinate and j the x coordinate.
// \pre sibling_exists: sibling!=0
// \pre sibling_3d: sibling->GetDimension()==3
// \pre sibling_not_leaf: !sibling->CurrentIsLeaf()
// \pre valid_axis: axis>=0 && axis<3
// \pre valid_k: k>=0 && k<=1
// \pre valid_j: j>=0 && j<=1
// \pre valid_level_not_leaf: level>=0 level<(this->GetNumberOfLevels()-1)
void vtkHyperOctree::GetPointsOnEdge(vtkHyperOctreeCursor *sibling,
                                     int level,
                                     int axis,
                                     int k,
                                     int j,
                                     vtkHyperOctreePointsGrabber *grabber)
{
  assert("pre: sibling_exists" && sibling!=0);
  assert("pre: sibling_3d" && sibling->GetDimension()==3);
  assert("pre: sibling_not_leaf" && !sibling->CurrentIsLeaf());
  assert("pre: valid_axis" && axis>=0 && axis<3);
  assert("pre: valid_k" && k>=0 && k<=1);
  assert("pre: valid_j" && j>=0 && j<=1);
  assert("pre: valid_level_not_leaf" && level>=0 &&
         level<(this->GetNumberOfLevels()-1));

  // Add the mid-point,  without merging

  vtkIdType resolution=(1<<(this->GetNumberOfLevels()-1))+1;
  int deltaLevel=this->GetNumberOfLevels()-1-level;

  assert("check positive" && deltaLevel>=0);

  double ratio=1.0/(resolution-1);

  vtkIdType sijk[3];
  int coord=0;
  while(coord<3)
    {
    sijk[coord]=sibling->GetIndex(coord)<<1;
    ++coord;
    }

  sijk[axis]+=1;
  sijk[(axis+1)%3]+=j<<1;
  sijk[(axis+2)%3]+=k<<1;


  double pcoords[3];
  double pt[3];

  double *size=this->GetSize();
  double *origin=this->GetOrigin();

  // build the point

  int ptIndices[3];

  coord=0;
  while(coord<3)
    {
    ptIndices[coord]=sijk[coord]<<(deltaLevel-1);
    pcoords[coord]=ptIndices[coord]*ratio;
    pt[coord]=pcoords[coord]*size[coord]+origin[coord];
    ++coord;
    }

  vtkIdType ptId=((sijk[2]<<(deltaLevel-1))*resolution+(sijk[1]<<(deltaLevel-1)))*resolution+(sijk[0]<<(deltaLevel-1));

  assert("check: in_bounds" && pt[0]>=this->GetBounds()[0] && pt[0]<=this->GetBounds()[1] && pt[1]>=this->GetBounds()[2] && pt[1]<=this->GetBounds()[3] && pt[2]>=this->GetBounds()[4] && pt[2]<=this->GetBounds()[5]);

  grabber->InsertPointWithMerge(ptId,pt,pcoords,ptIndices);

  int ijk[3];

  ijk[axis]=0;
  ijk[(axis+1)%3]=j;
  ijk[(axis+2)%3]=k;

  int child=(((ijk[2]<<1)+ijk[1])<<1)+ijk[0];
  // Go to each child (among 2) that shares this edge and that is not a leaf.
  sibling->ToChild(child);
  if(!sibling->CurrentIsLeaf())
    {
    this->GetPointsOnEdge(sibling,level+1,axis,k,j,grabber);
    }
  sibling->ToParent();
  ijk[axis]=1;
  child=(((ijk[2]<<1)+ijk[1])<<1)+ijk[0];
  sibling->ToChild(child);
  if(!sibling->CurrentIsLeaf())
    {
    this->GetPointsOnEdge(sibling,level+1,axis,k,j,grabber);
    }
  sibling->ToParent();
}

//----------------------------------------------------------------------------
// Description:
// Get the points of the parent node of `cursor' on its edge `axis','k','j'
// at level `level' or deeper.
// If axis==0, the edge is X-aligned and k gives the z coordinate and j the
// y-coordinate. If axis==1, the edge is Y-aligned and k gives the x
// coordinate and j the z coordinate. If axis==2, the edge is Z-aligned and
// k gives the y coordinate and j the x coordinate.
// \pre cursor_exists: cursor!=0
// \pre cursor_3d: cursor->GetDimension()==3
// \pre valid_level: level>=0
// \pre valid_range_axis: axis>=0 && axis<3
// \pre valid_range_k: k>=0 && k<=1
// \pre valid_range_j: j>=0 && j<=1
void vtkHyperOctree::GetPointsOnParentEdge(
  vtkHyperOctreeCursor *cursor,
  int level,
  int axis,
  int k,
  int j,
  vtkHyperOctreePointsGrabber *grabber)
{
  assert("pre: cursor_exists" && cursor!=0);
  assert("pre: cursor_3d" && cursor->GetDimension()==3);
  assert("pre: valid_level" && level>=0);
  assert("valid_range_axis" && axis>=0 && axis<3);
  assert("valid_range_k" && k>=0 && k<=1);
  assert("valid_range_j" && j>=0 && j<=1);

  int indices[3];
  int coord=0;
  while(coord<3)
    {
    indices[coord]=cursor->GetIndex(coord);
    ++coord;
    }

  int target[3];

  target[axis]=indices[axis];
  int i=(axis+1)%3;

  int skip=0;
  if(j==1)
    {
    target[i]=indices[i]+1;
    if(target[i]>=(1<<level))
      {
      // on boundary, skip
      skip=1;
      }
    }
  else
    {
    target[i]=indices[i]-1;
    if(target[i]<0)
      {
      // on boundary, skip
      skip=1;
      }
    }



  if(!skip)
    {
    i=(axis+2)%3;

    if(k==1)
      {
      target[i]=indices[i]+1;
      if(target[i]>=(1<<level))
        {
        // on boundary, skip
        skip=1;
        }
      }
    else
      {
      target[i]=indices[i]-1;
      if(target[i]<0)
        {
        // on boundary, skip
        skip=1;
        }
      }
    }

  if(!skip)
    {
    this->TmpChild->MoveToNode(target,level);
    if(this->TmpChild->Found())
      {
      if(!this->TmpChild->CurrentIsLeaf())
        {
        assert("check: requested_level" &&
               level==this->TmpChild->GetCurrentLevel());
        // There might be some new points.
        this->GetPointsOnEdge(this->TmpChild,level,axis,!k,!j,grabber);
        }
      }
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperOctree::GetNumberOfLeaves()
{
  return this->CellTree->GetNumberOfLeaves();
}

//=============================================================================
// DataSet API that returns dual grid.

//-----------------------------------------------------------------------------
// Description:
// Return the number of leaves.
// \post positive_result: result>=0
vtkIdType vtkHyperOctree::GetNumberOfCells()
{
  if (this->DualGridFlag)
    {
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    return cornerLeafIds->GetNumberOfTuples();
    }
  else
    {
    return this->CellTree->GetNumberOfLeaves();
    }
}

//-----------------------------------------------------------------------------
// Description:
// Return the number of points.
// \post positive_result: result>=0
vtkIdType vtkHyperOctree::GetNumberOfPoints()
{
  if (this->DualGridFlag)
    {
    return this->CellTree->GetNumberOfLeaves();
    }
  else
    {
    vtkPoints* cornerPoints = this->GetCornerPoints();
    return cornerPoints->GetNumberOfPoints();
    }
}


//-----------------------------------------------------------------------------
// Description:
// Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
// THIS METHOD IS NOT THREAD SAFE.
double *vtkHyperOctree::GetPoint(vtkIdType ptId)
{
  if (this->DualGridFlag)
    {
    vtkPoints* leafCenters = this->GetLeafCenters();
    assert("Index out of bounds." &&
           ptId >= 0 && ptId < leafCenters->GetNumberOfPoints());
    return leafCenters->GetPoint(ptId);
    }
  else
    {
    vtkPoints* cornerPoints = this->GetCornerPoints();
    assert("Index out of bounds." &&
           ptId >= 0 && ptId < cornerPoints->GetNumberOfPoints());
    return cornerPoints->GetPoint(ptId);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Copy point coordinates into user provided array x[3] for specified
// point id.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperOctree::GetPoint(vtkIdType id, double x[3])
{
  if (this->DualGridFlag)
    {
    vtkPoints* leafCenters = this->GetLeafCenters();
    assert("Index out of bounds." &&
           id >= 0 && id < leafCenters->GetNumberOfPoints());
    leafCenters->GetPoint(id, x);
    }
  else
    {
    vtkPoints* cornerPoints = this->GetCornerPoints();
    assert("Index out of bounds." &&
           id >= 0 && id < cornerPoints->GetNumberOfPoints());
    cornerPoints->GetPoint(id, x);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Get cell with cellId such that: 0 <= cellId < NumberOfCells.
// THIS METHOD IS NOT THREAD SAFE.
vtkCell *vtkHyperOctree::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  int numPts = 1<<this->GetDimension();
  int ptIdx;
  double x[3];

  switch (this->GetDimension())
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

  if (this->DualGridFlag)
    {
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    assert("Index out of bounds." &&
           cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples());
    vtkPoints* leafCenters = this->GetLeafCenters();
    vtkIdType* ptr = cornerLeafIds->GetPointer(0) + cellId*numPts;
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
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert("Index out of bounds." &&
           cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples());
    vtkPoints* cornerPoints = this->GetCornerPoints();
    vtkIdType* ptr = leafCornerIds->GetPointer(0) + cellId*numPts;
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
void vtkHyperOctree::GetCell(vtkIdType cellId,
                             vtkGenericCell *cell)
{
  int numPts = 1<<this->GetDimension();
  int ptIdx;
  double x[3];

  switch (this->GetDimension())
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

  if (this->DualGridFlag)
    {
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    assert("Index out of bounds." &&
           cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples());
    vtkPoints* leafCenters = this->GetLeafCenters();
    vtkIdType* ptr = cornerLeafIds->GetPointer(0) + cellId*numPts;
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
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert("Index out of bounds." &&
           cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples());
    vtkPoints* cornerPoints = this->GetCornerPoints();
    vtkIdType* ptr = leafCornerIds->GetPointer(0) + cellId*numPts;
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
int vtkHyperOctree::GetCellType(vtkIdType vtkNotUsed(cellId))
{
  int result;
  switch(this->Dimension)
    {
    case 3:
      result=VTK_VOXEL; // hexahedron=8 points
      break;
    case 2:
      result=VTK_PIXEL; // quad=4 points
      break;
    case 1:
      result=VTK_LINE; // line=2 points
      break;
    default:
      result=0; // impossible case
      break;
    }
  assert("post: positive_result" && result>0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Topological inquiry to get points defining cell.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperOctree::GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
{
  int ii;
  int numPts = 1 << this->GetDimension();
  ptIds->Initialize();

  if (this->DualGridFlag)
    {
    vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
    assert("Index out of bounds." &&
           cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples());
    vtkIdType* ptr = cornerLeafIds->GetPointer(0) + cellId*numPts;
    for (ii = 0; ii < numPts; ++ii)
      {
      ptIds->InsertId(ii, *ptr++);
      }
    }
  else
    {
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert("Index out of bounds." &&
           cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples());
    vtkIdType* ptr = leafCornerIds->GetPointer(0) + cellId*numPts;
    for (ii = 0; ii < numPts; ++ii)
      {
      ptIds->InsertId(ii, *ptr++);
      }
    }
}

//----------------------------------------------------------------------------
// Return a pointer to a list of point ids defining cell. (More efficient than alternative
// method.)
void vtkHyperOctree::GetCellPoints(vtkIdType cellId, vtkIdType& npts,
                                        vtkIdType* &pts)
{
  vtkIdTypeArray* cornerLeafIds = this->GetCornerLeafIds();
  assert("Index out of bounds." &&
         cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples());
  // Casting of 1 is necessary to remove 64bit Compiler warning C4334 on
  // Visual Studio 2005.
  npts = static_cast<vtkIdType>(1) << this->GetDimension();
  pts = cornerLeafIds->GetPointer(0) + cellId*npts;
}


//-----------------------------------------------------------------------------
void vtkHyperOctree::GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
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
void vtkHyperOctree::BuildLinks()
{
  this->Links = vtkCellLinks::New();
  this->Links->Allocate(this->GetNumberOfPoints());
  this->Links->Register(this);
  this->Links->BuildLinks(this);
  this->Links->Delete();
}


//-----------------------------------------------------------------------------
// This is exactly the same as GetCellNeighbors in unstructured grid.
void vtkHyperOctree::GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                                           vtkIdList *cellIds)
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
  pts = ptIds->GetPointer(0);
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
    vtkErrorMacro("input point ids empty.");
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
// Note: This only works for dual grid.  I expecte to get rid of the
// grid API, so it will not make a difference.
// Note: This always returns the closest point, even if the point is outside
// tree.
// Since dual points are leaves, use the structure of the octree instead
// of a point locator.
vtkIdType vtkHyperOctree::FindPoint(double x[3])
{
  vtkHyperOctreeLightWeightCursor cursor;
  cursor.Initialize(this);
  return this->RecursiveFindPoint(x, &cursor, this->Origin, this->Size);
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperOctree::RecursiveFindPoint(double x[3],
  vtkHyperOctreeLightWeightCursor* cursor,
  double *origin, double *size)
{
  if ( cursor->GetIsLeaf())
    {
    return cursor->GetLeafIndex();
    }

  vtkHyperOctreeLightWeightCursor newCursor;
  newCursor = *cursor;
  double newSize[3];
  double newOrigin[3];
  int ii;
  unsigned char child = 0;
  for (ii = 0; ii < 3; ++ii)
    {
    newSize[ii] = size[ii] * 0.5;
    newOrigin[ii] = origin[ii];
    if (x[ii] >= origin[ii] + newSize[ii])
      {
      child = child | (1 << ii);
      newOrigin[ii] += newSize[ii];
      }
    }
  newCursor.ToChild(child);

  return this->RecursiveFindPoint(x, &newCursor, newOrigin, newSize);
}

//----------------------------------------------------------------------------
// No need for a starting cell.  Just use the point.
// Octree is efficient enough.
vtkIdType vtkHyperOctree::FindCell(double x[3], vtkCell* cell,
                                vtkGenericCell *gencell, vtkIdType cellId,
                                double tol2, int& subId, double pcoords[3],
                                double *weights)
{
  vtkIdType       ptId;
  double          closestPoint[3];
  double          dist2;
  vtkIdList       *cellIds;

  ptId = this->FindPoint(x);
  if ( ptId < 0 )
    {
    return (-1); //if point completely outside of data
    }

  cellIds = vtkIdList::New();
  cellIds->Allocate(8,100);
  this->GetPointCells(ptId, cellIds);
  if ( cellIds->GetNumberOfIds() <= 0 )
    {
    cellIds->Delete();
    return -1;
    }

  int ii, num;
  num = cellIds->GetNumberOfIds();
  for (ii = 0; ii < num; ++ii)
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
  vtkErrorMacro("Could not find cell.");
  return -1;
}


//----------------------------------------------------------------------------
vtkIdType vtkHyperOctree::FindCell(double x[3], vtkCell *cell, vtkIdType cellId,
                               double tol2, int& subId,double pcoords[3],
                               double *weights)
{
  return
    this->FindCell( x, cell, NULL, cellId, tol2, subId, pcoords, weights );
}



//-----------------------------------------------------------------------------
// Generic way to set the leaf data attributes.
vtkDataSetAttributes* vtkHyperOctree::GetLeafData()
{
  if (this->DualGridFlag)
    {
    return this->PointData;
    }
  else
    {
    return this->CellData;
    }
}

//-----------------------------------------------------------------------------
void vtkHyperOctree::SetDualGridFlag(int flag)
{
  if (flag)
    {
    flag = 1;
    }
  if ((this->DualGridFlag && ! flag) ||
      ( ! this->DualGridFlag && flag))
    { // Swap point and cell data.
    vtkDataSetAttributes* attr = vtkDataSetAttributes::New();
    attr->ShallowCopy(this->CellData);
    this->CellData->ShallowCopy(this->PointData);
    this->PointData->ShallowCopy(attr);
    attr->Delete();
    }
  this->DeleteInternalArrays();
  this->DualGridFlag = flag;
  this->Modified();

  if (this->DualGridFlag)
    {
    this->GenerateDualNeighborhoodTraversalTable();
    }
  else
    {
    this->GenerateGridNeighborhoodTraversalTable();
    }
}

//----------------------------------------------------------------------------
unsigned long vtkHyperOctree::GetActualMemorySize()
{
  unsigned long size=this->vtkDataSet::GetActualMemorySize();
  size += this->CellTree->GetActualMemorySize();
  if (this->LeafCenters)
    {
    size += this->LeafCenters->GetActualMemorySize();
    }
  if (this->CornerLeafIds)
    {
    size += this->CornerLeafIds->GetActualMemorySize();
    }
  if (this->CornerPoints)
    {
    size += this->CornerPoints->GetActualMemorySize();
    }
  if (this->CornerLeafIds)
    {
    size += this->CornerLeafIds->GetActualMemorySize();
    }

  return size;
}

//=============================================================================
// Internal arrays used to generate dual grid.  Random access to cells
// requires the cell leaves connectively array which costs memory.


//-----------------------------------------------------------------------------
vtkPoints* vtkHyperOctree::GetLeafCenters()
{
  this->UpdateDualArrays();
  return this->LeafCenters;
}

//-----------------------------------------------------------------------------
vtkIdTypeArray* vtkHyperOctree::GetCornerLeafIds()
{
  this->UpdateDualArrays();
  return this->CornerLeafIds;
}

//-----------------------------------------------------------------------------
void vtkHyperOctree::UpdateDualArrays()
{
  int numLeaves = this->CellTree->GetNumberOfLeaves();
  if (this->LeafCenters)
    {
    if (this->LeafCenters->GetNumberOfPoints() == numLeaves)
      {
      return;
      }
    this->LeafCenters->Delete();
    this->LeafCenters = 0;
    this->CornerLeafIds->Delete();
    this->CornerLeafIds = 0;
    }
  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();

  this->LeafCenters = vtkPoints::New();
  this->LeafCenters->SetNumberOfPoints(this->CellTree->GetNumberOfLeaves());

  this->CornerLeafIds = vtkIdTypeArray::New();
  int dim = this->GetDimension();
  int numComps = 1 << dim;
  this->CornerLeafIds->SetNumberOfComponents(numComps);
  this->CornerLeafIds->Allocate(numLeaves*numComps);

  // Create an array of cursors that occupy 1 3x3x3 neighborhhod.  This
  // will traverse the tree as one.
  // Lower dimensions will not use them all.
  vtkHyperOctreeLightWeightCursor neighborhood[8];
  neighborhood[0].Initialize(this);

  // Keep an index of the root neighbor for computing leaf centers.
  unsigned short xyzIds[3];
  xyzIds[0] = xyzIds[1] = xyzIds[2] = 0;
  this->TraverseDualRecursively(neighborhood, xyzIds, 0);

  this->CornerLeafIds->Squeeze();

  //timer->StopTimer();
  //cout << "Internal dual update : " << timer->GetElapsedTime() << endl;
  //timer->Delete();
}

//----------------------------------------------------------------------------
// Contour the cell associated with this point
// if it has not already been contoured.
// Returns id if a new corner was created, -1 otherwise.
vtkIdType vtkHyperOctree::EvaluateGridCorner(
  int level, vtkHyperOctreeLightWeightCursor* neighborhood,
  unsigned char* visited, int* cornerNeighborIds)
{
  int numLeaves = 1 << this->GetDimension();
  int leaf;
  vtkIdType cornerId;

  for (leaf = 0; leaf < numLeaves; ++leaf)
    {
    // All corners must be leaves
    // Note: this test also makes sure all are initialized.
    if (neighborhood[cornerNeighborIds[leaf]].GetTree() &&
        !neighborhood[cornerNeighborIds[leaf]].GetIsLeaf())
      {
      return -1;
      }
    // If any neighbor on the same level has already generated this point ...
    if (neighborhood[cornerNeighborIds[leaf]].GetLevel() == level &&
        visited[neighborhood[cornerNeighborIds[leaf]].GetLeafIndex()])
      {
      return -1;
      }
    }

  // Point is actually inserted in the Traverse method that call this method.
  cornerId = this->CornerPoints->GetNumberOfPoints();

  // Loop through the leaves to determine which use this point.
  int leafId, sideLeaf;
  for (leaf = 0; leaf < numLeaves; ++leaf)
    {
    if (neighborhood[cornerNeighborIds[leaf]].GetTree())
      { // We know it is a leaf from the previous check.
      // use bitwise exculsive or to find neighbors of leaf.
      leafId = neighborhood[cornerNeighborIds[leaf]].GetLeafIndex();
      sideLeaf = leaf^1;
      if (neighborhood[cornerNeighborIds[sideLeaf]].GetTree() &&
          leafId == neighborhood[cornerNeighborIds[sideLeaf]].GetLeafIndex())
        { // Two neighbors are the same.
        // We are not inserting face or edge points.
        continue;
        }
      if (this->Dimension > 1)
        {
        sideLeaf = leaf^2;
        if (neighborhood[cornerNeighborIds[sideLeaf]].GetTree() &&
            leafId == neighborhood[cornerNeighborIds[sideLeaf]].GetLeafIndex())
          { // Two neighbors are the same.
          // We are not inserting face or edge points.
          continue;
          }
        }
      if (this->Dimension > 2)
        {
        sideLeaf = leaf^4;
        if (neighborhood[cornerNeighborIds[sideLeaf]].GetTree() &&
            leafId == neighborhood[cornerNeighborIds[sideLeaf]].GetLeafIndex())
          { // Two neighbors are the same.
          // We are not inserting face or edge points.
          continue;
          }
        }
      // Center point is opposite to the leaf position in neighborhood.
      this->LeafCornerIds->InsertComponent(leafId, numLeaves-leaf-1,
                                           static_cast<double>(cornerId));
      }
    }

  return cornerId;
}



//-----------------------------------------------------------------------------
vtkPoints* vtkHyperOctree::GetCornerPoints()
{
  this->UpdateGridArrays();
  return this->CornerPoints;
}

//-----------------------------------------------------------------------------
vtkIdTypeArray* vtkHyperOctree::GetLeafCornerIds()
{
  this->UpdateGridArrays();
  return this->LeafCornerIds;
}

//-----------------------------------------------------------------------------
void vtkHyperOctree::UpdateGridArrays()
{
  int numLeaves = this->CellTree->GetNumberOfLeaves();
  if (this->LeafCornerIds)
    {
    if (this->LeafCornerIds->GetNumberOfTuples() == numLeaves)
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

  this->CornerPoints = vtkPoints::New();
  // We cannot be sure exactly how many corners their will be.
  this->CornerPoints->Allocate(numLeaves);

  this->LeafCornerIds = vtkIdTypeArray::New();
  int dim = this->GetDimension();
  int numComps = 1 << dim;
  this->LeafCornerIds->SetNumberOfComponents(numComps);
  this->LeafCornerIds->SetNumberOfTuples(numLeaves);

  // Create a mask array to keep a record of which leaves have already
  // generated their corner cell entries.
  unsigned char* leafMask = new unsigned char[numLeaves];
  // Initialize to 0.
  memset(leafMask, 0, numLeaves);

  // Create an array of cursors that occupy 1 3x3x3 neighborhhod.  This
  // will traverse the tree as one.
  // Lower dimensions will not use them all.
  vtkHyperOctreeLightWeightCursor neighborhood[27];
  int midNeighborId = 0;
  if (dim == 1)
    {
    midNeighborId = 1;
    }
  if (dim == 2)
    {
    midNeighborId = 4;
    }
  if (dim == 3)
    {
    midNeighborId = 13;
    }
  neighborhood[midNeighborId].Initialize(this);

  // Needed as points for non-dual dataset API.
  double origin[3];
  double size[3];
  this->GetOrigin(origin);
  this->GetSize(size);
  // Normal grid API (leaves as hex cells) will be removed soon.
  vtkErrorMacro("This should never happen because I am removing this code soon.\n");
  this->TraverseGridRecursively(neighborhood, leafMask, origin, size);

  delete [] leafMask;

  timer->StopTimer();
  cerr << "Internal grid update : " << timer->GetElapsedTime() << endl;
  timer->Delete();
}

// I may be able to merge the two methods.
// Non dual create the corner points on the boundaries of the tree.
// Dual does not.
//----------------------------------------------------------------------------
// The purpose of traversing the neighborhood / cells is to visit
// every corner and have the leaves connected to that corner.
void vtkHyperOctree::TraverseGridRecursively(
  vtkHyperOctreeLightWeightCursor* neighborhood,
  unsigned char* visited,
  double* origin,
  double* size)
{
  int corner;
  int numCorners = 1 << this->GetDimension();
  int midNeighborId = 0;
  int numNeighbors = 1;
  switch (this->GetDimension())
    {
    case 1:
      midNeighborId = 1;
      numNeighbors = 3;
      break;
    case 2:
      midNeighborId = 4;
      numNeighbors = 9;
      break;
    case 3:
      midNeighborId = 13;
      numNeighbors = 27;
      break;
    }

  int cornerId;
  int cornerNeighborIds[8];
  int level = neighborhood[midNeighborId].GetLevel();
  if (neighborhood[midNeighborId].GetIsLeaf())
    { // Center is a leaf.
    // Evaluate each corner to see if we should process it now.
    for (corner = 0; corner < numCorners; ++corner)
      { // We will not use all of these if dim < 3, but generate anyway.
      cornerNeighborIds[0] = (corner&1) + 3*((corner>>1)&1) + 9*((corner>>2)&1);
      cornerNeighborIds[1] = cornerNeighborIds[0] + 1;
      cornerNeighborIds[2] = cornerNeighborIds[0] + 3;
      cornerNeighborIds[3] = cornerNeighborIds[1] + 3;
      cornerNeighborIds[4] = cornerNeighborIds[0] + 9;
      cornerNeighborIds[5] = cornerNeighborIds[1] + 9;
      cornerNeighborIds[6] = cornerNeighborIds[2] + 9;
      cornerNeighborIds[7] = cornerNeighborIds[3] + 9;
      cornerId = this->EvaluateGridCorner(level,neighborhood,
                                          visited,cornerNeighborIds);
      if (cornerId >= 0)
        { // A bit funny inserting the point here, but we need the
        // to determine the id for the corner leaves in EvaluateGridCorner,
        // and I do not want to compute the point unless absolutely necessary.
        double pt[3];
        // Create the corner point.
        pt[0] = origin[0];
        if (corner&1) { pt[0] += size[0]; }
        pt[1] = origin[1];
        if ((corner>>1)&1) { pt[1] += size[1]; }
        pt[2] = origin[2];
        if ((corner>>2)&1) { pt[2] += size[2]; }
        this->CornerPoints->InsertPoint(cornerId, pt);
        }
      }
    // Mark this leaf as visited.
    // Neighbor value is leafId for leaves, nodeId for nodes.
    visited[neighborhood[midNeighborId].GetLeafIndex()] = 1;
    return;
    }

  // Now recurse.
  double childOrigin[3];
  double childSize[3];
  childSize[0] = size[0]*0.5;
  childSize[1] = size[1]*0.5;
  childSize[2] = size[2]*0.5;
  // We will not use all of these if dim < 3.
  vtkHyperOctreeLightWeightCursor newNeighborhood[27];
  int child;
  int numChildren = 1 <<this->GetDimension();
  int neighbor;
  int tChild, tParent; // used to be uchar with uchar table. VS60 bug
  int* traversalTable = this->NeighborhoodTraversalTable;
  for (child = 0; child < numChildren; ++child)
    {
    // Compute origin for child
    childOrigin[0] = origin[0];
    if (child&1) { childOrigin[0] += childSize[0];}
    childOrigin[1] = origin[1];
    if ((child>>1)&1) { childOrigin[1] += childSize[1];}
    childOrigin[2] = origin[2];
    if ((child>>2)&1) { childOrigin[2] += childSize[2];}
    // Move each neighbor down to a child.
    for (neighbor = 0; neighbor < numNeighbors; ++neighbor)
      {
      // Extract the parent and child of the new node from the traversal table.
      // Child is encoded in the first three bits for all dimensions.
      tChild = (*traversalTable) & 7;
      tParent = ((*traversalTable) & 248)>>3;
      if (neighborhood[tParent].GetTree() == 0)
        { // No node for this neighbor.
        newNeighborhood[neighbor] = neighborhood[tParent];
        }
      else if (neighborhood[tParent].GetIsLeaf())
        { // Parent is a leaf.  Can't traverse anymore.
        // equal operator should work for this class.
        newNeighborhood[neighbor] = neighborhood[tParent];
        }
      else
        { // Move to child.
        // equal operator should work for this class.
        newNeighborhood[neighbor] = neighborhood[tParent];
        newNeighborhood[neighbor].ToChild(tChild);
        }
      ++traversalTable;
      }
    this->TraverseGridRecursively(newNeighborhood, visited,
                                          childOrigin, childSize);
    }
}


//----------------------------------------------------------------------------
// This table is used to move a 3x3x3 neighborhood of cursors through the tree.
void vtkHyperOctree::GenerateGridNeighborhoodTraversalTable()
{
  int xChild, yChild, zChild;
  int xCursor, yCursor, zCursor;
  int xNeighbor, yNeighbor, zNeighbor;
  int xNewCursor, yNewCursor, zNewCursor;
  int xNewChild, yNewChild, zNewChild;
  int cursor, child, newCursor, newChild;

  int numCursors = 1;
  int xChildDim = 1, yChildDim = 1, zChildDim = 1;
  int xCursorDim = 1, yCursorDim = 1, zCursorDim = 1;
  int yChildInc = 2, zChildInc = 4;
  int yCursorInc = 3, zCursorInc = 9;

  assert("Dimension cannot be 0." && this->GetDimension());

  switch (this->GetDimension())
    {
    case 1:
      xChildDim = 2;
      xCursorDim = 3;
      yChildInc = zChildInc = 0;
      yCursorInc = zCursorInc = 0;
      numCursors = 3;
      break;
    case 2:
      xChildDim = yChildDim = 2;
      xCursorDim = yCursorDim = 3;
      zChildInc = zCursorInc = 0;
      numCursors = 9;
      break;
    case 3:
      xChildDim = yChildDim = zChildDim = 2;
      xCursorDim = yCursorDim = zCursorDim = 3;
      numCursors = 27;
      break;
    }

  for (zChild = 0; zChild < zChildDim; ++zChild)
    {
    for (yChild = 0; yChild < yChildDim; ++yChild)
      {
      for (xChild = 0; xChild < xChildDim; ++xChild)
        {
        for (zCursor = 0; zCursor < zCursorDim; ++zCursor)
          {
          for (yCursor = 0; yCursor < yCursorDim; ++yCursor)
            {
            for (xCursor = 0; xCursor < xCursorDim; ++xCursor)
              {
              // Compute the x, y, z index into the
              // 6x6x6 neighborhood of children.
              xNeighbor = xCursor + xChild + 1;
              yNeighbor = yCursor + yChild + 1;
              zNeighbor = zCursor + zChild + 1;
              // Separate neighbor index into Cursor/Child index.
              xNewCursor = xNeighbor / 2;
              yNewCursor = yNeighbor / 2;
              zNewCursor = zNeighbor / 2;
              xNewChild = xNeighbor - xNewCursor*2;
              yNewChild = yNeighbor - yNewCursor*2;
              zNewChild = zNeighbor - zNewCursor*2;
              // Cursor and traversal child are for index into table.
              cursor = xCursor + yCursor*yCursorInc + zCursor*zCursorInc;
              child = xChild + yChild*yChildInc + zChild*zChildInc;
              // New cursor and new child are for the value of the table.
              newCursor = xNewCursor + yNewCursor*yCursorInc + zNewCursor*zCursorInc;
              newChild = xNewChild + yNewChild*yChildInc + zNewChild*zChildInc;
              // We could just incrent a table pointer.
              // Encoding of child in first three bits is the same for all dimensions.
              this->NeighborhoodTraversalTable[numCursors*child + cursor]
                      = newChild+ 8*newCursor;
              }
            }
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkHyperOctree::DeleteInternalArrays()
{
  if (this->LeafCenters)
    {
    this->LeafCenters->Delete();
    this->LeafCenters = 0;
    }
  if (this->CornerLeafIds)
    {
    this->CornerLeafIds->Delete();
    this->CornerLeafIds = 0;
    }
  if (this->CornerPoints)
    {
    this->CornerPoints->Delete();
    this->CornerPoints = 0;
    }
  if (this->LeafCornerIds)
    {
    this->LeafCornerIds->Delete();
    this->LeafCornerIds = 0;
    }

  if (this->Links)
    {
    this->Links->Delete();
    this->Links = 0;
    }
}

//=============================================================================
// Lightweight cursor stuf.
// I needed to include it into this file
// because it needs access to the internal classes.


//-----------------------------------------------------------------------------
// Constructor.
vtkHyperOctreeLightWeightCursor::vtkHyperOctreeLightWeightCursor()
{
  this->Level = 0;
  this->IsLeaf = 0;
  this->Index = 0;
  this->Tree = 0;
}

//-----------------------------------------------------------------------------
vtkHyperOctreeLightWeightCursor::~vtkHyperOctreeLightWeightCursor()
{
  this->Level = 0;
  this->IsLeaf = 1;
  this->Index = 0;
  // I can't reference count because of the default copy constructor.
  //if (this->Tree)
  //  {
  //  this->Tree->UnRegister(0);
  //  }
  this->Tree = 0;
}


//-----------------------------------------------------------------------------
void vtkHyperOctreeLightWeightCursor::Initialize(vtkHyperOctree* tree)
{
  //if (this->Tree)
  //  {
  //  this->Tree->UnRegister(0);
  //  }
  this->Tree = tree;
  if (tree == 0)
    {
    return;
    }
  //this->Tree->Register(0);

  this->ToRoot();
}

//-----------------------------------------------------------------------------
unsigned short vtkHyperOctreeLightWeightCursor::GetIsLeaf()
{
  // I want enpty cursors to appear like a leaf so recursion stops.
  if (this->Tree == 0)
    {
    return 1;
    }
  return this->IsLeaf;
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeLightWeightCursor::ToRoot()
{
  if (this->Tree == 0)
    {
    return;
    }
  this->Level = 0;
  if (this->Tree->CellTree->GetNumberOfLeaves() == 1)
    { // Root is a leaf.
    this->Index = 0;
    this->IsLeaf = 1;
    }
  else
    { // Root is a node.
    this->Index = 1; // First node (0) is a special empty node.
    this->IsLeaf = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeLightWeightCursor::ToChild(int child)
{
  if (this->Tree == 0)
    {
    return;
    }
  if (this->IsLeaf)
    { // Leaves do not have children.
    return;
    }

  if (this->Tree->Dimension == 3)
    {
    vtkCompactHyperOctree<3>* tree3;
    tree3 = static_cast<vtkCompactHyperOctree<3>*>(this->Tree->CellTree);
    vtkCompactHyperOctreeNode<3> *node=tree3->GetNode(this->Index);
    this->Index=node->GetChild(child);
    this->IsLeaf=node->IsChildLeaf(child);
    this->Level += 1;
    assert("Bad index" && this->Index >= 0);
    if (this->IsLeaf)
      {
      assert("Bad leaf index" && this->Index < this->Tree->CellTree->GetNumberOfLeaves());
      }
    else
      {
      assert("Bad node index" && this->Index < this->Tree->CellTree->GetNumberOfNodes());
      }
    }
  else if (this->Tree->Dimension == 2)
    {
    vtkCompactHyperOctree<2>* tree2;
    tree2 = static_cast<vtkCompactHyperOctree<2>*>(this->Tree->CellTree);
    vtkCompactHyperOctreeNode<2> *node=tree2->GetNode(this->Index);
    this->Index=node->GetChild(child);
    this->IsLeaf=node->IsChildLeaf(child);
    this->Level += 1;
    }
  else if (this->Tree->Dimension == 1)
    {
    vtkCompactHyperOctree<1>* tree1;
    tree1 = static_cast<vtkCompactHyperOctree<1>*>(this->Tree->CellTree);
    vtkCompactHyperOctreeNode<1> *node=tree1->GetNode(this->Index);
    this->Index=node->GetChild(child);
    this->IsLeaf=node->IsChildLeaf(child);
    this->Level += 1;
    }
  return;
}



//=============================================================================
// Here is a faster way to generate the dual grid.
// It uses a 2x2x2 cursor rather than a 3x3x3 cursor.

//----------------------------------------------------------------------------
// This table is used to move a 2x2x2 neighborhood of cursors through the tree.
void vtkHyperOctree::GenerateDualNeighborhoodTraversalTable()
{
  int xChild, yChild, zChild;
  int xCursor, yCursor, zCursor;
  int xNeighbor, yNeighbor, zNeighbor;
  int xNewCursor, yNewCursor, zNewCursor;
  int xNewChild, yNewChild, zNewChild;
  int cursor, child, newCursor, newChild;

  int numCursors;
  int xChildDim, yChildDim, zChildDim;
  int xCursorDim, yCursorDim, zCursorDim;
  int yChildInc, zChildInc;
  int yCursorInc, zCursorInc;
  xChildDim = yChildDim = zChildDim = 1;
  xCursorDim = yCursorDim = zCursorDim = 1;
  yChildInc = yCursorInc = 2;
  zChildInc = zCursorInc = 4;

  assert("Dimension cannot be 0." && this->GetDimension());

  numCursors = 1<<this->GetDimension();
  switch (this->GetDimension())
    {
    case 1:
      xChildDim = xCursorDim = 2;
      yChildInc = zChildInc = yCursorInc = zCursorInc = 0;
      break;
    case 2:
      xChildDim = yChildDim = xCursorDim = yCursorDim = 2;
      zChildInc = zCursorInc = 0;
      break;
    case 3:
      xChildDim = yChildDim = zChildDim = xCursorDim = yCursorDim = zCursorDim = 2;
      break;
    }


  for (zChild = 0; zChild < zChildDim; ++zChild)
    {
    for (yChild = 0; yChild < yChildDim; ++yChild)
      {
      for (xChild = 0; xChild < xChildDim; ++xChild)
        {
        for (zCursor = 0; zCursor < zCursorDim; ++zCursor)
          {
          for (yCursor = 0; yCursor < yCursorDim; ++yCursor)
            {
            for (xCursor = 0; xCursor < xCursorDim; ++xCursor)
              {
              // Compute the x, y, z index into the
              // 4x4x4 neighborhood of children.
              xNeighbor = xCursor + xChild;
              yNeighbor = yCursor + yChild;
              zNeighbor = zCursor + zChild;
              // Separate neighbor index into Cursor/Child index.
              xNewCursor = xNeighbor / 2;
              yNewCursor = yNeighbor / 2;
              zNewCursor = zNeighbor / 2;
              xNewChild = xNeighbor - xNewCursor*2;
              yNewChild = yNeighbor - yNewCursor*2;
              zNewChild = zNeighbor - zNewCursor*2;


              // Cursor and traversal child are for index into table.
              cursor = xCursor + yCursor*yCursorInc + zCursor*zCursorInc;
              child = xChild + yChild*yChildInc + zChild*zChildInc;
              // New cursor and new child are for the value of the table.
              newCursor = xNewCursor + yNewCursor*yCursorInc + zNewCursor*zCursorInc;
              newChild = xNewChild + yNewChild*yChildInc + zNewChild*zChildInc;
              // We could just incrent a table pointer.
              // Encoding of child in first three bits is the same for all dimensions.
              this->NeighborhoodTraversalTable[numCursors*child + cursor]
                      = newChild+ 8*newCursor;
              }
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
// The purpose of traversing the neighborhood / cells is to visit
// every point and have the cells connected to that point.
void vtkHyperOctree::TraverseDualRecursively(
  vtkHyperOctreeLightWeightCursor* neighborhood,
  unsigned short *xyzIds, int level)
{
  unsigned char divide = 0;
  unsigned char childrenToTraverse[8];
  memset(childrenToTraverse,0,8);
  static int debugStackOverflow = 0;
  int debugFlag = 0;


  //if (level > 15)
  //  { // something went wrong
  //  // After debugging, move this value up to 20 or 30.
  //  cerr << "Maximum recursion level reached\n";
  //  return;
  //  }

  if (debugStackOverflow < level)
    {
    debugStackOverflow = level;
    debugFlag = 1;

    cout << "Max depth " << level << ", ids: "
         << xyzIds[0] << " " << xyzIds[1] << " " << xyzIds[2] << endl;
    }

  if (debugFlag && level == 0)
    {
    cout << "Tree has " << neighborhood[0].GetTree()->GetNumberOfLeaves() << " leaves\n";
    }

  if ( ! neighborhood[0].GetIsLeaf())
    { // Main cursor is a node.  Traverse all children.
    divide = 1;
    childrenToTraverse[0] = childrenToTraverse[1]
       = childrenToTraverse[2] = childrenToTraverse[3]
       = childrenToTraverse[4] = childrenToTraverse[5]
       = childrenToTraverse[6] = childrenToTraverse[7] = 1;
    if (debugFlag)
      {
      cout << "  Divide because 0 is a node with id "
           << neighborhood[0].GetLeafIndex() << ".\n";
      }
    }
  else
    {
    if (debugFlag)
      {
      cout << "  Neighbor 0 is a leaf with id "
           << neighborhood[0].GetLeafIndex() << ".\n";
      }
    if (neighborhood[0].GetLevel() == level)
      {
      // Add the leaf center point.
      double levelDim = static_cast<double>(1<<neighborhood[0].GetLevel());
      double pt[3];
      // Compute point.  Expand if point is on boundary.
      // This will make the dual have the same bounds as tree.
      // X
      if (xyzIds[0] == 0)
        {
        pt[0]= this->Origin[0];
        }
      else if (neighborhood[1].GetTree() == 0)
        {
        pt[0]= this->Origin[0] + this->Size[0];
        }
      else
        {
        pt[0]= this->Origin[0] +
          (static_cast<double>(xyzIds[0])+0.5)*(this->Size[0])/levelDim;
        }
      // Y
      if (this->Dimension < 2 || xyzIds[1] == 0)
        {
        pt[1]= this->Origin[1];
        }
      else if (neighborhood[2].GetTree() == 0)
        {
        pt[1]= this->Origin[1] + this->Size[1];
        }
      else
        {
        pt[1]= this->Origin[1] +
          (static_cast<double>(xyzIds[1])+0.5)*(this->Size[1])/levelDim;
        }
      // Z
      if (this->Dimension < 3 || xyzIds[2] == 0)
        {
        pt[2]= this->Origin[2];
        }
      else if (neighborhood[4].GetTree() == 0)
        {
        pt[2]= this->Origin[2] + this->Size[2];
        }
      else
        {
        pt[2]= this->Origin[2] +
          (static_cast<double>(xyzIds[2])+0.5)*(this->Size[2])/levelDim;
        }
      this->LeafCenters->InsertPoint(neighborhood[0].GetLeafIndex(), pt);
      }
    if (! neighborhood[1].GetIsLeaf() )
      { // x face
      divide = 1;
      childrenToTraverse[1] = childrenToTraverse[3]
         = childrenToTraverse[5] = childrenToTraverse[7] = 1;
      if (debugFlag)
        {
        if (neighborhood[1].GetTree())
          {
          cout << "  Divide because 1 is a node with id "
              << neighborhood[1].GetLeafIndex() << ".\n";
          }
        else
          {
          cout << "  Divide because 1 is a NULL node.\n";
          }
        }
      }
    if (! neighborhood[2].GetIsLeaf() )
      { // y face
      divide = 1;
      childrenToTraverse[2] = childrenToTraverse[3]
         = childrenToTraverse[6] = childrenToTraverse[7] = 1;
      if (debugFlag)
        {
        if (neighborhood[2].GetTree())
          {
          cout << "  Divide because 2 is a node with id "
              << neighborhood[2].GetLeafIndex() << ".\n";
          }
        else
          {
          cout << "  Divide because 2 is a NULL node.\n";
          }
        }
      }
    if (! neighborhood[4].GetIsLeaf() )
      { // z face
      divide = 1;
      childrenToTraverse[4] = childrenToTraverse[5]
         = childrenToTraverse[6] = childrenToTraverse[7] = 1;
      if (debugFlag)
        {
        if (neighborhood[4].GetTree())
          {
          cout << "  Divide because 4 is a node with id "
              << neighborhood[4].GetLeafIndex() << ".\n";
          }
        else
          {
          cout << "  Divide because 4 is a NULL node.\n";
          }
        }
      }
    if (! neighborhood[3].GetIsLeaf() )
      { // xy edge
      divide = 1;
      childrenToTraverse[3] = childrenToTraverse[7] = 1;
      if (debugFlag)
        {
        if (neighborhood[3].GetTree())
          {
          cout << "  Divide because 3 is a node with id "
              << neighborhood[3].GetLeafIndex() << ".\n";
          }
        else
          {
          cout << "  Divide because 3 is a NULL node.\n";
          }
        }
      }
    if (! neighborhood[5].GetIsLeaf() )
      { // xz edge
      divide = 1;
      childrenToTraverse[5] = childrenToTraverse[7] = 1;
      if (debugFlag)
        {
        if (neighborhood[5].GetTree())
          {
          cout << "  Divide because 5 is a node with id "
              << neighborhood[5].GetLeafIndex() << ".\n";
          }
        else
          {
          cout << "  Divide because 5 is a NULL node.\n";
          }
        }
      }
    if (! neighborhood[6].GetIsLeaf() )
      { // xz edge
      divide = 1;
      childrenToTraverse[6] = childrenToTraverse[7] = 1;
      if (debugFlag)
        {
        if (neighborhood[6].GetTree())
          {
          cout << "  Divide because 6 is a node with id "
              << neighborhood[6].GetLeafIndex() << ".\n";
          }
        else
          {
          cout << "  Divide because 6 is a NULL node.\n";
          }
        }
      }
    if (! neighborhood[7].GetIsLeaf() )
      { // xyz corner
      divide = 1;
      childrenToTraverse[7] = 1;
      if (debugFlag)
        {
        if (neighborhood[7].GetTree())
          {
          cout << "  Divide because 7 is a node with id "
              << neighborhood[7].GetLeafIndex() << ".\n";
          }
        else
          {
          cout << "  Divide because 7 is a NULL node.\n";
          }
        }
      }
    }

  if (divide)
    {
    unsigned char numChildren = 1 << this->Dimension;
    unsigned char child;
    unsigned char neighbor;
    int tChild, tParent; // used to be uchar, but VS60 had a bug.
    int* traversalTable = this->NeighborhoodTraversalTable;
    vtkHyperOctreeLightWeightCursor newNeighborhood[8];
    // Storing 4 per neighbor for efficiency.
    // This might also be useful for 4d trees :)
    unsigned short newXYZIds[3];
    for (child = 0; child < numChildren; ++child)
      {
      if (childrenToTraverse[child])
        {
        // Move the xyz index of the root neighbor down.
        if (! neighborhood[0].GetIsLeaf())
          {
          // Multiply parent index by two for new level.
          // Increment by 1 if child requires.
          newXYZIds[0] = (xyzIds[0] << 1) | (child&1);
          newXYZIds[1] = (xyzIds[1] << 1) | ((child>>1)&1);
          newXYZIds[2] = (xyzIds[2] << 1) | ((child>>2)&1);
          }
        else
          { // This is not necessary because the indexes are not used
          // when traversing into a leaf for neighbors.
          // It is just for debugging.
          newXYZIds[0] = (xyzIds[0] << 1) | (child&1);
          newXYZIds[1] = (xyzIds[1] << 1) | ((child>>1)&1);
          newXYZIds[2] = (xyzIds[2] << 1) | ((child>>2)&1);
          }
        // Move each neighbor down to a child.
        for (neighbor = 0; neighbor < numChildren; ++neighbor)
          {
          tChild = (*traversalTable) & 7;
          tParent = ((*traversalTable) & 248)>>3;
          if (neighborhood[tParent].GetIsLeaf())
            { // Parent is a leaf or this is an empty node.
            // We can't traverse anymore.
            // equal operator should work for this class.
            newNeighborhood[neighbor] = neighborhood[tParent];
            }
          else
            { // Move to child.
            // equal operator should work for this class.
            newNeighborhood[neighbor] = neighborhood[tParent];
            newNeighborhood[neighbor].ToChild(tChild);
            }
          ++traversalTable;
          }
        this->TraverseDualRecursively(newNeighborhood,newXYZIds,level+1);
        }
      else
        {
        traversalTable += numChildren;
        }
      }
    return;
    }

  if (debugFlag){cout << "  All neighbors are leaves. Terminate recursion.\n";}

  // All neighbors must be leaves.

  // If we are not on the border, create the cell
  // associated with the center point of the neighborhood.
  this->EvaluateDualCorner(neighborhood);
}

//----------------------------------------------------------------------------
// Contour the cell assocaited with the center point.
// if it has not already been contoured.
void vtkHyperOctree::EvaluateDualCorner(
  vtkHyperOctreeLightWeightCursor* neighborhood)
{
  unsigned char numCorners = 1 << this->GetDimension();
  unsigned char corner;
  // We will not use all of these components if dim < 3.
  vtkIdType leaves[8];

  for (corner = 0; corner < numCorners; ++corner)
    {
    // If any neighbor is NULL, then we are on the border.
    // Do nothing if we are on a border.
    // We know that neighbor 0 is never NULL.
    if (!neighborhood[corner].GetTree())
      {
      return;
      }
    leaves[corner] = neighborhood[corner].GetLeafIndex();
    }

  this->CornerLeafIds->InsertNextTupleValue(leaves);
}

//----------------------------------------------------------------------------
vtkHyperOctree* vtkHyperOctree::GetData(vtkInformation* info)
{
  return info? vtkHyperOctree::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkHyperOctree* vtkHyperOctree::GetData(vtkInformationVector* v, int i)
{
  return vtkHyperOctree::GetData(v->GetInformationObject(i));
}
