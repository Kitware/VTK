// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkONNXInference.h"
#include "Private/vtkONNXInferenceInternals.h"

#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <numeric>
#include <onnxruntime_cxx_api.h>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkONNXInference);

namespace
{
//------------------------------------------------------------------------------
Ort::Value RawToTensor(float* data, const std::vector<int64_t>& shape)
{
  int64_t numberElements = std::accumulate(shape.begin(), shape.end(), 1LL, std::multiplies<>());
  Ort::MemoryInfo memInfo =
    Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
  Ort::Value tensor =
    Ort::Value::CreateTensor<float>(memInfo, data, numberElements, shape.data(), shape.size());
  return tensor;
}
} // anonymous namespace

//------------------------------------------------------------------------------
vtkONNXInference::vtkONNXInference()
  : Internals(std::make_unique<vtkONNXInferenceInternals>())
{
}

//------------------------------------------------------------------------------
void vtkONNXInference::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Path to ONNX model: " << this->ModelFile << endl;
  os << indent << "OutputDimension: " << this->OutputDimension << endl;
  os << indent << "ArrayAssociation: " << this->ArrayAssociation << endl;
  os << indent << "InputShape: (";
  for (size_t i = 0; i < this->InputShape.size(); ++i)
  {
    if (i > 0)
    {
      os << ", ";
    }
    os << this->InputShape[i];
  }
  os << ")" << std::endl;
}

