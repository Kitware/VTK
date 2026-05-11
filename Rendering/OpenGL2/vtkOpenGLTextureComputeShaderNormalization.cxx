// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLTextureComputeShaderNormalization.h"

#ifdef GL_COMPUTE_SHADER

#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLTextureCPUNormalization.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLTextureComputeShaderNormalization);

//----------------------------------------------------------------------------
vtkOpenGLTextureComputeShaderNormalization::vtkOpenGLTextureComputeShaderNormalization()
{
  this->Mode = ConversionMode::ComputeShader;
}

//----------------------------------------------------------------------------
vtkOpenGLTextureComputeShaderNormalization::~vtkOpenGLTextureComputeShaderNormalization()
{
  if (this->ComputeProgram)
  {
    glDeleteProgram(this->ComputeProgram);
  }
  if (this->IntermediateStorageTexture)
  {
    glDeleteTextures(1, &this->IntermediateStorageTexture);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLTextureComputeShaderNormalization::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOpenGLTextureComputeShaderNormalization::Initialize(vtkOpenGLRenderWindow* context)
{
  if (!context)
  {
    this->Mode = ConversionMode::Unsupported;
    return;
  }

  this->Context = context;
  context->MakeCurrent();

  if (!this->InitializeComputeProgram())
  {
    this->Mode = ConversionMode::Unsupported;
  }
}

//----------------------------------------------------------------------------
bool vtkOpenGLTextureComputeShaderNormalization::InitializeComputeProgram()
{
  // Compile compute shader that normalizes 16-bit integer to 32-bit float
  const char* computeShaderSource = R"(
#version 430
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, r16ui) uniform uimage2D inputImage;
layout(binding = 1, rgba32f) uniform image2D outputImage;

uniform float scaleFactor;

void main() {
  ivec2 coord = ivec2(gl_GlobalInvocationID.xy);

  // Read 16-bit unsigned integer
  uint value = imageLoad(inputImage, coord).x;

  // Normalize
  float normalized = float(value) * scaleFactor;

  // Write to output
  imageStore(outputImage, coord, vec4(normalized, 0.0, 0.0, 1.0));
}
)";

  GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(computeShader, 1, &computeShaderSource, nullptr);
  glCompileShader(computeShader);

  // Check compilation
  GLint compiled = 0;
  glGetShaderiv(computeShader, GL_COMPILE_STATUS, &compiled);
  if (!compiled)
  {
    vtkOpenGLStaticCheckErrorMacro("Failed to compile compute shader");
    glDeleteShader(computeShader);
    return false;
  }

  // Link program
  this->ComputeProgram = glCreateProgram();
  glAttachShader(this->ComputeProgram, computeShader);
  glLinkProgram(this->ComputeProgram);

  GLint linked = 0;
  glGetProgramiv(this->ComputeProgram, GL_LINK_STATUS, &linked);
  glDeleteShader(computeShader);

  if (!linked)
  {
    vtkOpenGLStaticCheckErrorMacro("Failed to link compute program");
    glDeleteProgram(this->ComputeProgram);
    this->ComputeProgram = 0;
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkOpenGLTextureComputeShaderNormalization::UploadToIntermediateTexture(const void* sourceData,
  size_t vtkNotUsed(numValues), int vtkNotUsed(numComps), unsigned int width, unsigned int height,
  GLenum vtkNotUsed(sourceFormat))
{
  // Create or reuse intermediate texture
  if (!this->IntermediateStorageTexture)
  {
    glGenTextures(1, &this->IntermediateStorageTexture);
  }

  glBindTexture(GL_TEXTURE_2D, this->IntermediateStorageTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Upload source data as integer texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, sourceData);

  vtkOpenGLStaticCheckErrorMacro("Failed to upload intermediate texture");
  return true;
}

//----------------------------------------------------------------------------
bool vtkOpenGLTextureComputeShaderNormalization::RunNormalizationCompute(
  int numComps, unsigned int targetTexture, unsigned int width, unsigned int height)
{
  glUseProgram(this->ComputeProgram);

  // Bind input texture
  glBindImageTexture(0, this->IntermediateStorageTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);

  // Bind output texture
  GLenum outputFormat = GL_R32F;
  if (numComps == 2)
  {
    outputFormat = GL_RG32F;
  }
  else if (numComps == 3)
  {
    outputFormat = GL_RGB32F;
  }
  else
  {
    outputFormat = GL_RGBA32F;
  }
  glBindImageTexture(1, targetTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, outputFormat);

  // Set scale factor
  GLint scaleLoc = glGetUniformLocation(this->ComputeProgram, "scaleFactor");
  glUniform1f(scaleLoc, 1.0f / 65535.0f);

  // Dispatch compute shader
  GLsizei groupSize = 8;
  glDispatchCompute((width + groupSize - 1) / groupSize, (height + groupSize - 1) / groupSize, 1);

  // Wait for computation
  glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);

  vtkOpenGLStaticCheckErrorMacro("Compute shader normalization failed");
  return true;
}

//----------------------------------------------------------------------------
bool vtkOpenGLTextureComputeShaderNormalization::ConvertUShortToFloat(const void* sourceData,
  size_t numValues, int numComps, unsigned int targetTexture, unsigned int width,
  unsigned int height)
{
  if (!this->ComputeProgram || !sourceData || numValues == 0)
  {
    return false;
  }

  // Pre-allocate target texture
  GLenum outputFormat = GL_R32F;
  if (numComps == 2)
  {
    outputFormat = GL_RG32F;
  }
  else if (numComps == 3)
  {
    outputFormat = GL_RGB32F;
  }
  else
  {
    outputFormat = GL_RGBA32F;
  }
  glBindTexture(GL_TEXTURE_2D, targetTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, outputFormat, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, GL_RED, GL_FLOAT, nullptr);

  // Upload to intermediate R16UI texture
  if (!this->UploadToIntermediateTexture(
        sourceData, numValues, numComps, width, height, GL_RED_INTEGER))
  {
    return false;
  }

  // Run compute shader
  return this->RunNormalizationCompute(numComps, targetTexture, width, height);
}

//----------------------------------------------------------------------------
bool vtkOpenGLTextureComputeShaderNormalization::ConvertShortToFloat(const void* sourceData,
  size_t numValues, int numComps, unsigned int targetTexture, unsigned int width,
  unsigned int height)
{
  if (!this->ComputeProgram || !sourceData || numValues == 0)
  {
    return false;
  }

  // Signed normalization requires a separate compute shader variant.
  // Delegate to CPU conversion for now.
  auto cpuConverter = vtkSmartPointer<vtkOpenGLTextureCPUNormalization>::New();
  return cpuConverter->ConvertShortToFloat(
    sourceData, numValues, numComps, targetTexture, width, height);
}

VTK_ABI_NAMESPACE_END

#endif // GL_COMPUTE_SHADER
