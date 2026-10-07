// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUShaderModuleInternals.h"

#include "Private/vtkWebGPUHelpersPrivate.h"
#include "vtkLogger.h"
#include "vtkStringFormatter.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
std::string FormatCompilationMessage(
  const std::string& source, const WGPUCompilationMessage& message)
{
  const char* kind = "info";
  if (message.type == WGPUCompilationMessageType_Error)
  {
    kind = "error";
  }
  else if (message.type == WGPUCompilationMessageType_Warning)
  {
    kind = "warning";
  }
  std::ostringstream text;
  text << "WGSL " << kind << " at line " << message.lineNum << ", column " << message.linePos
       << ": " << vtkWebGPUStringViewToStdString(message.message);
  if (message.lineNum == 0)
  {
    return text.str();
  }
  std::vector<std::string> lines;
  std::istringstream stream(source);
  for (std::string line; std::getline(stream, line);)
  {
    lines.push_back(line);
  }
  const std::size_t target = static_cast<std::size_t>(message.lineNum);
  const std::size_t first = target > 3 ? target - 3 : 1;
  const std::size_t last = std::min(lines.size(), target + 2);
  for (std::size_t i = first; i <= last; ++i)
  {
    text << "\n" << (i == target ? ">" : " ") << i << ": " << lines[i - 1];
    if (i == target && message.linePos > 0)
    {
      text << "\n" << std::string(vtk::to_string(i).size() + 3 + message.linePos - 1, ' ') << '^';
    }
  }
  return text.str();
}

void ReportCompilationInfo(WGPUCompilationInfoRequestStatus status, const WGPUCompilationInfo* info,
  void* userdata1, void* vtkNotUsed(userdata2))
{
  std::unique_ptr<std::string> source(static_cast<std::string*>(userdata1));
  if (status != WGPUCompilationInfoRequestStatus_Success || info == nullptr)
  {
    return;
  }
  for (std::size_t i = 0; i < info->messageCount; ++i)
  {
    const auto& message = info->messages[i];
    const std::string text = FormatCompilationMessage(*source, message);
    if (message.type == WGPUCompilationMessageType_Error)
    {
      vtkLog(ERROR, "" << text);
    }
    else if (message.type == WGPUCompilationMessageType_Warning)
    {
      vtkLog(WARNING, "" << text);
    }
    else
    {
      vtkLog(INFO, "" << text);
    }
  }
}
}

//------------------------------------------------------------------------------
WGPUShaderModule vtkWebGPUShaderModuleInternals::CreateFromWGSL(
  WGPUDevice device, const std::string& source)
{
  WGPUShaderSourceWGSL wgslDesc = WGPU_SHADER_SOURCE_WGSL_INIT;
  wgslDesc.code = vtkWebGPUMakeStringView(source);

  WGPUShaderModuleDescriptor descriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  descriptor.nextInChain = &wgslDesc.chain;

  WGPUShaderModule module = wgpuDeviceCreateShaderModule(device, &descriptor);
  if (module)
  {
    WGPUCompilationInfoCallbackInfo callbackInfo = WGPU_COMPILATION_INFO_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = &ReportCompilationInfo;
    callbackInfo.userdata1 = new std::string(source);
    wgpuShaderModuleGetCompilationInfo(module, callbackInfo);
  }
  return module;
}

//------------------------------------------------------------------------------
WGPUShaderModule vtkWebGPUShaderModuleInternals::CreateFromSPIRV(
  WGPUDevice device, const uint32_t* code)
{
  WGPUShaderSourceSPIRV sprivDescriptor = WGPU_SHADER_SOURCE_SPIRV_INIT;
  sprivDescriptor.code = code;

  WGPUShaderModuleDescriptor descriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  descriptor.nextInChain = &sprivDescriptor.chain;

  return wgpuDeviceCreateShaderModule(device, &descriptor);
}
VTK_ABI_NAMESPACE_END
