// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputePipeline.h"

#include <iostream>
#include <string>
#include <vector>

namespace
{
const char* BrokenShader = R"(@group(0) @binding(0) var<storage, read_write> values: array<f32>;

@compute @workgroup_size(1)
fn computeFunction(@builtin(global_invocation_id) id: vec3<u32>)
{
  let index: u32 = 1.5;
  values[id.x] = f32(index);
}
)";

void CollectMessage(void* userData, const vtkLogger::Message& message)
{
  static_cast<std::vector<std::string>*>(userData)->emplace_back(message.message);
}
}

int TestShaderCompilationMessages(int, char*[])
{
  std::vector<std::string> messages;
  vtkLogger::AddCallback(
    "TestShaderCompilationMessages", &CollectMessage, &messages, vtkLogger::VERBOSITY_WARNING);

  vtkNew<vtkWebGPUComputeBuffer> buffer;
  buffer->SetGroup(0);
  buffer->SetBinding(0);
  buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);
  buffer->SetByteSize(4 * sizeof(float));

  vtkNew<vtkWebGPUComputePipeline> pipeline;
  auto pass = pipeline->CreateComputePass();
  pass->SetShaderSource(BrokenShader);
  pass->SetShaderEntryPoint("computeFunction");
  pass->AddBuffer(buffer);
  pass->SetWorkgroups(1, 1, 1);
  pass->Dispatch();
  pipeline->Update();

  vtkLogger::RemoveCallback("TestShaderCompilationMessages");

  for (const auto& message : messages)
  {
    if (message.find("WGSL error at line 6") != std::string::npos &&
      message.find(">6:   let index: u32 = 1.5;") != std::string::npos)
    {
      return EXIT_SUCCESS;
    }
  }
  std::cerr << "The compilation error was not reported with its line and source." << std::endl;
  for (const auto& message : messages)
  {
    std::cerr << "  logged: " << message << std::endl;
  }
  return EXIT_FAILURE;
}
