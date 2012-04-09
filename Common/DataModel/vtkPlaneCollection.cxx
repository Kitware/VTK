/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlaneCollection.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPlaneCollection);

vtkPlane *vtkPlaneCollection::GetNextPlane(
  vtkCollectionSimpleIterator &cookie) 
{
  return static_cast<vtkPlane *>(this->GetNextItemAsObject(cookie));
}
