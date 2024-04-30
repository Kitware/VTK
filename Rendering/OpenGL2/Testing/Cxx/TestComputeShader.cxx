// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNew.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtk_glew.h"

namespace
{
// number of threads per group
// same as local_size_x in the shader
constexpr unsigned int LocalSize = 64;

// number of groups
constexpr unsigned int GroupSize = 1024;

// total number of elements
constexpr unsigned int TotalSize = LocalSize * GroupSize;
}

int TestComputeShader(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
#ifdef GL_COMPUTE_SHADER
  std::string shaderCode = R"==(
#version 430
layout(local_size_x = 64) in;

layout(std430, binding = 0) writeonly buffer ValuesBuffer
{
  uint values[];
};

void main() {
  uint i = gl_GlobalInvocationID.x;
  values[i] = i;
}
)==";

  // we need an OpenGL context
  vtkNew<vtkRenderWindow> renWin;
  renWin->OffScreenRenderingOn();
  renWin->Start();

  if (!vtkShader::IsComputeShaderSupported())
  {
    std::cerr << "Compute shaders are not supported on this system, skipping the test.\n";
    return EXIT_SUCCESS;
  }

  vtkNew<vtkShader> shader;
  shader->SetSource(shaderCode);
  shader->SetType(vtkShader::Compute);

  vtkNew<vtkShaderProgram> program;
  program->SetComputeShader(shader);

  vtkOpenGLRenderWindow* oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  if (!oglRenWin)
  {
    std::cerr << "Cannot create an OpenGL window" << std::endl;
    return EXIT_FAILURE;
  }

  vtkOpenGLShaderCache* shaderCache = oglRenWin->GetShaderCache();
  shaderCache->ReadyShaderProgram(program);

  std::array<unsigned int, ::TotalSize> values;

  // allocate the same size on the GPU
  vtkNew<vtkOpenGLBufferObject> buffer;
  buffer->Allocate(::TotalSize * sizeof(unsigned int), vtkOpenGLBufferObject::ArrayBuffer,
    vtkOpenGLBufferObject::DynamicCopy);

  buffer->BindShaderStorage(0);

  // fill the buffer on the GPU using the compute shader
  glDispatchCompute(::GroupSize, 1, 1);
  glMemoryBarrier(GL_ALL_BARRIER_BITS);

  // download from the GPU
  buffer->Download(values.data(), ::TotalSize);

  // check values
  for (unsigned int i = 0; i < ::TotalSize; i++)
  {
    if (values[i] != i)
    {
      std::cerr << "Value at index " << i << " is " << values[i] << std::endl;
      return EXIT_FAILURE;
    }
  }
#else
  (void)TotalSize;
#endif

  return EXIT_SUCCESS;
}
