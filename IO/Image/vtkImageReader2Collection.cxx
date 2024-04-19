// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageReader2Collection.h"

#include "vtkImageReader2.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageReader2Collection);

void vtkImageReader2Collection::AddItem(vtkImageReader2* f)
{
  this->vtkCollection::AddItem(f);
}

vtkImageReader2* vtkImageReader2Collection::GetNextItem()
{
  return static_cast<vtkImageReader2*>(this->GetNextItemAsObject());
}

vtkImageReader2* vtkImageReader2Collection::GetNextImageReader2(vtkCollectionSimpleIterator& cookie)
{
  return static_cast<vtkImageReader2*>(this->GetNextItemAsObject(cookie));
}

//------------------------------------------------------------------------------
void vtkImageReader2Collection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
