/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTree.cxx
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
#include "vtkTree.h"




//============================================================================
// Methods for the actual tree object.
// TreeNode and Tree are almost interchangable.

//----------------------------------------------------------------------------
vtkTree::vtkTree()
{
  this->Root = vtkTreeNode::New();
  this->TraversalList = this->Traversal = vtkTreeCons::New();
  this->Traversal->SetElement(this->Root);
  this->TraversalDepth = 0;
}

//----------------------------------------------------------------------------
vtkTree::~vtkTree()
{
  // This deletes the whole tree.
  this->DeleteElement(this->Root);
  this->Root = NULL;

  // Delete the cons explicitly
  this->Traversal = this->TraversalList;
  while (this->Traversal)
    {
    this->TraversalList = this->Traversal->GetNext();
    this->Traversal->Delete();
    this->Traversal = this->TraversalList;
    }
  this->Traversal = NULL;
}

//----------------------------------------------------------------------------
void vtkTree::DeleteElement(vtkTreeElement *element)
{
  void *item;
  vtkTreeElement *tmp, *child;

  item = element->GetItem();
  if (item)
    {
    this->DeleteItem(item);
    }

  // leaves and nodes respond to GetChildList.
  child = element->GetChildList();
  while (child)
    {
    tmp = child;
    child = child->GetNext();  
    this->DeleteElement(tmp);
    }

  element->Delete();
}

//----------------------------------------------------------------------------
// Subclass might actually delete something, but we do have not have
// enough information to delete anything in this superclass.
void vtkTree::DeleteItem(void *item)
{
  item = item;
}

//----------------------------------------------------------------------------
void vtkTree::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkTreeCons *cons;

  this->vtkReferenceCount::PrintSelf(os, indent);
  os << indent << "CurrentLevel: " << this->TraversalDepth << endl;
  os << indent << "CurrentElement: (" 
     << this->Traversal->GetElement() << ")\n";
  os << indent << "CurrentStack: ";
  cons = this->TraversalList;
  while (cons)
    {
    os << "(" << cons->GetElement() << ") ";
    cons = cons->GetNext();
    }
  os << endl;
  this->PrintElement(os, indent, this->Root);
}

//----------------------------------------------------------------------------
void vtkTree::PrintElement(ostream& os, vtkIndent indent, vtkTreeElement *e)
{
  void *item;
  vtkTreeElement *child;

  if (e->GetElementType() == VTK_TREE_NODE)
    {
    os << indent << "Node (" << e << ")";
    }
  else
    {
    os << indent << "Leaf (" << e << ")";
    }

  if (this->Traversal->GetElement() == e)
    {
    os << " --Current-- \n";
    }
  else
    {
    os << "\n";
    }
  
  indent = indent.GetNextIndent();
  item = e->GetItem();
  if (item)
    {
    this->PrintItem(os, indent, item);
    }

  // leaves always return NULL element lists.
  child = e->GetChildList();
  if (child)
    {
    os << indent << "Children: \n";
    indent = indent.GetNextIndent();
    }
  while (child)
    {
    this->PrintElement(os, indent, child);
    child = child->GetNext();
    }
}

//----------------------------------------------------------------------------
// This would be the method that the subclasses would override.
void vtkTree::PrintItem(ostream& os, vtkIndent indent, void *item)
{
  os << indent << "Item (" << item << ")\n";
}


