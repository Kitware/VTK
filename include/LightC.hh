/*=========================================================================

  Program:   Visualization Library
  Module:    LightC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlLightCollection - a list of lights
// .SECTION Description
// vlLightCollection represents and provides methods to manipulate list of
// lights (i.e., vlLight and subclasses). The list is unsorted and duplicate
// entries are not prevented.

#ifndef __vlLightC_hh
#define __vlLightC_hh

#include "Collect.hh"
#include "Light.hh"

class vlLightCollection : public vlCollection
{
 public:
  char *GetClassName() {return "vlLightCollection";};

  void AddItem(vlLight *a);
  void RemoveItem(vlLight *a);
  int IsItemPresent(vlLight *a);
  vlLight *GetNextItem();
};

// Description:
// Add an light to the list.
inline void vlLightCollection::AddItem(vlLight *a) 
{
  this->vlCollection::AddItem((vlObject *)a);
}

// Description:
// Remove an light from the list.
inline void vlLightCollection::RemoveItem(vlLight *a) 
{
  this->vlCollection::RemoveItem((vlObject *)a);
}

// Description:
// Determine whether a particular light is present. Returns its position
// in the list.
inline int vlLightCollection::IsItemPresent(vlLight *a) 
{
  return this->vlCollection::IsItemPresent((vlObject *)a);
}

// Description:
// Get the next light in the list.
inline vlLight *vlLightCollection::GetNextItem() 
{ 
  return (vlLight *)(this->vlCollection::GetNextItem());
}

#endif

