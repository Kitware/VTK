/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Collect.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkCollection - create and manipulate unsorted lists of objects
// .SECTION Description
// vtkCollection is a general object for creating and manipulating lists
// of objects. The lists are unsorted and allow duplicate entries. 
// vtkCollection also serves as a base class for lists of specific types 
// of objects.

#ifndef __vtkCollection_hh
#define __vtkCollection_hh

#include "Object.hh"

//BTX - begin tcl exclude
class vtkCollectionElement //;prevents pick-up by man page generator
{
 public:
  vtkCollectionElement():Item(NULL),Next(NULL) {};
  vtkObject *Item;
  vtkCollectionElement *Next;
};
//ETX end tcl exclude

class vtkCollection : public vtkObject
{
public:
  vtkCollection();
  virtual ~vtkCollection();
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkCollection";};

  void AddItem(vtkObject *);
  void RemoveItem(vtkObject *);
  void RemoveAllItems();
  int  IsItemPresent(vtkObject *);
  int  GetNumberOfItems();
  void InitTraversal();
  vtkObject *GetNextItem();  

protected:
  int NumberOfItems;
  vtkCollectionElement *Top;
  vtkCollectionElement *Bottom;
  vtkCollectionElement *Current;

};

// Description:
// Initialize the traversal of the collection. This means the data pointer
// is set at the beginning of the list.
inline void vtkCollection::InitTraversal()
{
  this->Current = this->Top;
}

// Description:
// Get the next item in the collection. NULL is returned if the collection
// is exhausted.
inline vtkObject *vtkCollection::GetNextItem()
{
  vtkCollectionElement *elem=this->Current;

  if ( elem != NULL )
    {
    this->Current = elem->Next;
    return elem->Item;
    }
  else
    {
    return NULL;
    }
}

#endif





