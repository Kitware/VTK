// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov

#include "vtkCriticalTime.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCriticalTime);

namespace
{
const std::string CRITICAL_TIME_SUFFIX = "critical_time";

//------------------------------------------------------------------------------
/**
 * Mangle originalName with given suffix
 */
inline std::string vtkCriticalTimeMangleName(
  const std::string& originalName, const std::string& suffix)
{
  if (originalName.empty())
  {
    return suffix;
  }
  return std::string(originalName) + "_" + suffix;
}

//------------------------------------------------------------------------------
/**
 * Base class for vtkCriticalTime workers. Subclasses implements logic to fill the
 * time array according to the the threshold criterion and threshold values.
 */
struct CheckCriticalTimeWorker
{
  CheckCriticalTimeWorker(int thresholdCriterion, double lowerThresold, double upperThreshold)
    : LowerThreshold(lowerThresold)
    , UpperThreshold(upperThreshold)
  {
    switch (thresholdCriterion)
    {
      case vtkCriticalTime::THRESHOLD_BETWEEN:
        this->ThresholdFunction = &CheckCriticalTimeWorker::Between;
        break;
      case vtkCriticalTime::THRESHOLD_LOWER:
        this->ThresholdFunction = &CheckCriticalTimeWorker::Lower;
        break;
      case vtkCriticalTime::THRESHOLD_UPPER:
      default:
        this->ThresholdFunction = &CheckCriticalTimeWorker::Upper;
        break;
    }
  }

protected:
  double LowerThreshold = 0.0;
  double UpperThreshold = 0.0;
  bool (CheckCriticalTimeWorker::*ThresholdFunction)(
    double) const = &CheckCriticalTimeWorker::Between;

private:
  // Returns true if value is comprised beetween the lower and upper thresholds
  bool Between(double value) const
  {
    return (value >= this->LowerThreshold ? (value <= this->UpperThreshold ? true : false) : false);
  }

  // Returns true if value is lower than the lower threshold
  bool Lower(double value) const { return (value <= this->LowerThreshold ? true : false); }

  // Returns true if value is upper than the upper threshold
  bool Upper(double value) const { return (value >= this->UpperThreshold ? true : false); }
};

//------------------------------------------------------------------------------
/**
 * For each point/cell, set the value in the output "time array" to the current timestep value
 * if the selected component (at current point / cell ID) meet the threshold criterion.
 * Check magnitude if selected component == number of components.
 */
struct CheckCriticalTimeComp : public CheckCriticalTimeWorker
{
  using CheckCriticalTimeWorker::CheckCriticalTimeWorker;

  template <typename ArrayT>
  double ComputeMagnitude(ArrayT* inArray, vtkIdType tupleIdx)
  {
    vtkDataArrayAccessor<ArrayT> inAcc(inArray);

    double squaredNorm = 0.0;
    for (int c = 0; c < inArray->GetNumberOfComponents(); ++c)
    {
      const double value = static_cast<double>(inAcc.Get(tupleIdx, c));
      squaredNorm += value * value;
    }

    return std::sqrt(squaredNorm);
  }

  template <typename ArrayT>
  void operator()(
    ArrayT* inArray, vtkDoubleArray* outArray, double currentTimeStep, int selectedComponent)
  {
    vtkDataArrayAccessor<ArrayT> inAcc(inArray);

    vtkSMPTools::For(0, inArray->GetNumberOfTuples(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
      {
        if (!std::isnan(outArray->GetValue(tupleIdx)))
        { // Critical time already exceeded
          continue;
        }

        if (selectedComponent == inArray->GetNumberOfComponents() &&
          inArray->GetNumberOfComponents() > 1)
        { // Magnitude requested
          if ((this->*(this->ThresholdFunction))(this->ComputeMagnitude(inArray, tupleIdx)))
          {
            outArray->SetValue(tupleIdx, currentTimeStep);
          }
        }
        else if ((this->*(this->ThresholdFunction))(inAcc.Get(tupleIdx, selectedComponent)))
        {
          outArray->SetValue(tupleIdx, currentTimeStep);
        }
      }
    });
  }
};

//------------------------------------------------------------------------------
/**
 * For each point/cell, set the value in the output "time array" to the current timestep value
 * if any components (at current point / cell ID) meet the threshold criterion.
 */
struct CheckCriticalTimeAny : public CheckCriticalTimeWorker
{
  using CheckCriticalTimeWorker::CheckCriticalTimeWorker;

