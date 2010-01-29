/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReaderCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataReaderCollection.h"

#include "vtkObjectFactory.h"
#include "vtkAbstractPolyDataReader.h"

vtkCxxRevisionMacro(vtkPolyDataReaderCollection, "1.1");
vtkStandardNewMacro(vtkPolyDataReaderCollection);

void vtkPolyDataReaderCollection::AddItem(vtkAbstractPolyDataReader *f) 
{
  this->vtkCollection::AddItem(f);
}

vtkAbstractPolyDataReader *vtkPolyDataReaderCollection::GetNextItem() 
{ 
  return static_cast<vtkAbstractPolyDataReader*>(this->GetNextItemAsObject());
}

vtkAbstractPolyDataReader *vtkPolyDataReaderCollection::GetNextPolyDataReader(
  vtkCollectionSimpleIterator &cookie) 
{
  return static_cast<vtkAbstractPolyDataReader *>(this->GetNextItemAsObject(cookie));
}

//----------------------------------------------------------------------------
void vtkPolyDataReaderCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
