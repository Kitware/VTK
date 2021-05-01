/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGroupTimeStepsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGroupTimeStepsFilter.h"

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>

vtkStandardNewMacro(vtkGroupTimeStepsFilter);
//----------------------------------------------------------------------------
vtkGroupTimeStepsFilter::vtkGroupTimeStepsFilter()
  : UpdateTimeIndex(0)
{
}

//----------------------------------------------------------------------------
vtkGroupTimeStepsFilter::~vtkGroupTimeStepsFilter() = default;

//----------------------------------------------------------------------------
int vtkGroupTimeStepsFilter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  const int outputType = vtkMultiBlockDataSet::SafeDownCast(inputDO) != nullptr
    ? VTK_MULTIBLOCK_DATA_SET
    : VTK_PARTITIONED_DATA_SET_COLLECTION;
  return vtkDataObjectAlgorithm::SetOutputDataObject(
           outputType, outputVector->GetInformationObject(0))
    ? 1
    : 0;
}

//----------------------------------------------------------------------------
int vtkGroupTimeStepsFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inInfo, vtkInformationVector* vtkNotUsed(outInfo))
{
  if (this->TimeSteps.size() > this->UpdateTimeIndex)
  {
    vtkInformation* info = inInfo[0]->GetInformationObject(0);
    info->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->TimeSteps[this->UpdateTimeIndex]);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkGroupTimeStepsFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inInfo, vtkInformationVector* outInfoVec)
{
  this->UpdateTimeIndex = 0;

  auto info = inInfo[0]->GetInformationObject(0);
  int len = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  const double* timeSteps = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  this->TimeSteps.resize(len);
  std::copy(timeSteps, timeSteps + len, this->TimeSteps.begin());

  auto outInfo = outInfoVec->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  return 1;
}

//----------------------------------------------------------------------------
int vtkGroupTimeStepsFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  auto inputDO = vtkDataObject::GetData(inInfo[0], 0);
  auto dataInfo = inputDO->GetInformation();
  auto outputDO = vtkDataObject::GetData(outInfo);

  vtkSmartPointer<vtkDataObject> clone =
    vtkSmartPointer<vtkDataObject>::Take(inputDO->NewInstance());
  clone->ShallowCopy(inputDO);

  if (this->AccumulatedData == nullptr)
  {
    assert(this->UpdateTimeIndex == 0);
    this->AccumulatedData.TakeReference(outputDO->NewInstance());
    this->AccumulatedData->Initialize();
    if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(this->AccumulatedData))
    {
      vtkNew<vtkDataAssembly> assembly;
      assembly->Initialize();
      assembly->SetRootNodeName("TimeSteps");
      pdc->SetDataAssembly(assembly);
    }
  }

  double time = dataInfo->Has(vtkDataObject::DATA_TIME_STEP())
    ? dataInfo->Get(vtkDataObject::DATA_TIME_STEP())
    : 0.0;
  const int timeStep = this->TimeSteps.empty() ? 0 : static_cast<int>(this->UpdateTimeIndex);

  if (auto inputMB = vtkMultiBlockDataSet::SafeDownCast(clone))
  {
    this->AddTimeStep(time, timeStep, inputMB);
  }
  else if (auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(clone))
  {
    this->AddTimeStep(time, timeStep, inputPDC);
  }
  else if (auto inputPDS = vtkPartitionedDataSet::SafeDownCast(clone))
  {
    this->AddTimeStep(time, timeStep, inputPDS);
  }
  else if (auto inputCD = vtkCompositeDataSet::SafeDownCast(clone))
  {
    this->AddTimeStep(time, timeStep, inputCD);
  }
  else
  {
    this->AddTimeStep(time, timeStep, clone);
  }

  if ((++this->UpdateTimeIndex) < this->TimeSteps.size())
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    this->UpdateTimeIndex = 0;
    outputDO->ShallowCopy(this->AccumulatedData);
    this->AccumulatedData = nullptr;
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkGroupTimeStepsFilter::AddTimeStep(
  double vtkNotUsed(time), int timeStep, vtkDataObject* data)
{
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(this->AccumulatedData);
  if (!pdc)
  {
    vtkErrorMacro("Mismatched output type was created. Did data type change between timesteps?");
    return false;
  }

  const auto idx = pdc->GetNumberOfPartitionedDataSets();
  pdc->SetPartition(idx, 0, data);

  const auto name = "timestep" + std::to_string(timeStep);
  auto assembly = pdc->GetDataAssembly();
  auto node = assembly->AddNode(name.c_str());
  assembly->AddDataSetIndex(node, idx);
  pdc->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), name.c_str());
  return true;
}

