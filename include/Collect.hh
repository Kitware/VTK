/*=========================================================================

  Program:   Visualization Library
  Module:    Collect.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlCollection - create and manipulate unsorted lists of objects
// .SECTION Description
// vlCollection is a general object for creating and manipulating lists
// of objects. The lists are unsorted and allow duplicate entries. 
// vlCollection also serves as a base class for lists of specific types 
// of objects.

#ifndef __vlCollection_hh
#define __vlCollection_hh

#include "Object.hh"

class vlCollectionElement
{
 public:
  vlCollectionElement():Item(NULL),Next(NULL) {};
  vlObject *Item;
  vlCollectionElement *Next;
};

class vlCollection : public vlObject
{
public:
  vlCollection();
  virtual ~vlCollection();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlCollection";};

  void AddItem(vlObject *);
  void RemoveItem(vlObject *);
  void RemoveAllItems();
  int  IsItemPresent(vlObject *);
  int  GetNumberOfItems();
  void InitTraversal();
  vlObject *GetNextItem();  

protected:
  int NumberOfItems;
  vlCollectionElement *Top;
  vlCollectionElement *Bottom;
  vlCollectionElement *Current;

};

// Description:
// Initialize the traversal of the collection. This means the data pointer
// is set at the beginning of the list.
inline void vlCollection::InitTraversal()
{
  this->Current = this->Top;
}

// Description:
// Get the next item in the collection. NULL is returned if the collection
// is exhausted.
inline vlObject *vlCollection::GetNextItem()
{
  vlCollectionElement *elem=this->Current;

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