//----------------------------------------------------------------------------
int vtkTree::GetNumberOfChildren()
{
  vtkTreeElement *tmp;
  int count;
  
  tmp = this->Traversal->GetElement()->GetChildList();
  count = 0;
  while (tmp != NULL)
    {
    ++count;
    tmp = tmp->GetNext();
    }
  
  return count;
}

  
//----------------------------------------------------------------------------
int vtkTree::MoveToChild(int idx)
{
  vtkTreeElement *current, *tmp;
  
  if (idx < 0)
    {
    vtkErrorMacro("Bad element index " << idx);
    return this->TraversalDepth;
    }
  
  // find the element we are going to move to
  current = this->Traversal->GetElement();
  tmp = current->GetChildList();
  while (1)
    {
    if (tmp == NULL)
      {
      vtkErrorMacro("Bad element index");
      return this->TraversalDepth;
      }
    if (idx == 0)
      {
      vtkTreeCons *cons = vtkTreeCons::New();
      cons->SetElement(tmp);
      this->Traversal->SetNext(cons);
      this->Traversal = cons;
      return ++this->TraversalDepth;
      }
    --idx;
    tmp = tmp->GetNext();
    }
}

  
//----------------------------------------------------------------------------
int vtkTree::MoveToParent()
{
  if (this->TraversalDepth == 0)
    {
    vtkErrorMacro("Cannot move up: Already at root.");
    return 0;
    }
  
  this->MoveToLevel(this->TraversalDepth - 1);
  
  return this->TraversalDepth;
}

  
//----------------------------------------------------------------------------
void vtkTree::MoveToLevel(int level)
{
  vtkTreeCons *tmp, *tmp2;
  
  if (level < 0 || level > this->TraversalDepth)
    {
    vtkErrorMacro("Can only move up levels: Level must be between 0 and " 
		  << this->TraversalDepth);
    return;
    }
  
  // Find the correct level in the traversal list.
  this->TraversalDepth = level;
  tmp = this->TraversalList;
  while (level > 0)
    {
    // sanity check
    if ( tmp == NULL)
      {
      vtkErrorMacro("I must be going insane!!!!");
      return;
      }
    tmp = tmp->GetNext();
    --level;
    }
  this->Traversal = tmp;
  
  // Delete the rest of the list.
  tmp = tmp->GetNext();
  while (tmp)
    {
    tmp2 = tmp;
    tmp = tmp->GetNext();
    tmp2->Delete();
    }
  this->Traversal->SetNext(NULL);
}

  
//----------------------------------------------------------------------------
// It might be possible to change a leaf to a node on the fly, if you wanted 
// to add a node to a leaf, but for now leaves cannot be extended.
int vtkTree::AddNewNode()
{
  vtkTreeNode *newNode;
  vtkTreeElement *current;

  current = this->Traversal->GetElement();
  // make sure the current node is not a leaf
  if (current->GetElementType() == VTK_TREE_LEAF)
    {
    vtkErrorMacro("CurrentElement is a leaf: Cannot add a node to a leaf.");
    return -1;
    }
    
  newNode = vtkTreeNode::New();
  return current->AddChild(newNode);
}



  
//----------------------------------------------------------------------------
void vtkTree::SetItem(void *item)
{
  void *oldItem;
  vtkTreeElement *e;

  e = this->Traversal->GetElement();
  oldItem = e->GetItem();
  if (oldItem)
    {
    this->DeleteItem(oldItem);
    }
  e->SetItem(item);
}
  
//----------------------------------------------------------------------------
void *vtkTree::GetItem()
{
  vtkTreeElement *e;

  e = this->Traversal->GetElement();
  return e->GetItem();
}
  
