// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPURenderPipelineCache.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"

#include "Private/vtkWebGPUHandle.h"
#include "Private/vtkWebGPUHelpersPrivate.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"

#include "vtksys/MD5.h"
#include "vtksys/SystemTools.hxx"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderPipelineCache::vtkInternals
{
public:
  vtksysMD5* md5;
  // Key is a unique hash of all the properties that make a unique WebGPU render pipeline.
  // Value is an owned WGPURenderPipeline.
  std::unordered_map<std::string, vtkWebGPU::RenderPipeline> PipelineCache;
  // map of a unique hash to webgpu shader module.
  std::unordered_map<std::string, vtkWebGPU::ShaderModule> ShaderCache;

  // Pipelines started by CreateRenderPipelineAsync() that have not reported back yet.
  // The callback owns the entry, so each one is stable in memory for the whole request.
  struct PendingPipeline
  {
    std::string Key;
    vtkInternals* Owner = nullptr;
    bool Done = false;
  };
  std::vector<std::unique_ptr<PendingPipeline>> PendingPipelines;

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

  vtkWebGPU::ShaderModule HasShaderModule(const std::string& source)
  {
    auto it = this->ShaderCache.find(source);
    if (it != this->ShaderCache.end())
    {
      return it->second;
    }
    else
    {
      return {};
    }
  }

  void InsertShader(const std::string& source, vtkWebGPU::ShaderModule shader)
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
vtkWebGPURenderPipelineCache::~vtkWebGPURenderPipelineCache()
{
  // See ReleaseGraphicsResources().
  this->WaitForPendingRenderPipelines();
}

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
  // Outstanding callbacks write into the internals, so they have to be done with before
  // those go away.
  this->WaitForPendingRenderPipelines();
  this->Internals.reset(new vtkInternals());
}

//------------------------------------------------------------------------------
WGPURenderPipeline vtkWebGPURenderPipelineCache::GetRenderPipeline(const std::string& key)
{
  auto iter = this->Internals->PipelineCache.find(key);
  if (iter != this->Internals->PipelineCache.end())
  {
    vtkDebugMacro(<< "Pipeline exists for key=" << key << "...");
    return iter->second.Get();
  }
  else
  {
    vtkDebugMacro(<< "Pipeline does not exist for key=" << key << "...");
    return nullptr;
  }
}

namespace
{
/**
 * Serialize the fixed-function state that, together with the shader sources,
 * defines a render pipeline. Two descriptors that differ in any of this need
 * distinct cache entries, so all of it has to reach the key.
 */
std::string DescribePipelineState(const WGPURenderPipelineDescriptor* descriptor)
{
  std::ostringstream os;
  os << 'L' << reinterpret_cast<std::uintptr_t>(descriptor->layout);

  const auto& primitive = descriptor->primitive;
  os << "|P" << primitive.topology << ',' << primitive.stripIndexFormat << ','
     << primitive.frontFace << ',' << primitive.cullMode << ',' << primitive.unclippedDepth;

  const auto& multisample = descriptor->multisample;
  os << "|M" << multisample.count << ',' << multisample.mask << ','
     << multisample.alphaToCoverageEnabled;

  os << "|V" << descriptor->vertex.bufferCount;
  for (std::size_t i = 0; i < descriptor->vertex.bufferCount; ++i)
  {
    const auto& buffer = descriptor->vertex.buffers[i];
    os << ';' << buffer.arrayStride << ',' << buffer.stepMode << ',' << buffer.attributeCount;
    for (std::size_t j = 0; j < buffer.attributeCount; ++j)
    {
      const auto& attribute = buffer.attributes[j];
      os << ':' << attribute.format << ',' << attribute.offset << ',' << attribute.shaderLocation;
    }
  }

  if (const auto* depthStencil = descriptor->depthStencil)
  {
    os << "|D" << depthStencil->format << ',' << depthStencil->depthWriteEnabled << ','
       << depthStencil->depthCompare << ',' << depthStencil->stencilReadMask << ','
       << depthStencil->stencilWriteMask << ',' << depthStencil->depthBias << ','
       << depthStencil->depthBiasSlopeScale << ',' << depthStencil->depthBiasClamp << ','
       << depthStencil->stencilFront.compare << ',' << depthStencil->stencilFront.failOp << ','
       << depthStencil->stencilFront.depthFailOp << ',' << depthStencil->stencilFront.passOp << ','
       << depthStencil->stencilBack.compare << ',' << depthStencil->stencilBack.failOp << ','
       << depthStencil->stencilBack.depthFailOp << ',' << depthStencil->stencilBack.passOp;
  }
  else
  {
    os << "|D-";
  }

  if (const auto* fragment = descriptor->fragment)
  {
    os << "|F" << fragment->targetCount;
    for (std::size_t i = 0; i < fragment->targetCount; ++i)
    {
      const auto& target = fragment->targets[i];
      os << ';' << target.format << ',' << target.writeMask;
      if (const auto* blend = target.blend)
      {
        os << ':' << blend->color.operation << ',' << blend->color.srcFactor << ','
           << blend->color.dstFactor << ',' << blend->alpha.operation << ','
           << blend->alpha.srcFactor << ',' << blend->alpha.dstFactor;
      }
      else
      {
        os << ":-";
      }
    }
  }
  else
  {
    os << "|F-";
  }
  return os.str();
}
}

