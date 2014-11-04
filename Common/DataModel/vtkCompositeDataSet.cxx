/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataSet.h"

#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkInformationKeyMacro(vtkCompositeDataSet, NAME, String);
vtkInformationKeyMacro(vtkCompositeDataSet, CURRENT_PROCESS_CAN_LOAD_BLOCK, Integer);

//----------------------------------------------------------------------------
vtkCompositeDataSet::vtkCompositeDataSet()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataSet::~vtkCompositeDataSet()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSet::GetData(vtkInformation* info)
{
  return info? vtkCompositeDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSet::GetData(vtkInformationVector* v,
                                                  int i)
{
  return vtkCompositeDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::ShallowCopy(vtkDataObject* src)
{
  if (src == this)
    {
    return;
    }

  this->Superclass::ShallowCopy(src);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::DeepCopy(vtkDataObject* src)
{
  if (src == this)
    {
    return;
    }

  this->Superclass::DeepCopy(src);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::Initialize()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
unsigned long vtkCompositeDataSet::GetActualMemorySize()
{
  unsigned long memSize = 0;
  vtkCompositeDataIterator* iter = this->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    memSize += dobj->GetActualMemorySize();
    }
  iter->Delete();
  return memSize;
}

//----------------------------------------------------------------------------
vtkIdType vtkCompositeDataSet::GetNumberOfPoints()
{
  vtkIdType numPts = 0;
  vtkCompositeDataIterator* iter = this->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      numPts += ds->GetNumberOfPoints();
      }
    }
  iter->Delete();
  return numPts;
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
