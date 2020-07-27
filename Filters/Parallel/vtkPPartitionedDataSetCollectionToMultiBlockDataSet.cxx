/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPartitionedDataSetCollectionToMultiBlockDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPPartitionedDataSetCollectionToMultiBlockDataSet.h"

#include "vtkCommunicator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"

#include <vector>

vtkObjectFactoryNewMacro(vtkPPartitionedDataSetCollectionToMultiBlockDataSet);
vtkCxxSetObjectMacro(
  vtkPPartitionedDataSetCollectionToMultiBlockDataSet, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkPPartitionedDataSetCollectionToMultiBlockDataSet::
  vtkPPartitionedDataSetCollectionToMultiBlockDataSet()
  : Controller(nullptr)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPPartitionedDataSetCollectionToMultiBlockDataSet::
  ~vtkPPartitionedDataSetCollectionToMultiBlockDataSet()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkPPartitionedDataSetCollectionToMultiBlockDataSet::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPartitionedDataSetCollection> input =
    vtkPartitionedDataSetCollection::GetData(inputVector[0], 0);
  auto output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1 &&
    input->GetNumberOfPartitionedDataSets() > 0)
  {
    // ensure that we have exactly the same number of partitions on all ranks.
    vtkNew<vtkPartitionedDataSetCollection> clone;
    clone->ShallowCopy(input);

    const auto count = input->GetNumberOfPartitionedDataSets();
    std::vector<unsigned int> piece_counts(count);
    for (unsigned int cc = 0; cc < count; ++cc)
    {
      piece_counts[cc] = clone->GetPartitionedDataSet(cc)
        ? clone->GetPartitionedDataSet(cc)->GetNumberOfPartitions()
        : 0;
    }

    std::vector<unsigned int> result(piece_counts.size());
    this->Controller->AllReduce(
      &piece_counts[0], &result[0], static_cast<vtkIdType>(count), vtkCommunicator::MAX_OP);

    for (unsigned int cc = 0; cc < count; ++cc)
    {
      if (result[cc] > 0)
      {
        if (clone->GetPartitionedDataSet(cc) == nullptr)
        {
          clone->SetPartitionedDataSet(cc, vtkNew<vtkPartitionedDataSet>{});
        }
        clone->GetPartitionedDataSet(cc)->SetNumberOfPartitions(result[cc]);
      }
    }
    input = clone;
  }

  return this->Execute(input, output) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkPPartitionedDataSetCollectionToMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
