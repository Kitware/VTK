/*=========================================================================

  Program:   Visualization Library
  Module:    DataSetC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#ifndef __vlDataSetCollection_hh
#define __vlDataSetCollection_hh

#include "Object.hh"
#include "DataSet.hh"

class vlDataSetCollectionElement
{
 public:
  vlDataSet *Item;
  vlDataSetCollectionElement *Next;
};

class vlDataSetCollection : public vlObject
{
 public:
  int NumberOfItems;

 private:
  vlDataSetCollectionElement *Top;
  vlDataSetCollectionElement *Bottom;

 public:
  vlDataSetCollection();
  void AddItem(vlDataSet *);
  void RemoveItem(vlDataSet *);
  int  IsItemPresent(vlDataSet *);
  int  GetNumberOfItems();
  vlDataSet *GetItem(int num);
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlDataSetCollection";};
};

#endif
