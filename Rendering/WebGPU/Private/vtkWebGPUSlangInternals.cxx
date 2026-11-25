// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUSlangInternals.h"

#include "vtkCallbackCommand.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkTimeStamp.h"
#include "vtkWebGPUShaderProperty.h"

#include "vtkPolyDataSurfaceSlang.h"

#include <algorithm>
#include <cstdlib>
#include <mutex>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
//------------------------------------------------------------------------------
// Every mapper shares the manager below, and its GetLast*() calls describe its most recent
// compile, so a compile holds this lock until its results have been read.
std::mutex& GetManagerMutex()
{
  static std::mutex mutex;
  return mutex;
}

//------------------------------------------------------------------------------
vtkSlangShaderManager* GetManager()
{
  static vtkNew<vtkSlangShaderManager> manager;
  static const bool quiet = []()
  {
    vtkNew<vtkCallbackCommand> ignore;
    manager->AddObserver(vtkCommand::ErrorEvent, ignore);
    return true;
  }();
  (void)quiet;
  return manager;
}

//------------------------------------------------------------------------------
bool Unsupported(
  const std::string& name, unsigned int group, unsigned int index, std::string& error)
{
  error = "Slang overrides with parameters are not supported; binding '" + name + "' (group " +
    std::to_string(group) + ", binding " + std::to_string(index) + ") is not supported";
  return false;
}

struct GlobalRequest
{
  bool Requested = false;
  vtkTimeStamp Modified;
};

//------------------------------------------------------------------------------
std::vector<std::string> GetProgramInterfaces(const char* source)
{
  std::vector<std::string> interfaces;
  const std::string text = source ? source : "";
  const std::string keyword = "type_param ";
  for (std::size_t at = text.find(keyword); at != std::string::npos;
       at = text.find(keyword, at + keyword.size()))
  {
    const std::size_t colon = text.find(':', at);
    const std::size_t end = text.find(';', at);
    if ((at > 0 && text[at - 1] != '\n') || colon == std::string::npos ||
      end == std::string::npos || colon > end)
    {
      continue;
    }
    const std::size_t first = text.find_first_not_of(" \t", colon + 1);
    const std::size_t last = text.find_last_not_of(" \t", end - 1);
    if (first != std::string::npos && first <= last)
    {
      interfaces.push_back(text.substr(first, last - first + 1));
    }
  }
  return interfaces;
}

//------------------------------------------------------------------------------
bool TakesInterface(const std::vector<std::string>& interfaces, const char* name)
{
  return name && std::find(interfaces.begin(), interfaces.end(), name) != interfaces.end();
}

//------------------------------------------------------------------------------
GlobalRequest& GetGlobalRequestState()
{
  static GlobalRequest state = []()
  {
    GlobalRequest initial;
    const char* env = std::getenv("VTK_WEBGPU_SLANG");
    initial.Requested = env && env[0] == '1';
    return initial;
  }();
  return state;
}
}

//------------------------------------------------------------------------------
bool vtkWebGPUSlangInternals::IsRequested(vtkShaderProperty* property)
{
  auto* wgpuProperty = vtkWebGPUShaderProperty::SafeDownCast(property);
  return (wgpuProperty && wgpuProperty->HasSlang()) || GetGlobalRequest();
}

//------------------------------------------------------------------------------
bool vtkWebGPUSlangInternals::GetGlobalRequest()
{
  return GetGlobalRequestState().Requested;
}

