// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAggregateToPartitionedDataSetCollection.h"

#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"

VTK_ABI_NAMESPACE_BEGIN

struct vtkAggregateToPartitionedDataSetCollection::Internals
{
  vtkSmartPointer<vtkPartitionedDataSetCollection> Output;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkAggregateToPartitionedDataSetCollection);

//------------------------------------------------------------------------------
vtkAggregateToPartitionedDataSetCollection::vtkAggregateToPartitionedDataSetCollection()
  : Internal(new Internals)
{
  this->Internal->Output = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
}

//------------------------------------------------------------------------------
vtkAggregateToPartitionedDataSetCollection::~vtkAggregateToPartitionedDataSetCollection() = default;

//------------------------------------------------------------------------------
void vtkAggregateToPartitionedDataSetCollection::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkAggregateToPartitionedDataSetCollection::RequestDataObject(
  vtkDataObject* input)
{
  if (!input)
  {
    return nullptr;
  }
  return vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
}

//------------------------------------------------------------------------------
bool vtkAggregateToPartitionedDataSetCollection::Aggregate(vtkDataObject* input)
{
  if (!this->Internal->Output)
  {
    vtkErrorMacro("Current output is nullptr");
    return false;
  }

  if (!input)
  {
    // don't append anything to the output
    return true;
  }

  // Ensure we do not reuse the same pointer
  auto newPartition = vtkSmartPointer<vtkDataObject>::Take(input->NewInstance());
  newPartition->ShallowCopy(input);

  unsigned int LastPDS = this->Internal->Output->GetNumberOfPartitionedDataSets();
  this->Internal->Output->SetNumberOfPartitionedDataSets(LastPDS + 1);
  this->Internal->Output->SetPartition(LastPDS, 0, newPartition);

  return true;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkAggregateToPartitionedDataSetCollection::GetOutputDataObject()
{
  return this->Internal->Output;
}

//------------------------------------------------------------------------------
void vtkAggregateToPartitionedDataSetCollection::Clear()
{
  this->Internal->Output = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
}

VTK_ABI_NAMESPACE_END