  template <typename ArrayT>
  void operator()(ArrayT* inArray, vtkDoubleArray* outArray, double currentTimeStep) const
  {
    vtkDataArrayAccessor<ArrayT> inAcc(inArray);

    vtkSMPTools::For(0, inArray->GetNumberOfTuples(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
      {
        if (!std::isnan(outArray->GetValue(tupleIdx)))
        { // Critical time already exceeded
          continue;
        }

        for (int comp = 0; comp < inArray->GetNumberOfComponents(); ++comp)
        {
          if ((this->*(this->ThresholdFunction))(inAcc.Get(tupleIdx, comp)))
          {
            outArray->SetValue(tupleIdx, currentTimeStep);
            break;
          }
        }
      }
    });
  }
};

//------------------------------------------------------------------------------
/**
 * For each point/cell, set the value in the output "time array" to the current timestep value
 * if all components (at current point / cell ID) meet the threshold criterion.
 */
struct CheckCriticalTimeAll : public CheckCriticalTimeWorker
{
  using CheckCriticalTimeWorker::CheckCriticalTimeWorker;

  template <typename ArrayT>
  void operator()(ArrayT* inArray, vtkDoubleArray* outArray, double currentTimeStep) const
  {
    vtkDataArrayAccessor<ArrayT> inAcc(inArray);

    vtkSMPTools::For(0, inArray->GetNumberOfTuples(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
      {
        if (!std::isnan(outArray->GetValue(tupleIdx)))
        { // Critical time already exceeded
          continue;
        }

        bool allExceed = true;
        for (int comp = 0; comp < inArray->GetNumberOfComponents(); ++comp)
        {
          if (!(this->*(this->ThresholdFunction))(inAcc.Get(tupleIdx, comp)))
          {
            allExceed = false;
            break;
          }
        }
        if (allExceed)
        {
          outArray->SetValue(tupleIdx, currentTimeStep);
        }
      }
    });
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
struct vtkCriticalTime::vtkCriticalTimeInternals
{
  vtkCriticalTimeInternals(vtkCriticalTime* self)
    : Self(self)
  {
  }

  ///@{
  /**
   * Helper methods called during Initialize().
   * Fill the output critical time array with NaN values.
   */
  int InitializeCriticalTimeArray(
    vtkDataObject* input, vtkDataObject* output, vtkDataObject* cache);
  int InitializeCriticalTimeArray(vtkDataSet* input, vtkDataSet* output, vtkDataSet* cache);
  int InitializeCriticalTimeArray(
    vtkCompositeDataSet* input, vtkCompositeDataSet* output, vtkCompositeDataSet* cache);
  int InitializeCriticalTimeArray(vtkDataArray* array, vtkFieldData* outFd);
  ///@}

  ///@{
  /**
   * Helper methods called during Execute().
   * Update the output critical time array by checking, for each point / cell, if the criterion
   * has been met.
   */
  int UpdateCriticalTimeArray(vtkDataObject* input, vtkDataObject* output);
  int UpdateCriticalTimeArray(vtkDataSet* input, vtkDataSet* output);
  int UpdateCriticalTimeArray(vtkCompositeDataSet* input, vtkCompositeDataSet* output);
  int UpdateCriticalTimeArray(vtkDataArray* array, vtkFieldData* outFd);
  ///@}

  /**
   * Helper method to retrieve the output critical time array.
   */
  vtkDoubleArray* GetCriticalTimeArray(
    vtkFieldData* fieldData, vtkDataArray* inArray, const std::string& nameSuffix);

  /**
   * Output result, that can be returned at each Finalize() call
   */
  vtkSmartPointer<vtkDataObject> OutputCache;

  /**
   * Used to avoid multiple warnings for the same filter when
   * the number of points or cells in the data set is changing
   * between time steps.
   */
  bool GeneratedChangingTopologyWarning = false;

  vtkCriticalTime* Self = nullptr;
};

//------------------------------------------------------------------------------
int vtkCriticalTime::vtkCriticalTimeInternals::InitializeCriticalTimeArray(
  vtkDataObject* input, vtkDataObject* output, vtkDataObject* cache)
{
  if (input->IsA("vtkDataSet"))
  {
    return this->InitializeCriticalTimeArray(vtkDataSet::SafeDownCast(input),
      vtkDataSet::SafeDownCast(output), vtkDataSet::SafeDownCast(cache));
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    return this->InitializeCriticalTimeArray(vtkCompositeDataSet::SafeDownCast(input),
      vtkCompositeDataSet::SafeDownCast(output), vtkCompositeDataSet::SafeDownCast(cache));
  }

  vtkErrorWithObjectMacro(this->Self, << "Unsupported input type: " << input->GetClassName());
  return 0;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::vtkCriticalTimeInternals::InitializeCriticalTimeArray(
  vtkDataSet* input, vtkDataSet* output, vtkDataSet* cache)
{
  output->CopyStructure(input);
  cache->CopyStructure(input);

  int association = this->Self->GetInputArrayAssociation(0, input);
  vtkFieldData* fieldData = cache->GetAttributesAsFieldData(association);

  return this->InitializeCriticalTimeArray(this->Self->GetInputArrayToProcess(0, input), fieldData);
}

//------------------------------------------------------------------------------
int vtkCriticalTime::vtkCriticalTimeInternals::InitializeCriticalTimeArray(
  vtkCompositeDataSet* input, vtkCompositeDataSet* output, vtkCompositeDataSet* cache)
{
  output->CopyStructure(input);
  cache->CopyStructure(input);

  vtkSmartPointer<vtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());
  inputItr->SkipEmptyNodesOn();

  for (inputItr->InitTraversal(); !inputItr->IsDoneWithTraversal(); inputItr->GoToNextItem())
  {
    vtkDataObject* inputObj = inputItr->GetCurrentDataObject();

    vtkSmartPointer<vtkDataObject> outputObj;
    outputObj.TakeReference(inputObj->NewInstance());
    vtkSmartPointer<vtkDataObject> cacheObj;
    cacheObj.TakeReference(inputObj->NewInstance());

    if (!this->InitializeCriticalTimeArray(inputObj, outputObj, cacheObj))
    {
      return 0;
    }

    output->SetDataSet(inputItr, outputObj);
    cache->SetDataSet(inputItr, cacheObj);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::vtkCriticalTimeInternals::InitializeCriticalTimeArray(
  vtkDataArray* array, vtkFieldData* outFd)
{
  if (!array)
  {
    vtkErrorWithObjectMacro(this->Self, "No input array to process as been provided, aborting.");
    return 0;
  }

  if (array->GetNumberOfComponents() > 1 &&
    this->Self->SelectedComponent > array->GetNumberOfComponents())
  {
    vtkErrorWithObjectMacro(
      this->Self, << "Selected component is out of range. Number of components of the input array "
                     "to process: "
                  << array->GetNumberOfComponents() - 1 << ", so max is "
                  << array->GetNumberOfComponents() << "(magnitude).");
    return 0;
  }

  vtkNew<vtkDoubleArray> newArray;
  newArray->SetName(vtkCriticalTimeMangleName(array->GetName(), CRITICAL_TIME_SUFFIX).c_str());

  newArray->SetNumberOfComponents(1);
  newArray->SetNumberOfTuples(array->GetNumberOfTuples());
  newArray->Fill(vtkMath::Nan());

  outFd->AddArray(newArray);
  return 1;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::vtkCriticalTimeInternals::UpdateCriticalTimeArray(
  vtkDataObject* input, vtkDataObject* output)
{
  if (input->IsA("vtkDataSet"))
  {
    return this->UpdateCriticalTimeArray(
      vtkDataSet::SafeDownCast(input), vtkDataSet::SafeDownCast(output));
  }

  if (input->IsA("vtkCompositeDataSet"))
  {
    return this->UpdateCriticalTimeArray(
      vtkCompositeDataSet::SafeDownCast(input), vtkCompositeDataSet::SafeDownCast(output));
  }

  vtkErrorWithObjectMacro(this->Self, << "Unsupported input type: " << input->GetClassName());
  return 0;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::vtkCriticalTimeInternals::UpdateCriticalTimeArray(
  vtkDataSet* input, vtkDataSet* output)
{
  int association = this->Self->GetInputArrayAssociation(0, input);
  vtkFieldData* fieldData = output->GetAttributesAsFieldData(association);

  return this->UpdateCriticalTimeArray(this->Self->GetInputArrayToProcess(0, input), fieldData);
}

//------------------------------------------------------------------------------
int vtkCriticalTime::vtkCriticalTimeInternals::UpdateCriticalTimeArray(
  vtkCompositeDataSet* input, vtkCompositeDataSet* output)
{
  vtkSmartPointer<vtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal(); !inputItr->IsDoneWithTraversal(); inputItr->GoToNextItem())
  {
    vtkDataObject* inputObj = inputItr->GetCurrentDataObject();
    vtkDataObject* outputObj = output->GetDataSet(inputItr);

    if (!this->UpdateCriticalTimeArray(inputObj, outputObj))
    {
      return 0;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::vtkCriticalTimeInternals::UpdateCriticalTimeArray(
  vtkDataArray* inArray, vtkFieldData* outFd)
{
  vtkDoubleArray* outTimeArray = this->GetCriticalTimeArray(outFd, inArray, CRITICAL_TIME_SUFFIX);
  if (!outTimeArray)
  {
    vtkErrorWithObjectMacro(this->Self, "Unable to retrieve ouput critical time array.");
    return 0;
  }

  using Dispatcher = vtkArrayDispatch::Dispatch;

  switch (this->Self->ComponentMode)
  {
    case COMPONENT_MODE_USE_SELECTED:
    {
      ::CheckCriticalTimeComp worker(
        this->Self->ThresholdCriterion, this->Self->LowerThreshold, this->Self->UpperThreshold);
      if (!Dispatcher::Execute(inArray, worker, outTimeArray, this->Self->GetCurrentTimeStep(),
            this->Self->SelectedComponent))
      { // Fallback to slow path
        worker(
          inArray, outTimeArray, this->Self->GetCurrentTimeStep(), this->Self->SelectedComponent);
      }
      break;
    }

    case COMPONENT_MODE_USE_ALL:
    {
      ::CheckCriticalTimeAll worker(
        this->Self->ThresholdCriterion, this->Self->LowerThreshold, this->Self->UpperThreshold);
      if (!Dispatcher::Execute(inArray, worker, outTimeArray, this->Self->GetCurrentTimeStep()))
      { // Fallback to slow path
        worker(inArray, outTimeArray, this->Self->GetCurrentTimeStep());
      }
      break;
    }
    case COMPONENT_MODE_USE_ANY:
    default:
    {
      ::CheckCriticalTimeAny worker(
        this->Self->ThresholdCriterion, this->Self->LowerThreshold, this->Self->UpperThreshold);
      if (!Dispatcher::Execute(inArray, worker, outTimeArray, this->Self->GetCurrentTimeStep()))
      { // Fallback to slow path
        worker(inArray, outTimeArray, this->Self->GetCurrentTimeStep());
      }
      break;
    }
  }

  // Alert change in data.
  outTimeArray->DataChanged();
  return 1;
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkCriticalTime::vtkCriticalTimeInternals::GetCriticalTimeArray(
  vtkFieldData* fieldData, vtkDataArray* inArray, const std::string& nameSuffix)
{
  if (!inArray)
  {
    return nullptr;
  }

  std::string outArrayName = vtkCriticalTimeMangleName(inArray->GetName(), nameSuffix);
  vtkDoubleArray* outArray =
    vtkDoubleArray::SafeDownCast(fieldData->GetArray(outArrayName.c_str()));
  if (!outArray)
  {
    return nullptr;
  }

  if (inArray->GetNumberOfTuples() != outArray->GetNumberOfTuples())
  {
    if (!this->GeneratedChangingTopologyWarning)
    {
      std::string fieldType = vtkCellData::SafeDownCast(fieldData) == nullptr ? "points" : "cells";
      vtkWarningWithObjectMacro(this->Self,
        "The number of " << fieldType << " has changed between time "
                         << "steps. No arrays of this type will be output since this "
                         << "filter can not handle topology that change over time.");
      this->GeneratedChangingTopologyWarning = true;
    }
    fieldData->RemoveArray(outArray->GetName());
    return nullptr;
  }

  return outArray;
}

//------------------------------------------------------------------------------
vtkCriticalTime::vtkCriticalTime()
  : Internals(new vtkCriticalTimeInternals(this))
{
  this->IntegrateFullTimeSeries = true;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
void vtkCriticalTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LowerThreshold: " << this->LowerThreshold << endl;
  os << indent << "UpperThreshold: " << this->UpperThreshold << endl;
  os << indent << "ComponentMode: " << this->GetComponentModeAsString() << endl;
  os << indent << "SelectedComponent: " << this->SelectedComponent << endl;
  os << indent << "Threshold function: " << this->GetThresholdFunctionAsString() << endl;
}

//------------------------------------------------------------------------------
std::string vtkCriticalTime::GetThresholdFunctionAsString() const
{
  switch (this->ThresholdCriterion)
  {
    case THRESHOLD_BETWEEN:
      return "Between lower and upper thresholds";
    case THRESHOLD_LOWER:
      return "Lower threshold";
    case THRESHOLD_UPPER:
    default:
      return "Upper threshold";
  }
}

//------------------------------------------------------------------------------
std::string vtkCriticalTime::GetComponentModeAsString() const
{
  switch (this->ComponentMode)
  {
    case COMPONENT_MODE_USE_SELECTED:
      return "UseSelected";
    case COMPONENT_MODE_USE_ANY:
      return "UseAny";
    case COMPONENT_MODE_USE_ALL:
    default:
      return "UseAll";
  }
}

//------------------------------------------------------------------------------
int vtkCriticalTime::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::RequestDataObject(vtkInformation* vtkNotUsed(request),
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
    this->Internals->OutputCache.TakeReference(input->NewInstance());
  }

  if (newOutput)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::Initialize(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = vtkDataObject::GetData(inInfo);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  if (!input || !output)
  {
    return 0;
  }

  this->Internals->OutputCache->Initialize();
  return this->Internals->InitializeCriticalTimeArray(input, output, this->Internals->OutputCache);
}

//------------------------------------------------------------------------------
int vtkCriticalTime::Execute(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = vtkDataObject::GetData(inInfo);

  return this->Internals->UpdateCriticalTimeArray(input, this->Internals->OutputCache);
}

//------------------------------------------------------------------------------
int vtkCriticalTime::Finalize(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  output->DeepCopy(this->Internals->OutputCache);

  return 1;
}
VTK_ABI_NAMESPACE_END