//------------------------------------------------------------------------------
void vtkWebGPUSlangInternals::SetGlobalRequest(bool request)
{
  auto& state = GetGlobalRequestState();
  if (state.Requested != request)
  {
    state.Requested = request;
    state.Modified.Modified();
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkWebGPUSlangInternals::GetRequestMTime(vtkShaderProperty* property)
{
  const vtkMTimeType global = GetGlobalRequestState().Modified.GetMTime();
  return property ? std::max(property->GetMTime(), global) : global;
}

//------------------------------------------------------------------------------
std::string vtkWebGPUSlangInternals::GetUnsupportedCustomizations(vtkShaderProperty* property)
{
  std::string unsupported;
  if (!property)
  {
    return unsupported;
  }
  const bool code[5] = { property->HasVertexShaderCode(), property->HasFragmentShaderCode(),
    property->HasGeometryShaderCode(), property->HasTessControlShaderCode(),
    property->HasTessEvalShaderCode() };
  const char* stages[5] = { "vertex", "fragment", "geometry", "tessellation control",
    "tessellation evaluation" };
  for (int i = 0; i < 5; ++i)
  {
    if (code[i])
    {
      unsupported += std::string(" ") + stages[i] + " shader code;";
    }
  }
  return unsupported;
}

//------------------------------------------------------------------------------
std::string vtkWebGPUSlangInternals::GetIgnoredSpecializations(
  const Program& program, vtkShaderProperty* property)
{
  std::string ignored;
  auto* wgpuProperty = vtkWebGPUShaderProperty::SafeDownCast(property);
  if (!wgpuProperty || wgpuProperty->GetNumberOfSlangSpecializations() == 0)
  {
    return ignored;
  }
  const auto interfaces = GetProgramInterfaces(program.Source);
  for (int i = 0; i < wgpuProperty->GetNumberOfSlangSpecializations(); ++i)
  {
    const char* name = wgpuProperty->GetNthSlangSpecializationInterface(i);
    if (!TakesInterface(interfaces, name))
    {
      ignored += std::string(ignored.empty() ? "" : ", ") + name + " = " +
        wgpuProperty->GetNthSlangSpecializationType(i);
    }
  }
  return ignored;
}

//------------------------------------------------------------------------------
void vtkWebGPUSlangInternals::ReportIgnoredSpecializations(
  vtkObject* reporter, const Program& program, vtkShaderProperty* property, std::string& reported)
{
  const std::string ignored = GetIgnoredSpecializations(program, property);
  if (ignored != reported && !ignored.empty())
  {
    vtkWarningWithObjectMacro(reporter,
      << "Ignoring Slang specializations that this mapper's program does not take: " << ignored
      << ".");
  }
  reported = ignored;
}

//------------------------------------------------------------------------------
const vtkWebGPUSlangInternals::Program& vtkWebGPUSlangInternals::GetSurfaceProgram()
{
  static const Program program{ vtkPolyDataSurfaceSlang, { "vertexMain", "fragmentMain" } };
  return program;
}

//------------------------------------------------------------------------------
bool vtkWebGPUSlangInternals::Compile(vtkShaderProperty* property,
  const vtkSlangShaderManager::StringPairs& defines, std::string& wgsl, Parameters& parameters,
  std::string& error, const std::string& label)
{
  return Compile(GetSurfaceProgram(), property, defines, wgsl, parameters, error, label);
}

//------------------------------------------------------------------------------
bool vtkWebGPUSlangInternals::Compile(const Program& program, vtkShaderProperty* property,
  const vtkSlangShaderManager::StringPairs& defines, std::string& wgsl, Parameters& parameters,
  std::string& error, const std::string& label, std::string* customizationError)
{
  if (customizationError)
  {
    customizationError->clear();
  }
  if (CompileOnce(program, property, defines, wgsl, parameters, error, label))
  {
    return true;
  }
  auto* wgpuProperty = vtkWebGPUShaderProperty::SafeDownCast(property);
  const char* module = wgpuProperty ? wgpuProperty->GetSlangModule() : nullptr;
  const bool customized =
    wgpuProperty && ((module && *module) || wgpuProperty->GetNumberOfSlangSpecializations() > 0);
  std::string defaultError;
  if (!customizationError || !customized ||
    !CompileOnce(program, nullptr, defines, wgsl, parameters, defaultError, label))
  {
    return false;
  }
  *customizationError = error;
  error.clear();
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUSlangInternals::CompileOnce(const Program& program, vtkShaderProperty* property,
  const vtkSlangShaderManager::StringPairs& defines, std::string& wgsl, Parameters& parameters,
  std::string& error, const std::string& label)
{
  std::vector<std::string> modules;
  std::vector<std::string> searchPaths;
  vtkSlangShaderManager::StringPairs specializations;
  vtkSlangShaderManager::StringPairs allDefines = defines;
  if (auto* wgpuProperty = vtkWebGPUShaderProperty::SafeDownCast(property))
  {
    if (wgpuProperty->HasSlang())
    {
      const char* module = wgpuProperty->GetSlangModule();
      if (module && *module)
      {
        modules.emplace_back(module);
      }
      const auto interfaces = GetProgramInterfaces(program.Source);
      for (int i = 0; i < wgpuProperty->GetNumberOfSlangSpecializations(); ++i)
      {
        const char* name = wgpuProperty->GetNthSlangSpecializationInterface(i);
        if (TakesInterface(interfaces, name))
        {
          specializations.emplace_back(name, wgpuProperty->GetNthSlangSpecializationType(i));
        }
      }
      for (int i = 0; i < wgpuProperty->GetNumberOfSlangSearchPaths(); ++i)
      {
        searchPaths.emplace_back(wgpuProperty->GetNthSlangSearchPath(i));
      }
    }
  }

  std::lock_guard<std::mutex> lock(GetManagerMutex());
  vtkSlangShaderManager* manager = GetManager();
  std::string compiled = manager->CompileSourceToWGSL(
    program.Source, program.EntryPoints, allDefines, modules, specializations, searchPaths, label);
  if (compiled.empty())
  {
    error = "the Slang shaders failed to compile:\n" + manager->GetLastDiagnostics();
    return false;
  }

  using Binding = vtkSlangShaderManager::Binding;
  const std::uint32_t group = program.ParameterGroup;
  const std::uint32_t first = program.FirstParameterBinding;
  Parameters plan = manager->GetLastPlan([group, first](const Binding& binding)
    { return binding.Group != group || binding.Index < first; });
  if (!plan.Blocks.empty())
  {
    const auto& block = plan.Blocks.front();
    return Unsupported(block.Name, block.Group, block.Index, error);
  }
  if (!plan.Resources.empty())
  {
    const auto& resource = plan.Resources.front();
    return Unsupported(resource.Name, resource.Group, resource.Index, error);
  }
  if (!plan.Unsupported.empty())
  {
    const auto& binding = plan.Unsupported.front();
    return Unsupported(binding.Name, binding.Group, binding.Index, error);
  }
  parameters = std::move(plan);
  wgsl = std::move(compiled);
  return true;
}

VTK_ABI_NAMESPACE_END
