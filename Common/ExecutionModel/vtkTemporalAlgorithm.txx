// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTemporalAlgorithm.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//=============================================================================
VTK_ABI_NAMESPACE_BEGIN

namespace
{
inline int FindTimeIndex(double target, const std::vector<double>& timeSteps)
{
  // dichotomy search
  int left = 0, right = static_cast<int>(timeSteps.size()) - 1;
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
} // anonymous namespace

//------------------------------------------------------------------------------
template <class AlgorithmT>
vtkTemporalAlgorithm<AlgorithmT>::vtkTemporalAlgorithm()
{
  this->ProcessedTimeSteps->SetName(this->TimeStepsArrayName());
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
int vtkTemporalAlgorithm<AlgorithmT>::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  assert(this->GetNumberOfInputPorts() > 0);
  using vtkSDDP = vtkStreamingDemandDrivenPipeline;
  int retVal = this->Superclass::RequestInformation(request, inputVector, outputVector);

  this->CurrentTimeIndex = 0;

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->IntegrateFullTimeSeries && !inInfo->Has(vtkSDDP::NO_PRIOR_TEMPORAL_ACCESS()))
  {
    for (int outPort = 0; outPort < outputVector->GetNumberOfInformationObjects(); ++outPort)
    {
      vtkInformation* outInfo = outputVector->GetInformationObject(outPort);
      if (this->IntegrateFullTimeSeries)
      {
        outInfo->Remove(vtkSDDP::TIME_STEPS());
        outInfo->Remove(vtkSDDP::TIME_RANGE());
      }
    }
  }

  if (inInfo->Has(vtkSDDP::TIME_STEPS()))
  {
    this->InputTimeSteps.resize(inInfo->Length(vtkSDDP::TIME_STEPS()));
    inInfo->Get(vtkSDDP::TIME_STEPS(), this->InputTimeSteps.data());
  }
  else
  {
    this->InputTimeSteps.clear();
  }

  this->TerminationTimeIndex = static_cast<int>(this->InputTimeSteps.size()) - 1;

  return retVal;
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
int vtkTemporalAlgorithm<AlgorithmT>::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  using vtkSDDP = vtkStreamingDemandDrivenPipeline;
  int retVal = this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  if (vtkInformation* inInfo = inputVector[0]->GetInformationObject(0))
  {
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::NO_PRIOR_TEMPORAL_ACCESS()))
    {
      if (inInfo->Get(vtkSDDP::NO_PRIOR_TEMPORAL_ACCESS()) ==
        vtkSDDP::NO_PRIOR_TEMPORAL_ACCESS_RESET)
      {
        this->ProcessedTimeSteps->Initialize();
      }
      this->NoPriorTimeStepAccess = true;
      return retVal;
    }
  }

  if (this->InputTimeSteps.empty())
  {
    return retVal;
  }

  vtkInformation* outInfo =
    outputVector->GetInformationObject(request->Get(vtkSDDP::FROM_OUTPUT_PORT()));

  if (outInfo->Has(vtkSDDP::UPDATE_TIME_STEP()))
  {
    int newTerminationTimeIndex = static_cast<int>(this->IntegrateFullTimeSeries
        ? this->InputTimeSteps.size() - 1
        : ::FindTimeIndex(outInfo->Get(vtkSDDP::UPDATE_TIME_STEP()), this->InputTimeSteps));

    if (newTerminationTimeIndex < this->CurrentTimeIndex)
    {
      this->CurrentTimeIndex = 0;
    }
    this->TerminationTimeIndex = newTerminationTimeIndex;
  }

  if (vtkInformation* inInfo = inputVector[0]->GetInformationObject(0))
  {
    inInfo->Set(vtkSDDP::UPDATE_TIME_STEP(), this->InputTimeSteps[this->CurrentTimeIndex]);
  }

  return retVal;
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
int vtkTemporalAlgorithm<AlgorithmT>::RequestUpdateTime(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
int vtkTemporalAlgorithm<AlgorithmT>::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  using vtkSDDP = vtkStreamingDemandDrivenPipeline;

  vtkInformation* outInfo =
    outputVector->GetInformationObject(request->Get(vtkSDDP::FROM_OUTPUT_PORT()));

  if (this->MustReset() && !this->Initialize(request, inputVector, outputVector))
  {
    vtkErrorMacro("Failed to initialize time series.");
    return 0;
  }

  if (this->NoPriorTimeStepAccess && outInfo->Has(vtkSDDP::UPDATE_TIME_STEP()))
  {
    this->ProcessedTimeSteps->InsertNextValue(outInfo->Get(vtkSDDP::UPDATE_TIME_STEP()));
  }
  else if (this->NoPriorTimeStepAccess)
  {
    vtkErrorMacro("Missing UPDATE_TIME_STEP when NoPriorTimeStepAccess is ON");
    return 0;
  }
  else
  {
    ++this->CurrentTimeIndex;
  }

  if (!this->Execute(request, inputVector, outputVector))
  {
    vtkErrorMacro("Failed executing time step " << this->GetCurrentTimeStep());
    return 0;
  }

  // Handling temporal looping when needed
  if (this->MustContinue() && !this->CheckAbort())
  {
    request->Set(vtkSDDP::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    if (!this->Finalize(request, inputVector, outputVector))
    {
      vtkErrorMacro("Failed to finalize at time step " << this->GetCurrentTimeStep());
      return 0;
    }
    request->Remove(vtkSDDP::CONTINUE_EXECUTING());
    if (!this->IntegrateFullTimeSeries || this->NoPriorTimeStepAccess)
    {
      vtkDataObject::GetData(outInfo)->GetInformation()->Set(
        vtkDataObject::DATA_TIME_STEP(), this->GetCurrentTimeStep());
    }
  }

  if (this->NoPriorTimeStepAccess)
  {
    vtkNew<vtkDoubleArray> tsteps;
    tsteps->DeepCopy(this->ProcessedTimeSteps);
    vtkDataObject* output = vtkDataObject::GetData(outInfo);
    output->GetFieldData()->AddArray(tsteps);
  }

  return 1;
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
bool vtkTemporalAlgorithm<AlgorithmT>::MustReset() const
{
  return this->CurrentTimeIndex == 0 && !this->ProcessedTimeSteps->GetNumberOfValues();
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
bool vtkTemporalAlgorithm<AlgorithmT>::MustContinue() const
{
  return this->CurrentTimeIndex <= this->TerminationTimeIndex && !this->NoPriorTimeStepAccess;
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
int vtkTemporalAlgorithm<AlgorithmT>::GetCurrentTimeIndex() const
{
  return this->NoPriorTimeStepAccess
    ? static_cast<int>(this->ProcessedTimeSteps->GetNumberOfValues()) - 1
    : this->CurrentTimeIndex - 1;
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
double vtkTemporalAlgorithm<AlgorithmT>::GetCurrentTimeStep() const
{
  return this->NoPriorTimeStepAccess
    ? this->ProcessedTimeSteps->GetValue(this->ProcessedTimeSteps->GetNumberOfValues() - 1)
    : this->InputTimeSteps[this->CurrentTimeIndex - 1];
}

VTK_ABI_NAMESPACE_END
