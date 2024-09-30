// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test ensures that setting a pipeline up, releasing its resources and setting it up again
 * works as expected and the pipeline produces expected results after it's been released +
 * reconfigured.
 */

#include <numeric> // For std::iota
#include <vector>

#include "TestComputePassShader.h"
#include "vtkDataArrayRange.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputePipeline.h"

namespace
{
constexpr int DATA_SIZE = 128;
}

int TestComputePipelineRelease(int, char*[])
{
  std::vector<int> inputVector1Values(::DATA_SIZE);
  std::iota(inputVector1Values.begin(), inputVector1Values.end(), 0);

  std::vector<float> inputVector2Values(::DATA_SIZE);
  std::iota(inputVector2Values.begin(), inputVector2Values.end(), 0.0f);

  // Creating the input buffer to the compute shader
  vtkNew<vtkWebGPUComputeBuffer> inputValues1Buffer;
  inputValues1Buffer->SetLabel("First input buffer");
  inputValues1Buffer->SetGroup(0);
  inputValues1Buffer->SetBinding(0);
  inputValues1Buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputValues1Buffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
  inputValues1Buffer->SetData(inputVector1Values);

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
  outputBuffer->SetByteSize(::DATA_SIZE * sizeof(float));

  // Creating the compute pipeline
  vtkNew<vtkWebGPUComputePipeline> multiplyComputePipeline;

  // Creating the compute pass
  vtkSmartPointer<vtkWebGPUComputePass> multiplyComputePass =
    multiplyComputePipeline->CreateComputePass();
  multiplyComputePass->SetShaderSource(TestComputePassShader);
  multiplyComputePass->SetShaderEntryPoint("computeFunction");
  multiplyComputePass->AddBuffer(inputValues1Buffer);
  multiplyComputePass->AddBuffer(inputValues2Buffer);
  multiplyComputePass->AddBuffer(outputBuffer);

  // Simulating work with the pipeline
  int nbXGroups = std::ceil(::DATA_SIZE / 32.0f);
  multiplyComputePass->SetWorkgroups(nbXGroups, 1, 1);
  multiplyComputePass->Dispatch();
  multiplyComputePipeline->Update();

  // Releasing the resources of the pipeline
  multiplyComputePipeline->ReleaseResources();

  vtkNew<vtkWebGPUConfiguration> configuration;
  configuration->Initialize();
  multiplyComputePipeline->SetWGPUConfiguration(configuration);

  // Setting the pipeline up again
  multiplyComputePass = multiplyComputePipeline->CreateComputePass();
  multiplyComputePass->SetShaderSource(TestComputePassShader);
  multiplyComputePass->SetShaderEntryPoint("computeFunction");
  multiplyComputePass->AddBuffer(inputValues1Buffer);
  multiplyComputePass->AddBuffer(inputValues2Buffer);

  // Getting the index of the output buffer for later mapping with ReadBufferFromGPU()
  int outputBufferIndex = multiplyComputePass->AddBuffer(outputBuffer);

  nbXGroups = std::ceil(::DATA_SIZE / 32.0f);
  multiplyComputePass->SetWorkgroups(nbXGroups, 1, 1);
  multiplyComputePass->Dispatch();

  // Output buffer for the result data
  std::vector<float> outputData;
  outputData.resize(::DATA_SIZE);

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
  multiplyComputePass->ReadBufferFromGPU(outputBufferIndex, onBufferMapped, &outputData);
  // Update() to actually execute WebGPU commands. Without this, the compute shader won't execute
  // and the data that we try to map here may not be available yet
  multiplyComputePipeline->Update();

  for (int i = 0; i < ::DATA_SIZE; i++)
  {
    auto input1Value = inputVector1Values[i];
    auto input2Value = inputVector2Values[i];

    // The compute shader is expected to multiply both inputs
    if (input1Value * input2Value != outputData[i])
    {
      vtkLog(ERROR,
        "Incorrect result from the mapped buffer. Expected " << input1Value * input2Value
                                                             << " but got " << outputData[i]);

      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
