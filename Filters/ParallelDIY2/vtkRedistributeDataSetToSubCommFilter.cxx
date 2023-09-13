// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRedistributeDataSetToSubCommFilter.h"

#include "vtkDIYAggregateDataSetFilter.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkProcessGroup.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include "cassert"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRedistributeDataSetToSubCommFilter);
vtkCxxSetObjectMacro(vtkRedistributeDataSetToSubCommFilter, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkRedistributeDataSetToSubCommFilter, SubGroup, vtkProcessGroup);
//------------------------------------------------------------------------------
vtkRedistributeDataSetToSubCommFilter::vtkRedistributeDataSetToSubCommFilter()
  : Controller(nullptr)
  , SubGroup(nullptr)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}
//------------------------------------------------------------------------------
vtkRedistributeDataSetToSubCommFilter::~vtkRedistributeDataSetToSubCommFilter()
{
  this->SetController(nullptr);
  this->SetSubGroup(nullptr);
}

//------------------------------------------------------------------------------
int vtkRedistributeDataSetToSubCommFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkRedistributeDataSetToSubCommFilter::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    int wholeExtent[6];
    inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    // Overwrite the whole extent if there's an input whole extent is set. This is needed
    // for distributed structured data.
    outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);
  }

  // We assume that whoever sets up the input handles  partitioned data properly.
  // For structured data, this means setting up WHOLE_EXTENT as above. For
  // unstructured data, nothing special is required
  outputInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//------------------------------------------------------------------------------
int vtkRedistributeDataSetToSubCommFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  auto outInfo = outputVector->GetInformationObject(0);

  if (vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    if (!vtkPartitionedDataSet::SafeDownCast(outputDO))
    {
      auto output = vtkPartitionedDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    if (!vtkMultiBlockDataSet::SafeDownCast(outputDO))
    {
      auto output = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (vtkPartitionedDataSetCollection::SafeDownCast(inputDO) ||
    (vtkMultiBlockDataSet::SafeDownCast(inputDO) != nullptr))
  {
    if (!vtkPartitionedDataSetCollection::SafeDownCast(outputDO))
    {
      auto output = vtkPartitionedDataSetCollection::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (vtkUnstructuredGrid::SafeDownCast(outputDO) == nullptr)
  {
    auto output = vtkUnstructuredGrid::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->FastDelete();
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkRedistributeDataSetToSubCommFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);

  if (inputDO == nullptr)
  {
    std::cout << "ERROR! inputDO in RequestData is null!\n";
    return -1;
  }

  const int nProcs = Controller->GetNumberOfProcesses();
  const int myRank = Controller->GetLocalProcessId();

  const int nTargetProcs = SubGroup->GetNumberOfProcessIds();

  // redistribute onto all procs then aggregate onto target number of procs
  vtkNew<vtkRedistributeDataSetFilter> rds;
  vtkNew<vtkDIYAggregateDataSetFilter> aggregator;
  rds->SetController(Controller);
  rds->SetInputDataObject(inputDO);
  rds->SetNumberOfPartitions(-1);

  aggregator->SetNumberOfTargetProcesses(nTargetProcs);
  aggregator->SetInputConnection(rds->GetOutputPort());
  aggregator->Update();

  // get output
  vtkSmartPointer<vtkPartitionedDataSet> apds =
    vtkPartitionedDataSet::SafeDownCast(aggregator->GetOutputDataObject(0));
  vtkSmartPointer<vtkMultiBlockDataSet> ambds =
    vtkMultiBlockDataSet::SafeDownCast(aggregator->GetOutputDataObject(0));
  vtkSmartPointer<vtkPartitionedDataSetCollection> apdsc =
    vtkPartitionedDataSetCollection::SafeDownCast(aggregator->GetOutputDataObject(0));
  vtkSmartPointer<vtkUnstructuredGrid> aug =
    vtkUnstructuredGrid::SafeDownCast(aggregator->GetOutputDataObject(0));

  // figure out which procs have data on them
  std::vector<vtkIdType> pointCount(nProcs, 0);
  vtkIdType numPoints;

  if (apds)
  {
    numPoints = apds->GetNumberOfPoints();
  }
  else if (ambds)
  {
    numPoints = ambds->GetNumberOfPoints();
  }
  else if (apdsc)
  {
    numPoints = apdsc->GetNumberOfPoints();
  }
  else if (aug)
  {
    numPoints = aug->GetNumberOfPoints();
  }

  Controller->AllGather(&numPoints, pointCount.data(), 1);

  std::vector<vtkIdType> procsWithData;
  for (vtkIdType i = 0; i < nProcs; ++i)
  {
    if (pointCount[i] != 0)
    {
      procsWithData.push_back(i);
    }
  }

  // create a map from aggregated data to SubGroup
  std::vector<int> preFilledWriterRanks;
  std::vector<int> unFilledWriterRanks;
  std::vector<int> moveReadyDataRanks;
  std::map<int, int> aggregatedToWriterRank;
  for (auto p : procsWithData)
  {
    int groupLoc = SubGroup->FindProcessId(p);
    if (groupLoc != -1)
    {
      preFilledWriterRanks.push_back(p);
    }
    else
    {
      moveReadyDataRanks.push_back(p);
    }
  }

  for (int i = 0; i < SubGroup->GetNumberOfProcessIds(); ++i)
  {
    int proc = SubGroup->GetProcessId(i);
    auto loc = std::find(preFilledWriterRanks.begin(), preFilledWriterRanks.end(), proc);
    if (loc == std::end(preFilledWriterRanks))
    {
      unFilledWriterRanks.push_back(proc);
    }
  }

  assert(unFilledWriterRanks.size() == moveReadyDataRanks.size());

  for (int i = 0; i < unFilledWriterRanks.size(); ++i)
  {
    aggregatedToWriterRank[unFilledWriterRanks[i]] = moveReadyDataRanks[i];
  }

  // aggregatedToWriterRank has the mapping for what data needs to be moved where.
  for (auto const& m : aggregatedToWriterRank)
  {
    int recvRank = m.first;
    int sendRank = m.second;
    int sendChannel = 90991 + recvRank;
    if (myRank == recvRank)
    {
      vtkSmartPointer<vtkDataObject> recvBuffer =
        Controller->ReceiveDataObject(sendRank, sendChannel);
      if (recvBuffer)
      {
        outputDO->ShallowCopy(recvBuffer);
      }
    }
    if (myRank == sendRank)
    {
      if (apds)
        Controller->Send(apds, recvRank, sendChannel);
      else if (ambds)
        Controller->Send(ambds, recvRank, sendChannel);
      else if (apdsc)
        Controller->Send(apdsc, recvRank, sendChannel);
      else if (aug)
        Controller->Send(aug, recvRank, sendChannel);
    }
  }

  // copy the data that doesn't need to be communicated onto the output dataset
  for (auto& r : preFilledWriterRanks)
  {
    if (myRank == r)
    {
      if (apds)
        outputDO->ShallowCopy(apds);
      else if (ambds)
        outputDO->ShallowCopy(ambds);
      else if (apdsc)
        outputDO->ShallowCopy(apdsc);
      else if (aug)
        outputDO->ShallowCopy(aug);
    }
  }

  // check that we pulled this off
  // int groupLoc = SubGroup->FindProcessId(myRank);
  // if(groupLoc != -1)
  // {
  //   assert(outputDO->GetNumberOfPoints() > 0);
  // }
  // else
  // {
  //   assert(outputDO->GetNumberOfPoints() == 0);
  // }

  return 1;
}

//------------------------------------------------------------------------------
void vtkRedistributeDataSetToSubCommFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "SubGroup: " << this->SubGroup << endl;
}
VTK_ABI_NAMESPACE_END
