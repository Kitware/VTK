/*=========================================================================
      </ProxyProperty>

  Program:   Visualization Toolkit
  Module:    vtkPartitionBalancer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPartitionBalancer.h"

#include "vtkCommunicator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <numeric>
#include <vector>

vtkStandardNewMacro(vtkPartitionBalancer);
vtkCxxSetObjectMacro(vtkPartitionBalancer, Controller, vtkMultiProcessController);

namespace
{
//------------------------------------------------------------------------------
void ShallowCopy(vtkPartitionedDataSet* inputPDS, vtkPartitionedDataSet* outputPDS,
  int numberOfNonNullPartitionsInInput, int offset = 0)
{
  for (int outPartitionId = 0, inPartitionId = 0; outPartitionId < numberOfNonNullPartitionsInInput;
       ++inPartitionId, ++outPartitionId)
  {
    vtkDataObject* inputDO = inputPDS->GetPartitionAsDataObject(inPartitionId);

    while (!inputDO)
    {
      ++inPartitionId;
      inputDO = inputPDS->GetPartitionAsDataObject(inPartitionId);
    }

    outputPDS->SetPartition(outPartitionId + offset, inputDO);
  }
}
} // anonymous namespace

//----------------------------------------------------------------------------
vtkPartitionBalancer::vtkPartitionBalancer()
  : Controller(nullptr)
  , Mode(vtkPartitionBalancer::Squash)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPartitionBalancer::~vtkPartitionBalancer()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkPartitionBalancer::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPartitionedDataSet* inputPDS = vtkPartitionedDataSet::GetData(inputVector[0], 0);
  vtkPartitionedDataSet* outputPDS = vtkPartitionedDataSet::GetData(outputVector, 0);

  int numberOfNonNullPartitionsInInput = 0;

  for (unsigned int partitionId = 0; partitionId < inputPDS->GetNumberOfPartitions(); ++partitionId)
  {
    numberOfNonNullPartitionsInInput +=
      (inputPDS->GetPartitionAsDataObject(partitionId) != nullptr) ? 1 : 0;
  }

  if (!this->Controller)
  {
    outputPDS->ShallowCopy(inputPDS);
    outputPDS->RemoveNullPartitions();
    return 1;
  }

  std::vector<int> recvBuf(this->Controller->GetNumberOfProcesses());

  this->Controller->AllGather(&numberOfNonNullPartitionsInInput, recvBuf.data(), 1);

  if (this->Mode == vtkPartitionBalancer::Expand)
  {
    const int localProcessId = this->Controller->GetLocalProcessId();
    const int numberOfPartitions = std::accumulate(recvBuf.begin(), recvBuf.end(), 0);
    const int offset = std::accumulate(recvBuf.begin(), recvBuf.begin() + localProcessId, 0);

    outputPDS->SetNumberOfPartitions(numberOfPartitions);
    ShallowCopy(inputPDS, outputPDS, numberOfNonNullPartitionsInInput, offset);
  }
  else if (this->Mode == vtkPartitionBalancer::Squash)
  {
    const int numberOfPartitions = *std::max_element(recvBuf.begin(), recvBuf.end());

    outputPDS->SetNumberOfPartitions(numberOfPartitions);
    ShallowCopy(inputPDS, outputPDS, numberOfNonNullPartitionsInInput);
  }
  else
  {
    vtkErrorMacro(<< "Unknown mode: " << this->Mode);
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPartitionBalancer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  switch (this->Mode)
  {
    case vtkPartitionBalancer::Expand:
      os << indent << "Mode: Expand" << endl;
      break;
    case vtkPartitionBalancer::Squash:
      os << indent << "Mode: Squash" << endl;
      break;
    default:
      os << indent << "Mode: Wrong value" << endl;
      break;
  }
}
