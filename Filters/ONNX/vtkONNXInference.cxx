// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkONNXInference.h"
#include "Private/vtkONNXInferenceInternals.h"

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <onnxruntime_cxx_api.h>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkONNXInference);

namespace
{
//------------------------------------------------------------------------------
Ort::Value VecToTensor(std::vector<float>& data, int64_t shape)
{
  Ort::MemoryInfo memInfo =
    Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
  Ort::Value tensor = Ort::Value::CreateTensor<float>(memInfo, data.data(), data.size(), &shape, 1);
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
  os << indent << "InputSize: " << this->InputSize << endl;
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
  this->ModelFile = file;
  this->Initialized = false;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetTimeStepValue(vtkIdType idx, double timeStepValue)
{
  if (idx < 0 || static_cast<size_t>(idx) > this->TimeStepValues.size())
  {
    vtkErrorMacro("Time step index is out of bounds.");
    return;
  }
  this->TimeStepValues[idx] = timeStepValue;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetNumberOfTimeStepValues(vtkIdType nb)
{
  this->TimeStepValues.resize(nb);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::ClearTimeStepValues()
{
  this->TimeStepValues.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetInputParameter(vtkIdType idx, float inputParameter)
{
  if (idx < 0 || static_cast<size_t>(idx) > this->InputParameters.size())
  {
    vtkErrorMacro("Input parameter index is out of bounds.");
    return;
  }
  this->InputParameters[idx] = inputParameter;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::SetNumberOfInputParameters(vtkIdType nb)
{
  this->InputParameters.resize(nb);
  this->InputSize = nb;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkONNXInference::ClearInputParameters()
{
  this->InputParameters.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkONNXInference::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (this->TimeStepValues.empty())
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  else
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
  vtkUnstructuredGrid* input = vtkUnstructuredGrid::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(input);

  if (!this->Initialized)
  {
    this->InitializeSession();
  }

  if (!this->Internals->Session)
  {
    vtkErrorMacro("No model loaded or session not initialized.");
    return 0;
  }

  // Time handling
  float timeValue = static_cast<float>(outputVector->GetInformationObject(0)->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));

  // Prepare and run model
  std::vector<float> parameters = this->InputParameters;
  if (this->TimeStepIndex >= 0 && this->TimeStepIndex < this->InputSize)
  {
    parameters[this->TimeStepIndex] = timeValue;
  }

  int64_t numElements = input->GetNumberOfElements(this->ArrayAssociation);
  float* outData = nullptr;

  try
  {
    Ort::Value inputTensor = ::VecToTensor(parameters, this->InputSize);
    std::vector<Ort::Value> outputTensors = this->RunModel(inputTensor);

    // Retrieve output
    outData = outputTensors[0].GetTensorMutableData<float>();
    Ort::TensorTypeAndShapeInfo shapeInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
    std::vector<int64_t> outputShape = shapeInfo.GetShape();

    if (outputShape.size() == 1)
    {
      if (numElements != outputShape[0] / this->OutputDimension)
      {
        vtkErrorMacro("Model output number of elements does not match number of cells or points.");
        return 0;
      }
    }
    else
    {
      vtkErrorMacro("Output shape size should be 1. Here it is: " << outputShape.size());
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

  output->GetAttributes(this->ArrayAssociation)->AddArray(outputArray);
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeValue);

  return 1;
}
VTK_ABI_NAMESPACE_END
