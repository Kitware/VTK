/*=========================================================================

  Program:   Visualization Library
  Module:    DataSetC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataSetCollection - maintain an unordered list of dataset objects
// .SECTION Description
// vlDataSetCollection is an object that creates and manipulates lists of
// datasets. See also vlCollection and subclasses.

#ifndef __vlDataSetCollection_hh
#define __vlDataSetCollection_hh

#include "Object.hh"
#include "DataSet.hh"

class vlDataSetCollectionElement
{
 public:
  vlDataSetCollectionElement():Item(NULL),Next(NULL) {};
  vlDataSet *Item;
  vlDataSetCollectionElement *Next;
};

class vlDataSetCollection : public vlObject
{
public:
  vlDataSetCollection();
  ~vlDataSetCollection();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlDataSetCollection";};

  void AddItem(vlDataSet *);
  void RemoveItem(vlDataSet *);
  int IsItemPresent(vlDataSet *);
  int GetNumberOfItems();
  void InitTraversal();
  vlDataSet *GetNextItem();

protected:
  int NumberOfItems;
  vlDataSetCollectionElement *Top;
  vlDataSetCollectionElement *Bottom;
  vlDataSetCollectionElement *Current;

};

// Description:
// Initialize the traversal of the collection. This means the data pointer
// is set at the beginning of the list.
inline void vlDataSetCollection::InitTraversal()
{
  this->Current = this->Top;
}

// Description:
// Get the next item in the collection. NULL is returned if the collection
// is exhausted.
inline vlDataSet *vlDataSetCollection::GetNextItem()
{
  vlDataSetCollectionElement *elem=this->Current;

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
