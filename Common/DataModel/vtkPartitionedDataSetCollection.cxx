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

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"

vtkStandardNewMacro(vtkPartitionedDataSetCollection);
//----------------------------------------------------------------------------
vtkPartitionedDataSetCollection::vtkPartitionedDataSetCollection() {}

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollection::~vtkPartitionedDataSetCollection() {}

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollection* vtkPartitionedDataSetCollection::GetData(vtkInformation* info)
{
  return info ? vtkPartitionedDataSetCollection::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollection* vtkPartitionedDataSetCollection::GetData(
  vtkInformationVector* v, int i)
{
  return vtkPartitionedDataSetCollection::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::SetNumberOfPartitionedDataSets(unsigned int numDataSets)
{
  this->Superclass::SetNumberOfChildren(numDataSets);
}

//----------------------------------------------------------------------------
unsigned int vtkPartitionedDataSetCollection::GetNumberOfPartitionedDataSets()
{
  return this->Superclass::GetNumberOfChildren();
}

//----------------------------------------------------------------------------
vtkPartitionedDataSet* vtkPartitionedDataSetCollection::GetPartitionedDataSet(unsigned int idx)
{
  return vtkPartitionedDataSet::SafeDownCast(this->Superclass::GetChild(idx));
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::SetPartitionedDataSet(
  unsigned int idx, vtkPartitionedDataSet* dataset)
{
  this->Superclass::SetChild(idx, dataset);
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::RemovePartitionedDataSet(unsigned int idx)
{
  this->Superclass::RemoveChild(idx);
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
