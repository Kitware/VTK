/*=========================================================================

  Program:   Visualization Library
  Module:    PolyDatC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolyDataCollection - maintain a list of polygonal data objects
// .SECTION Description
// vlPolyDataCollection is an object that creates and manipulates lists of
// datasets. See also vlDataSetCollection and vlCollection and subclasses.

#ifndef __vlPolyDataCollection_hh
#define __vlPolyDataCollection_hh

#include "Object.hh"
#include "PolyData.hh"

class vlPolyDataCollectionElement
{
 public:
  vlPolyDataCollectionElement():Item(NULL),Next(NULL) {};
  vlPolyData *Item;
  vlPolyDataCollectionElement *Next;
};

class vlPolyDataCollection : public vlObject
{
public:
  vlPolyDataCollection();
  ~vlPolyDataCollection();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlPolyDataCollection";};

  void AddItem(vlPolyData *);
  void RemoveItem(vlPolyData *);
  int IsItemPresent(vlPolyData *);
  int GetNumberOfItems();
  void InitTraversal();
  vlPolyData *GetNextItem();

protected:
  int NumberOfItems;
  vlPolyDataCollectionElement *Top;
  vlPolyDataCollectionElement *Bottom;
  vlPolyDataCollectionElement *Current;

};

// Description:
// Initialize the traversal of the collection. This means the data pointer
// is set at the beginning of the list.
inline void vlPolyDataCollection::InitTraversal()
{
  this->Current = this->Top;
}

// Description:
// Get the next item in the collection. NULL is returned if the collection
// is exhausted.
inline vlPolyData *vlPolyDataCollection::GetNextItem()
{
  vlPolyDataCollectionElement *elem=this->Current;

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
