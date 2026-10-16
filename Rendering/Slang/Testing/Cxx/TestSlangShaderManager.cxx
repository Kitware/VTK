// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNew.h"
#include "vtkSlangShaderManager.h"
#include "vtkTestErrorObserver.h"
#include "vtkTesting.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
const char* const Source = R"(
import vtk_test_material;
type_param M : ITestMaterial;
typealias Default_ITestMaterial = Plain;
[vk::binding(0, 0)] RWStructuredBuffer<float4> outColor;
[vk::binding(0, 1)] ConstantBuffer<M> material;
[shader("compute")] [numthreads(1, 1, 1)]
void run(uint3 id : SV_DispatchThreadID) { outColor[id.x] = material.shade() * SCALE; }
)";

const char* const Module = R"(
module vtk_test_material;
public interface ITestMaterial { float4 shade(); };
public struct Plain : ITestMaterial { public float4 shade() { return float4(1.f); } };
public struct Tinted : ITestMaterial {
  float4 tint;
  float3x3 colorMatrix;
  public float4 shade() { return float4(mul(colorMatrix, tint.rgb), tint.a); }
};
)";

bool Contains(const std::string& text, const std::string& part)
{
  return text.find(part) != std::string::npos;
}

bool Check(bool condition, const char* message, vtkSlangShaderManager* manager)
{
  if (!condition)
  {
    std::cerr << "FAILED: " << message << "\n" << manager->GetLastDiagnostics() << "\n";
  }
  return condition;
}
}

int TestSlangShaderManager(int, char*[])
{
  vtkNew<vtkSlangShaderManager> manager;
  if (!vtkSlangShaderManager::IsSlangAvailable())
  {
    vtkNew<vtkTest::ErrorObserver> errors;
    manager->AddObserver(vtkCommand::ErrorEvent, errors);
    const bool reported =
      manager->CompileSourceToWGSL(Source).empty() && !manager->GetLastDiagnostics().empty();
    std::cout << "Slang is unavailable; skipping.\n";
    return reported && errors->GetError() ? VTK_SKIP_RETURN_CODE : EXIT_FAILURE;
  }

  bool success = true;
  const vtkSlangShaderManager::StringPairs defines{ { "SCALE", "0.5f" } };
  std::string wgsl = manager->CompileSourceToWGSL(Source, {}, defines, { Module }, {});
  success &= Check(Contains(wgsl, "fn run(") && Contains(wgsl, "0.5"),
    "a source compiles with defines and a module it imports", manager);

  wgsl = manager->CompileSourceToWGSL(
    Source, { "run" }, defines, { Module }, { { "ITestMaterial", "Tinted" } });
  const auto& bindings = manager->GetLastBindings();
  success &=
    Check(!wgsl.empty() && bindings.size() == 2, "a specialization adds its block", manager);

  const auto plan =
    manager->GetLastPlan([](const vtkSlangShaderManager::Binding& b) { return b.Group == 0; });
  success &=
    Check(plan.Blocks.size() == 1 && plan.Blocks[0].Group == 1 && plan.Blocks[0].Fields.size() == 2,
      "the plan lists the consumer block and its fields", manager);

  std::vector<std::uint8_t> bytes = vtkSlangShaderManager::BlockBytes(plan, 0);
  const float tint[4] = { 1.f, 0.5f, 0.25f, 1.f };
  const vtkSlangShaderManager::HostValue value{ VTK_FLOAT, 4, 1, tint };
  success &= Check(
    vtkSlangShaderManager::Pack(plan, 0, "tint", value, bytes) == vtkSlangShaderManager::Packed &&
      vtkSlangShaderManager::Pack(plan, 0, "nope", value, bytes) ==
        vtkSlangShaderManager::NoField &&
      vtkSlangShaderManager::Unmatched(plan, { "tint", "nope" }) ==
        std::vector<std::string>{ "nope" },
    "values pack by field name", manager);
  success &= Check(Contains(vtkSlangShaderManager::GetParameterSchema(plan), "\"colorMatrix\""),
    "the schema lists the fields", manager);

  vtkNew<vtkTest::ErrorObserver> errors;
  manager->AddObserver(vtkCommand::ErrorEvent, errors);
  wgsl = manager->CompileSourceToWGSL("float4 broken(");
  success &= Check(wgsl.empty() && errors->GetError() && !manager->GetLastDiagnostics().empty(),
    "a failed compile reports its diagnostics", manager);

  const auto modules = vtkSlangShaderManager::GetLibraryModules();
  success &= Check(!modules.empty() &&
      Contains(vtkSlangShaderManager::GetLibraryModuleSource("slangkit_shading"), "IMaterial"),
    "the library modules are listed with their sources", manager);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
