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

#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkInformationKeyMacro(vtkCompositeDataSet, NAME, String);
vtkInformationKeyMacro(vtkCompositeDataSet, CURRENT_PROCESS_CAN_LOAD_BLOCK, Integer);

//----------------------------------------------------------------------------
vtkCompositeDataSet::vtkCompositeDataSet() = default;

//----------------------------------------------------------------------------
vtkCompositeDataSet::~vtkCompositeDataSet() = default;

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSet::GetData(vtkInformation* info)
{
  return info ? vtkCompositeDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSet::GetData(vtkInformationVector* v, int i)
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
  return this->GetNumberOfElements(vtkDataSet::POINT);
}

//----------------------------------------------------------------------------
vtkIdType vtkCompositeDataSet::GetNumberOfCells()
{
  return this->GetNumberOfElements(vtkDataSet::CELL);
}

//----------------------------------------------------------------------------
vtkIdType vtkCompositeDataSet::GetNumberOfElements(int type)
{
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(this->NewIterator());
  iter->SkipEmptyNodesOn();
  vtkIdType numElements = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    numElements += iter->GetCurrentDataObject()->GetNumberOfElements(type);
  }

  return numElements;
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::GetBounds(double bounds[6])
{
  double bds[6];
  vtkBoundingBox bbox;
  vtkCompositeDataIterator* iter = this->NewIterator();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
    {
      ds->GetBounds(bds);
      bbox.AddBounds(bds);
    }
  }

  bbox.GetBounds(bounds);
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
