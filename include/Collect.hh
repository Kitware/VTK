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

 private:
  vlCollectionElement *Top;
  vlCollectionElement *Bottom;

 public:
  vlCollection();
  void AddItem(vlObject *);
  void RemoveItem(vlObject *);
  int  IsItemPresent(vlObject *);
  int  GetNumberOfItems();
  vlObject *GetItem(int num);
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlCollection";};
};

#endif





