/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TransC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTransformCollection - maintain a list of transforms
// .SECTION Description
// vtkTransformCollection is an object that creates and manipulates lists of
// objects of type vtkTransform. See also vtkCollection and subclasses.

#ifndef __vtkTransformCollection_hh
#define __vtkTransformCollection_hh

#include "Collect.hh"
#include "Trans.hh"

class vtkTransformCollection : public vtkCollection
{
public:
  char *GetClassName() {return "vtkTransformCollection";};

  void AddItem(vtkTransform *);
  void RemoveItem(vtkTransform *);
  int IsItemPresent(vtkTransform *);
  vtkTransform *GetNextItem();
};

// Description:
// Add a Transform to the list.
inline void vtkTransformCollection::AddItem(vtkTransform *t) 
{
  this->vtkCollection::AddItem((vtkObject *)t);
}

// Description:
// Remove a Transform from the list.
inline void vtkTransformCollection::RemoveItem(vtkTransform *t) 
{
  this->vtkCollection::RemoveItem((vtkObject *)t);
}

// Description:
// Determine whether a particular Transform is present. Returns its position
// in the list.
inline int vtkTransformCollection::IsItemPresent(vtkTransform *t) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)t);
}

// Description:
// Get the next Transform in the list.
inline vtkTransform *vtkTransformCollection::GetNextItem() 
{ 
  return (vtkTransform *)(this->vtkCollection::GetNextItem());
}

#endif
