/*=========================================================================

  Program:   Visualization Library
  Module:    TransC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTransformCollection - maintain a list of transforms
// .SECTION Description
// vlTransformCollection is an object that creates and manipulates lists of
// objects of type vlTransform. See also vlCollection and subclasses.

#ifndef __vlTransformCollection_hh
#define __vlTransformCollection_hh

#include "Collect.hh"
#include "Trans.hh"

class vlTransformCollection : public vlCollection
{
public:
  char *GetClassName() {return "vlTransformCollection";};

  void AddItem(vlTransform *);
  void RemoveItem(vlTransform *);
  int IsItemPresent(vlTransform *);
  vlTransform *GetNextItem();
};

// Description:
// Add a Transform to the list.
inline void vlTransformCollection::AddItem(vlTransform *t) 
{
  this->vlCollection::AddItem((vlObject *)t);
}

// Description:
// Remove a Transform from the list.
inline void vlTransformCollection::RemoveItem(vlTransform *t) 
{
  this->vlCollection::RemoveItem((vlObject *)t);
}

// Description:
// Determine whether a particular Transform is present. Returns its position
// in the list.
inline int vlTransformCollection::IsItemPresent(vlTransform *t) 
{
  return this->vlCollection::IsItemPresent((vlObject *)t);
}

// Description:
// Get the next Transform in the list.
inline vlTransform *vlTransformCollection::GetNextItem() 
{ 
  return (vlTransform *)(this->vlCollection::GetNextItem());
}

#endif
