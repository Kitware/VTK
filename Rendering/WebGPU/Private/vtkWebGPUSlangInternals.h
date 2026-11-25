// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUSlangInternals_h
#define vtkWebGPUSlangInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtkSlangShaderManager.h"
#include "vtkType.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkObject;
class vtkShaderProperty;

class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUSlangInternals
{
public:
  static constexpr std::uint32_t ParameterBinding = 16;

  using Parameters = vtkSlangShaderManager::Plan;

  struct Program
  {
    const char* Source = nullptr;
    std::vector<std::string> EntryPoints;
    std::uint32_t ParameterGroup = 1;
    std::uint32_t FirstParameterBinding = ParameterBinding;
  };

  static const Program& GetSurfaceProgram();

  static bool IsRequested(vtkShaderProperty* property);

  static bool GetGlobalRequest();
  static void SetGlobalRequest(bool request);
  static vtkMTimeType GetRequestMTime(vtkShaderProperty* property);

  static std::string GetUnsupportedCustomizations(vtkShaderProperty* property);

  static std::string GetIgnoredSpecializations(const Program& program, vtkShaderProperty* property);

  static void ReportIgnoredSpecializations(vtkObject* reporter, const Program& program,
    vtkShaderProperty* property, std::string& reported);

  static bool Compile(vtkShaderProperty* property,
    const vtkSlangShaderManager::StringPairs& defines, std::string& wgsl, Parameters& parameters,
    std::string& error, const std::string& label = std::string());

  static bool Compile(const Program& program, vtkShaderProperty* property,
    const vtkSlangShaderManager::StringPairs& defines, std::string& wgsl, Parameters& parameters,
    std::string& error, const std::string& label = std::string(),
    std::string* customizationError = nullptr);

private:
  static bool CompileOnce(const Program& program, vtkShaderProperty* property,
    const vtkSlangShaderManager::StringPairs& defines, std::string& wgsl, Parameters& parameters,
    std::string& error, const std::string& label);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUSlangInternals.h
