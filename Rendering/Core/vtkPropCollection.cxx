// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPropCollection.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPropCollection);

void vtkPropCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkPropCollection::GetNumberOfPaths()
{
  int numPaths = 0;
  vtkProp* aProp;

  vtkCollectionSimpleIterator pit;
  for (this->InitTraversal(pit); (aProp = this->GetNextProp(pit));)
  {
    numPaths += aProp->GetNumberOfPaths();
  }
  return numPaths;
}
VTK_ABI_NAMESPACE_END
