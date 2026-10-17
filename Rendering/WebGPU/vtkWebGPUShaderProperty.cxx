// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUShaderProperty.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtksys/FStream.hxx"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUShaderProperty);

vtkWebGPUShaderProperty::vtkWebGPUShaderProperty() = default;

vtkWebGPUShaderProperty::~vtkWebGPUShaderProperty() = default;

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::SetSlangSpecialization(
  const char* interfaceName, const char* typeName)
{
  if (!interfaceName)
  {
    return;
  }
  for (auto it = this->SlangSpecializations.begin(); it != this->SlangSpecializations.end(); ++it)
  {
    if (it->first == interfaceName)
    {
      if (!typeName || !*typeName)
      {
        this->SlangSpecializations.erase(it);
        this->Modified();
      }
      else if (it->second != typeName)
      {
        it->second = typeName;
        this->Modified();
      }
      return;
    }
  }
  if (typeName && *typeName)
  {
    this->SlangSpecializations.emplace_back(interfaceName, typeName);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
const char* vtkWebGPUShaderProperty::GetSlangSpecialization(const char* interfaceName)
{
  for (const auto& s : this->SlangSpecializations)
  {
    if (interfaceName && s.first == interfaceName)
    {
      return s.second.c_str();
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::RemoveAllSlangSpecializations()
{
  if (!this->SlangSpecializations.empty())
  {
    this->SlangSpecializations.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkWebGPUShaderProperty::GetNumberOfSlangSpecializations()
{
  return static_cast<int>(this->SlangSpecializations.size());
}

//------------------------------------------------------------------------------
const char* vtkWebGPUShaderProperty::GetNthSlangSpecializationInterface(int n)
{
  return n >= 0 && n < this->GetNumberOfSlangSpecializations()
    ? this->SlangSpecializations[n].first.c_str()
    : nullptr;
}

//------------------------------------------------------------------------------
const char* vtkWebGPUShaderProperty::GetNthSlangSpecializationType(int n)
{
  return n >= 0 && n < this->GetNumberOfSlangSpecializations()
    ? this->SlangSpecializations[n].second.c_str()
    : nullptr;
}

//------------------------------------------------------------------------------
bool vtkWebGPUShaderProperty::HasSlang()
{
  return this->UseSlang || !this->SlangModule.empty() || !this->SlangSpecializations.empty();
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::AddSlangSearchPath(const char* directory)
{
  if (directory && *directory)
  {
    this->SlangSearchPaths.emplace_back(directory);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::RemoveAllSlangSearchPaths()
{
  if (!this->SlangSearchPaths.empty())
  {
    this->SlangSearchPaths.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkWebGPUShaderProperty::GetNumberOfSlangSearchPaths()
{
  return static_cast<int>(this->SlangSearchPaths.size());
}

//------------------------------------------------------------------------------
const char* vtkWebGPUShaderProperty::GetNthSlangSearchPath(int n)
{
  return n >= 0 && n < this->GetNumberOfSlangSearchPaths() ? this->SlangSearchPaths[n].c_str()
                                                           : nullptr;
}

//------------------------------------------------------------------------------
bool vtkWebGPUShaderProperty::LoadSlangModule(const char* fileName)
{
  vtksys::ifstream file(fileName ? fileName : "", std::ios::binary);
  if (!file)
  {
    vtkErrorMacro(<< "Cannot read the Slang module " << (fileName ? fileName : "(null)"));
    return false;
  }
  std::ostringstream source;
  source << file.rdbuf();
  this->SetSlangModule(source.str().c_str());
  return true;
}

namespace
{
const char* const kReplacementsUnsupported =
  "String shader replacements are not supported on WebGPU and are ignored; customize the "
  "Slang shaders with typed hooks instead (IShaderHooks, see vtkWebGPUShaderProperty).";
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::AddVertexShaderReplacement(
  const std::string&, bool, const std::string&, bool)
{
  vtkWarningMacro(<< kReplacementsUnsupported);
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::AddFragmentShaderReplacement(
  const std::string&, bool, const std::string&, bool)
{
  vtkWarningMacro(<< kReplacementsUnsupported);
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::AddGeometryShaderReplacement(
  const std::string&, bool, const std::string&, bool)
{
  vtkWarningMacro(<< kReplacementsUnsupported);
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::AddTessControlShaderReplacement(
  const std::string&, bool, const std::string&, bool)
{
  vtkWarningMacro(<< kReplacementsUnsupported);
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderProperty::AddTessEvaluationShaderReplacement(
  const std::string&, bool, const std::string&, bool)
{
  vtkWarningMacro(<< kReplacementsUnsupported);
}

vtkOverrideAttribute* vtkWebGPUShaderProperty::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

VTK_ABI_NAMESPACE_END
