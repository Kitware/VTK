/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedDataSetCollection.h"

#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"

#include <algorithm>

vtkStandardNewMacro(vtkPartitionedDataSetCollection);
vtkCxxSetObjectMacro(vtkPartitionedDataSetCollection, DataAssembly, vtkDataAssembly);
//------------------------------------------------------------------------------
vtkPartitionedDataSetCollection::vtkPartitionedDataSetCollection()
  : DataAssembly(nullptr)
{
}

//------------------------------------------------------------------------------
vtkPartitionedDataSetCollection::~vtkPartitionedDataSetCollection()
{
  this->SetDataAssembly(nullptr);
}

//------------------------------------------------------------------------------
vtkPartitionedDataSetCollection* vtkPartitionedDataSetCollection::GetData(vtkInformation* info)
{
  return info ? vtkPartitionedDataSetCollection::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkPartitionedDataSetCollection* vtkPartitionedDataSetCollection::GetData(
  vtkInformationVector* v, int i)
{
  return vtkPartitionedDataSetCollection::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::SetNumberOfPartitionedDataSets(unsigned int numDataSets)
{
  this->Superclass::SetNumberOfChildren(numDataSets);
}

//------------------------------------------------------------------------------
unsigned int vtkPartitionedDataSetCollection::GetNumberOfPartitionedDataSets()
{
  return this->Superclass::GetNumberOfChildren();
}

//------------------------------------------------------------------------------
vtkPartitionedDataSet* vtkPartitionedDataSetCollection::GetPartitionedDataSet(unsigned int idx)
{
  return vtkPartitionedDataSet::SafeDownCast(this->Superclass::GetChild(idx));
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::SetPartitionedDataSet(
  unsigned int idx, vtkPartitionedDataSet* dataset)
{
  this->Superclass::SetChild(idx, dataset);
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::RemovePartitionedDataSet(unsigned int idx)
{
  this->Superclass::RemoveChild(idx);
}

//------------------------------------------------------------------------------
vtkMTimeType vtkPartitionedDataSetCollection::GetMTime()
{
  return this->DataAssembly ? std::max(this->Superclass::GetMTime(), this->DataAssembly->GetMTime())
                            : this->Superclass::GetMTime();
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::Initialize()
{
  this->Superclass::Initialize();
  this->SetDataAssembly(nullptr);
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::CopyStructure(vtkCompositeDataSet* input)
{
  this->Superclass::CopyStructure(input);
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(input))
  {
    this->SetDataAssembly(pdc->GetDataAssembly());
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::ShallowCopy(vtkDataObject* src)
{
  this->Superclass::ShallowCopy(src);
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(src))
  {
    this->SetDataAssembly(pdc->GetDataAssembly());
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::DeepCopy(vtkDataObject* src)
{
  this->Superclass::DeepCopy(src);
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(src))
  {
    if (auto srcDA = pdc->GetDataAssembly())
    {
      vtkNew<vtkDataAssembly> destDA;
      destDA->DeepCopy(srcDA);
      this->SetDataAssembly(destDA);
    }
    else
    {
      this->SetDataAssembly(nullptr);
    }
  }
}

//------------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataAssembly: " << this->DataAssembly << endl;
}