//----------------------------------------------------------------------------
int vtkTree::GetNumberOfItems()
{
  vtkTreeElement *current;

  current = this->Traversal->GetElement();
  return current->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void *vtkTree::GetItem(int idx)
{
  vtkTreeElement *current;

  current = this->Traversal->GetElement();
  return current->GetItem(&idx);
}

//----------------------------------------------------------------------------
int vtkTree::AddNewLeaf()
{
  vtkTreeLeaf *newLeaf;
  vtkTreeElement *current;

  current = this->Traversal->GetElement();
  
  // make sure the current node is not a leaf
  if (current->GetElementType() == VTK_TREE_LEAF)
    {
    vtkErrorMacro("CurrentElement is a leaf: Cannot add a leaf to a leaf.");
    return -1;
    }
    
  newLeaf = vtkTreeLeaf::New();
  return current->AddChild(newLeaf);
}


//----------------------------------------------------------------------------
// meant for internal use
vtkTreeElement *vtkTree::PopChild(int idx)
{
  vtkTreeElement *eOld, *e, *parent;
  
  if (idx < 0)
    {
    vtkErrorMacro("Bad index " << idx);
    return NULL;
    }
  
  parent = this->Traversal->GetElement();
  if (parent->GetElementType() != VTK_TREE_NODE)
    {
    vtkErrorMacro("Current element is a leaf.");
    return NULL;
    
    }
  
  e = parent->GetChildList();
  if (idx == 0)
    {
    if (e == NULL)
      {
      vtkErrorMacro("Bad index");
      return NULL;
      }
    parent->SetChildList(e->GetNext());
    e->SetNext(NULL);
    return e;
    }
  
  while (idx > 0)
    {
    if (e == NULL)
      {
      vtkErrorMacro("Bad index");
      return NULL;
      }
    eOld = e;
    e = e->GetNext();
    --idx;
    }
  
  eOld->SetNext(e->GetNext());
  e->SetNext(NULL);
  return e;
}

  
//----------------------------------------------------------------------------
// meant for internal use: Returns the index of the element.
int vtkTree::AddChild(vtkTreeElement *e)
{
  vtkTreeElement *end, *parent, *next;
  int idx;
  
  if (e == NULL)
    {
    vtkErrorMacro("Tried to add NULL element.");
    return -1;
    }

  parent = this->Traversal->GetElement();
  if (parent->GetElementType() != VTK_TREE_NODE)
    {
    vtkErrorMacro("Current element is a leaf.");
    return -1;
    }
  
  // just in case.
  e->SetNext(NULL);
  
  end = parent->GetChildList();
  if (end == NULL)
    {
    parent->SetChildList(e);
    return 0;
    }
  
  idx = 1;
  next = end->GetNext();
  while (next)
    {
    ++idx;
    end = next;
    next = end->GetNext();
    }
  end->SetNext(e);
  
  return idx;
}


//----------------------------------------------------------------------------
int vtkTree::CutChild(int idx, vtkTree *destination)
{
  vtkTreeElement *e;
  
  e = this->PopChild(idx);
  if (e != NULL)
    {
    if (destination != NULL)
      {
      return destination->AddChild(e);
      }
    else
      {
      this->DeleteElement(e);
      }
    }
  
  return -1;
}

    
  
//----------------------------------------------------------------------------
int vtkTree::PasteChild(vtkTree *source, int idx)
{
  vtkTreeElement *e;
  
  e = source->PopChild(idx);
  if (e)
    {
    return this->AddChild(e);
    }
  
  return -1;
}


  




//============================================================================
// vtkTreeElement: Superclass of the tree helper objects (node and leaf).

//----------------------------------------------------------------------------
vtkTreeElement::vtkTreeElement()
{
  this->Next = NULL;
  this->Item = NULL;
}

//----------------------------------------------------------------------------
vtkTreeElement::~vtkTreeElement()
{
}

//----------------------------------------------------------------------------
void vtkTreeElement::Delete()
{
  delete this;
}

//----------------------------------------------------------------------------
// Access to list (lateral neighbors)
vtkTreeElement *vtkTreeElement::GetNext()
{
  return this->Next;
}

//----------------------------------------------------------------------------
// Access to list (lateral neighbors)
void vtkTreeElement::SetNext(vtkTreeElement *element)
{
  this->Next = element;
}





//============================================================================
// vtkTreeNode helper.

//----------------------------------------------------------------------------
vtkTreeNode::vtkTreeNode()
{
  this->ChildList = NULL;
}

//----------------------------------------------------------------------------
vtkTreeNode::~vtkTreeNode()
{
}

//----------------------------------------------------------------------------
int vtkTreeNode::GetNumberOfItems()
{
  int count;
  vtkTreeElement *e;
  
  count = 0;
  if (this->Item) 
    {
    ++count;
    }
  e = this->ChildList;
  while (e != NULL)
    {
    count += e->GetNumberOfItems();
    e = e->GetNext();
    }

  return count;
}


//----------------------------------------------------------------------------
void *vtkTreeNode::GetItem(int *pIdx)
{
  vtkTreeElement *e;
  void *tmp;
  
  if (*pIdx < 0)
    {
    return NULL;
    }
  if (*pIdx == 0 && this->Item != NULL)
    {
    return this->Item;
    }
  if (this->Item != NULL)
    {
    --*pIdx;
    }

  e = this->ChildList;
  tmp = NULL;
  while (tmp == NULL)
    {
    if ( e == NULL)
      {
      return NULL;
      }
    tmp = e->GetItem(pIdx);
    e = e->GetNext();
    }
  
  return tmp;
}

//----------------------------------------------------------------------------
int vtkTreeNode::AddChild(vtkTreeElement *element)
{
  vtkTreeElement *tmp, *next;
  int idx;
  
  if (this->ChildList == NULL)
    {
    this->ChildList = element;
    return 0;
    }
  
  // find the end of the list
  idx = 1;
  tmp = this->ChildList;
  next = tmp->GetNext();
  while (next != NULL)
    {
    tmp = next;
    next = tmp->GetNext();
    ++idx;
    }

  tmp->SetNext(element);
  return idx;
}


//============================================================================
// helper vtkTreeLeaf object.

//----------------------------------------------------------------------------
vtkTreeLeaf::vtkTreeLeaf()
{
  this->Item = NULL;
}

//----------------------------------------------------------------------------
vtkTreeLeaf::~vtkTreeLeaf()
{
  this->Item = NULL;
}

//----------------------------------------------------------------------------
// Now leafs always have an item, but for sanity ...
int vtkTreeLeaf::GetNumberOfItems()
{
  if (this->Item != NULL)
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void *vtkTreeLeaf::GetItem(int *pIdx)
{
  // Now leafs always have an item, but for sanity ...
  if (this->Item == NULL)
    {
    return NULL;
    }
  
  if (*pIdx == 0)
    {
    return this->Item;
    }
  --(*pIdx);
  return NULL;
}


