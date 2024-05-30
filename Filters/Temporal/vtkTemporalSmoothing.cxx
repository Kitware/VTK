// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTemporalSmoothing.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataArrayMeta.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <functional>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTemporalSmoothing);

namespace
{

//------------------------------------------------------------------------------
inline int FindTimeIndex(double target, const std::vector<double>& timeSteps)
{
  // dichotomy search
  int left = 0;
  int right = static_cast<int>(timeSteps.size()) - 1;
  while (left < right)
  {
    int idx = left + (right - left) / 2;
    if (target == timeSteps[idx])
    {
      return idx;
    }
    else if (target > timeSteps[idx])
    {
      left = idx + 1;
    }
    else
    {
      right = idx - 1;
    }
  }

  return timeSteps[left] <= target ? left : left - 1;
}

//------------------------------------------------------------------------------
struct AccumulateUniformSum
{
  template <typename InArrayT, typename OutArrayT>
  void operator()(InArrayT* inArray, OutArrayT* outArray) const
  {
    // These share APIType:
    using T = vtk::GetAPIType<InArrayT>;

    const auto in = vtk::DataArrayValueRange(inArray);
    auto out = vtk::DataArrayValueRange(outArray);

    std::transform(in.cbegin(), in.cend(), out.cbegin(), out.begin(), std::plus<T>{});
  }
};

//------------------------------------------------------------------------------
struct FinishAverage
{
  template <typename ArrayT>
  void operator()(ArrayT* array, unsigned int numSamples) const
  {
    auto range = vtk::DataArrayValueRange(array);
    using RefT = typename decltype(range)::ReferenceType;
    for (RefT ref : range)
    {
      ref /= numSamples;
    }
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
struct vtkTemporalSmoothingInternals
{
  std::vector<double> InputTimeSteps;
  unsigned int TemporalWindowWidth = 0;
  vtkSmartPointer<vtkDataObject> Cache;
  unsigned int RequestedTimeIndex;
  unsigned int StartTimeIndex;
  unsigned int EndTimeIndex;
  unsigned int CurrentTimeIndex;
  double AvailableTimeRange[2];
  bool Executing = false;
  bool FirstStep = false;
};

//------------------------------------------------------------------------------
vtkTemporalSmoothing::vtkTemporalSmoothing()
{
  this->Internals = std::make_shared<vtkTemporalSmoothingInternals>();
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TemporalWindowHalfWidth: " << this->TemporalWindowHalfWidth << endl;
}

//------------------------------------------------------------------------------
int vtkTemporalSmoothing::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  if (!input)
  {
    return 0;
  }

  vtkSmartPointer<vtkDataObject> newOutput;

  if (!output || !output->IsA(input->GetClassName()))
  {
    newOutput.TakeReference(input->NewInstance());
    this->Internals->Cache.TakeReference(input->NewInstance());
  }

  if (newOutput)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalSmoothing::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  using vtkSDDP = vtkStreamingDemandDrivenPipeline;

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->Internals->TemporalWindowWidth = 2 * this->TemporalWindowHalfWidth + 1;

  int inNumAvailableTimeSteps = inInfo->Length(vtkSDDP::TIME_STEPS());
  this->Internals->InputTimeSteps.resize(inNumAvailableTimeSteps);
  inInfo->Get(vtkSDDP::TIME_STEPS(), this->Internals->InputTimeSteps.data());

  if (this->Internals->InputTimeSteps.empty())
  {
    vtkWarningMacro("Filter input is not temporal.");
    outInfo->Remove(vtkSDDP::TIME_STEPS());
    outInfo->Remove(vtkSDDP::TIME_RANGE());
    return 1;
  }

  if (this->Internals->InputTimeSteps.size() < this->Internals->TemporalWindowWidth)
  {
    vtkWarningMacro("Requested time window is larger than available time steps");
    outInfo->Remove(vtkSDDP::TIME_STEPS());
    outInfo->Remove(vtkSDDP::TIME_RANGE());
    return 1;
  }

  // Available time steps are clipped on each side
  // to only allow requests on time steps where the full time window fits.
  auto firstAvailableTime = this->Internals->InputTimeSteps.begin() + this->TemporalWindowHalfWidth;
  auto lastAvailableTime = this->Internals->InputTimeSteps.end() - this->TemporalWindowHalfWidth;
  this->Internals->AvailableTimeRange[0] = *firstAvailableTime;
  this->Internals->AvailableTimeRange[1] = *lastAvailableTime;

  int outNumAvailableTimeSteps = inNumAvailableTimeSteps - (2 * this->TemporalWindowHalfWidth);
  outInfo->Set(vtkSDDP::TIME_STEPS(), &*firstAvailableTime, outNumAvailableTimeSteps);
  outInfo->Set(vtkSDDP::TIME_RANGE(), this->Internals->AvailableTimeRange, 2);

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalSmoothing::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  using vtkSDDP = vtkStreamingDemandDrivenPipeline;

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  double nextTimeStep = -1;

  // Initialize
  if (!this->Internals->Executing)
  {
    double requestedTimeStep = -1.;
    if (outInfo->Has(vtkSDDP::UPDATE_TIME_STEP()))
    {
      requestedTimeStep = outInfo->Get(vtkSDDP::UPDATE_TIME_STEP());
    }
    else
    {
      vtkWarningMacro("No update time step requested, defaulting to first available time step.");
      requestedTimeStep = this->Internals->AvailableTimeRange[0];
    }

    // Clamp requested time step to available values
    if (requestedTimeStep < this->Internals->AvailableTimeRange[0])
    {
      vtkWarningMacro(
        "Requested time step out of available range. Using first available time step instead.");
      requestedTimeStep = this->Internals->AvailableTimeRange[0];
    }
    else if (requestedTimeStep > this->Internals->AvailableTimeRange[1])
    {
      vtkWarningMacro(
        "Requested time step out of available range. Using last available time step instead.");
      requestedTimeStep = this->Internals->AvailableTimeRange[1];
    }

    unsigned int requestedTimeIndex =
      FindTimeIndex(requestedTimeStep, this->Internals->InputTimeSteps);

    this->Internals->RequestedTimeIndex = requestedTimeIndex;
    this->Internals->StartTimeIndex = requestedTimeIndex - this->TemporalWindowHalfWidth;
    this->Internals->EndTimeIndex = requestedTimeIndex + this->TemporalWindowHalfWidth;
    this->Internals->CurrentTimeIndex = requestedTimeIndex;
    this->Internals->Executing = true;
    this->Internals->FirstStep = true;

    nextTimeStep = this->Internals->InputTimeSteps[requestedTimeIndex];
  }
  else
  {
    if (this->Internals->CurrentTimeIndex == this->Internals->RequestedTimeIndex)
    {
      // Skip the requested time step since it has been accounted for during initialization
      this->Internals->CurrentTimeIndex++;
    }
    nextTimeStep = this->Internals->InputTimeSteps[this->Internals->CurrentTimeIndex];
  }

  inInfo->Set(vtkSDDP::UPDATE_TIME_STEP(), nextTimeStep);

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalSmoothing::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inputInformation = inputVector[0]->GetInformationObject(0);
  vtkDataObject* inputData = vtkDataObject::GetData(inputInformation);

  vtkInformation* outputInformation = outputVector->GetInformationObject(0);
  vtkDataObject* outputData = vtkDataObject::GetData(outputInformation);

  // Validate parameters
  if (this->Internals->InputTimeSteps.empty())
  {
    vtkErrorMacro("No time steps in input data!");
    return 0;
  }

  if (this->Internals->InputTimeSteps.size() < this->Internals->TemporalWindowWidth)
  {
    vtkErrorMacro("Requested time window is larger than available time steps");
    return 0;
  }

  if (this->Internals->FirstStep)
  {
    // Initialize cache
    this->Internals->Cache->Initialize();
    this->Initialize(inputData, outputData, this->Internals->Cache);

    // Start processing temporal window
    this->Internals->FirstStep = false;
    this->Internals->CurrentTimeIndex = this->Internals->StartTimeIndex;
  }
  else
  {
    // Accumulate values
    this->AccumulateSum(inputData, this->Internals->Cache);
    this->Internals->CurrentTimeIndex++;
  }

  // Finalize
  if (this->Internals->CurrentTimeIndex > this->Internals->EndTimeIndex)
  {
    outputData->DeepCopy(this->Internals->Cache);

    // Finalize average over cache
    this->PostExecute(inputData, outputData);

    // We're done
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->Internals->Executing = false;
  }
  else
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::Initialize(
  vtkDataObject* input, vtkDataObject* output, vtkDataObject* cache)
{
  if (input->IsA("vtkDataSet"))
  {
    this->Initialize(vtkDataSet::SafeDownCast(input), vtkDataSet::SafeDownCast(output),
      vtkDataSet::SafeDownCast(cache));
    return;
  }

  if (input->IsA("vtkGraph"))
  {
    this->Initialize(
      vtkGraph::SafeDownCast(input), vtkGraph::SafeDownCast(output), vtkGraph::SafeDownCast(cache));
    return;
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    this->Initialize(vtkCompositeDataSet::SafeDownCast(input),
      vtkCompositeDataSet::SafeDownCast(output), vtkCompositeDataSet::SafeDownCast(cache));
    return;
  }

  vtkWarningMacro(<< "Unsupported input type: " << input->GetClassName());
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::Initialize(vtkDataSet* input, vtkDataSet* output, vtkDataSet* cache)
{
  output->CopyStructure(input);
  cache->CopyStructure(input);
  this->InitializeArrays(input->GetFieldData(), cache->GetFieldData());
  this->InitializeArrays(input->GetPointData(), cache->GetPointData());
  this->InitializeArrays(input->GetCellData(), cache->GetCellData());
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::Initialize(vtkGraph* input, vtkGraph* output, vtkGraph* cache)
{
  output->CopyStructure(input);
  cache->CopyStructure(input);
  this->InitializeArrays(input->GetFieldData(), cache->GetFieldData());
  this->InitializeArrays(input->GetEdgeData(), cache->GetEdgeData());
  this->InitializeArrays(input->GetVertexData(), cache->GetVertexData());
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::Initialize(
  vtkCompositeDataSet* input, vtkCompositeDataSet* output, vtkCompositeDataSet* cache)
{
  output->CopyStructure(input);
  cache->CopyStructure(input);

  vtkSmartPointer<vtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal(); !inputItr->IsDoneWithTraversal(); inputItr->GoToNextItem())
  {
    vtkDataObject* inputObj = inputItr->GetCurrentDataObject();

    vtkSmartPointer<vtkDataObject> outputObj;
    outputObj.TakeReference(inputObj->NewInstance());
    vtkSmartPointer<vtkDataObject> cacheObj;
    cacheObj.TakeReference(inputObj->NewInstance());

    this->Initialize(inputObj, outputObj, cacheObj);
    output->SetDataSet(inputItr, outputObj);
    cache->SetDataSet(inputItr, cacheObj);
  }
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::InitializeArrays(vtkFieldData* inFd, vtkFieldData* outFd)
{
  // Because we need to do mathematical operations, we require all arrays we
  // process to be numeric data (i.e. a vtkDataArray).  We also handle global
  // ids and petigree ids special (we just pass them).  Ideally would just let
  // vtkFieldData or vtkDataSetAttributes handle this for us, but no such method
  // that fits our needs here.  Thus, we pass data a bit differently then other
  // filters.  If I miss something important, it should be added here.

  outFd->Initialize();

  vtkDataSetAttributes* inDsa = vtkDataSetAttributes::SafeDownCast(inFd);
  vtkDataSetAttributes* outDsa = vtkDataSetAttributes::SafeDownCast(outFd);
  if (inDsa)
  {
    vtkDataArray* globalIds = inDsa->GetGlobalIds();
    vtkAbstractArray* pedigreeIds = inDsa->GetPedigreeIds();
    vtkDataArray* processIds = inDsa->GetProcessIds();
    if (globalIds)
    {
      outDsa->SetGlobalIds(globalIds);
    }
    if (pedigreeIds)
    {
      outDsa->SetPedigreeIds(pedigreeIds);
    }
    if (processIds)
    {
      outDsa->SetProcessIds(processIds);
    }
  }

  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }

    vtkDataArray* array = inFd->GetArray(i);
    if (!array)
    {
      continue; // Array is not numeric.
    }

    if (outFd->HasArray(array->GetName()))
    {
      continue; // Array is ids attribute.
    }

    this->InitializeArray(array, outFd);
  }
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::InitializeArray(vtkDataArray* array, vtkFieldData* outFd)
{
  vtkSmartPointer<vtkDataArray> newArray;
  newArray.TakeReference(array->NewInstance());
  newArray->DeepCopy(array);
  outFd->AddArray(newArray);
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::AccumulateSum(vtkDataObject* input, vtkDataObject* output)
{
  if (input->IsA("vtkDataSet"))
  {
    this->AccumulateSum(vtkDataSet::SafeDownCast(input), vtkDataSet::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkGraph"))
  {
    this->AccumulateSum(vtkGraph::SafeDownCast(input), vtkGraph::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    this->AccumulateSum(
      vtkCompositeDataSet::SafeDownCast(input), vtkCompositeDataSet::SafeDownCast(output));
  }
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::AccumulateSum(vtkDataSet* input, vtkDataSet* output)
{
  this->AccumulateArrays(input->GetFieldData(), output->GetFieldData());
  this->AccumulateArrays(input->GetPointData(), output->GetPointData());
  this->AccumulateArrays(input->GetCellData(), output->GetCellData());
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::AccumulateSum(vtkGraph* input, vtkGraph* output)
{
  this->AccumulateArrays(input->GetFieldData(), output->GetFieldData());
  this->AccumulateArrays(input->GetVertexData(), output->GetVertexData());
  this->AccumulateArrays(input->GetEdgeData(), output->GetEdgeData());
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::AccumulateSum(vtkCompositeDataSet* input, vtkCompositeDataSet* output)
{
  vtkSmartPointer<vtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal(); !inputItr->IsDoneWithTraversal(); inputItr->GoToNextItem())
  {
    vtkDataObject* inputObj = inputItr->GetCurrentDataObject();
    vtkDataObject* outputObj = output->GetDataSet(inputItr);

    this->AccumulateSum(inputObj, outputObj);
  }
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::AccumulateArrays(vtkFieldData* inFd, vtkFieldData* outFd)
{
  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }

    vtkDataArray* inArray = inFd->GetArray(i);
    if (!inArray)
    {
      continue;
    }

    vtkDataArray* outArray = outFd->GetArray(i);
    if (outArray)
    {
      using Dispatcher = vtkArrayDispatch::Dispatch2SameValueType;
      AccumulateUniformSum worker;
      if (!Dispatcher::Execute(inArray, outArray, worker))
      { // Fallback to slow path:
        worker(inArray, outArray);
      }
    }

    // Alert change in data.
    outArray->DataChanged();
  }
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::PostExecute(vtkDataObject* input, vtkDataObject* output)
{
  if (input->IsA("vtkDataSet"))
  {
    this->PostExecute(vtkDataSet::SafeDownCast(input), vtkDataSet::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkGraph"))
  {
    this->PostExecute(vtkGraph::SafeDownCast(input), vtkGraph::SafeDownCast(output));
    return;
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    this->PostExecute(
      vtkCompositeDataSet::SafeDownCast(input), vtkCompositeDataSet::SafeDownCast(output));
  }
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::PostExecute(vtkDataSet* input, vtkDataSet* output)
{
  this->FinishArrays(input->GetFieldData(), output->GetFieldData());
  this->FinishArrays(input->GetPointData(), output->GetPointData());
  this->FinishArrays(input->GetCellData(), output->GetCellData());
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::PostExecute(vtkGraph* input, vtkGraph* output)
{
  this->FinishArrays(input->GetFieldData(), output->GetFieldData());
  this->FinishArrays(input->GetVertexData(), output->GetVertexData());
  this->FinishArrays(input->GetEdgeData(), output->GetEdgeData());
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::PostExecute(vtkCompositeDataSet* input, vtkCompositeDataSet* output)
{
  vtkSmartPointer<vtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal(); !inputItr->IsDoneWithTraversal(); inputItr->GoToNextItem())
  {
    vtkDataObject* inputObj = inputItr->GetCurrentDataObject();
    vtkDataObject* outputObj = output->GetDataSet(inputItr);

    this->PostExecute(inputObj, outputObj);
  }
}

//------------------------------------------------------------------------------
void vtkTemporalSmoothing::FinishArrays(vtkFieldData* inFd, vtkFieldData* outFd)
{
  using Dispatcher = vtkArrayDispatch::Dispatch;

  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }

    vtkDataArray* inArray = inFd->GetArray(i);
    if (!inArray)
    {
      continue;
    }

    vtkDataArray* outArray = outFd->GetArray(i);
    if (outArray)
    {
      FinishAverage worker;
      if (!Dispatcher::Execute(outArray, worker, this->Internals->TemporalWindowWidth))
      { // fall-back to slow path
        worker(outArray, this->Internals->TemporalWindowWidth);
      }
    }
  }
}
VTK_ABI_NAMESPACE_END
