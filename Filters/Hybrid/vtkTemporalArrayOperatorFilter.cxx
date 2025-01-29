// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTemporalArrayOperatorFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <functional>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTemporalArrayOperatorFilter);

//------------------------------------------------------------------------------
vtkTemporalArrayOperatorFilter::vtkTemporalArrayOperatorFilter()
{
  // Set the default input data array that the algorithm will process (point scalars)
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);

  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkTemporalArrayOperatorFilter::~vtkTemporalArrayOperatorFilter()
{
  this->SetOutputArrayNameSuffix(nullptr);
}

//------------------------------------------------------------------------------
void vtkTemporalArrayOperatorFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Operator: " << this->GetOperatorAsString() << " (" << this->Operator << ")"
     << endl;
  os << indent << "First time step: " << this->FirstTimeStepIndex << endl;
  os << indent << "Second time step: " << this->SecondTimeStepIndex << endl;
  os << indent << "Output array name suffix: "
     << (this->OutputArrayNameSuffix ? this->OutputArrayNameSuffix : "") << endl;
  os << indent << "Field association: "
     << vtkDataObject::GetAssociationTypeAsString(this->GetInputArrayAssociation()) << endl;
}

//------------------------------------------------------------------------------
int vtkTemporalArrayOperatorFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalArrayOperatorFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalArrayOperatorFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputInfoVector, vtkInformationVector* outputInfoVector)
{
  vtkDataObject* inputObj = vtkDataObject::GetData(inputInfoVector[0]);
  if (inputObj != nullptr)
  {
    vtkDataObject* outputObj = vtkDataObject::GetData(outputInfoVector);
    if (!outputObj || !outputObj->IsA(inputObj->GetClassName()))
    {
      vtkDataObject* newOutputObj = inputObj->NewInstance();
      vtkInformation* outputInfo = outputInfoVector->GetInformationObject(0);
      outputInfo->Set(vtkDataObject::DATA_OBJECT(), newOutputObj);
      newOutputObj->Delete();
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkTemporalArrayOperatorFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputInfoVector, vtkInformationVector* outputInfoVector)
{
  // Get input and output information objects
  vtkInformation* inputInfo = inputInfoVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputInfoVector->GetInformationObject(0);

  // Check for presence more than one time step
  if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    // Find time on input
    this->NumberTimeSteps = inputInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (this->NumberTimeSteps < 2)
    {
      vtkErrorMacro(<< "Not enough numbers of time steps: " << this->NumberTimeSteps);
      return 0;
    }

    double* inputTimes = inputInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double timeRange[2] = { inputTimes[0], inputTimes[this->NumberTimeSteps - 1] };
    if (this->RelativeMode)
    {
      int absoluteShift = std::abs(this->TimeStepShift);
      if (absoluteShift >= this->NumberTimeSteps)
      {
        vtkErrorMacro(
          << "Shift is too big: second timestep is always out of range. Absolute max is "
          << this->NumberTimeSteps);
        return 0;
      }
      int outNumberTimeSteps = this->NumberTimeSteps - absoluteShift;
      if (this->TimeStepShift < 0)
      {
        // skip first timesteps
        timeRange[0] = inputTimes[absoluteShift];
        outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &inputTimes[absoluteShift],
          outNumberTimeSteps);
      }
      else
      {
        // skip last timesteps
        outInfo->Set(
          vtkStreamingDemandDrivenPipeline::TIME_STEPS(), inputTimes, outNumberTimeSteps);
        timeRange[1] = inputTimes[outNumberTimeSteps - 1];
      }
    }
    else
    {
      double meshTime = inputTimes[this->FirstTimeStepIndex];
      double outTime[1] = { meshTime };
      timeRange[0] = meshTime;
      timeRange[1] = meshTime;
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), outTime, 1);
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  else
  {
    vtkErrorMacro(<< "No time steps in input data.");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkTemporalArrayOperatorFilter::GetTimeStepsToUse(int timeSteps[2])
{
  if (this->RelativeMode)
  {
    vtkInformation* outInfo = this->GetOutputInformation(0);
    double requestedTime = 0;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      requestedTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }

    vtkInformation* inputInfo = this->GetInputInformation();
    double* inputTime = inputInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    assert(inputTime);
    for (int step = 0; step < this->NumberTimeSteps && inputTime[step] <= requestedTime; step++)
    {
      timeSteps[0] = step;
    }
    timeSteps[1] = timeSteps[0] + this->TimeStepShift;
  }
  else
  {
    timeSteps[0] = this->FirstTimeStepIndex;
    timeSteps[1] = this->SecondTimeStepIndex;
  }
}

//------------------------------------------------------------------------------
int vtkTemporalArrayOperatorFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputInfoVector, vtkInformationVector* outputInfoVector)
{
  int timeSteps[2];
  this->GetTimeStepsToUse(timeSteps);

  if (timeSteps[0] < 0 || timeSteps[1] < 0 || timeSteps[0] >= this->NumberTimeSteps ||
    timeSteps[1] >= this->NumberTimeSteps)
  {
    vtkErrorMacro(<< "Specified timesteps (" << timeSteps[0] << " and " << timeSteps[1] << ") "
                  << "are outside the range of"
                     " available time steps ("
                  << this->NumberTimeSteps << ")");
    return 0;
  }

  if (timeSteps[0] == timeSteps[1])
  {
    vtkWarningMacro(<< "First and second time steps are the same.");
  }

  vtkInformation* outputInfo = outputInfoVector->GetInformationObject(0);
  // Find the required input time steps and request them
  if (outputInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    vtkInformation* inputInfo = inputInfoVector[0]->GetInformationObject(0);
    double* inputTime = inputInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    assert(inputTime);

    double inputUpdateTimes[2] = { inputTime[timeSteps[0]], inputTime[timeSteps[1]] };
    inputInfo->Set(vtkMultiTimeStepAlgorithm::UPDATE_TIME_STEPS(), inputUpdateTimes, 2);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalArrayOperatorFilter::Execute(vtkInformation*,
  const std::vector<vtkSmartPointer<vtkDataObject>>& inputs, vtkInformationVector* outputVector)
{
  if (inputs.size() != 2)
  {
    vtkErrorMacro(<< "The number of time blocks is incorrect.");
    return 0;
  }

  auto& data0 = inputs[0];
  auto& data1 = inputs[1];
  if (!data0 || !data1)
  {
    vtkErrorMacro(<< "Unable to retrieve data objects.");
    return 0;
  }

  vtkSmartPointer<vtkDataObject> newOutData;
  newOutData.TakeReference(this->Process(data0, data1));

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* outData = vtkDataObject::GetData(outInfo);
  outData->ShallowCopy(newOutData);

  this->CheckAbort();

  return newOutData != nullptr ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkTemporalArrayOperatorFilter::GetInputArrayAssociation()
{
  vtkInformation* inputArrayInfo =
    this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS())->GetInformationObject(0);
  return inputArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
}

//------------------------------------------------------------------------------
vtkDataObject* vtkTemporalArrayOperatorFilter::Process(
  vtkDataObject* inputData0, vtkDataObject* inputData1)
{
  if (inputData0->IsA("vtkCompositeDataSet"))
  {
    // We suppose input data are same type and have same structure (they should!)
    vtkCompositeDataSet* compositeDataSet0 = vtkCompositeDataSet::SafeDownCast(inputData0);
    vtkCompositeDataSet* compositeDataSet1 = vtkCompositeDataSet::SafeDownCast(inputData1);

    vtkCompositeDataSet* outputCompositeDataSet = compositeDataSet0->NewInstance();
    outputCompositeDataSet->CompositeShallowCopy(compositeDataSet0);

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(compositeDataSet0->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (this->CheckAbort())
      {
        break;
      }
      vtkDataObject* dataObj0 = iter->GetCurrentDataObject();
      vtkDataObject* dataObj1 = compositeDataSet1->GetDataSet(iter);
      if (!dataObj0 || !dataObj1)
      {
        vtkWarningMacro("The composite datasets have different structure.");
        continue;
      }

      vtkSmartPointer<vtkDataObject> resultDataObj;
      resultDataObj.TakeReference(this->ProcessDataObject(dataObj0, dataObj1));
      if (!resultDataObj)
      {
        return nullptr;
      }
      outputCompositeDataSet->SetDataSet(iter, resultDataObj);
    }
    return outputCompositeDataSet;
  }

  return this->ProcessDataObject(inputData0, inputData1);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkTemporalArrayOperatorFilter::ProcessDataObject(
  vtkDataObject* inputData0, vtkDataObject* inputData1)
{
  vtkDataArray* inputArray0 = this->GetInputArrayToProcess(0, inputData0);
  vtkDataArray* inputArray1 = this->GetInputArrayToProcess(0, inputData1);
  if (!inputArray0 || !inputArray1)
  {
    vtkErrorMacro(<< "Unable to retrieve data arrays to process.");
    return nullptr;
  }

  if (inputArray0->GetDataType() != inputArray1->GetDataType())
  {
    vtkErrorMacro(<< "Array type in each time step are different.");
    return nullptr;
  }

  if (strcmp(inputArray0->GetName(), inputArray1->GetName()) != 0)
  {
    vtkErrorMacro(<< "Array name in each time step are different.");
    return nullptr;
  }

  if (inputArray0->GetNumberOfComponents() != inputArray1->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "The number of components of the array in each time "
                     "step are different.");
    return nullptr;
  }

  if (inputArray0->GetNumberOfTuples() != inputArray1->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "The number of tuples of the array in each time step"
                     "are different.");
    return nullptr;
  }

  // Copy input structure into output
  vtkDataObject* outputDataObject = inputData0->NewInstance();
  outputDataObject->ShallowCopy(inputData0);

  vtkSmartPointer<vtkDataArray> outputArray;
  outputArray.TakeReference(this->ProcessDataArray(inputArray0, inputArray1));

  vtkFieldData* field =
    outputDataObject->GetAttributesAsFieldData(this->GetInputArrayAssociation());
  if (!field)
  {
    vtkErrorMacro(<< "Bad input association ("
                  << vtkDataObject::GetAssociationTypeAsString(this->GetInputArrayAssociation())
                  << ") for input data object (" << outputDataObject->GetClassName() << ")");
  }
  else
  {
    field->AddArray(outputArray);
  }

  this->CheckAbort();

  return outputDataObject;
}

//------------------------------------------------------------------------------
struct TemporalDataOperatorWorker
{
  TemporalDataOperatorWorker(int op)
    : Operator(op)
  {
  }

  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T* src1, Array2T* src2, Array3T* dst)
  {
    using T = vtk::GetAPIType<Array3T>;

    VTK_ASSUME(src1->GetNumberOfComponents() == dst->GetNumberOfComponents());
    VTK_ASSUME(src2->GetNumberOfComponents() == dst->GetNumberOfComponents());

    const auto srcRange1 = vtk::DataArrayValueRange(src1);
    const auto srcRange2 = vtk::DataArrayValueRange(src2);
    auto dstRange = vtk::DataArrayValueRange(dst);

    switch (this->Operator)
    {
      case vtkTemporalArrayOperatorFilter::ADD:
        std::transform(srcRange1.cbegin(), srcRange1.cend(), srcRange2.cbegin(), dstRange.begin(),
          std::plus<T>{});
        break;
      case vtkTemporalArrayOperatorFilter::SUB:
        std::transform(srcRange1.cbegin(), srcRange1.cend(), srcRange2.cbegin(), dstRange.begin(),
          std::minus<T>{});
        break;
      case vtkTemporalArrayOperatorFilter::MUL:
        std::transform(srcRange1.cbegin(), srcRange1.cend(), srcRange2.cbegin(), dstRange.begin(),
          std::multiplies<T>{});
        break;
      case vtkTemporalArrayOperatorFilter::DIV:
        std::transform(srcRange1.cbegin(), srcRange1.cend(), srcRange2.cbegin(), dstRange.begin(),
          std::divides<T>{});
        break;
      default:
        std::copy(srcRange1.cbegin(), srcRange1.cend(), dstRange.begin());
        break;
    }
  }

  int Operator;
};