//------------------------------------------------------------------------------
void vtkONNXInference::InitializeSession()
{
  Ort::SessionOptions sessionOptions;

  // Prioritize GPU if available
  std::vector<std::string> providers = Ort::GetAvailableProviders();
  if (std::find(providers.begin(), providers.end(), "CUDAExecutionProvider") != providers.end())
  {
    OrtCUDAProviderOptions options;
    options.device_id = 0;
    options.arena_extend_strategy = 0;
    options.gpu_mem_limit = SIZE_MAX;
    options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
    options.do_copy_in_default_stream = 1;

    sessionOptions.AppendExecutionProvider_CUDA(options);
  }

  try
  {
    this->Internals->Session = std::make_unique<Ort::Session>(
      this->Internals->OrtEnv, this->ModelFile.c_str(), sessionOptions);
  }
  catch (const Ort::Exception& e)
  {
    vtkErrorMacro(<< e.what());
    this->Internals->Session.reset();
  }
  this->Initialized = true;
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetModelFile(const std::string& file)
{
  vtkDebugMacro("setting ModelFile to " << file);
  this->ModelFile = file;
  this->Initialized = false;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetTimeStepValues(const std::vector<double>& times)
{
  if (this->TimeStepValues != times)
  {
    vtkDebugMacro("setting TimeStepValues");
    this->TimeStepValues = times;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetTimeStepValue(vtkIdType idx, double timeStepValue)
{
  if (idx < 0 || static_cast<size_t>(idx) > this->TimeStepValues.size())
  {
    vtkErrorMacro("Time step index is out of bounds.");
    return;
  }
  vtkDebugMacro("setting TimeStepValues index " << idx << " to " << timeStepValue);
  this->TimeStepValues[idx] = timeStepValue;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetNumberOfTimeStepValues(vtkIdType nb)
{
  vtkDebugMacro("setting NumberOfTimeStepValues to nb");
  this->TimeStepValues.resize(nb);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::ClearTimeStepValues()
{
  vtkDebugMacro("setting TimeStepValues to empty list");
  this->TimeStepValues.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetInputParameters(const std::vector<float>& inputParameters)
{
  if (this->InputParameters != inputParameters)
  {
    vtkDebugMacro("setting InputParameters");
    this->InputParameters = inputParameters;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetInputParameter(vtkIdType idx, float inputParameter)
{
  if (idx < 0 || static_cast<size_t>(idx) > this->InputParameters.size())
  {
    vtkErrorMacro("Input parameter index is out of bounds.");
    return;
  }
  vtkDebugMacro("setting InputParameters index " << idx << " to " << inputParameter);
  this->InputParameters[idx] = inputParameter;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetInputShape(const std::vector<int64_t>& shape)
{
  if (this->InputShape != shape)
  {
    vtkDebugMacro("setting InputShape");
    this->InputShape = shape;
    if (!this->FieldArrayInput)
    {
      this->InputParameters.resize(shape[0]);
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
const std::vector<int64_t>& vtkONNXInference::GetInputShape() const
{
  return this->InputShape;
}

//------------------------------------------------------------------------------
void vtkONNXInference::ClearInputParameters()
{
  vtkDebugMacro("setting InputParameters to empty list");
  this->InputParameters.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkONNXInference::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (this->ShouldGenerateTimeSteps())
  {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeStepValues.data(),
      static_cast<int>(this->TimeStepValues.size()));

    const auto rangePair =
      std::minmax_element(this->TimeStepValues.cbegin(), this->TimeStepValues.cend());
    double range[2] = { *rangePair.first, *rangePair.second };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
  }
  return 1;
}

//------------------------------------------------------------------------------
std::vector<Ort::Value> vtkONNXInference::RunModel(Ort::Value& inputTensor)
{
  Ort::AllocatorWithDefaultOptions allocator;
  // Retrieve names
  std::vector<std::string> inputNames;
  inputNames.reserve(this->Internals->Session->GetInputCount());
  for (size_t i = 0; i < this->Internals->Session->GetInputCount(); ++i)
  {
    inputNames.emplace_back(this->Internals->Session->GetInputNameAllocated(i, allocator).get());
  }
  std::vector<std::string> outputNames;
  outputNames.reserve(this->Internals->Session->GetOutputCount());
  for (size_t i = 0; i < this->Internals->Session->GetOutputCount(); ++i)
  {
    outputNames.emplace_back(this->Internals->Session->GetOutputNameAllocated(i, allocator).get());
  }

  std::vector<const char*> inputNamesChar(inputNames.size(), nullptr);
  std::transform(inputNames.begin(), inputNames.end(), inputNamesChar.begin(),
    [&](const std::string& str) { return str.c_str(); });
  std::vector<const char*> outputNamesChar(outputNames.size(), nullptr);
  std::transform(outputNames.begin(), outputNames.end(), outputNamesChar.begin(),
    [&](const std::string& str) { return str.c_str(); });

  return this->Internals->Session->Run(Ort::RunOptions{ nullptr }, inputNamesChar.data(),
    &inputTensor, inputNamesChar.size(), outputNamesChar.data(), outputNamesChar.size());
}

//------------------------------------------------------------------------------
int vtkONNXInference::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Initialized)
  {
    this->InitializeSession();
  }

  // Time handling: snap requested time to one of available time.
  double timeValue = outputVector->GetInformationObject(0)->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  if (this->ShouldGenerateTimeSteps())
  {
    auto time =
      std::lower_bound(this->TimeStepValues.begin(), this->TimeStepValues.end(), timeValue);
    if (time != this->TimeStepValues.end())
    {
      timeValue = *time;
    }
    else
    {
      timeValue = this->TimeStepValues.back();
    }
  }

  vtkDataObjectTree* compositeInput = vtkDataObjectTree::GetData(inputVector[0], 0);
  vtkDataObjectTree* compositeOutput = vtkDataObjectTree::GetData(outputVector, 0);
  if (compositeInput && compositeOutput)
  {
    compositeOutput->CopyStructure(compositeInput);
    compositeOutput->CompositeShallowCopy(compositeInput);
    auto opts =
      vtk::DataObjectTreeOptions::VisitOnlyLeaves | vtk::DataObjectTreeOptions::TraverseSubTree;
    auto inputRange = vtk::Range(compositeInput, opts);
    auto outputRange = vtk::Range(compositeOutput, opts);
    auto inputBlock = inputRange.begin();
    auto outputBlock = outputRange.begin();
    while (inputBlock != inputRange.end() && outputBlock != outputRange.end())
    {
      int ret = this->ExecuteData(*inputBlock, *outputBlock, timeValue);
      if (ret != 1)
      {
        return ret;
      }
      inputBlock++;
      outputBlock++;
    }
    compositeOutput->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeValue);
    return 1;
  }

  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  if (input)
  {
    vtkDataObject* output = vtkDataObject::GetData(outputVector->GetInformationObject(0));
    return this->ExecuteData(input, output, timeValue);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkONNXInference::ExecuteData(vtkDataObject* input, vtkDataObject* output, double timeValue)
{
  output->ShallowCopy(input);

  vtkDataSetAttributes* outAttributes = output->GetAttributes(this->ArrayAssociation);
  if (!outAttributes)
  {
    vtkWarningMacro(
      "input (" << vtkLogIdentifier(output) << ") do not have the requested attribute ("
                << vtkDataObject::GetAssociationTypeAsString(this->ArrayAssociation) << ")");
    return 1;
  }

  if (!this->Internals->Session)
  {
    vtkErrorMacro("No model loaded or session not initialized.");
    return 0;
  }

  // Prepare model input
  Ort::Value inputTensor;
  std::vector<float> parameters;
  if (this->FieldArrayInput)
  {
    if (!this->GenerateInputTensorFromFieldArray(
          inputTensor, output->GetAttributes(this->ArrayAssociation)))
    {
      vtkErrorMacro("Could not generate the input tensor for ONNX Runtime.");
      return 0;
    }
  }
  else
  {
    if (!this->GenerateInputTensorFromParameters(parameters, inputTensor, timeValue))
    {
      return 0;
    }
  }
  int64_t numElements = input->GetNumberOfElements(this->ArrayAssociation);
  float* outData = nullptr;

  try
  {
    std::vector<Ort::Value> outputTensors = this->RunModel(inputTensor);

    // Retrieve output
    outData = outputTensors[0].GetTensorMutableData<float>();
    Ort::TensorTypeAndShapeInfo shapeInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
    std::vector<int64_t> outputShape = shapeInfo.GetShape();
    int64_t outputNumElements =
      std::accumulate(outputShape.begin(), outputShape.end(), 1LL, std::multiplies<>());

    if (numElements != outputNumElements / this->OutputDimension)
    {
      vtkErrorMacro("Model output number of elements does not match number of cells or points.");
      return 0;
    }
  }
  catch (const Ort::Exception& exception)
  {
    vtkErrorMacro(<< "Error during the ONNX inference. " << exception.what());
    return 0;
  }

  // Prepare vtkArray
  vtkNew<vtkFloatArray> outputArray;
  outputArray->SetName("PredictedField");
  outputArray->SetNumberOfComponents(this->OutputDimension);
  outputArray->SetNumberOfTuples(numElements);

  for (int64_t i = 0; i < numElements; ++i)
  {
    outputArray->SetTuple(i, &outData[i * this->OutputDimension]);
  }

  outAttributes->AddArray(outputArray);
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeValue);

  return 1;
}

//------------------------------------------------------------------------------
bool vtkONNXInference::ShouldGenerateTimeSteps()
{
  return !this->TimeStepValues.empty() && this->TimeStepIndex > -1;
}

//------------------------------------------------------------------------------
bool vtkONNXInference::GenerateInputTensorFromParameters(
  std::vector<float>& parameters, Ort::Value& inputTensor, double timeValue)
{
  parameters = this->InputParameters;
  if (this->TimeStepIndex >= 0 && this->TimeStepIndex < this->InputShape[0])
  {
    parameters[this->TimeStepIndex] = timeValue;
  }
  try
  {
    inputTensor = ::RawToTensor(parameters.data(), { this->InputShape[0] });
  }
  catch (const Ort::Exception& exception)
  {
    vtkErrorMacro(<< "Error during the input tensor creation. " << exception.what());
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkONNXInference::GenerateInputTensorFromFieldArray(
  Ort::Value& inputTensor, vtkDataSetAttributes* inAttributes)
{
  vtkDataArray* modelInput = inAttributes->GetArray(this->ProcessedFieldArrayName.c_str());
  if (!modelInput)
  {
    vtkErrorMacro(<< "No array named \"" << this->ProcessedFieldArrayName
                  << "\" was found in the input.");
    return false;
  }
  vtkFloatArray* floatModelInput = vtkFloatArray::SafeDownCast(modelInput);
  float* modelInputData;
  if (floatModelInput)
  {
    modelInputData = floatModelInput->GetPointer(0);
  }
  else
  {
    vtkErrorMacro(<< "Only input field of type vtkFloatArray can be used for prediction.");
    return false;
  }

  try
  {
    inputTensor = ::RawToTensor(modelInputData, this->InputShape);
  }
  catch (const Ort::Exception& exception)
  {
    vtkErrorMacro(<< "Error during the input tensor creation. " << exception.what());
    return false;
  }
  return true;
}

VTK_ABI_NAMESPACE_END
