/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StrPtsC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredPointsCollection - maintain a list of structured points data objects
// .SECTION Description
// vtkStructuredPointsCollection is an object that creates and manipulates lists of
// structured points datasets. See also vtkCollection and subclasses.

#ifndef __vtkStructuredPointsCollection_hh
#define __vtkStructuredPointsCollection_hh

#include "Collect.hh"
#include "StrPts.hh"

class vtkStructuredPointsCollection : public vtkCollection
{
public:
  char *GetClassName() {return "vtkStructuredPointsCollection";};

  void AddItem(vtkStructuredPoints *);
  void RemoveItem(vtkStructuredPoints *);
  int IsItemPresent(vtkStructuredPoints *);
  vtkStructuredPoints *GetNextItem();
};

// Description:
// Add an StructuredPoints to the list.
inline void vtkStructuredPointsCollection::AddItem(vtkStructuredPoints *ds) 
{
  this->vtkCollection::AddItem((vtkObject *)ds);
}

// Description:
// Remove an StructuredPoints from the list.
inline void vtkStructuredPointsCollection::RemoveItem(vtkStructuredPoints *ds) 
{
  this->vtkCollection::RemoveItem((vtkObject *)ds);
}

// Description:
// Determine whether a particular StructuredPoints is present. 
// Returns its position in the list.
inline int vtkStructuredPointsCollection::IsItemPresent(vtkStructuredPoints *ds) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)ds);
}


// Description:
// Get the next item in the collection. NULL is returned if the collection
// is exhausted.
inline vtkStructuredPoints *vtkStructuredPointsCollection::GetNextItem()
{
 return (vtkStructuredPoints *)(this->vtkCollection::GetNextItem());
}

#endif
