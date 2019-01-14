/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedDataSet.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPartitionedDataSet);
//----------------------------------------------------------------------------
vtkPartitionedDataSet::vtkPartitionedDataSet()
{
}

//----------------------------------------------------------------------------
vtkPartitionedDataSet::~vtkPartitionedDataSet()
{
}

//----------------------------------------------------------------------------
vtkPartitionedDataSet* vtkPartitionedDataSet::GetData(vtkInformation* info)
{
  return
    info? vtkPartitionedDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkPartitionedDataSet* vtkPartitionedDataSet::GetData(vtkInformationVector* v,
                                                      int i)
{
  return vtkPartitionedDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSet::SetNumberOfPartitions(unsigned int numPartitions)
{
  this->Superclass::SetNumberOfChildren(numPartitions);
}


//----------------------------------------------------------------------------
unsigned int vtkPartitionedDataSet::GetNumberOfPartitions()
{
  return this->Superclass::GetNumberOfChildren();
}

//----------------------------------------------------------------------------
vtkDataSet* vtkPartitionedDataSet::GetPartition(unsigned int idx)
{
  return vtkDataSet::SafeDownCast(this->GetPartitionAsDataObject(idx));
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPartitionedDataSet::GetPartitionAsDataObject(unsigned int idx)
{
  return this->Superclass::GetChild(idx);
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSet::SetPartition(unsigned int idx, vtkDataObject* partition)
{
  if (partition && partition->IsA("vtkCompositeDataSet"))
  {
    vtkErrorMacro("Partition cannot be a vtkCompositeDataSet.");
    return;
  }

  this->Superclass::SetChild(idx, partition);
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
