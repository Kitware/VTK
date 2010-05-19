/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader2Collection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageReader2Collection.h"

#include "vtkObjectFactory.h"
#include "vtkImageReader2.h"

vtkStandardNewMacro(vtkImageReader2Collection);

void vtkImageReader2Collection::AddItem(vtkImageReader2 *f) 
{
  this->vtkCollection::AddItem(f);
}

vtkImageReader2 *vtkImageReader2Collection::GetNextItem() 
{ 
  return static_cast<vtkImageReader2*>(this->GetNextItemAsObject());
}

vtkImageReader2 *vtkImageReader2Collection::GetNextImageReader2(
  vtkCollectionSimpleIterator &cookie) 
{
  return static_cast<vtkImageReader2 *>(this->GetNextItemAsObject(cookie));
}

//----------------------------------------------------------------------------
void vtkImageReader2Collection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
