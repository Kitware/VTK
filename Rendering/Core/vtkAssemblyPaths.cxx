// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAssemblyPaths.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAssemblyPaths);

void vtkAssemblyPaths::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkMTimeType vtkAssemblyPaths::GetMTime()
{
  vtkMTimeType mtime = this->vtkCollection::GetMTime();

  vtkAssemblyPath* path;
  for (this->InitTraversal(); (path = this->GetNextItem());)
  {
    vtkMTimeType pathMTime = path->GetMTime();
    if (pathMTime > mtime)
    {
      mtime = pathMTime;
    }
  }
  return mtime;
}
VTK_ABI_NAMESPACE_END
