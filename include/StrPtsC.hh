/*=========================================================================

  Program:   Visualization Library
  Module:    StrPtsC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPointsCollection - maintain a list of structured points data objects
// .SECTION Description
// vlStructuredPointsCollection is an object that creates and manipulates lists of
// structured points datasets. See also vlCollection and subclasses.

#ifndef __vlStructuredPointsCollection_hh
#define __vlStructuredPointsCollection_hh

#include "Collect.hh"
#include "StrPts.hh"

class vlStructuredPointsCollection : public vlCollection
{
public:
  char *GetClassName() {return "vlStructuredPointsCollection";};

  void AddItem(vlStructuredPoints *);
  void RemoveItem(vlStructuredPoints *);
  int IsItemPresent(vlStructuredPoints *);
  vlStructuredPoints *GetNextItem();
};

// Description:
// Add an StructuredPoints to the list.
inline void vlStructuredPointsCollection::AddItem(vlStructuredPoints *ds) 
{
  this->vlCollection::AddItem((vlObject *)ds);
}

// Description:
// Remove an StructuredPoints from the list.
inline void vlStructuredPointsCollection::RemoveItem(vlStructuredPoints *ds) 
{
  this->vlCollection::RemoveItem((vlObject *)ds);
}

// Description:
// Determine whether a particular StructuredPoints is present. 
// Returns its position in the list.
inline int vlStructuredPointsCollection::IsItemPresent(vlStructuredPoints *ds) 
{
  return this->vlCollection::IsItemPresent((vlObject *)ds);
}


// Description:
// Get the next item in the collection. NULL is returned if the collection
// is exhausted.
inline vlStructuredPoints *vlStructuredPointsCollection::GetNextItem()
{
 return (vlStructuredPoints *)(this->vlCollection::GetNextItem());
}

#endif
