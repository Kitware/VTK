// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUTexture.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArrayMeta.h"
#include "vtkDataArrayRange.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkTexture.h"
#include "vtkWebGPURenderTextureDeviceResource.h"
#include "vtkWebGPURenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
struct ColorCopy
{
  template <typename ArrayType>
  void operator()(ArrayType* dataArray, void*& outDataPlane)
  {
    using ValueType = typename vtk::GetAPIType<ArrayType>;
    const auto srcRange = vtk::DataArrayValueRange(dataArray);
    outDataPlane = new unsigned char[srcRange.size() * sizeof(ValueType)];
    auto* dstPtr = reinterpret_cast<ValueType*>(outDataPlane);
    std::copy(srcRange.begin(), srcRange.end(), dstPtr);
  }
};
}
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUTexture);

//------------------------------------------------------------------------------
vtkWebGPUTexture::vtkWebGPUTexture()
  : RenderWindow(nullptr)
  , TextureIndex(vtkWebGPURenderTextureCache::INVALID_TEXTURE_INDEX)
{
}

//------------------------------------------------------------------------------
vtkWebGPUTexture::~vtkWebGPUTexture()
{
  if (this->RenderWindow)
  {
    this->ReleaseGraphicsResources(this->RenderWindow);
    this->RenderWindow = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkWebGPUTexture::IsValidTextureIndex()
{
  return this->TextureIndex != vtkWebGPURenderTextureCache::INVALID_TEXTURE_INDEX;
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::Render(vtkRenderer* renderer)
{
  this->Superclass::Render(renderer);
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::Load(vtkRenderer* renderer)
{
  vtkLog(TRACE, "Loading texture host resource=" << this << " to renderer=" << renderer);
  auto* wgpuRenderWindow = static_cast<vtkWebGPURenderWindow*>(renderer->GetRenderWindow());
  if (!wgpuRenderWindow)
  {
    vtkErrorMacro("No WebGPU render window found.");
    return;
  }
  auto* input = this->GetInput();
  if (!input)
  {
    vtkErrorMacro("No input data found.");
    return;
  }
  std::vector<vtkImageData*> inputs;
  vtkMTimeType inputTime = input->GetMTime();
  int dims[3] = { 0, 0, 0 };
  input->GetDimensions(dims);
  auto* firstInputScalars = this->GetInputArrayToProcess(0, input);
  if (!firstInputScalars)
  {
    vtkErrorMacro("No scalar values found for texture input 0");
    return;
  }
  int numComponents = firstInputScalars->GetNumberOfComponents();
  int dataType = firstInputScalars->GetDataType();
  if (this->CubeMap)
  {
    for (int i = 1; i < this->GetNumberOfInputPorts(); ++i)
    {
      vtkImageData* image = vtkImageData::SafeDownCast(this->GetInputDataObject(i, 0));
      if (!image)
      {
        vtkErrorMacro("No input data found for texture input " << i);
        continue;
      }
      inputTime = std::max(inputTime, image->GetMTime());
      inputs.emplace_back(image);
      int otherDims[3] = { 0, 0, 0 };
      image->GetDimensions(otherDims);
      if (otherDims[0] != dims[0] || otherDims[1] != dims[1] || otherDims[2] != dims[2])
      {
        vtkErrorMacro("All cube map faces must have the same dimensions");
        return;
      }
      auto* otherScalars = this->GetInputArrayToProcess(i, image);
      if (!otherScalars)
      {
        vtkErrorMacro("No scalar values found for texture input " << i);
        return;
      }
      if (otherScalars->GetDataType() != dataType)
      {
        vtkErrorMacro("All cube map faces must have the same scalar data type");
        return;
      }
      if (otherScalars->GetNumberOfComponents() != numComponents)
      {
        vtkErrorMacro("All cube map faces must have the same number of scalar components");
        return;
      }
    }
    if (inputs.size() < 6)
    {
      vtkErrorMacro("Cube Maps require 6 inputs");
      return;
    }
  }
  else
  {
    inputs.emplace_back(input);
  }
  if (this->RenderWindow == nullptr && this->LoadTime.GetMTime() > this->GetMTime())
  {
    vtkErrorMacro("A render window was deleted without releasing graphics resources");
  }

  bool doReload = this->GetMTime() > this->LoadTime.GetMTime(); // if this object changed
  doReload |= (inputTime > this->LoadTime.GetMTime());          // if the input changed
  doReload |= (this->GetLookupTable() &&
    this->GetLookupTable()->GetMTime() > this->LoadTime.GetMTime()); // if the LUT changed
  doReload = (this->RenderWindow == nullptr ||
    (wgpuRenderWindow->GetGenericContext() !=
      this->RenderWindow->GetGenericContext())); // if the render window changed
  doReload |= (wgpuRenderWindow->GetWGPUConfiguration()->GetMTime() >
    this->LoadTime); // if wgpu configuration changed
  if (doReload)
  {
    this->RenderWindow = wgpuRenderWindow;

    auto [dimension, size] = this->GetTextureDimensionAndSize(input);
    auto* wgpuTextureCache = wgpuRenderWindow->GetWGPUTextureCache();
    vtkSmartPointer<vtkWebGPURenderTextureDeviceResource> deviceResource;
    if (wgpuTextureCache)
    {
      // get current device resource when texture index is valid
      if (this->IsValidTextureIndex())
      {
        deviceResource = wgpuTextureCache->GetRenderTexture(this->TextureIndex);
      }
      if (deviceResource == nullptr)
      {
        // Create and add to cache
        deviceResource = vtkSmartPointer<vtkWebGPURenderTextureDeviceResource>::New();
        deviceResource->SetLabel(this->GetObjectDescription());
        this->TextureIndex = wgpuTextureCache->AddRenderTexture(deviceResource);
      }
      if (!this->IsValidTextureIndex())
      {
        vtkErrorMacro("Failed to add texture to texture cache.");
        return;
      }
    }
    deviceResource->SetDimension(dimension);
    deviceResource->SetSize(size.data());

    std::vector<void*> dataPlanes(inputs.size(), nullptr);
    std::vector<bool> needToDeleteDataPlane(inputs.size(), false);
    // Prepare data planes
    for (size_t i = 0; i < inputs.size(); ++i)
    {
      auto* imageData = inputs[i];
      if (!imageData)
      {
        vtkErrorMacro("No input data found for texture input " << i);
        continue;
      }
      auto* inputScalars = this->GetInputArrayToProcess(static_cast<int>(i), imageData);
      if (!inputScalars)
      {
        vtkErrorMacro("No scalar values found for texture input " << i);
        continue;
      }
      if (this->CanDataPlaneBeUsedDirectlyAsColors(inputScalars))
      {
        bool directAccessSuccess = false;
        switch (inputScalars->GetDataType())
        {
          case VTK_UNSIGNED_CHAR:
            if (auto* asUCharAoSArray =
                  vtkArrayDownCast<vtkAOSDataArrayTemplate<unsigned char>>(inputScalars))
            {
              dataPlanes[i] = reinterpret_cast<void*>(asUCharAoSArray->GetPointer(0));
              directAccessSuccess = true;
            }
            break;
          case VTK_UNSIGNED_SHORT:
            if (auto* asUShortAoSArray =
                  vtkArrayDownCast<vtkAOSDataArrayTemplate<unsigned short>>(inputScalars))
            {
              dataPlanes[i] = reinterpret_cast<void*>(asUShortAoSArray->GetPointer(0));
              directAccessSuccess = true;
            }
            break;
          case VTK_FLOAT:
            if (auto* asFloatAoSArray =
                  vtkArrayDownCast<vtkAOSDataArrayTemplate<float>>(inputScalars))
            {
              dataPlanes[i] = reinterpret_cast<void*>(asFloatAoSArray->GetPointer(0));
              directAccessSuccess = true;
            }
            break;
          default:
            vtkErrorMacro("Unsupported data type for direct color texture: " << dataType);
            break;
        }
        if (!directAccessSuccess)
        {
          using ScalarTypeList = vtkTypeList::Create<unsigned char, unsigned short, float>;
          using DispatchT = vtkArrayDispatch::DispatchByValueType<ScalarTypeList>;
          ColorCopy extractor;
          if (!DispatchT::Execute(inputScalars, extractor, dataPlanes[i]))
          {
            vtkErrorMacro("Failed to extract direct color data for texture input " << i);
            continue;
          }
          needToDeleteDataPlane[i] = true;
        }
      }
      else
      {
        dataPlanes[i] = this->MapScalarsToColors(inputScalars);
        if (!dataPlanes[i])
        {
          vtkErrorMacro("Failed to map scalars for texture input " << i);
          continue;
        }
        dataType = VTK_UNSIGNED_CHAR;
        numComponents = 4;
      }
    }
    auto format = this->GetTextureFormatFromImageData(numComponents, dataType);
    deviceResource->SetFormat(format);
    if (this->Interpolate)
    {
      deviceResource->SetSamplerBindingType(
        vtkWebGPURenderTextureDeviceResource::SamplerMode::FILTERING);
      deviceResource->SetMagFilter(vtkWebGPURenderTextureDeviceResource::FilterMode::LINEAR);
      const int levels = floor(log2(vtkMath::Max(size[0], size[1]))) + 1;
      if (this->Mipmap && levels > 1 && (!this->CubeMap && !this->UseSRGBColorSpace))
      {
        deviceResource->SetMinFilter(vtkWebGPURenderTextureDeviceResource::FilterMode::LINEAR);
        deviceResource->SetMipmapFilter(vtkWebGPURenderTextureDeviceResource::FilterMode::LINEAR);
        deviceResource->SetMipLevelCount(levels - 1);
        deviceResource->SetMaxAnisotropy(this->MaximumAnisotropicFiltering);
      }
      else
      {
        deviceResource->SetMinFilter(vtkWebGPURenderTextureDeviceResource::FilterMode::LINEAR);
      }
    }
    else
    {
      deviceResource->SetSamplerBindingType(
        vtkWebGPURenderTextureDeviceResource::SamplerMode::NON_FILTERING);
      deviceResource->SetMagFilter(vtkWebGPURenderTextureDeviceResource::FilterMode::NEAREST);
      deviceResource->SetMinFilter(vtkWebGPURenderTextureDeviceResource::FilterMode::NEAREST);
    }
    switch (this->GetWrap())
    {
      case ClampToEdge:
      case ClampToBorder:
        deviceResource->SetAddressModeU(
          vtkWebGPURenderTextureDeviceResource::AddressMode::CLAMP_TO_EDGE);
        deviceResource->SetAddressModeV(
          vtkWebGPURenderTextureDeviceResource::AddressMode::CLAMP_TO_EDGE);
        deviceResource->SetAddressModeW(
          vtkWebGPURenderTextureDeviceResource::AddressMode::CLAMP_TO_EDGE);
        break;
      case Repeat:
        deviceResource->SetAddressModeU(vtkWebGPURenderTextureDeviceResource::AddressMode::REPEAT);
        deviceResource->SetAddressModeV(vtkWebGPURenderTextureDeviceResource::AddressMode::REPEAT);
        deviceResource->SetAddressModeW(vtkWebGPURenderTextureDeviceResource::AddressMode::REPEAT);
        break;
      case MirroredRepeat:
        deviceResource->SetAddressModeU(
          vtkWebGPURenderTextureDeviceResource::AddressMode::MIRROR_REPEAT);
        deviceResource->SetAddressModeV(
          vtkWebGPURenderTextureDeviceResource::AddressMode::MIRROR_REPEAT);
        deviceResource->SetAddressModeW(
          vtkWebGPURenderTextureDeviceResource::AddressMode::MIRROR_REPEAT);
        break;
      default:
        vtkErrorMacro("Unknown wrap mode: " << this->GetWrap());
        break;
    }
    deviceResource->SetMode(vtkWebGPUTextureDeviceResource::READ_WRITE_STORAGE);
    deviceResource->SendToWebGPUDevice(
      dataPlanes, wgpuRenderWindow->GetWGPUConfiguration(), this->CubeMap);
    for (size_t i = 0; i < inputs.size(); ++i)
    {
      if (needToDeleteDataPlane[i])
      {
        delete[] static_cast<unsigned char*>(dataPlanes[i]);
        dataPlanes[i] = nullptr;
      }
    }
    this->LoadTime.Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUTexture::CanDataPlaneBeUsedDirectlyAsColors(vtkDataArray* inputScalars)
{
  if (!inputScalars)
  {
    return false;
  }

  int numComponents = inputScalars->GetNumberOfComponents();
  int dataType = inputScalars->GetDataType();

  switch (dataType)
  {
    case VTK_UNSIGNED_CHAR:
      return this->ColorMode != VTK_COLOR_MODE_MAP_SCALARS &&
        (numComponents == 3 || numComponents == 4);
    case VTK_UNSIGNED_SHORT:
      return this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS &&
        (numComponents == 3 || numComponents == 4);
    case VTK_FLOAT:
      return this->ColorMode == VTK_COLOR_MODE_DIRECT_SCALARS &&
        (numComponents == 3 || numComponents == 4);
    default:
      return false;
  }
}

//------------------------------------------------------------------------------
std::tuple<vtkWebGPURenderTextureDeviceResource::TextureDimension, std::array<unsigned int, 3>>
vtkWebGPUTexture::GetTextureDimensionAndSize(vtkImageData* imageData)
{
  std::array<unsigned int, 3> size = { 1, 1, 1 };
  int dims[3] = { 0, 0, 0 };
  imageData->GetDimensions(dims);
  vtkWebGPURenderTextureDeviceResource::TextureDimension dimension;

  if (dims[2] > 1)
  {
    dimension = vtkWebGPURenderTextureDeviceResource::DIMENSION_3D;
    size[0] = static_cast<unsigned int>(dims[0]);
    size[1] = static_cast<unsigned int>(dims[1]);
    size[2] = static_cast<unsigned int>(dims[2]);
  }
  else if (dims[1] > 1)
  {
    dimension = vtkWebGPURenderTextureDeviceResource::DIMENSION_2D;
    size[0] = static_cast<unsigned int>(dims[0]);
    size[1] = static_cast<unsigned int>(dims[1]);
  }
  else
  {
    dimension = vtkWebGPURenderTextureDeviceResource::DIMENSION_1D;
    size[0] = static_cast<unsigned int>(dims[0]);
  }

  return std::make_tuple(dimension, size);
}

//------------------------------------------------------------------------------
vtkWebGPURenderTextureDeviceResource::TextureFormat vtkWebGPUTexture::GetTextureFormatFromImageData(
  int numComponents, int dataType)
{
  switch (numComponents)
  {
    case 1:
      switch (dataType)
      {
        case VTK_UNSIGNED_CHAR:
          return vtkWebGPURenderTextureDeviceResource::R8_UNORM;
        case VTK_UNSIGNED_SHORT:
          return vtkWebGPURenderTextureDeviceResource::R16_UINT;
        case VTK_FLOAT:
          return vtkWebGPURenderTextureDeviceResource::R32_FLOAT;
        default:
          vtkErrorMacro("Unsupported data type for single component texture: " << dataType);
          return vtkWebGPURenderTextureDeviceResource::R8_UNORM;
      }
    case 2:
      switch (dataType)
      {
        case VTK_UNSIGNED_CHAR:
          return vtkWebGPURenderTextureDeviceResource::RG8_UNORM;
        case VTK_UNSIGNED_SHORT:
          return vtkWebGPURenderTextureDeviceResource::RG16_UINT;
        case VTK_FLOAT:
          return vtkWebGPURenderTextureDeviceResource::RG32_FLOAT;
        default:
          vtkErrorMacro("Unsupported data type for single component texture: " << dataType);
          return vtkWebGPURenderTextureDeviceResource::RG8_UNORM;
      }
    case 4:
      switch (dataType)
      {
        case VTK_UNSIGNED_CHAR:
          return vtkWebGPURenderTextureDeviceResource::RGBA8_UNORM;
        case VTK_UNSIGNED_SHORT:
          return vtkWebGPURenderTextureDeviceResource::RGBA16_UINT;
        case VTK_FLOAT:
          return vtkWebGPURenderTextureDeviceResource::RGBA32_FLOAT;
        default:
          vtkErrorMacro("Unsupported data type for single component texture: " << dataType);
          return vtkWebGPURenderTextureDeviceResource::RGBA8_UNORM;
      }
    default:
      vtkErrorMacro("Unsupported number of components: " << numComponents);
      return vtkWebGPURenderTextureDeviceResource::RGBA8_UNORM;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::PostRender(vtkRenderer* renderer)
{
  this->Superclass::PostRender(renderer);
}

//------------------------------------------------------------------------------
void vtkWebGPUTexture::ReleaseGraphicsResources(vtkWindow* window)
{
  if (auto* wgpuRenderWindow = static_cast<vtkWebGPURenderWindow*>(window))
  {
    if (auto* textureCache = wgpuRenderWindow->GetWGPUTextureCache())
    {
      textureCache->RemoveRenderTexture(this->TextureIndex);
    }
  }
  this->RenderWindow = nullptr;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkWebGPUTexture::GetTextureUnit()
{
  return this->TextureIndex;
}

//------------------------------------------------------------------------------
int vtkWebGPUTexture::IsTranslucent()
{
  return this->Superclass::IsTranslucent();
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPURenderTextureDeviceResource> vtkWebGPUTexture::GetDeviceResource() const
{
  if (this->RenderWindow)
  {
    if (auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->RenderWindow))
    {
      if (auto* textureCache = wgpuRenderWindow->GetWGPUTextureCache())
      {
        return textureCache->GetRenderTexture(this->TextureIndex);
      }
    }
  }
  return nullptr;
}

VTK_ABI_NAMESPACE_END
