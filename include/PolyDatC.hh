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

#include "Collect.hh"
#include "PolyData.hh"

class vlPolyDataCollection : public vlCollection
{
public:
  char *GetClassName() {return "vlPolyDataCollection";};

  void AddItem(vlPolyData *);
  void RemoveItem(vlPolyData *);
  int IsItemPresent(vlPolyData *);
  vlPolyData *GetNextItem();
};

// Description:
// Add an PolyData to the list.
inline void vlPolyDataCollection::AddItem(vlPolyData *ds) 
{
  this->vlCollection::AddItem((vlObject *)ds);
}

// Description:
// Remove an PolyData from the list.
inline void vlPolyDataCollection::RemoveItem(vlPolyData *ds) 
{
  this->vlCollection::RemoveItem((vlObject *)ds);
}

// Description:
// Determine whether a particular PolyData is present. Returns its position
// in the list.
inline int vlPolyDataCollection::IsItemPresent(vlPolyData *ds) 
{
  return this->vlCollection::IsItemPresent((vlObject *)ds);
}

// Description:
// Get the next PolyData in the list.
inline vlPolyData *vlPolyDataCollection::GetNextItem() 
{ 
  return (vlPolyData *)(this->vlCollection::GetNextItem());
}

#endif
