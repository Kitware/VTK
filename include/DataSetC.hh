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

#include "Collect.hh"
#include "DataSet.hh"

class vlDataSetCollection : public vlCollection
{
public:
  char *GetClassName() {return "vlDataSetCollection";};

  void AddItem(vlDataSet *);
  void RemoveItem(vlDataSet *);
  int IsItemPresent(vlDataSet *);
  vlDataSet *GetNextItem();

};

// Description:
// Add an DataSet to the list.
inline void vlDataSetCollection::AddItem(vlDataSet *ds) 
{
  this->vlCollection::AddItem((vlObject *)ds);
}

// Description:
// Remove an DataSet from the list.
inline void vlDataSetCollection::RemoveItem(vlDataSet *ds) 
{
  this->vlCollection::RemoveItem((vlObject *)ds);
}

// Description:
// Determine whether a particular DataSet is present. Returns its position
// in the list.
inline int vlDataSetCollection::IsItemPresent(vlDataSet *ds) 
{
  return this->vlCollection::IsItemPresent((vlObject *)ds);
}

// Description:
// Get the next DataSet in the list.
inline vlDataSet *vlDataSetCollection::GetNextItem() 
{ 
  return (vlDataSet *)(this->vlCollection::GetNextItem());
}

#endif
