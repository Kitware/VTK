// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUHelpers.h"

#include "Private/vtkWebGPUComputePassInternals.h"
#include "vtkImageData.h"
#include "vtkPNGWriter.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePass.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkWebGPUHelpers::WriteComputeTextureToDisk(const std::string& filepath,
  vtkSmartPointer<vtkWebGPUComputePass> computePass, int textureIndex, int mipLevel, bool flipY)
{
  if (!computePass->Internals->CheckTextureIndex(textureIndex, "WriteComputeTextureToDisk"))
  {
    return;
  }

  // Callback data
  struct MapTextureData
  {
    int width, height;
    int dataType, nbComponents;
    bool flipY;

    std::string filepath;
  };

  // Callback for the data is mapped to copy the texture data in the std::vector
  vtkWebGPUComputePass::TextureMapAsyncCallback writeTextureToDiskCallback =
    [](const void* mappedTexture, int bytesPerRow, void* userdata)
  {
    MapTextureData* mapData = reinterpret_cast<MapTextureData*>(userdata);

    vtkNew<vtkImageData> pixelData;
    pixelData->SetDimensions(mapData->width, mapData->height, 1);
    pixelData->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
    for (int y = 0; y < mapData->height; y++)
    {
      int yIndex = y;
      if (mapData->flipY)
      {
        yIndex = mapData->height - 1 - y;
      }

      for (int x = 0; x < mapData->width; x++)
      {
        unsigned char* pixel =
          static_cast<unsigned char*>(pixelData->GetScalarPointer(x, yIndex, 0));
        int indexPadded = y * (bytesPerRow / 4) + x;

        switch (mapData->dataType)
        {
          case VTK_FLOAT:
          {
            const float* mappedDataFloat = static_cast<const float*>(mappedTexture);

            switch (mapData->nbComponents)
            {
              case 4:
                pixel[0] = static_cast<unsigned char>(mappedDataFloat[indexPadded * 4 + 0] * 255);
                pixel[1] = static_cast<unsigned char>(mappedDataFloat[indexPadded * 4 + 1] * 255);
                pixel[2] = static_cast<unsigned char>(mappedDataFloat[indexPadded * 4 + 2] * 255);
                pixel[3] = static_cast<unsigned char>(mappedDataFloat[indexPadded * 4 + 3] * 255);
                break;

              case 1:
                pixel[0] = static_cast<unsigned char>(mappedDataFloat[indexPadded * 1 + 0] * 255);
                pixel[1] = pixel[0];
                pixel[2] = pixel[0];
                pixel[3] = 255;
                break;

              default:
                break;
            }

            break;
          }

          case VTK_UNSIGNED_CHAR:
          {
            const unsigned char* mappedDataUnsignedChar =
              static_cast<const unsigned char*>(mappedTexture);

            switch (mapData->nbComponents)
            {
              case 4:
                pixel[0] = mappedDataUnsignedChar[indexPadded * 4 + 0];
                pixel[1] = mappedDataUnsignedChar[indexPadded * 4 + 1];
                pixel[2] = mappedDataUnsignedChar[indexPadded * 4 + 2];
                pixel[3] = mappedDataUnsignedChar[indexPadded * 4 + 3];
                break;

              case 1:
                pixel[0] = mappedDataUnsignedChar[indexPadded * 1 + 0];
                pixel[1] = pixel[0];
                pixel[2] = pixel[0];
                pixel[3] = 255;
                break;

              default:
                break;
            }

            break;
          }

          default:
            break;
        }
      }
    }

    vtkNew<vtkPNGWriter> writer;
    writer->SetFileName(mapData->filepath.c_str());
    writer->SetInputData(pixelData);
    writer->Write();

    delete mapData;
  };

  vtkSmartPointer<vtkWebGPUComputeTexture> texture = computePass->GetComputeTexture(textureIndex);

  int textureWidth = texture->GetWidth() / std::pow(2, mipLevel);
  int textureHeight = texture->GetHeight() / std::pow(2, mipLevel);

  MapTextureData* mapTextureData = new MapTextureData;
  mapTextureData->width = textureWidth;
  mapTextureData->height = textureHeight;
  mapTextureData->flipY = flipY;
  mapTextureData->dataType =
    vtkWebGPUHelpers::ComputeTextureFormatToVTKDataType(texture->GetFormat());
  mapTextureData->nbComponents = texture->GetPixelComponentsCount();
  mapTextureData->filepath = filepath;

  computePass->ReadTextureFromGPU(
    textureIndex, mipLevel, writeTextureToDiskCallback, mapTextureData);
}

//------------------------------------------------------------------------------
int vtkWebGPUHelpers::ComputeTextureFormatToVTKDataType(
  vtkWebGPUComputeTexture::TextureFormat format)
{
  switch (format)
  {
    case vtkWebGPUComputeTexture::TextureFormat::RGBA8_UNORM:
    case vtkWebGPUComputeTexture::TextureFormat::BGRA8_UNORM:
      return VTK_UNSIGNED_CHAR;

    case vtkWebGPUComputeTexture::TextureFormat::R32_FLOAT:
    case vtkWebGPUComputeTexture::TextureFormat::DEPTH_24_PLUS:
    case vtkWebGPUComputeTexture::TextureFormat::DEPTH_24_PLUS_8_STENCIL:
      return VTK_FLOAT;

    default:
      vtkLog(ERROR, "Unhandled texture format in ComputeTextureFormatToDataType: " << format);
      return -1;
  }
}

//------------------------------------------------------------------------------
std::string vtkWebGPUHelpers::StringViewToStdString(wgpu::StringView sv)
{
  if (sv.length == wgpu::kStrlen)
  {
    if (sv.IsUndefined())
    {
      return {};
    }
    return { sv.data };
  }
  return { sv.data, sv.length };
}
VTK_ABI_NAMESPACE_END
