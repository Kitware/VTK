/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLightCollection.h"

#include "vtkObjectFactory.h"
#include "vtkLight.h"

#include <cmath>

vtkStandardNewMacro(vtkLightCollection);

// Add a light to the list.
void vtkLightCollection::AddItem(vtkLight *a)
{
  this->vtkCollection::AddItem(a);
}

// Get the next light in the list. NULL is returned when the collection is
// exhausted.
vtkLight *vtkLightCollection::GetNextItem()
{
  return static_cast<vtkLight *>(this->GetNextItemAsObject());
}

vtkLight *vtkLightCollection::GetNextLight(
  vtkCollectionSimpleIterator &cookie)
{
  return static_cast<vtkLight *>(this->GetNextItemAsObject(cookie));
}

//----------------------------------------------------------------------------
void vtkLightCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
