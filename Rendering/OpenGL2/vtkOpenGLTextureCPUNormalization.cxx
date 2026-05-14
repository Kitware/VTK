// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLTextureCPUNormalization.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLTextureCPUNormalization);

//----------------------------------------------------------------------------
vtkOpenGLTextureCPUNormalization::vtkOpenGLTextureCPUNormalization()
{
  this->Mode = ConversionMode::CPU;
}

//----------------------------------------------------------------------------
void vtkOpenGLTextureCPUNormalization::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkOpenGLTextureCPUNormalization::ConvertUShortToFloat(const void* sourceData,
  size_t numValues, int numComps, unsigned int targetTexture, unsigned int width,
  unsigned int height)
{
  if (!sourceData || numValues == 0)
  {
    return false;
  }

  // Allocate or reuse buffer
  if (this->ConversionBuffer.size() < numValues)
  {
    this->ConversionBuffer.resize(numValues);
  }

  // Convert unsigned short to normalized float
  const unsigned short* src = static_cast<const unsigned short*>(sourceData);
  float* dst = this->ConversionBuffer.data();
  for (size_t i = 0; i < numValues; ++i)
  {
    dst[i] = src[i] / 65535.0f;
  }

  // Upload converted data
  glBindTexture(GL_TEXTURE_2D, targetTexture);
  GLenum format = GL_RED;
  if (numComps == 2)
  {
    format = GL_RG;
  }
  else if (numComps == 3)
  {
    format = GL_RGB;
  }
  else
  {
    format = GL_RGBA;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F + (numComps - 1), static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, format, GL_FLOAT, dst);

  vtkOpenGLStaticCheckErrorMacro("Failed to upload converted texture data");
  return true;
}

//----------------------------------------------------------------------------
bool vtkOpenGLTextureCPUNormalization::ConvertShortToFloat(const void* sourceData, size_t numValues,
  int numComps, unsigned int targetTexture, unsigned int width, unsigned int height)
{
  if (!sourceData || numValues == 0)
  {
    return false;
  }

  // Allocate or reuse buffer
  if (this->ConversionBuffer.size() < numValues)
  {
    this->ConversionBuffer.resize(numValues);
  }

  // Convert signed short to normalized float
  const short* src = static_cast<const short*>(sourceData);
  float* dst = this->ConversionBuffer.data();
  for (size_t i = 0; i < numValues; ++i)
  {
    dst[i] = src[i] / 32767.0f;
  }

  // Upload converted data
  glBindTexture(GL_TEXTURE_2D, targetTexture);
  GLenum format2 = GL_RED;
  if (numComps == 2)
  {
    format2 = GL_RG;
  }
  else if (numComps == 3)
  {
    format2 = GL_RGB;
  }
  else
  {
    format2 = GL_RGBA;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F + (numComps - 1), static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, format2, GL_FLOAT, dst);

  vtkOpenGLStaticCheckErrorMacro("Failed to upload converted texture data");
  return true;
}

VTK_ABI_NAMESPACE_END
