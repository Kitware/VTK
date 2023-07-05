// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLightCollection.h"

#include "vtkLight.h"
#include "vtkObjectFactory.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLightCollection);

// Add a light to the bottom of the list.
void vtkLightCollection::AddItem(vtkLight* a)
{
  this->vtkCollection::AddItem(a);
}

// Get the next light in the list. nullptr is returned when the collection is
// exhausted.
vtkLight* vtkLightCollection::GetNextItem()
{
  return static_cast<vtkLight*>(this->GetNextItemAsObject());
}

vtkLight* vtkLightCollection::GetNextLight(vtkCollectionSimpleIterator& cookie)
{
  return static_cast<vtkLight*>(this->GetNextItemAsObject(cookie));
}

//------------------------------------------------------------------------------
void vtkLightCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
