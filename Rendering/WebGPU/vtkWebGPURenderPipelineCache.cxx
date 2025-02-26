// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPURenderPipelineCache.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"

#include <string>
#include <unordered_map>

#include "vtksys/MD5.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderPipelineCache::vtkInternals
{
public:
  vtksysMD5* md5;
  // Key is a unique hash of all the properties that make a unique WebGPU render pipeline.
  // Value is a wgpu::RenderPipeline object.
  std::unordered_map<std::string, wgpu::RenderPipeline> PipelineCache;
  // map of a unique hash to webgpu shader module.
  std::unordered_map<std::string, wgpu::ShaderModule> ShaderCache;

  vtkInternals() { md5 = vtksysMD5_New(); }

  ~vtkInternals() { vtksysMD5_Delete(this->md5); }

  //-----------------------------------------------------------------------------
  void ComputeMD5(std::initializer_list<std::string_view> contents, std::string& hash)
  {
    unsigned char digest[16];
    char md5Hash[33];
    md5Hash[32] = '\0';

    vtksysMD5_Initialize(this->md5);
    for (const auto& content : contents)
    {
      if (!content.empty())
      {
        vtksysMD5_Append(this->md5, reinterpret_cast<const unsigned char*>(content.data()),
          static_cast<int>(content.length()));
      }
    }
    vtksysMD5_Finalize(this->md5, digest);
    vtksysMD5_DigestToHex(digest, md5Hash);

    hash = md5Hash;
  }

  wgpu::ShaderModule HasShaderModule(const std::string& source)
  {
    auto it = this->ShaderCache.find(source);
    if (it != this->ShaderCache.end())
    {
      return it->second;
    }
    else
    {
      return nullptr;
    }
  }

  void InsertShader(const std::string& source, wgpu::ShaderModule shader)
  {
    this->ShaderCache[source] = shader;
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPURenderPipelineCache);

//------------------------------------------------------------------------------
vtkWebGPURenderPipelineCache::vtkWebGPURenderPipelineCache()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkWebGPURenderPipelineCache::~vtkWebGPURenderPipelineCache() = default;

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "PipelineCache: \n";
  for (const auto& iter : this->Internals->PipelineCache)
  {
    os << iter.first << ": " << iter.second.Get() << '\n';
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::ReleaseGraphicsResources(vtkWindow*)
{
  this->Internals.reset(new vtkInternals());
}

//------------------------------------------------------------------------------
wgpu::RenderPipeline vtkWebGPURenderPipelineCache::GetRenderPipeline(const std::string& key)
{
  auto iter = this->Internals->PipelineCache.find(key);
  if (iter != this->Internals->PipelineCache.end())
  {
    vtkDebugMacro(<< "Pipeline exists for key=" << key << "...");
    return iter->second;
  }
  else
  {
    vtkDebugMacro(<< "Pipeline does not exist for key=" << key << "...");
    return {};
  }
}

//------------------------------------------------------------------------------
std::string vtkWebGPURenderPipelineCache::GetPipelineKey(
  wgpu::RenderPipelineDescriptor* descriptor, const char* shaderSource)
{
  const auto cullModeStr = std::to_string(
    static_cast<std::underlying_type<wgpu::CullMode>::type>(descriptor->primitive.cullMode));
  const auto topologyStr =
    std::to_string(static_cast<std::underlying_type<wgpu::PrimitiveTopology>::type>(
      descriptor->primitive.cullMode));
  std::string hash;
  this->Internals->ComputeMD5({ shaderSource, cullModeStr, topologyStr,
                                descriptor->vertex.entryPoint, descriptor->fragment->entryPoint },
    hash);
  return hash;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::CreateRenderPipeline(wgpu::RenderPipelineDescriptor* descriptor,
  vtkWebGPURenderer* wgpuRenderer, const char* shaderSource)
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(wgpuRenderer->GetRenderWindow());
  this->CreateRenderPipeline(descriptor, wgpuRenderWindow, shaderSource);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::CreateRenderPipeline(wgpu::RenderPipelineDescriptor* descriptor,
  vtkWebGPURenderWindow* wgpuRenderWindow, const char* shaderSource)
{
  // apply all shader source replacements.
  const auto source = wgpuRenderWindow->PreprocessShaderSource(shaderSource);

  // compute md5sum for the final shader source code.
  std::string shaderHash;
  this->Internals->ComputeMD5({ source }, shaderHash);
  wgpu::ShaderModule shaderModule = this->Internals->HasShaderModule(shaderHash);
  if (shaderModule == nullptr)
  {
    shaderModule =
      vtkWebGPUShaderModuleInternals::CreateFromWGSL(wgpuRenderWindow->GetDevice(), source);
    this->Internals->InsertShader(shaderHash, shaderModule);
  }

  // set shader module
  auto* pipelineDescriptorVtk =
    static_cast<vtkWebGPURenderPipelineDescriptorInternals*>(descriptor);
  pipelineDescriptorVtk->vertex.module = shaderModule;
  pipelineDescriptorVtk->cFragment.module = shaderModule;

  // create pipeline
  auto pipeline = wgpuRenderWindow->GetDevice().CreateRenderPipeline(pipelineDescriptorVtk);
  std::string pipelineHash = this->GetPipelineKey(descriptor, shaderSource);
  this->Internals->PipelineCache[pipelineHash] = pipeline;
  vtkDebugMacro(<< "Create pipeline " << pipeline.Get() << " for key=" << pipelineHash);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::DestroyRenderPipeline(const std::string& key)
{
  this->Internals->PipelineCache.erase(key);
}

VTK_ABI_NAMESPACE_END
