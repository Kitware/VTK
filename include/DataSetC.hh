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
// .NAME vlDataSetCollection - maintain a list of dataset objects
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
  vlDataSet *Item;
  vlDataSetCollectionElement *Next;
};

class vlDataSetCollection : public vlObject
{

 public:
  vlDataSetCollection();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlDataSetCollection";};

  void AddItem(vlDataSet *);
  void RemoveItem(vlDataSet *);
  int IsItemPresent(vlDataSet *);
  int GetNumberOfItems();
  vlDataSet *GetItem(int num);

 private:
  int NumberOfItems;
  vlDataSetCollectionElement *Top;
  vlDataSetCollectionElement *Bottom;

};

#endif
