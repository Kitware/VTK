// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActorCollection.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkActorCollection);

void vtkActorCollection::ApplyProperties(vtkProperty* p)
{
  vtkActor* actor;

  if (p == nullptr)
  {
    return;
  }

  vtkCollectionSimpleIterator ait;
  for (this->InitTraversal(ait); (actor = this->GetNextActor(ait));)
  {
    actor->GetProperty()->DeepCopy(p);
  }
}

//------------------------------------------------------------------------------
void vtkActorCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
