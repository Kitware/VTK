// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPURenderPipelineCache.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkStringFormatter.h"
#include "vtkWebGPURenderWindow.h"

#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"

#include "vtksys/MD5.h"
#include "vtksys/SystemTools.hxx"

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

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

namespace
{
/**
 * Sanitize a pipeline label by removing spaces, quotes, backslashes, and parentheses.
 */
std::string SanitizePipelineLabel(std::string label)
{
  vtksys::SystemTools::ReplaceString(label, " ", "");
  vtksys::SystemTools::ReplaceString(label, "'", "");
  vtksys::SystemTools::ReplaceString(label, "\"", "");
  vtksys::SystemTools::ReplaceString(label, "(", "-");
  vtksys::SystemTools::ReplaceString(label, ")", "-");
  return label;
}
}

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
  if (descriptor == nullptr)
  {
    vtkErrorMacro(<< "Invalid render pipeline descriptor: nullptr");
    return {};
  }
  if (shaderSource == nullptr)
  {
    vtkErrorMacro(<< "Invalid vertex shader source: nullptr");
    return {};
  }
  return this->GetPipelineKey(descriptor, shaderSource, "");
}

//------------------------------------------------------------------------------
std::string vtkWebGPURenderPipelineCache::GetPipelineKey(wgpu::RenderPipelineDescriptor* descriptor,
  const char* vertexShaderSource, const char* fragmentShaderSource)
{
  if (descriptor == nullptr)
  {
    vtkErrorMacro(<< "Invalid render pipeline descriptor: nullptr");
    return {};
  }
  if (vertexShaderSource == nullptr)
  {
    vtkErrorMacro(<< "Invalid vertex shader source: nullptr");
    return {};
  }
  if (fragmentShaderSource == nullptr)
  {
    vtkErrorMacro(<< "Invalid fragment shader source: nullptr");
    return {};
  }
  const auto cullModeStr = vtk::to_string(
    static_cast<std::underlying_type<wgpu::CullMode>::type>(descriptor->primitive.cullMode));
  const auto topologyStr =
    vtk::to_string(static_cast<std::underlying_type<wgpu::PrimitiveTopology>::type>(
      descriptor->primitive.topology));
  std::string hash;
  this->Internals->ComputeMD5({ vertexShaderSource, fragmentShaderSource, cullModeStr, topologyStr,
                                descriptor->vertex.entryPoint, descriptor->fragment->entryPoint },
    hash);
  return hash;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::CreateRenderPipeline(wgpu::RenderPipelineDescriptor* descriptor,
  vtkWebGPURenderWindow* wgpuRenderWindow, const char* shaderSource)
{
  if (descriptor == nullptr)
  {
    vtkErrorMacro(<< "Invalid render pipeline descriptor: nullptr");
    return;
  }
  if (wgpuRenderWindow == nullptr)
  {
    vtkErrorMacro(<< "Invalid render window: nullptr");
    return;
  }
  if (shaderSource == nullptr)
  {
    vtkErrorMacro(<< "Invalid vertex shader source: nullptr");
    return;
  }
  // apply all shader source include statements.
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
void vtkWebGPURenderPipelineCache::CreateRenderPipeline(wgpu::RenderPipelineDescriptor* descriptor,
  vtkWebGPURenderWindow* wgpuRenderWindow, const char* vertexShaderSource,
  const char* fragmentShaderSource)
{
  if (descriptor == nullptr)
  {
    vtkErrorMacro(<< "Invalid render pipeline descriptor: nullptr");
    return;
  }
  if (wgpuRenderWindow == nullptr)
  {
    vtkErrorMacro(<< "Invalid render window: nullptr");
    return;
  }
  if (vertexShaderSource == nullptr)
  {
    vtkErrorMacro(<< "Invalid vertex shader source: nullptr");
    return;
  }
  if (fragmentShaderSource == nullptr)
  {
    vtkErrorMacro(<< "Invalid fragment shader source: nullptr");
    return;
  }
  // apply all shader source include statements.
  const auto vertexShaderSourceFinal = wgpuRenderWindow->PreprocessShaderSource(vertexShaderSource);
  const auto fragmentShaderSourceFinal =
    wgpuRenderWindow->PreprocessShaderSource(fragmentShaderSource);

  if (const char* path = std::getenv("VTK_WEBGPU_SHADER_DUMP_PREFIX"))
  {
    const std::string sanitizedLabel = SanitizePipelineLabel(std::string(descriptor->label));
    std::error_code ec;
    std::filesystem::create_directory(path, ec);
    if (ec)
    {
      vtkErrorMacro(<< "Failed to make directory " << path << ". error=" << ec.message() << "("
                    << ec.value() << ")");
    }
    else
    {
      const auto vertPath = std::string(path) + '/' + sanitizedLabel + ".vert.wgsl";
      std::ofstream vertOS(vertPath, std::iostream::binary | std::iostream::trunc);
      vertOS << vertexShaderSourceFinal << '\n';
      vtkLog(INFO, << "Wrote vertex shader " << vertPath);
      vertOS.close();

      const auto fragPath = std::string(path) + '/' + sanitizedLabel + ".frag.wgsl";
      std::ofstream fragOS(fragPath, std::iostream::binary | std::iostream::trunc);
      fragOS << fragmentShaderSourceFinal << '\n';
      vtkLog(INFO, << "Wrote fragment shader " << fragPath);
      fragOS.close();
    }
  }

  // compute md5sum for the final shader source code.
  std::string vertexShaderHash;
  this->Internals->ComputeMD5({ vertexShaderSourceFinal }, vertexShaderHash);
  wgpu::ShaderModule vertexShaderModule = this->Internals->HasShaderModule(vertexShaderHash);
  if (vertexShaderModule == nullptr)
  {
    vertexShaderModule = vtkWebGPUShaderModuleInternals::CreateFromWGSL(
      wgpuRenderWindow->GetDevice(), vertexShaderSourceFinal);
    this->Internals->InsertShader(vertexShaderHash, vertexShaderModule);
  }
  std::string fragmentShaderHash;
  this->Internals->ComputeMD5({ fragmentShaderSourceFinal }, fragmentShaderHash);
  wgpu::ShaderModule fragmentShaderModule = this->Internals->HasShaderModule(fragmentShaderHash);
  if (fragmentShaderModule == nullptr)
  {
    fragmentShaderModule = vtkWebGPUShaderModuleInternals::CreateFromWGSL(
      wgpuRenderWindow->GetDevice(), fragmentShaderSourceFinal);
    this->Internals->InsertShader(fragmentShaderHash, fragmentShaderModule);
  }

  // set shader module
  auto* pipelineDescriptorVtk =
    static_cast<vtkWebGPURenderPipelineDescriptorInternals*>(descriptor);
  pipelineDescriptorVtk->vertex.module = vertexShaderModule;
  pipelineDescriptorVtk->cFragment.module = fragmentShaderModule;

  // create pipeline
  auto pipeline = wgpuRenderWindow->GetDevice().CreateRenderPipeline(pipelineDescriptorVtk);
  const std::string pipelineHash =
    this->GetPipelineKey(descriptor, vertexShaderSource, fragmentShaderSource);
  this->Internals->PipelineCache[pipelineHash] = pipeline;
  vtkDebugMacro(<< "Create pipeline " << pipeline.Get() << " for key=" << pipelineHash);
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderPipelineCache::Substitute(
  std::string& source, const std::string& search, const std::string& replace, bool all)
{
  std::string::size_type currentPosition = 0;
  bool replaced = false;
  do
  {
    currentPosition = source.find(search, currentPosition);
    if (currentPosition == std::string::npos)
    {
      break;
    }
    source.replace(currentPosition, search.length(), replace);
    replaced = true;
    if (!all)
    {
      break;
    }
    currentPosition += replace.length();
  } while (true);
  return replaced;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::DestroyRenderPipeline(const std::string& key)
{
  this->Internals->PipelineCache.erase(key);
}

VTK_ABI_NAMESPACE_END
