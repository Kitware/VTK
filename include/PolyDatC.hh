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
  vlPolyData *Item;
  vlPolyDataCollectionElement *Next;
};

class vlPolyDataCollection : public vlObject
{
public:
  vlPolyDataCollection();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlPolyDataCollection";};

  void AddItem(vlPolyData *);
  void RemoveItem(vlPolyData *);
  int IsItemPresent(vlPolyData *);
  int GetNumberOfItems();
  vlPolyData *GetItem(int num);

private:
  int NumberOfItems;
  vlPolyDataCollectionElement *Top;
  vlPolyDataCollectionElement *Bottom;

};

#endif
