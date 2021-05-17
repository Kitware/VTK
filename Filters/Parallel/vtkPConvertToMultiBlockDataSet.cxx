/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPConvertToMultiBlockDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPConvertToMultiBlockDataSet.h"

#include "vtkCommunicator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"

vtkObjectFactoryNewMacro(vtkPConvertToMultiBlockDataSet);
vtkCxxSetObjectMacro(vtkPConvertToMultiBlockDataSet, Controller, vtkMultiProcessController)
  //----------------------------------------------------------------------------
  vtkPConvertToMultiBlockDataSet::vtkPConvertToMultiBlockDataSet()
  : Controller(nullptr)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPConvertToMultiBlockDataSet::~vtkPConvertToMultiBlockDataSet()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkPConvertToMultiBlockDataSet::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPartitionedDataSetCollection> input;
  if (auto pdc = vtkPartitionedDataSetCollection::GetData(inputVector[0], 0))
  {
    input = pdc;
  }
  else if (auto pd = vtkPartitionedDataSet::GetData(inputVector[0], 0))
  {
    input = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    input->SetPartitionedDataSet(0, pd);
  }

  const int numRanks = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  if (numRanks == 1 || input == nullptr || input->GetNumberOfPartitionedDataSets() == 0)
  {
    // nothing to do.
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  auto output = vtkMultiBlockDataSet::GetData(outputVector, 0);

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
  return this->Execute(clone, output) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkPConvertToMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
