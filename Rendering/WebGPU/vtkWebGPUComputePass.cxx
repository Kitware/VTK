// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputePass.h"
#include "Private/vtkWebGPUComputePassBufferStorageInternals.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "Private/vtkWebGPUComputePassTextureStorageInternals.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkWebGPUComputeTexture.h"
#include "vtkWebGPUComputeTextureView.h"
#include "vtksys/FStream.hxx"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputePass);

//------------------------------------------------------------------------------
vtkWebGPUComputePass::vtkWebGPUComputePass()
{
  this->Internals = vtkSmartPointer<vtkWebGPUComputePassInternals>::New();
  this->Internals->SetParentPass(this);

  this->Internals->TextureStorage =
    vtkSmartPointer<vtkWebGPUComputePassTextureStorageInternals>::New();
  this->Internals->TextureStorage->SetComputePass(this);

  this->Internals->BufferStorage =
    vtkSmartPointer<vtkWebGPUComputePassBufferStorageInternals>::New();
  this->Internals->BufferStorage->SetComputePass(this);
}

//------------------------------------------------------------------------------
vtkWebGPUComputePass::~vtkWebGPUComputePass()
{
  this->ReleaseResources();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "ShaderSource: " << this->ShaderSource << std::endl;
  os << indent << "ShaderEntryPoint: " << this->ShaderEntryPoint << std::endl;

  os << indent << "Groups X/Y/Z: " << this->GroupsX << ", " << this->GroupsY << ", "
     << this->GroupsZ << std::endl;

  os << indent << "Label: " << this->Label << std::endl;

  this->Internals->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::SetLabel(const std::string& label)
{
  this->Label = label;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::SetShaderSourceFromPath(const char* shaderFilePath)
{
  if (!vtksys::SystemTools::FileExists(shaderFilePath))
  {
    vtkLogF(ERROR, "Given shader file path '%s' doesn't exist", shaderFilePath);

    return;
  }

  vtksys::ifstream inputFileStream(shaderFilePath);
  std::string source(
    (std::istreambuf_iterator<char>(inputFileStream)), std::istreambuf_iterator<char>());

  this->SetShaderSource(source);
}

//------------------------------------------------------------------------------
int vtkWebGPUComputePass::AddBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer)
{
  return this->Internals->BufferStorage->AddBuffer(buffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::AddRenderBuffer(
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer)
{
  this->Internals->BufferStorage->AddRenderBuffer(renderBuffer);
}

//------------------------------------------------------------------------------
int vtkWebGPUComputePass::AddRenderTexture(
  vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture)
{
  return this->Internals->TextureStorage->AddRenderTexture(renderTexture);
}

//------------------------------------------------------------------------------
int vtkWebGPUComputePass::AddTexture(vtkSmartPointer<vtkWebGPUComputeTexture> texture)
{
  return this->Internals->TextureStorage->AddTexture(texture);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeTextureView> vtkWebGPUComputePass::CreateTextureView(
  int textureIndex)
{
  return this->Internals->TextureStorage->CreateTextureView(textureIndex);
}

//------------------------------------------------------------------------------
int vtkWebGPUComputePass::AddTextureView(vtkSmartPointer<vtkWebGPUComputeTextureView> textureView)
{
  return this->Internals->TextureStorage->AddTextureView(textureView);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::RebindTextureView(int group, int binding, int textureViewIndex)
{
  this->Internals->TextureStorage->RebindTextureView(group, binding, textureViewIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::DeleteTextureViews(int textureIndex)
{
  this->Internals->TextureStorage->DeleteTextureViews(textureIndex);
}

//------------------------------------------------------------------------------
unsigned int vtkWebGPUComputePass::GetBufferByteSize(int bufferIndex)
{
  return this->Internals->BufferStorage->GetBufferByteSize(bufferIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::ResizeBuffer(int bufferIndex, vtkIdType newByteSize)
{
  this->Internals->BufferStorage->ResizeBuffer(bufferIndex, newByteSize);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeTexture> vtkWebGPUComputePass::GetComputeTexture(int textureIndex)
{
  return this->Internals->TextureStorage->GetComputeTexture(textureIndex);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeTextureView> vtkWebGPUComputePass::GetTextureView(
  int textureViewIndex)
{
  return this->Internals->TextureStorage->GetTextureView(textureViewIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::RecreateComputeTexture(int textureIndex)
{
  this->Internals->TextureStorage->RecreateComputeTexture(textureIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::RecreateTextureView(int textureIndex)
{
  this->Internals->TextureStorage->RecreateTextureView(textureIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::ReadBufferFromGPU(
  int bufferIndex, BufferMapAsyncCallback callback, void* userdata)
{
  this->Internals->BufferStorage->ReadBufferFromGPU(bufferIndex, callback, userdata);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::ReadTextureFromGPU(
  int textureIndex, int mipLevel, TextureMapAsyncCallback callback, void* userdata)
{
  this->Internals->TextureStorage->ReadTextureFromGPU(textureIndex, mipLevel, callback, userdata);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::UpdateBufferData(int bufferIndex, vtkDataArray* newData)
{
  this->Internals->BufferStorage->UpdateBufferData(bufferIndex, newData);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::UpdateBufferData(
  int bufferIndex, vtkIdType byteOffset, vtkDataArray* newData)
{
  this->Internals->BufferStorage->UpdateBufferData(bufferIndex, byteOffset, newData);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::SetWorkgroups(int groupsX, int groupsY, int groupsZ)
{
  this->GroupsX = groupsX;
  this->GroupsY = groupsY;
  this->GroupsZ = groupsZ;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::Dispatch()
{
  if (!this->Internals->Initialized)
  {
    this->Internals->CreateShaderModule();

    this->Internals->Initialized = true;
  }

  if (this->Internals->BindGroupOrLayoutsInvalidated || !this->Internals->Initialized)
  {
    this->Internals->CreateBindGroupsAndLayouts();
    this->Internals->CreateWebGPUComputePipeline();

    this->Internals->BindGroupOrLayoutsInvalidated = false;
  }

  this->Internals->WebGPUDispatch(this->GroupsX, this->GroupsY, this->GroupsZ);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::ReleaseResources()
{
  this->Internals->ReleaseResources();

  this->ShaderSource = "";
  this->ShaderEntryPoint = "";

  this->GroupsX = 0;
  this->GroupsY = 0;
  this->GroupsZ = 0;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::WriteBufferData(
  int bufferIndex, vtkIdType byteOffset, const void* data, std::size_t numBytes)
{
  this->Internals->BufferStorage->WriteBuffer(bufferIndex, byteOffset, data, numBytes);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePass::WriteTextureData(
  int textureIndex, const void* data, std::size_t numBytes)
{
  this->Internals->TextureStorage->WriteTexture(textureIndex, data, numBytes);
}
VTK_ABI_NAMESPACE_END
