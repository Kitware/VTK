// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test multiplies element-wise two vectors and then adds 42 to the result using two compute
 * passes. One for each operation.
 */

#include <numeric> // For std::iota
#include <vector>

#include "TestComputePassChainedShader.h"
#include "TestComputePassShader.h"
#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputePipeline.h"

namespace
{
constexpr int DATA_SIZE = 128;
} // namespace

int TestComputePassChained(int, char*[])
{
  // This first vector will be using a vtkDataArray as its data source
  vtkNew<vtkIntArray> inputDataArray;
  inputDataArray->SetNumberOfComponents(1);
  inputDataArray->Allocate(DATA_SIZE);
  for (int i = 0; i < DATA_SIZE; i++)
  {
    inputDataArray->InsertNextValue(i);
  }

  // The second vector uses a std::vector. The two vectors use different types (they are not both
  // std::vector / vtkDataArray) for testing purposes as both std::vector and vtkDataArray are
  // supposed to be supported by the compute pipeline
  std::vector<float> inputVector2Values(DATA_SIZE);
  std::iota(inputVector2Values.begin(), inputVector2Values.end(), 0.0f);

  // Creating the input buffer to the compute shader
  vtkNew<vtkWebGPUComputeBuffer> inputValues1Buffer;
  inputValues1Buffer->SetLabel("First input buffer");
  inputValues1Buffer->SetGroup(0);
  inputValues1Buffer->SetBinding(0);
  inputValues1Buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputValues1Buffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);
  inputValues1Buffer->SetData(inputDataArray);

  // Creating the second input buffer for the compute shader
  vtkNew<vtkWebGPUComputeBuffer> inputValues2Buffer;
  inputValues2Buffer->SetLabel("Second input buffer");
  inputValues2Buffer->SetGroup(0);
  inputValues2Buffer->SetBinding(1);
  inputValues2Buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputValues2Buffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
  inputValues2Buffer->SetData(inputVector2Values);

  // Creating the output buffer to the compute shader
  vtkNew<vtkWebGPUComputeBuffer> outputBuffer;
  outputBuffer->SetLabel("Output buffer");
  outputBuffer->SetGroup(0);
  outputBuffer->SetBinding(2);
  outputBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputBuffer->SetByteSize(DATA_SIZE * sizeof(float));

  // Creating the compute pipeline
  vtkNew<vtkWebGPUComputePipeline> computePipeline;

  // Creating the compute pass
  vtkSmartPointer<vtkWebGPUComputePass> multiplyComputePass = computePipeline->CreateComputePass();
  multiplyComputePass->SetShaderSource(TestComputePassShader);
  multiplyComputePass->SetShaderEntryPoint("computeFunction");
  multiplyComputePass->AddBuffer(inputValues1Buffer);
  multiplyComputePass->AddBuffer(inputValues2Buffer);
  // Getting the index of the output buffer for later mapping with ReadBufferFromGPU()
  multiplyComputePass->AddBuffer(outputBuffer);

  vtkSmartPointer<vtkWebGPUComputePass> addComputePass = computePipeline->CreateComputePass();
  addComputePass->SetShaderSource(TestComputePassChainedShader);
  addComputePass->SetShaderEntryPoint("computeFunctionAdd");
  // Because we're only using 1 buffer for the second pass, we'll set it to (0, 0) group/binding.
  // This is not necessary but this is to keep things clean instead of having one single buffer but
  // that is set to (0, 2) (because outputBuffer was bound to (0, 2) by the first compute pass).
  outputBuffer->SetGroup(0);
  outputBuffer->SetBinding(0);
  // Using the output buffer of the first as the input and output (we're going to read and write
  // from it) of the second pass
  int outputBufferIndex = addComputePass->AddBuffer(outputBuffer);

  int nbXGroups = std::ceil(DATA_SIZE / 32.0f);

  // Dispatching the compute passes with nbXGroups
  multiplyComputePass->SetWorkgroups(nbXGroups, 1, 1);
  multiplyComputePass->Dispatch();

  addComputePass->SetWorkgroups(nbXGroups, 1, 1);
  addComputePass->Dispatch();

  // Output buffer for the result data
  std::vector<float> outputData;
  outputData.resize(DATA_SIZE);

  auto onBufferMapped = [](const void* mappedData, void* userdata)
  {
    std::vector<float>* out = reinterpret_cast<std::vector<float>*>(userdata);
    vtkIdType elementCount = out->size();

    const float* mappedDataTyped = static_cast<const float*>(mappedData);
    for (int i = 0; i < elementCount; i++)
    {
      (*out)[i] = mappedDataTyped[i];
    }
  };

  // Mapping the buffer on the CPU to get the results from the GPU
  addComputePass->ReadBufferFromGPU(outputBufferIndex, onBufferMapped, &outputData);
  // Update() to actually execute WebGPU commands. Without this, the compute shader won't execute
  // and the data that we try to map here may not be available yet
  computePipeline->Update();

  for (int i = 0; i < DATA_SIZE; i++)
  {
    auto input1Value = inputDataArray->GetValue(i);
    auto input2Value = inputVector2Values[i];

    // The compute shader is expected to multiply both inputs
    if (input1Value * input2Value + 42 != outputData[i])
    {
      vtkLog(ERROR, "Incorrect result from the mapped buffer");

      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
