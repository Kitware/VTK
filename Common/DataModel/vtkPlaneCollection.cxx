// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPlaneCollection.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPlaneCollection);

void vtkPlaneCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkPlane* vtkPlaneCollection::GetNextPlane(vtkCollectionSimpleIterator& cookie)
{
  return static_cast<vtkPlane*>(this->GetNextItemAsObject(cookie));
}
VTK_ABI_NAMESPACE_END
