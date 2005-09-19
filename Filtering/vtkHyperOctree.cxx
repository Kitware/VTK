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

#include "vtkHyperOctreeCursor.h"
#include <vtkstd/vector>
#include "vtkDataSetAttributes.h"
#include "vtkCellData.h"
#include <assert.h>
#include "vtkObjectFactory.h"
#include "vtkCellType.h"
#include <vtkstd/deque>
//#include <vtkstd/set>
#include "vtkOrderedTriangulator.h"
#include "vtkPolygon.h"
#include "vtkPoints.h"
#include "vtkHyperOctreePointsGrabber.h"

#include "vtkInformationIntegerKey.h"
#include "vtkInformationDoubleVectorKey.h"

const int VTK_HYPEROCTREE=15;
vtkInformationKeyMacro(vtkHyperOctree, LEVELS, Integer);
vtkInformationKeyMacro(vtkHyperOctree, DIMENSION, Integer);
vtkInformationKeyRestrictedMacro(vtkHyperOctree, SIZES, DoubleVector, 3);

class vtkHyperOctreeInternal
  : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkHyperOctreeInternal,vtkObject);
  virtual void Initialize()=0;
  virtual vtkHyperOctreeCursor *NewCursor()=0;
  virtual vtkIdType GetNumberOfLeaves()=0;
  
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
  
  this->CellTree->PrintSelf(os,indent);
}


vtkCxxRevisionMacro(vtkHyperOctreeInternal, "1.3");

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
  
  vtkTypeRevisionMacro(vtkCompactHyperOctreeCursor<D>,vtkHyperOctreeCursor);
  
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
  
  vtkstd::deque<int> ChildHistory; // a stack, but stack does not have clear()
  int Index[D]; // index in each dimension of the current node, as if the
  // tree at the current level was a uniform grid.
private:
  vtkCompactHyperOctreeCursor(const vtkCompactHyperOctreeCursor<D> &);  // Not implemented.
  void operator=(const vtkCompactHyperOctreeCursor<D> &);    // Not implemented.
};

// vtkCxxRevisionMacro(vtkCompactHyperOctreeCursor, "1.3");
template<unsigned int D>
void vtkCompactHyperOctreeCursor<D>::CollectRevisions(ostream& sos)
{
  vtkOStreamWrapper os(sos);
  this->Superclass::CollectRevisions(os);
  os << "vtkCompactHyperOctreeCursor<" << D <<"> " << "1.3" << '\n';
}
  

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
      
//      vtkstd::bitset<8> b=this->LeafFlags;
      
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

