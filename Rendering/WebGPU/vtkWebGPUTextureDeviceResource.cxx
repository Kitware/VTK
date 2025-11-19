// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUTextureDeviceResource.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUTextureDeviceResource);

//------------------------------------------------------------------------------
vtkWebGPUTextureDeviceResource::vtkWebGPUTextureDeviceResource() = default;

//------------------------------------------------------------------------------
vtkWebGPUTextureDeviceResource::~vtkWebGPUTextureDeviceResource() = default;

//------------------------------------------------------------------------------
void vtkWebGPUTextureDeviceResource::PrintSelf(ostream& os, vtkIndent indent)
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
unsigned int vtkWebGPUTextureDeviceResource::GetBytesPerPixel() const
{
  switch (this->Format)
  {
    case R8_UNORM:
      return 1;
    case RG8_UNORM:
      return 2;
    case RGBA8_UNORM:
    case BGRA8_UNORM:
      return 4;
    case R16_UINT:
      return 2;
    case RG16_UINT:
      return 4;
    case RGBA16_UINT:
      return 8;
    case R32_FLOAT:
      return 4;
    case RG32_FLOAT:
      return 8;
    case RGBA32_FLOAT:
      return 16;
    case DEPTH_24_PLUS:
      return 3;
    case DEPTH_24_PLUS_8_STENCIL:
      return 4;
    default:
      vtkLog(ERROR,
        "Unhandled texture format in vtkWebGPUTextureDeviceResource::GetBytesPerPixel: "
          << this->Format);
      break;
  }

  return 0;
}

//------------------------------------------------------------------------------
unsigned int vtkWebGPUTextureDeviceResource::GetPixelComponentsCount() const
{
  switch (this->Format)
  {
    case R8_UNORM:
    case R16_UINT:
    case R32_FLOAT:
      return 1;
    case RG8_UNORM:
    case RG16_UINT:
    case RG32_FLOAT:
      return 2;
    case RGBA8_UNORM:
    case BGRA8_UNORM:
    case RGBA16_UINT:
    case RGBA32_FLOAT:
      return 4;
    case DEPTH_24_PLUS:
      return 1;
    case DEPTH_24_PLUS_8_STENCIL:
      return 2;
    default:
      vtkLog(ERROR,
        "Unhandled texture format in vtkWebGPUTextureDeviceResource::GetPixelComponentsCount: "
          << this->Format);
      break;
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureDeviceResource::GetSize(
  unsigned int& x, unsigned int& y, unsigned int& z) const
{
  x = this->Extents[0];
  y = this->Extents[1];
  z = this->Extents[2];
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureDeviceResource::GetSize(unsigned int& x, unsigned int& y) const
{
  x = this->Extents[0];
  y = this->Extents[1];
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureDeviceResource::GetSize(unsigned int* xyz)
{
  xyz[0] = this->Extents[0];
  xyz[1] = this->Extents[1];
  xyz[2] = this->Extents[2];
}

//------------------------------------------------------------------------------
unsigned int* vtkWebGPUTextureDeviceResource::GetSize()
{
  return this->Extents;
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureDeviceResource::SetSize(unsigned int x, unsigned int y, unsigned int z)
{
  this->Extents[0] = x;
  this->Extents[1] = y;
  this->Extents[2] = z;
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureDeviceResource::SetSize(unsigned int* xyz)
{
  this->Extents[0] = xyz[0];
  this->Extents[1] = xyz[1];
  this->Extents[2] = xyz[2];
}

VTK_ABI_NAMESPACE_END
