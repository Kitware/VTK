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

vtkCxxRevisionMacro(vtkImageReader2Collection, "1.6");
vtkStandardNewMacro(vtkImageReader2Collection);

void vtkImageReader2Collection::AddItem(vtkImageReader2 *f) 
{
  this->vtkCollection::AddItem((vtkObject *)f);
}

vtkImageReader2 *vtkImageReader2Collection::GetNextItem() 
{ 
  return static_cast<vtkImageReader2*>(this->GetNextItemAsObject());
}

//----------------------------------------------------------------------------
void vtkImageReader2Collection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
