/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImpFuncC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkImplicitFunctionCollection - maintain a list of implicit functions
// .SECTION Description
// vtkImplicitFunctionCollection is an object that creates and manipulates
// lists of objects of type vtkImplicitFunction. See also vtkCollection and 
// subclasses.

#ifndef __vtkImplicitFunctionCollection_hh
#define __vtkImplicitFunctionCollection_hh

#include "Collect.hh"
#include "ImpFunc.hh"

class vtkImplicitFunctionCollection : public vtkCollection
{
public:
  char *GetClassName() {return "vtkImplicitFunctionCollection";};

  void AddItem(vtkImplicitFunction *);
  void RemoveItem(vtkImplicitFunction *);
  int IsItemPresent(vtkImplicitFunction *);
  vtkImplicitFunction *GetNextItem();
};

// Description:
// Add a implicit functionl to the list.
inline void vtkImplicitFunctionCollection::AddItem(vtkImplicitFunction *f) 
{
  this->vtkCollection::AddItem((vtkObject *)f);
}

// Description:
// Remove a implicit function from the list.
inline void vtkImplicitFunctionCollection::RemoveItem(vtkImplicitFunction *f) 
{
  this->vtkCollection::RemoveItem((vtkObject *)f);
}

// Description:
// Determine whether a particular implicit function is present. Returns its position
// in the list.
inline int vtkImplicitFunctionCollection::IsItemPresent(vtkImplicitFunction *f) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)f);
}

// Description:
// Get the next implicit function in the list.
inline vtkImplicitFunction *vtkImplicitFunctionCollection::GetNextItem() 
{ 
  return (vtkImplicitFunction *)(this->vtkCollection::GetNextItem());
}

#endif
