/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTree.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkTree - Generic tree.
// .SECTION Description
// OK Here is the approach: Tree objects are similar to vtkCollections, but
// they manage generic pointers, and have multiple levels (see note1).
// Trees have internal elements (nodes and leaves) that are not subclasses
// of vtkObject.  Memory usage is conhsidered an important issue, especially
// for leaves.
// There are two methods for accessing the data in tree: 
// 1: Slow and stable:  Tree allow traversal up and down hierachy.
// 2: Fast but risky:  Internal objects can be delt with directly.
//    This is risky because changes to the internal tree code could
//    Affect other objects.

// Note 1:
// We may be able to make vtkTree a subclass of vtkCollection. Although 
// appealing, to be truely useful, all subclasses of vtkCollection should 
// then be changed to subclasses of vtkTree.

// Note 2:
// To save space and a level of indirection, elements define a linked
// list with their Next ivar.  This make sharing protions of trees
// with shallow copies almost impossible.  If this becomes an issue, 
// we may consider creating a Node list or an element list so nodes
// or elements could be members of multiple lists.

// Note 3:
// Only subclasses of vtkTree need to be defined (not vtkElements),
// but Items may have speciall ways of pringting themselves, or deleting
// thems selves.  There are two ways of doing this:  Have tree Print and 
// Delete every thing, or pass Tree pointer (or function pointer) to
// element print and delete methods so tree methods are used to print or 
// delete items.

// Note 4:
// Now that nodes and leaves have items, maybe they should have different
// ways of printing/different themselves.  This extension should not be 
// a problem because PrintItem could be called by the defualt PrintNodeItem
// or PrintLeafItem.

// Note 5:
// For the c++ access to the internals to be divorced from actual
// implementation, no pointers should be used.  Elements and other
// internal objects should copy themselves.  Since these objects are 
// small, time to copy should not be a big performance hit.  
// An example:  Current implementation does not have an ChildList object.
// one could be created as {vtkTreeElement *start; vtkTreeElement *traversal;}
// and created on the fly to look like Element list are actually part of the 
// internal tree structure.  Deleting the list objects would not be an issue,
// because returned objects are copied.

// For tcl wrapping:
// For now elements are not wrapped. ... but if they were wrapped ...
// Nodes and Leafs must have same methods but different results.
// Virtual methods could still be avoided with vtkTreeElement using
// an internal check.  This approach avoids any need to do typecasting.

// Todo:
// Safe but fast access to tree elements (element lists).


#ifndef __vtkTree_h
#define __vtkTree_h

#include "vtkReferenceCount.h"

// helper objects
class vtkTreeElement;
class vtkTreeNode;
class vtkTreeLeaf;
class vtkTreeCons;

//============================================================================
// The actual tree object.
//============================================================================

class VTK_EXPORT vtkTree : public vtkReferenceCount
{
public:
  vtkTree();
  ~vtkTree();
  const char *GetClassName() {return "vtkTree";};
  static vtkTree *New() {return new  vtkTree;};

  void PrintSelf(ostream& os, vtkIndent indent);

  // Methods for traversing tree.  
  // They are all relative to the current element.
  int GetNumberOfChildren();
  int GetCurrentLevel() {return this->TraversalDepth;}
  // Moves current node down tree.  Returns current level.
  int  MoveToChild(int idx);
  // Moves up one level. Returns current level.
  int MoveToParent();
  // Moves to a level between current level and 0 (root).
  void MoveToLevel(int idx);
  // Adds a node to the end of the element list of the current node.
  // Returns the position of the new node in the element list.
  // Returns -1 on error.
  int AddNewNode();
  // Create and add a leaf to the end of the current element.
  // Returns the position of the new leaf in the element list.
  // Returns -1 on error.
  int AddNewLeaf();
  
  // Access to the item in the current element
  void SetItem(void *item);
  void *GetItem();
  
  // Access to items as generic collection.
  // Right now the serach is depth first, 
  // but I envision breadth first as an option.
  int GetNumberOfItems();
  // Returns NULL if index is out bounds.
  void *GetItem(int idx);

//BTX - begin tcl exclude
  // This is for fast traversal of the tree.  Warning, access protocall
  // to tree elements may change (unlike access through current element
  // methods of tree).
  vtkTreeElement *GetRoot() {return this->Root;}
  // Be carfull!  vtkTreeElements are not reference counted.
  // These method are meant for internal use (Cut/PasteChild method)
  vtkTreeElement *PopChild(int idx);
  int AddChild(vtkTreeElement *element);
//ETX - end tcl exclude

