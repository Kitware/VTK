/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockDataSet.h"

#include "vtkMultiBlockDataIterator.h"
#include "vtkMultiBlockDataSetInternal.h"
#include "vtkMultiBlockDataVisitor.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiBlockDataSet, "1.2");
vtkStandardNewMacro(vtkMultiBlockDataSet);

//----------------------------------------------------------------------------
vtkMultiBlockDataSet::vtkMultiBlockDataSet()
{
  this->Internal = new vtkMultiBlockDataSetInternal;
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet::~vtkMultiBlockDataSet()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::AddDataSet(vtkDataObject* data)
{
  if (data)
    {
    this->Internal->DataSets.push_back(data);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkMultiBlockDataSet::NewIterator()
{
  vtkMultiBlockDataIterator* iter = vtkMultiBlockDataIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//----------------------------------------------------------------------------
vtkCompositeDataVisitor* vtkMultiBlockDataSet::NewVisitor()
{
  vtkMultiBlockDataVisitor* vis = vtkMultiBlockDataVisitor::New();
  vtkMultiBlockDataIterator* it = 
    vtkMultiBlockDataIterator::SafeDownCast(this->NewIterator());
  vis->SetDataIterator(it);
  it->Delete();
  return vis;
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::Initialize()
{
  this->Superclass::Initialize();
  this->Internal->DataSets.clear();
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

