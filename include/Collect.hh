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
// .NAME vlCollection - create and manipulate lists of objects
// .SECTION Description
// vlCollection is a general object for creating and manipulating lists
// of objects. vlCollection also serves as a base class for lists of
// specific types of objects.

#ifndef __vlCollection_hh
#define __vlCollection_hh

#include "Object.hh"

class vlCollectionElement
{
 public:
  vlObject *Item;
  vlCollectionElement *Next;
};

class vlCollection : public vlObject
{
 public:
  int NumberOfItems;
  vlCollection();
  void AddItem(vlObject *);
  void RemoveItem(vlObject *);
  int  IsItemPresent(vlObject *);
  int  GetNumberOfItems();
  vlObject *GetItem(int num);
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlCollection";};

 private:
  vlCollectionElement *Top;
  vlCollectionElement *Bottom;

};

#endif





