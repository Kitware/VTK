// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSlangShaderManager.h"
#include "vtkObjectFactory.h"

#ifdef VTK_RENDERINGSLANG_USE_SLANGKIT
#include <SlangKitCompiler.h>
#include <SlangKitParameters.h>
#endif

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlangShaderManager);

#ifdef VTK_RENDERINGSLANG_USE_SLANGKIT

namespace
{
//------------------------------------------------------------------------------
vtkSlangShaderManager::Binding::KindType ToKind(slangkit::BindingKind kind)
{
  using Binding = vtkSlangShaderManager::Binding;
  switch (kind)
  {
    case slangkit::BindingKind::UniformBuffer:
      return Binding::UniformBuffer;
    case slangkit::BindingKind::ReadOnlyStorageBuffer:
    case slangkit::BindingKind::StorageBuffer:
      return Binding::StorageBuffer;
    case slangkit::BindingKind::SampledTexture:
      return Binding::Texture;
    case slangkit::BindingKind::Sampler:
      return Binding::Sampler;
    default:
      return Binding::Other;
  }
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::Binding::ViewDimensionType ToViewDimension(slangkit::TextureDimension d)
{
  using Binding = vtkSlangShaderManager::Binding;
  switch (d)
  {
    case slangkit::TextureDimension::D1:
      return Binding::Dimension1D;
    case slangkit::TextureDimension::D2:
      return Binding::Dimension2D;
    case slangkit::TextureDimension::D2Array:
      return Binding::Dimension2DArray;
    case slangkit::TextureDimension::D3:
      return Binding::Dimension3D;
    case slangkit::TextureDimension::Cube:
      return Binding::DimensionCube;
    case slangkit::TextureDimension::CubeArray:
      return Binding::DimensionCubeArray;
    default:
      return Binding::NoDimension;
  }
}

//------------------------------------------------------------------------------
int ToVTKScalar(slangkit::ScalarType scalar)
{
  switch (scalar)
  {
    case slangkit::ScalarType::Float32:
      return VTK_FLOAT;
    case slangkit::ScalarType::Int32:
      return VTK_INT;
    case slangkit::ScalarType::UInt32:
      return VTK_UNSIGNED_INT;
    default:
      return VTK_VOID;
  }
}

//------------------------------------------------------------------------------
slangkit::ScalarType ToSlangScalar(int scalar)
{
  switch (scalar)
  {
    case VTK_FLOAT:
      return slangkit::ScalarType::Float32;
    case VTK_INT:
      return slangkit::ScalarType::Int32;
    case VTK_UNSIGNED_INT:
      return slangkit::ScalarType::UInt32;
    default:
      return slangkit::ScalarType::Unknown;
  }
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::UniformField ToField(const slangkit::UniformField& f)
{
  vtkSlangShaderManager::UniformField field;
  field.Name = f.name;
  field.Offset = static_cast<size_t>(f.offset);
  field.SizeBytes = static_cast<size_t>(f.size);
  field.Type = f.type;
  field.ScalarType = ToVTKScalar(f.scalar);
  field.Width = f.width;
  field.Runs = f.runs;
  field.Stride = static_cast<size_t>(f.stride);
  for (const auto& attribute : f.attributes)
  {
    field.Attributes.emplace_back(attribute.name, attribute.arguments);
  }
  return field;
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::Binding ToBinding(const slangkit::ReflectedBinding& b)
{
  vtkSlangShaderManager::Binding binding;
  binding.Name = b.name;
  binding.Group = b.group;
  binding.Index = b.binding;
  binding.Kind = ToKind(b.kind);
  binding.ViewDimension = ToViewDimension(b.viewDimension);
  binding.SizeBytes = static_cast<size_t>(b.sizeBytes);
  for (const auto& f : b.fields)
  {
    binding.Fields.push_back(ToField(f));
  }
  return binding;
}
}

struct vtkSlangShaderManager::Plan::Data
{
  slangkit::BindingPlan Plan;
};

struct vtkSlangShaderManager::Internal
{
  slangkit::Compiler Compiler;
  std::string LastDiagnostics;
  std::vector<Binding> LastBindings;
  slangkit::Artifact LastReflection;

  void Record(const slangkit::Artifact& artifact)
  {
    this->LastDiagnostics = artifact.diagnostics;
    this->LastBindings.clear();
    this->LastReflection = slangkit::Artifact();
    this->LastReflection.bindings = artifact.bindings;
    for (const auto& b : artifact.bindings)
    {
      if (b.used)
      {
        this->LastBindings.push_back(ToBinding(b));
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkSlangShaderManager::vtkSlangShaderManager()
  : Pimpl(std::make_unique<Internal>())
{
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::~vtkSlangShaderManager() = default;

//------------------------------------------------------------------------------
bool vtkSlangShaderManager::IsSlangAvailable()
{
  return slangkit::Compiler::slangAvailable();
}

//------------------------------------------------------------------------------
std::string vtkSlangShaderManager::CompileSourceToWGSL(
  const std::string& source, const std::vector<std::string>& entryPoints)
{
  return this->CompileSourceToWGSL(source, entryPoints, {}, {}, {});
}

//------------------------------------------------------------------------------
std::string vtkSlangShaderManager::CompileSourceToWGSL(const std::string& source,
  const std::vector<std::string>& entryPoints, const StringPairs& defines,
  const std::vector<std::string>& modules, const StringPairs& specializations,
  const std::vector<std::string>& searchPaths, const std::string& label)
{
  slangkit::CompileOptions options;
  options.label = label;
  options.defines = defines;
  options.modules = modules;
  options.specializations = specializations;
  options.searchPaths = searchPaths;
  slangkit::Artifact artifact = this->Pimpl->Compiler.compileSource(source, entryPoints, options);
  this->Pimpl->Record(artifact);
  if (!artifact)
  {
    vtkErrorMacro("Failed to compile Slang source:\n" << artifact.diagnostics);
    return {};
  }
  return artifact.wgsl;
}

//------------------------------------------------------------------------------
void vtkSlangShaderManager::SetCacheDirectory(const std::string& directory)
{
  this->Pimpl->Compiler.setCacheDirectory(directory);
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkSlangShaderManager::GetLibraryModules()
{
  return slangkit::Compiler::libraryModules();
}

//------------------------------------------------------------------------------
std::string vtkSlangShaderManager::GetLibraryModuleSource(const std::string& name)
{
  return slangkit::Compiler::librarySource(name);
}

//------------------------------------------------------------------------------
bool vtkSlangShaderManager::WriteLibraryModules(const std::string& directory)
{
  return slangkit::Compiler::writeLibraryModules(directory);
}

//------------------------------------------------------------------------------
const std::vector<vtkSlangShaderManager::Binding>& vtkSlangShaderManager::GetLastBindings() const
{
  return this->Pimpl->LastBindings;
}

//------------------------------------------------------------------------------
const std::string& vtkSlangShaderManager::GetLastDiagnostics() const
{
  return this->Pimpl->LastDiagnostics;
}

//------------------------------------------------------------------------------
bool vtkSlangShaderManager::Plan::operator==(const Plan& other) const
{
  static const slangkit::BindingPlan empty;
  const auto& a = this->Native ? this->Native->Plan : empty;
  const auto& b = other.Native ? other.Native->Plan : empty;
  return a == b;
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::Plan vtkSlangShaderManager::GetLastPlan(
  const std::function<bool(const Binding&)>& hostOwned) const
{
  auto data = std::make_shared<Plan::Data>();
  data->Plan = slangkit::planBindings(this->Pimpl->LastReflection,
    [&hostOwned](const slangkit::ReflectedBinding& b)
    { return !b.used || (hostOwned && hostOwned(ToBinding(b))); });
  Plan plan;
  for (const auto& b : data->Plan.blocks)
  {
    Plan::Block block;
    block.Name = b.name;
    block.Group = b.group;
    block.Index = b.binding;
    block.SizeBytes = static_cast<size_t>(b.sizeBytes);
    for (const auto& f : b.fields)
    {
      block.Fields.push_back(ToField(f));
    }
    plan.Blocks.push_back(std::move(block));
  }
  for (const auto& r : data->Plan.resources)
  {
    Plan::Resource resource;
    resource.Name = r.name;
    resource.Field = r.field;
    resource.Group = r.group;
    resource.Index = r.binding;
    resource.Kind = ToKind(r.kind);
    resource.ViewDimension = ToViewDimension(r.viewDimension);
    resource.Texture = r.texture;
    plan.Resources.push_back(std::move(resource));
  }
  for (const auto& b : data->Plan.unsupported)
  {
    plan.Unsupported.push_back(ToBinding(b));
  }
  plan.Native = std::move(data);
  return plan;
}

//------------------------------------------------------------------------------
std::vector<std::uint8_t> vtkSlangShaderManager::BlockBytes(const Plan& plan, size_t block)
{
  if (!plan.Native || block >= plan.Native->Plan.blocks.size())
  {
    return {};
  }
  return slangkit::blockBytes(plan.Native->Plan.blocks[block]);
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::PackStatus vtkSlangShaderManager::Pack(const Plan& plan, size_t block,
  const std::string& field, const HostValue& value, std::vector<std::uint8_t>& bytes)
{
  if (!plan.Native || block >= plan.Native->Plan.blocks.size())
  {
    return NoField;
  }
  const slangkit::UniformField* f = slangkit::findField(plan.Native->Plan.blocks[block], field);
  if (!f)
  {
    return NoField;
  }
  const slangkit::HostValue host{ ToSlangScalar(value.ScalarType), value.Width, value.Runs,
    value.Values };
  switch (slangkit::pack(*f, host, bytes))
  {
    case slangkit::PackStatus::Packed:
      return Packed;
    case slangkit::PackStatus::NoField:
      return NoField;
    case slangkit::PackStatus::Unsupported:
      return Unsupported;
    case slangkit::PackStatus::ScalarMismatch:
      return ScalarMismatch;
    default:
      return ShapeMismatch;
  }
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkSlangShaderManager::Unmatched(
  const Plan& plan, const std::vector<std::string>& names)
{
  if (!plan.Native)
  {
    return names;
  }
  return slangkit::unmatched(plan.Native->Plan, names);
}

//------------------------------------------------------------------------------
std::string vtkSlangShaderManager::GetParameterSchema(const Plan& plan)
{
  return slangkit::parameterSchema(plan.Native ? plan.Native->Plan : slangkit::BindingPlan());
}

#else

struct vtkSlangShaderManager::Plan::Data
{
};

struct vtkSlangShaderManager::Internal
{
  std::string LastDiagnostics;
  std::vector<Binding> LastBindings;
};

//------------------------------------------------------------------------------
vtkSlangShaderManager::vtkSlangShaderManager()
  : Pimpl(std::make_unique<Internal>())
{
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::~vtkSlangShaderManager() = default;

//------------------------------------------------------------------------------
bool vtkSlangShaderManager::IsSlangAvailable()
{
  return false;
}

//------------------------------------------------------------------------------
std::string vtkSlangShaderManager::CompileSourceToWGSL(
  const std::string& source, const std::vector<std::string>& entryPoints)
{
  return this->CompileSourceToWGSL(source, entryPoints, {}, {}, {});
}

//------------------------------------------------------------------------------
std::string vtkSlangShaderManager::CompileSourceToWGSL(const std::string&,
  const std::vector<std::string>&, const StringPairs&, const std::vector<std::string>&,
  const StringPairs&, const std::vector<std::string>&, const std::string&)
{
  this->Pimpl->LastDiagnostics = "VTK was built without SlangKit (VTK_USE_SLANG is OFF)\n";
  vtkErrorMacro("Failed to compile Slang source:\n" << this->Pimpl->LastDiagnostics);
  return {};
}

//------------------------------------------------------------------------------
void vtkSlangShaderManager::SetCacheDirectory(const std::string&) {}

//------------------------------------------------------------------------------
std::vector<std::string> vtkSlangShaderManager::GetLibraryModules()
{
  return {};
}

//------------------------------------------------------------------------------
std::string vtkSlangShaderManager::GetLibraryModuleSource(const std::string&)
{
  return {};
}

//------------------------------------------------------------------------------
bool vtkSlangShaderManager::WriteLibraryModules(const std::string&)
{
  return false;
}

//------------------------------------------------------------------------------
const std::vector<vtkSlangShaderManager::Binding>& vtkSlangShaderManager::GetLastBindings() const
{
  return this->Pimpl->LastBindings;
}

//------------------------------------------------------------------------------
const std::string& vtkSlangShaderManager::GetLastDiagnostics() const
{
  return this->Pimpl->LastDiagnostics;
}

//------------------------------------------------------------------------------
bool vtkSlangShaderManager::Plan::operator==(const Plan& other) const
{
  return this->Blocks.empty() && this->Resources.empty() && other.Blocks.empty() &&
    other.Resources.empty();
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::Plan vtkSlangShaderManager::GetLastPlan(
  const std::function<bool(const Binding&)>&) const
{
  return {};
}

//------------------------------------------------------------------------------
std::vector<std::uint8_t> vtkSlangShaderManager::BlockBytes(const Plan&, size_t)
{
  return {};
}

//------------------------------------------------------------------------------
vtkSlangShaderManager::PackStatus vtkSlangShaderManager::Pack(
  const Plan&, size_t, const std::string&, const HostValue&, std::vector<std::uint8_t>&)
{
  return NoField;
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkSlangShaderManager::Unmatched(
  const Plan&, const std::vector<std::string>& names)
{
  return names;
}

//------------------------------------------------------------------------------
std::string vtkSlangShaderManager::GetParameterSchema(const Plan&)
{
  return "{\n  \"blocks\": [],\n  \"resources\": []\n}\n";
}

#endif

//------------------------------------------------------------------------------
void vtkSlangShaderManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SlangAvailable: " << (IsSlangAvailable() ? "true" : "false") << "\n";
}

VTK_ABI_NAMESPACE_END
