/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LightC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkLightCollection - a list of lights
// .SECTION Description
// vtkLightCollection represents and provides methods to manipulate list of
// lights (i.e., vtkLight and subclasses). The list is unsorted and duplicate
// entries are not prevented.

#ifndef __vtkLightC_hh
#define __vtkLightC_hh

#include "Collect.hh"
#include "Light.hh"

class vtkLightCollection : public vtkCollection
{
 public:
  char *GetClassName() {return "vtkLightCollection";};

  void AddItem(vtkLight *a);
  void RemoveItem(vtkLight *a);
  int IsItemPresent(vtkLight *a);
  vtkLight *GetNextItem();
};

// Description:
// Add an light to the list.
inline void vtkLightCollection::AddItem(vtkLight *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove an light from the list.
inline void vtkLightCollection::RemoveItem(vtkLight *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular light is present. Returns its position
// in the list.
inline int vtkLightCollection::IsItemPresent(vtkLight *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next light in the list.
inline vtkLight *vtkLightCollection::GetNextItem() 
{ 
  return (vtkLight *)(this->vtkCollection::GetNextItem());
}

#endif

