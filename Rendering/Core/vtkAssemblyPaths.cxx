/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPaths.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAssemblyPaths.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkAssemblyPaths);

vtkMTimeType vtkAssemblyPaths::GetMTime()
{
  vtkMTimeType mtime = this->vtkCollection::GetMTime();

  vtkAssemblyPath *path;
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
