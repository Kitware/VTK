/*=========================================================================

  Program:   Visualization Library
  Module:    PolyDatC.hh
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
  void AddItem(vlPolyData *);
  void RemoveItem(vlPolyData *);
  int IsItemPresent(vlPolyData *);
  int GetNumberOfItems();
  vlPolyData *GetItem(int num);
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlPolyDataCollection";};

private:
  int NumberOfItems;
  vlPolyDataCollectionElement *Top;
  vlPolyDataCollectionElement *Bottom;

};

#endif