//------------------------------------------------------------------------------
std::string vtkWebGPURenderPipelineCache::GetPipelineKey(
  WGPURenderPipelineDescriptor* descriptor, const char* shaderSource)
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
std::string vtkWebGPURenderPipelineCache::GetPipelineKey(WGPURenderPipelineDescriptor* descriptor,
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
  const auto stateStr = DescribePipelineState(descriptor);
  std::string hash;
  this->Internals->ComputeMD5(
    { vertexShaderSource, fragmentShaderSource,
      vtkWebGPUStringViewToStdString(descriptor->vertex.entryPoint),
      vtkWebGPUStringViewToStdString(descriptor->fragment->entryPoint), stateStr },
    hash);
  return hash;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::CreateRenderPipeline(WGPURenderPipelineDescriptor* descriptor,
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
  vtkWebGPU::ShaderModule shaderModule = this->Internals->HasShaderModule(shaderHash);
  if (shaderModule == nullptr)
  {
    shaderModule = vtkWebGPU::ShaderModule::Acquire(
      vtkWebGPUShaderModuleInternals::CreateFromWGSL(wgpuRenderWindow->GetDevice(), source));
    this->Internals->InsertShader(shaderHash, shaderModule);
  }

  // set shader module
  auto* pipelineDescriptorVtk =
    reinterpret_cast<vtkWebGPURenderPipelineDescriptorInternals*>(descriptor);
  pipelineDescriptorVtk->vertex.module = shaderModule;
  pipelineDescriptorVtk->cFragment.module = shaderModule;

  // create pipeline
  auto pipeline = vtkWebGPU::RenderPipeline::Acquire(
    wgpuDeviceCreateRenderPipeline(wgpuRenderWindow->GetDevice(), pipelineDescriptorVtk));
  std::string pipelineHash = this->GetPipelineKey(descriptor, shaderSource);
  this->Internals->PipelineCache[pipelineHash] = pipeline;
  vtkDebugMacro(<< "Create pipeline " << pipeline.Get() << " for key=" << pipelineHash);
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderPipelineCache::PrepareShaderModules(WGPURenderPipelineDescriptor* descriptor,
  vtkWebGPURenderWindow* wgpuRenderWindow, const char* vertexShaderSource,
  const char* fragmentShaderSource)
{
  if (descriptor == nullptr)
  {
    vtkErrorMacro(<< "Invalid render pipeline descriptor: nullptr");
    return false;
  }
  if (wgpuRenderWindow == nullptr)
  {
    vtkErrorMacro(<< "Invalid render window: nullptr");
    return false;
  }
  if (vertexShaderSource == nullptr)
  {
    vtkErrorMacro(<< "Invalid vertex shader source: nullptr");
    return false;
  }
  if (fragmentShaderSource == nullptr)
  {
    vtkErrorMacro(<< "Invalid fragment shader source: nullptr");
    return false;
  }
  // apply all shader source include statements.
  const auto vertexShaderSourceFinal = wgpuRenderWindow->PreprocessShaderSource(vertexShaderSource);
  const auto fragmentShaderSourceFinal =
    wgpuRenderWindow->PreprocessShaderSource(fragmentShaderSource);

  if (const char* path = std::getenv("VTK_WEBGPU_SHADER_DUMP_PREFIX"))
  {
    const std::string sanitizedLabel =
      SanitizePipelineLabel(vtkWebGPUStringViewToStdString(descriptor->label));
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
  vtkWebGPU::ShaderModule vertexShaderModule = this->Internals->HasShaderModule(vertexShaderHash);
  if (vertexShaderModule == nullptr)
  {
    vertexShaderModule =
      vtkWebGPU::ShaderModule::Acquire(vtkWebGPUShaderModuleInternals::CreateFromWGSL(
        wgpuRenderWindow->GetDevice(), vertexShaderSourceFinal));
    this->Internals->InsertShader(vertexShaderHash, vertexShaderModule);
  }
  std::string fragmentShaderHash;
  this->Internals->ComputeMD5({ fragmentShaderSourceFinal }, fragmentShaderHash);
  vtkWebGPU::ShaderModule fragmentShaderModule =
    this->Internals->HasShaderModule(fragmentShaderHash);
  if (fragmentShaderModule == nullptr)
  {
    fragmentShaderModule =
      vtkWebGPU::ShaderModule::Acquire(vtkWebGPUShaderModuleInternals::CreateFromWGSL(
        wgpuRenderWindow->GetDevice(), fragmentShaderSourceFinal));
    this->Internals->InsertShader(fragmentShaderHash, fragmentShaderModule);
  }

  // set shader module
  auto* pipelineDescriptorVtk =
    reinterpret_cast<vtkWebGPURenderPipelineDescriptorInternals*>(descriptor);
  pipelineDescriptorVtk->vertex.module = vertexShaderModule;
  pipelineDescriptorVtk->cFragment.module = fragmentShaderModule;
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::CreateRenderPipeline(WGPURenderPipelineDescriptor* descriptor,
  vtkWebGPURenderWindow* wgpuRenderWindow, const char* vertexShaderSource,
  const char* fragmentShaderSource)
{
  if (!this->PrepareShaderModules(
        descriptor, wgpuRenderWindow, vertexShaderSource, fragmentShaderSource))
  {
    return;
  }
  auto* pipelineDescriptorVtk =
    reinterpret_cast<vtkWebGPURenderPipelineDescriptorInternals*>(descriptor);
  // create pipeline
  auto pipeline = vtkWebGPU::RenderPipeline::Acquire(
    wgpuDeviceCreateRenderPipeline(wgpuRenderWindow->GetDevice(), pipelineDescriptorVtk));
  const std::string pipelineHash =
    this->GetPipelineKey(descriptor, vertexShaderSource, fragmentShaderSource);
  this->Internals->PipelineCache[pipelineHash] = pipeline;
  vtkDebugMacro(<< "Create pipeline " << pipeline.Get() << " for key=" << pipelineHash);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::CreateRenderPipelineAsync(
  WGPURenderPipelineDescriptor* descriptor, vtkWebGPURenderWindow* wgpuRenderWindow,
  const char* vertexShaderSource, const char* fragmentShaderSource)
{
  if (!this->PrepareShaderModules(
        descriptor, wgpuRenderWindow, vertexShaderSource, fragmentShaderSource))
  {
    return;
  }
  auto* pipelineDescriptorVtk =
    reinterpret_cast<vtkWebGPURenderPipelineDescriptorInternals*>(descriptor);

  auto pending = std::make_unique<vtkInternals::PendingPipeline>();
  pending->Key = this->GetPipelineKey(descriptor, vertexShaderSource, fragmentShaderSource);
  pending->Owner = this->Internals.get();
  auto* pendingRaw = pending.get();
  this->Internals->PendingPipelines.push_back(std::move(pending));

  WGPUCreateRenderPipelineAsyncCallbackInfo callbackInfo = {};
  callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
  callbackInfo.callback = [](WGPUCreatePipelineAsyncStatus status, WGPURenderPipeline pipeline,
                            WGPUStringView message, void* userdata1, void* /*userdata2*/)
  {
    auto* entry = static_cast<vtkInternals::PendingPipeline*>(userdata1);
    entry->Done = true;
    if (status != WGPUCreatePipelineAsyncStatus_Success)
    {
      vtkGenericWarningMacro(
        "Failed to create render pipeline: " << vtkWebGPUStringViewToStdString(message));
      return;
    }
    // The callback is handed ownership of the pipeline, so adopt it.
    entry->Owner->PipelineCache[entry->Key] = vtkWebGPU::RenderPipeline::Acquire(pipeline);
  };
  callbackInfo.userdata1 = pendingRaw;
  wgpuDeviceCreateRenderPipelineAsync(
    wgpuRenderWindow->GetDevice(), pipelineDescriptorVtk, callbackInfo);
  this->Configuration = wgpuRenderWindow->GetWGPUConfiguration();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineCache::WaitForPendingRenderPipelines()
{
  auto& pending = this->Internals->PendingPipelines;
  if (pending.empty())
  {
    return;
  }
  if (this->Configuration == nullptr)
  {
    vtkErrorMacro(<< "Pipelines are outstanding but there is no configuration to wait on them.");
    pending.clear();
    return;
  }
  const auto start = std::chrono::steady_clock::now();
  const double timeoutMS = this->Configuration->GetTimeout();
  bool allDone = false;
  while (!allDone)
  {
    allDone = true;
    for (const auto& entry : pending)
    {
      allDone &= entry->Done;
    }
    if (allDone)
    {
      break;
    }
    // The implementation reports completion from ProcessEvents(), so it has to keep
    // being driven while anything is outstanding.
    this->Configuration->ProcessEvents();
    const auto elapsed =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (elapsed >= timeoutMS)
    {
      // Give up rather than spin forever. The pipelines that did arrive are usable; the
      // ones that did not leave their cache entry empty and the draw is skipped.
      vtkErrorMacro(<< "Timed out waiting for render pipelines to compile.");
      break;
    }
  }
  pending.clear();
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