  // These methods move a child (a whole branch) from
  // one tree to another.  They return the index of the element
  // in the destination tree.  Although a little awkward,
  // they provide a safe way (available from tcl) to edit trees
  // branches at a time.
  int CutChild(int idx, vtkTree *destination);
  int PasteChild(vtkTree *source, int idx);
  
protected:
  // This list keeps the Traversal History for moving up the current node.
  vtkTreeCons *TraversalList;
  // Traversal is always the last element in TraversalList
  vtkTreeCons *Traversal;
  // Position of traversal in TraversalList.
  int TraversalDepth;
  
  // One node created when constructing.
  vtkTreeElement *Root;

  // Helper methods
  virtual void DeleteElement(vtkTreeElement *element);
  virtual void DeleteItem(void *item);
  virtual void PrintElement(ostream& os, vtkIndent indent, vtkTreeElement *e);
  virtual void PrintItem(ostream& os, vtkIndent indent, void *item);
};











// Element returns one of these to specify what type it is ???????
#define VTK_TREE_NODE 0
#define VTK_TREE_LEAF 1


//BTX - begin tcl exclude
class VTK_EXPORT vtkTreeElement
{
public:
  vtkTreeElement();
  virtual ~vtkTreeElement();  
  // Make this object look like a vtkObject.
  virtual void Delete(); 
  static vtkTreeElement *New() {return new vtkTreeElement;};
  virtual const char *GetClassName() {return "vtkTreeElement";};

  // These could be pure virtual but, ...
  // a quick way to tell if the element is a leaf
  virtual int GetElementType() {return -1;}
  // Get the subelements (first element one lower level)
  virtual vtkTreeElement *GetChildList() {return NULL;}
  virtual void SetChildList(vtkTreeElement *e) {e = e;}
  virtual int AddChild(vtkTreeElement *e) {e=e; return -1;}
  
  // Returns the number of items in the whole tree.
  virtual int GetNumberOfItems() {return 0;}
  virtual void *GetItem(int *pIdx) {pIdx = pIdx; return NULL;}
  virtual void *GetItem() {return this->Item;}
  void SetItem(void *item) {this->Item = item;}

  // Next element same depth as this (linked list)
  vtkTreeElement *GetNext();
  void SetNext(vtkTreeElement *element);

protected:
  // Put an item at every element (nodes and leaves)
  void *Item;
  // built in linked list
  vtkTreeElement *Next;
};


class VTK_EXPORT vtkTreeNode : public vtkTreeElement
{
public:
  vtkTreeNode();
  ~vtkTreeNode();
  // Make this object look like a vtkObject.
  static vtkTreeNode *New() {return new vtkTreeNode;};
  const char *GetClassName() {return "vtkTreeNode";};
  void Delete() {delete this;} 

  // These could be pure virtual but, ...
  // a quick way to tell if the element is a leaf
  int GetElementType() {return VTK_TREE_NODE;}
  // Get the subelements (first element one lower level)
  vtkTreeElement *GetChildList() {return this->ChildList;}
  // be careful!!! (meant for internal use)
  void SetChildList(vtkTreeElement *e) {this->ChildList = e;}
  // Returns the number of items in the whole tree.
  int GetNumberOfItems();
  // Returns NULL if error.
  // Idx is decremented by the number of items in the node 
  // (but not less than 0).
  void *GetItem(int *pIdx);
  // Redefine because c++ hides the element method.
  void *GetItem() {return this->Item;}

  // Adds to the begining of the list.
  int AddChild(vtkTreeElement *e);
  
private:
  // This is actually a linked list of elements
  vtkTreeElement *ChildList;
};


class VTK_EXPORT vtkTreeLeaf : public vtkTreeElement
{
public:
  vtkTreeLeaf();
  ~vtkTreeLeaf();
  // Make this object look like a vtkObject.
  static vtkTreeLeaf *New() {return new vtkTreeLeaf;}
  const char *GetClassName() {return "vtkTreeLeaf";}
  void Delete() {delete this;} 
  
  // A way to tell if the element is a leaf or node.
  int GetElementType() {return VTK_TREE_LEAF;}
  // Returns the number of items in the whole tree.
  int GetNumberOfItems();
  void *GetItem(int *pIdx);
  void *GetItem() {return this->Item;}  
private:
};



//----------------------------------------------------------------------------
// This is for storing the vtkTree's traversal levels.
// I could have used vtkTreeLeaf which has a built in list, but that
// would have been too confusing.
//----------------------------------------------------------------------------
class VTK_EXPORT vtkTreeCons
{
public:
  vtkTreeCons() {this->Element = NULL; this->Next = NULL;}
  static vtkTreeCons *New() {return new vtkTreeCons;}
  void Delete() { delete this;}

  void SetElement(vtkTreeElement *e) {this->Element = e;}
  vtkTreeElement *GetElement() {return this->Element;}
  void SetNext(vtkTreeCons *c) {this->Next = c;}
  vtkTreeCons *GetNext() {return this->Next;}
  
private:
  vtkTreeElement *Element;
  vtkTreeCons *Next;
};


//ETX - end tcl exclude




#endif


