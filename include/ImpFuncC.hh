/*=========================================================================

  Program:   Visualization Library
  Module:    ImpFuncC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlImplicitFunctionCollection - maintain a list of implicit functions
// .SECTION Description
// vlImplicitFunctionCollection is an object that creates and manipulates
// lists of objects of type vlImplicitFunction. See also vlCollection and 
// subclasses.

#ifndef __vlImplicitFunctionCollection_hh
#define __vlImplicitFunctionCollection_hh

#include "Collect.hh"
#include "ImpFunc.hh"

class vlImplicitFunctionCollection : public vlCollection
{
public:
  char *GetClassName() {return "vlImplicitFunctionCollection";};

  void AddItem(vlImplicitFunction *);
  void RemoveItem(vlImplicitFunction *);
  int IsItemPresent(vlImplicitFunction *);
  vlImplicitFunction *GetNextItem();
};

// Description:
// Add a implicit functionl to the list.
inline void vlImplicitFunctionCollection::AddItem(vlImplicitFunction *f) 
{
  this->vlCollection::AddItem((vlObject *)f);
}

// Description:
// Remove a implicit function from the list.
inline void vlImplicitFunctionCollection::RemoveItem(vlImplicitFunction *f) 
{
  this->vlCollection::RemoveItem((vlObject *)f);
}

// Description:
// Determine whether a particular implicit function is present. Returns its position
// in the list.
inline int vlImplicitFunctionCollection::IsItemPresent(vlImplicitFunction *f) 
{
  return this->vlCollection::IsItemPresent((vlObject *)f);
}

// Description:
// Get the next implicit function in the list.
inline vlImplicitFunction *vlImplicitFunctionCollection::GetNextItem() 
{ 
  return (vlImplicitFunction *)(this->vlCollection::GetNextItem());
}

#endif