//------------------------------------------------------------------------------
vtkDataArray* vtkTemporalArrayOperatorFilter::ProcessDataArray(
  vtkDataArray* inputArray0, vtkDataArray* inputArray1)
{
  vtkAbstractArray* outputArray = vtkAbstractArray::CreateArray(inputArray0->GetDataType());
  vtkDataArray* outputDataArray = vtkDataArray::SafeDownCast(outputArray);

  outputDataArray->SetNumberOfComponents(inputArray0->GetNumberOfComponents());
  outputDataArray->SetNumberOfTuples(inputArray0->GetNumberOfTuples());
  outputDataArray->CopyComponentNames(inputArray0);

  std::string arrayName = inputArray0->GetName() ? inputArray0->GetName() : "input_array";
  if (this->OutputArrayNameSuffix && strlen(this->OutputArrayNameSuffix) != 0)
  {
    arrayName += this->OutputArrayNameSuffix;
  }
  else
  {
    arrayName += "_" + this->GetOperatorAsString();
  }
  outputDataArray->SetName(arrayName.c_str());

  // Let's perform the operation on the array
  TemporalDataOperatorWorker worker(this->Operator);

  if (!vtkArrayDispatch::Dispatch3SameValueType::Execute(
        inputArray0, inputArray1, outputDataArray, worker))
  {
    worker(inputArray0, inputArray1, outputDataArray); // vtkDataArray fallback
  }

  return outputDataArray;
}

//------------------------------------------------------------------------------
std::string vtkTemporalArrayOperatorFilter::GetOperatorAsString()
{
  switch (this->Operator)
  {
    case OperatorType::SUB:
      return "sub";
    case OperatorType::MUL:
      return "mul";
    case OperatorType::DIV:
      return "div";
    case OperatorType::ADD:
    default:
      return "add";
  }
}

VTK_ABI_NAMESPACE_END
