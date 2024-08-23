// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUTexture.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUTexture);

//------------------------------------------------------------------------------
vtkWebGPUTexture::vtkWebGPUTexture() = default;

//------------------------------------------------------------------------------
vtkWebGPUTexture::~vtkWebGPUTexture() = default;

//------------------------------------------------------------------------------
void vtkWebGPUTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Extents X/Y/Z: " << this->Extents[0] << ", " << this->Extents[1] << ", "
     << this->Extents[2] << std::endl;

  os << indent << "Dimension: " << this->Dimension << std::endl;
  os << indent << "Format: " << this->Format << std::endl;
  os << indent << "Mode: " << this->Mode << std::endl;
  os << indent << "SampleType: " << this->SampleType << std::endl;
  os << indent << "MipLevelCount: " << this->MipLevelCount << std::endl;
}

//------------------------------------------------------------------------------
unsigned int vtkWebGPUTexture::GetBytesPerPixel() const
{
  switch (this->Format)
  {
    case vtkWebGPUTexture::TextureFormat::RGBA8_UNORM:
    case vtkWebGPUTexture::TextureFormat::BGRA8_UNORM:
      return 4;

    case vtkWebGPUTexture::TextureFormat::DEPTH_24_PLUS:
      return 3;

    case vtkWebGPUTexture::TextureFormat::DEPTH_24_PLUS_8_STENCIL:
      return 4;

    case vtkWebGPUTexture::TextureFormat::R32_FLOAT:
      return 4;

    default:
      vtkLog(
        ERROR, "Unhandled texture format in vtkWebGPUTexture::GetBytesPerPixel: " << this->Format);
  }

  return 0;
}

//------------------------------------------------------------------------------
unsigned int vtkWebGPUTexture::GetPixelComponentsCount() const
{
  switch (this->Format)
  {
    case vtkWebGPUTexture::TextureFormat::RGBA8_UNORM:
      return 4;

    case vtkWebGPUTexture::TextureFormat::R32_FLOAT:
      return 1;

    default:
      vtkLog(ERROR,
        "Unhandled texture format in vtkWebGPUTexture::GetPixelComponentsCount: " << this->Format);
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::GetSize(unsigned int& x, unsigned int& y, unsigned int& z) const
{
  x = this->Extents[0];
  y = this->Extents[1];
  z = this->Extents[2];
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::GetSize(unsigned int& x, unsigned int& y) const
{
  x = this->Extents[0];
  y = this->Extents[1];
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::GetSize(unsigned int* xyz)
{
  xyz[0] = this->Extents[0];
  xyz[1] = this->Extents[1];
  xyz[2] = this->Extents[2];
}

//------------------------------------------------------------------------------
unsigned int* vtkWebGPUTexture::GetSize()
{
  return this->Extents;
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::SetSize(unsigned int x, unsigned int y, unsigned int z)
{
  this->Extents[0] = x;
  this->Extents[1] = y;
  this->Extents[2] = z;
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::SetSize(unsigned int* xyz)
{
  this->Extents[0] = xyz[0];
  this->Extents[1] = xyz[1];
  this->Extents[2] = xyz[2];
}

VTK_ABI_NAMESPACE_END