//----------------------------------------------------------------------------
bool vtkGroupTimeStepsFilter::AddTimeStep(
  double vtkNotUsed(time), int timeStep, vtkPartitionedDataSet* data)
{
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(this->AccumulatedData);
  if (!pdc)
  {
    vtkErrorMacro("Mismatched output type was created. Did data type change between timesteps?");
    return false;
  }

  const auto idx = pdc->GetNumberOfPartitionedDataSets();
  pdc->SetPartitionedDataSet(idx, data);

  const auto name = "timestep" + std::to_string(timeStep);
  auto assembly = pdc->GetDataAssembly();
  auto node = assembly->AddNode(name.c_str());
  assembly->AddDataSetIndex(node, idx);
  pdc->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), name.c_str());
  return true;
}

//----------------------------------------------------------------------------
bool vtkGroupTimeStepsFilter::AddTimeStep(
  double vtkNotUsed(time), int timeStep, vtkPartitionedDataSetCollection* data)
{
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(this->AccumulatedData);
  if (!pdc)
  {
    vtkErrorMacro("Mismatched output type was created. Did data type change between timesteps?");
    return false;
  }

  const auto idx = pdc->GetNumberOfPartitionedDataSets();
  for (unsigned int cc = 0; cc < data->GetNumberOfPartitionedDataSets(); ++cc)
  {
    pdc->SetPartitionedDataSet(idx + cc, data->GetPartitionedDataSet(cc));
    if (data->HasMetaData(cc))
    {
      pdc->GetMetaData(idx + cc)->Copy(data->GetMetaData(cc));
    }
  }

  const auto name = "timestep" + std::to_string(timeStep);
  auto assembly = pdc->GetDataAssembly();
  auto node = assembly->AddNode(name.c_str());
  assembly->AddDataSetIndexRange(
    node, idx, static_cast<int>(data->GetNumberOfPartitionedDataSets()));
  if (auto inputAssembly = data->GetDataAssembly())
  {
    vtkNew<vtkDataAssembly> clone;
    clone->DeepCopy(inputAssembly);
    auto dsIndices = clone->GetDataSetIndices(vtkDataAssembly::GetRootNode());
    std::map<unsigned int, unsigned int> remap;
    for (const auto& val : dsIndices)
    {
      remap[val] = val + idx;
    }
    clone->RemapDataSetIndices(remap, /*remove_unmapped*/ true);
    assembly->AddSubtree(node, clone);
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkGroupTimeStepsFilter::AddTimeStep(
  double vtkNotUsed(time), int timeStep, vtkMultiBlockDataSet* data)
{
  auto mb = vtkMultiBlockDataSet::SafeDownCast(this->AccumulatedData);
  if (!mb)
  {
    vtkErrorMacro("Mismatched output type was created. Did data type change between timesteps?");
    return false;
  }

  const auto idx = mb->GetNumberOfBlocks();
  mb->SetBlock(idx, data);

  const auto name = "timestep" + std::to_string(timeStep);
  mb->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), name.c_str());
  return true;
}

//----------------------------------------------------------------------------
bool vtkGroupTimeStepsFilter::AddTimeStep(double time, int timeStep, vtkCompositeDataSet* data)
{
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(this->AccumulatedData);
  if (!pdc)
  {
    vtkErrorMacro("Mismatched output type was created. Did data type change between timesteps?");
    return false;
  }

  vtkNew<vtkDataAssembly> hierarchy;
  vtkNew<vtkPartitionedDataSetCollection> xformedData;
  if (vtkDataAssemblyUtilities::GenerateHierarchy(data, hierarchy, xformedData))
  {
    this->AddTimeStep(time, timeStep, xformedData);
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkGroupTimeStepsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
