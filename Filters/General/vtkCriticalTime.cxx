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

inline std::string vtkCriticalTimeMangleName(
  const std::string& originalName, const std::string& suffix)
{
  if (originalName.empty())
  {
    return suffix;
  }
  return std::string(originalName) + "_" + suffix;
}
} // anonymous namespace

//------------------------------------------------------------------------------
/**
 * For each point/cell, set the value in the output "time array" to the current timestep value
 * if the selected component (at current point / cell ID) meet the threshold criterion.
 * Check magnitude if selected component == number of components.
 */
struct vtkCriticalTime::CheckCriticalTimeComp
{
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
  void operator()(ArrayT* inArray, vtkDoubleArray* outArray, vtkCriticalTime* self)
  {
    vtkDataArrayAccessor<ArrayT> inAcc(inArray);

    vtkSMPTools::For(0, inArray->GetNumberOfTuples(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType tupleIdx = begin; tupleIdx < end; ++tupleIdx)
      {
        if (!std::isnan(outArray->GetValue(tupleIdx)))
        { // Critical time already exceeded
          continue;
        }

        if (self->SelectedComponent == inArray->GetNumberOfComponents() &&
          inArray->GetNumberOfComponents() > 1)
        { // Magnitude requested
          if ((self->*(self->ThresholdFunction))(this->ComputeMagnitude(inArray, tupleIdx)))
          {
            outArray->SetValue(tupleIdx, self->GetCurrentTimeStep());
          }
        }
        else if ((self->*(self->ThresholdFunction))(inAcc.Get(tupleIdx, self->SelectedComponent)))
        {
          outArray->SetValue(tupleIdx, self->GetCurrentTimeStep());
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
struct vtkCriticalTime::CheckCriticalTimeAny
{
  template <typename ArrayT>
  void operator()(ArrayT* inArray, vtkDoubleArray* outArray, vtkCriticalTime* self) const
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
          if ((self->*(self->ThresholdFunction))(inAcc.Get(tupleIdx, comp)))
          {
            outArray->SetValue(tupleIdx, self->GetCurrentTimeStep());
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
struct vtkCriticalTime::CheckCriticalTimeAll
{
  template <typename ArrayT>
  void operator()(ArrayT* inArray, vtkDoubleArray* outArray, vtkCriticalTime* self) const
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
          if (!(self->*(self->ThresholdFunction))(inAcc.Get(tupleIdx, comp)))
          {
            allExceed = false;
            break;
          }
        }
        if (allExceed)
        {
          outArray->SetValue(tupleIdx, self->GetCurrentTimeStep());
        }
      }
    });
  }
};

//------------------------------------------------------------------------------
vtkCriticalTime::vtkCriticalTime()
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
// Return a string representation of the component mode
std::string vtkCriticalTime::GetComponentModeAsString() const
{
  if (this->ComponentMode == COMPONENT_MODE_USE_SELECTED)
  {
    return "UseSelected";
  }
  else if (this->ComponentMode == COMPONENT_MODE_USE_ANY)
  {
    return "UseAny";
  }
  else
  {
    return "UseAll";
  }
}

//------------------------------------------------------------------------------
// Return a string representation of the threshold function used
std::string vtkCriticalTime::GetThresholdFunctionAsString() const
{
  if (this->ThresholdFunction == &vtkCriticalTime::Lower)
  {
    return "Lower threshold";
  }
  else if (this->ThresholdFunction == &vtkCriticalTime::Upper)
  {
    return "Upper threshold";
  }
  else
  {
    return "Between lower and upper thresholds";
  }
}

//------------------------------------------------------------------------------
bool vtkCriticalTime::Lower(double s) const
{
  return (s <= this->LowerThreshold ? 1 : 0);
}

//------------------------------------------------------------------------------
bool vtkCriticalTime::Upper(double s) const
{
  return (s >= this->UpperThreshold ? 1 : 0);
}

//------------------------------------------------------------------------------
bool vtkCriticalTime::Between(double s) const
{
  return (s >= this->LowerThreshold ? (s <= this->UpperThreshold ? 1 : 0) : 0);
}

//------------------------------------------------------------------------------
void vtkCriticalTime::SetThresholdFunction(int function)
{
  int clampedFunction = function < THRESHOLD_BETWEEN
    ? THRESHOLD_BETWEEN
    : (function > THRESHOLD_UPPER ? THRESHOLD_UPPER : function);

  if (this->GetThresholdFunction() != clampedFunction)
  {
    switch (clampedFunction)
    {
      case vtkCriticalTime::THRESHOLD_BETWEEN:
        this->ThresholdFunction = &vtkCriticalTime::Between;
        break;
      case vtkCriticalTime::THRESHOLD_LOWER:
        this->ThresholdFunction = &vtkCriticalTime::Lower;
        break;
      case vtkCriticalTime::THRESHOLD_UPPER:
      default:
        this->ThresholdFunction = &vtkCriticalTime::Upper;
        break;
    }

    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkCriticalTime::GetThresholdFunction() const
{
  if (this->ThresholdFunction == &vtkCriticalTime::Lower)
  {
    return vtkCriticalTime::THRESHOLD_LOWER;
  }
  else if (this->ThresholdFunction == &vtkCriticalTime::Upper)
  {
    return vtkCriticalTime::THRESHOLD_UPPER;
  }
  else
  {
    return vtkCriticalTime::THRESHOLD_BETWEEN;
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
    this->OutputCache.TakeReference(input->NewInstance());
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

  this->OutputCache->Initialize();
  return this->InitializeCriticalTimeArray(input, output, this->OutputCache);
}

//------------------------------------------------------------------------------
int vtkCriticalTime::Execute(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = vtkDataObject::GetData(inInfo);

  return this->UpdateCriticalTimeArray(input, this->OutputCache);
}

//------------------------------------------------------------------------------
int vtkCriticalTime::Finalize(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  output->DeepCopy(this->OutputCache);

  return 1;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::InitializeCriticalTimeArray(
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

  vtkErrorMacro(<< "Unsupported input type: " << input->GetClassName());
  return 0;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::InitializeCriticalTimeArray(
  vtkDataSet* input, vtkDataSet* output, vtkDataSet* cache)
{
  output->CopyStructure(input);
  cache->CopyStructure(input);

  int association = this->GetInputArrayAssociation(0, input);
  vtkFieldData* fieldData = cache->GetAttributesAsFieldData(association);

  return this->InitializeCriticalTimeArray(this->GetInputArrayToProcess(0, input), fieldData);
}

//------------------------------------------------------------------------------
int vtkCriticalTime::InitializeCriticalTimeArray(
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
int vtkCriticalTime::InitializeCriticalTimeArray(vtkDataArray* array, vtkFieldData* outFd)
{
  if (!array)
  {
    vtkErrorMacro("No input array to process as been provided, aborting.");
    return 0;
  }

  if (array->GetNumberOfComponents() > 1 &&
    this->SelectedComponent > array->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Selected component is out of range. Number of components of the input array "
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
int vtkCriticalTime::UpdateCriticalTimeArray(vtkDataObject* input, vtkDataObject* output)
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

  vtkErrorMacro(<< "Unsupported input type: " << input->GetClassName());
  return 0;
}

//------------------------------------------------------------------------------
int vtkCriticalTime::UpdateCriticalTimeArray(vtkDataSet* input, vtkDataSet* output)
{
  int association = this->GetInputArrayAssociation(0, input);
  vtkFieldData* fieldData = output->GetAttributesAsFieldData(association);

  return this->UpdateCriticalTimeArray(this->GetInputArrayToProcess(0, input), fieldData);
}

//------------------------------------------------------------------------------
int vtkCriticalTime::UpdateCriticalTimeArray(
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
int vtkCriticalTime::UpdateCriticalTimeArray(vtkDataArray* inArray, vtkFieldData* outFd)
{
  vtkDoubleArray* outTimeArray = this->GetCriticalTimeArray(outFd, inArray, CRITICAL_TIME_SUFFIX);
  if (!outTimeArray)
  {
    vtkErrorMacro("Unable to retrieve ouput critical time array.");
    return 0;
  }

  using Dispatcher = vtkArrayDispatch::Dispatch;

  switch (this->ComponentMode)
  {
    case COMPONENT_MODE_USE_SELECTED:
    {
      CheckCriticalTimeComp worker;
      if (!Dispatcher::Execute(inArray, worker, outTimeArray, this))
      { // Fallback to slow path
        worker(inArray, outTimeArray, this);
      }
      break;
    }

    case COMPONENT_MODE_USE_ALL:
    {
      CheckCriticalTimeAll worker;
      if (!Dispatcher::Execute(inArray, worker, outTimeArray, this))
      { // Fallback to slow path
        worker(inArray, outTimeArray, this);
      }
      break;
    }
    case COMPONENT_MODE_USE_ANY:
    default:
    {
      CheckCriticalTimeAny worker;
      if (!Dispatcher::Execute(inArray, worker, outTimeArray, this))
      { // Fallback to slow path
        worker(inArray, outTimeArray, this);
      }
      break;
    }
  }

  // Alert change in data.
  outTimeArray->DataChanged();
  return 1;
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkCriticalTime::GetCriticalTimeArray(
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
      vtkWarningMacro("The number of " << fieldType << " has changed between time "
                                       << "steps. No arrays of this type will be output since this "
                                       << "filter can not handle grids that change over time.");
      this->GeneratedChangingTopologyWarning = true;
    }
    fieldData->RemoveArray(outArray->GetName());
    return nullptr;
  }

  return outArray;
}
VTK_ABI_NAMESPACE_END