//vtkCxxRevisionMacro(vtkCompactHyperOctree, "1.3");

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
  
  vtkTypeRevisionMacro(vtkCompactHyperOctree<D>,vtkHyperOctreeInternal);
  
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
  int GetNumberOfNodes()
    {
      assert("post: not_empty" && this->Nodes.size()>0);
      return this->Nodes.size();
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
      int nodeIndex=this->Nodes.size();
      cursor->SetCursor(nodeIndex);
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
      parent->SetChild(i,nodeIndex);
      
      // The first new child
      this->Nodes[nodeIndex].SetChild(0,leafIndex);
      this->LeafParent[leafIndex]=nodeIndex;
        
      // The other (c-1) new children.
      int nextLeaf=this->LeafParent.size();
      this->LeafParent.resize(nextLeaf+(c-1));
      i=1;
      while(i<c)
        {
        this->Nodes[nodeIndex].SetChild(i,nextLeaf);
        this->LeafParent[nextLeaf]=nodeIndex;
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
      return this->LeafParent.size();
    }
  
  //---------------------------------------------------------------------------
  void PrintSelf(ostream& os, vtkIndent indent)
    {
      this->Superclass::PrintSelf(os,indent);
      
      os << indent << "Nodes="<<this->Nodes.size()<<endl;
      os << indent << "LeafParent="<<this->LeafParent.size()<<endl;
      
      os << indent << "Nodes="<<this->Nodes.size()<<endl;
      int i;
      os << indent;
      i=0;
      int c=this->Nodes.size();
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
  
  vtkstd::vector<int> NumberOfLeavesPerLevel; // number of leaves in each level
  // its size is NumberOfLevels;
  
  vtkIdType NumberOfLevels;
  vtkstd::vector<vtkCompactHyperOctreeNode<D> > Nodes;
  vtkstd::vector<int> LeafParent; // record the parent of each leaf
  vtkDataSetAttributes *Attributes; // cell data or point data.
private:
  vtkCompactHyperOctree(const vtkCompactHyperOctree<D> &);  // Not implemented.
  void operator=(const vtkCompactHyperOctree<D> &);    // Not implemented.
};

// vtkCxxRevisionMacro(vtkCompactHyperOctree, "1.3");
template<unsigned int D>
void vtkCompactHyperOctree<D>::CollectRevisions(ostream& sos)
{
  vtkOStreamWrapper os(sos);
  this->Superclass::CollectRevisions(os);
  os << "vtkCompactHyperOctree<" << D <<"> " << "1.3" << '\n';
}
  

// octree: vtkHyperOctreeInternal<3>
// quadtree: vtkHyperOctreeInternal<2>
// bittree: vtkHyperOctreeInternal<1>

vtkCxxRevisionMacro(vtkHyperOctree, "1.3");
vtkStandardNewMacro(vtkHyperOctree);

//-----------------------------------------------------------------------------
// Default constructor.
vtkHyperOctree::vtkHyperOctree()
{
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
  this->PointTree=0;
  
  this->TmpChild=this->NewCellCursor();
}

//-----------------------------------------------------------------------------
// Destructor.
vtkHyperOctree::~vtkHyperOctree()
{
  if(this->CellTree!=0)
    {
    this->CellTree->UnRegister(this);
    }
  if(this->PointTree!=0)
    {
    this->PointTree->UnRegister(this);
    }
   this->TmpChild->UnRegister(this);
}


//-----------------------------------------------------------------------------
// Description:
  // Return what type of dataset this is.
int vtkHyperOctree::GetDataObjectType()
{
  return VTK_HYPEROCTREE;
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
  
  if(this->PointTree!=0)
    {
    this->PointTree->UnRegister(this);
    }
  this->PointTree=ho->PointTree;
  if(this->PointTree!=0)
    {
    this->PointTree->Register(this);
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
// Return the number of points.
// \post positive_result: result>=0
vtkIdType vtkHyperOctree::GetNumberOfPoints()
{
  return 0;
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
// Return the number of leaves.
// \post positive_result: result>=0
vtkIdType vtkHyperOctree::GetNumberOfCells()
{
  vtkIdType result=this->CellTree->GetNumberOfLeaves();
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
}

//-----------------------------------------------------------------------------
// Description:
// Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
// THIS METHOD IS NOT THREAD SAFE.
double *vtkHyperOctree::GetPoint(vtkIdType vtkNotUsed(ptId))
{
  assert("check: TODO" && 0);
  return 0;
}

//-----------------------------------------------------------------------------
// Description:
// Copy point coordinates into user provided array x[3] for specified
// point id.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperOctree::GetPoint(vtkIdType vtkNotUsed(id), double x[3])
{
  (void)x;
  assert("check: TODO" && 0);
}
  
//-----------------------------------------------------------------------------
// Description:
// Get cell with cellId such that: 0 <= cellId < NumberOfCells.
// THIS METHOD IS NOT THREAD SAFE.
vtkCell *vtkHyperOctree::GetCell(vtkIdType vtkNotUsed(cellId))
{
  assert("check: TODO" && 0);
  return 0;
}

//-----------------------------------------------------------------------------
// Description:
// Get cell with cellId such that: 0 <= cellId < NumberOfCells. 
// This is a thread-safe alternative to the previous GetCell()
// method.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperOctree::GetCell(vtkIdType vtkNotUsed(cellId),
                             vtkGenericCell *vtkNotUsed(cell))
{
  assert("check: TODO" && 0);
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
      result=0; // useless, just to avoid a warning
      assert("check: impossible_case" && 0);
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
void vtkHyperOctree::GetCellPoints(vtkIdType vtkNotUsed(cellId),
                                   vtkIdList *vtkNotUsed(ptIds))
{
  assert("check: TODO" && 0);
}

//-----------------------------------------------------------------------------
  // Description:
  // Topological inquiry to get cells using point.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
void vtkHyperOctree::GetPointCells(vtkIdType vtkNotUsed(ptId),
                                   vtkIdList *vtkNotUsed(cellIds))
{
  assert("check: TODO" && 0);
}

  
//-----------------------------------------------------------------------------
// Description:
// Topological inquiry to get all cells using list of points exclusive of
// cell specified (e.g., cellId). Note that the list consists of only
// cells that use ALL the points provided.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperOctree::GetCellNeighbors(vtkIdType vtkNotUsed(cellId),
                                      vtkIdList *vtkNotUsed(ptIds), 
                                      vtkIdList *vtkNotUsed(cellIds))
{
  assert("check: TODO" && 0);
}


vtkIdType vtkHyperOctree::FindPoint(double x[3])
{
  (void)x;
  assert("check: TODO" && 0);
  return 0;
}

//-----------------------------------------------------------------------------
// Description:
// Locate cell based on global coordinate x and tolerance
// squared. If cell and cellId is non-NULL, then search starts from
// this cell and looks at immediate neighbors.  Returns cellId >= 0
// if inside, < 0 otherwise.  The parametric coordinates are
// provided in pcoords[3]. The interpolation weights are returned in
// weights[]. (The number of weights is equal to the number of
// points in the found cell). Tolerance is used to control how close
// the point is to be considered "in" the cell.
// THIS METHOD IS NOT THREAD SAFE.
vtkIdType vtkHyperOctree::FindCell(double x[3],
                                   vtkCell *vtkNotUsed(cell),
                                   vtkIdType vtkNotUsed(cellId),
                                   double vtkNotUsed(tol2),
                                   int& vtkNotUsed(subId),
                                   double pcoords[3],
                                   double *vtkNotUsed(weights))
{
  (void)x;
  (void)pcoords;
  assert("check: TODO" && 0);
  return 0;
}

//-----------------------------------------------------------------------------
// Description:
// This is a version of the above method that can be used with 
// multithreaded applications. A vtkGenericCell must be passed in
// to be used in internal calls that might be made to GetCell()
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
vtkIdType vtkHyperOctree::FindCell(double x[3],
                                   vtkCell *vtkNotUsed(cell),
                                   vtkGenericCell *vtkNotUsed(gencell),
                                   vtkIdType vtkNotUsed(cellId),
                                   double vtkNotUsed(tol2),
                                   int& vtkNotUsed(subId),
                                   double pcoords[3],
                                   double *vtkNotUsed(weights))
{
  (void)x;
  (void)pcoords;
  assert("check: TODO" && 0);
  return 0;
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
    this->CellTree->SetAttributes(this->CellData);
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
  
//  this->Width=1;
//  this->Height=1;
//  this->Depth=1;
  if(this->PointTree!=0)
    {
    this->PointTree->UnRegister(this);
    this->PointTree=0;
    }
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
      result=0; // useless, just to avoid a warning
      assert("check: impossible_case" && 0);
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

