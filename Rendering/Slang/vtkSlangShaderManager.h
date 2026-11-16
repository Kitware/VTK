// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSlangShaderManager
 * @brief   compile Slang shaders to WGSL with SlangKit
 *
 * vtkSlangShaderManager compiles Slang source to WGSL and reflects the bindings of the result.
 * When VTK is built without SlangKit (VTK_USE_SLANG is OFF), IsSlangAvailable() returns false
 * and every compile fails with a diagnostic.
 *
 * GetLastBindings(), GetLastPlan() and GetLastDiagnostics() describe the most recent compile on
 * an instance, so an instance must not be shared by threads that compile at the same time. Use
 * one instance per thread, or hold a lock from a compile until its results have been read.
 */
#ifndef vtkSlangShaderManager_h
#define vtkSlangShaderManager_h

#include "vtkObject.h"
#include "vtkRenderingSlangModule.h" // For export macro

#include "vtkWrappingHints.h" // For VTK_WRAPEXCLUDE

#include <cstdint>    // For std::uint8_t
#include <functional> // For std::function
#include <memory>     // For std::unique_ptr
#include <string>     // For std::string
#include <utility>    // For std::pair
#include <vector>     // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGSLANG_EXPORT vtkSlangShaderManager : public vtkObject
{
public:
  static vtkSlangShaderManager* New();
  vtkTypeMacro(vtkSlangShaderManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static bool IsSlangAvailable();

  std::string CompileSourceToWGSL(
    const std::string& source, const std::vector<std::string>& entryPoints = {});

  using StringPairs = std::vector<std::pair<std::string, std::string>>;

  VTK_WRAPEXCLUDE std::string CompileSourceToWGSL(const std::string& source,
    const std::vector<std::string>& entryPoints, const StringPairs& defines,
    const std::vector<std::string>& modules, const StringPairs& specializations,
    const std::vector<std::string>& searchPaths = {}, const std::string& label = std::string());

  void SetCacheDirectory(const std::string& directory);

  static std::vector<std::string> GetLibraryModules();
  static std::string GetLibraryModuleSource(const std::string& name);
  static bool WriteLibraryModules(const std::string& directory);

  struct UniformField
  {
    std::string Name;
    size_t Offset = 0;
    size_t SizeBytes = 0;
    std::string Type;
    int ScalarType = 0;
    unsigned int Width = 0;
    unsigned int Runs = 0;
    size_t Stride = 0;
    std::vector<std::pair<std::string, std::vector<std::string>>> Attributes;
  };

  struct Binding
  {
    enum KindType
    {
      UniformBuffer,
      StorageBuffer,
      Texture,
      Sampler,
      Other
    };
    enum ViewDimensionType
    {
      NoDimension,
      Dimension1D,
      Dimension2D,
      Dimension2DArray,
      Dimension3D,
      DimensionCube,
      DimensionCubeArray
    };
    std::string Name;
    unsigned int Group = 0;
    unsigned int Index = 0;
    KindType Kind = Other;
    ViewDimensionType ViewDimension = NoDimension;
    size_t SizeBytes = 0;
    std::vector<UniformField> Fields;
  };

  VTK_WRAPEXCLUDE const std::vector<Binding>& GetLastBindings() const;

  struct Plan
  {
    struct Block
    {
      std::string Name;
      unsigned int Group = 0;
      unsigned int Index = 0;
      size_t SizeBytes = 0;
      std::vector<UniformField> Fields;
    };
    struct Resource
    {
      std::string Name;
      std::string Field;
      unsigned int Group = 0;
      unsigned int Index = 0;
      Binding::KindType Kind = Binding::Other;
      Binding::ViewDimensionType ViewDimension = Binding::NoDimension;
      int Texture = -1;
    };
    std::vector<Block> Blocks;
    std::vector<Resource> Resources;
    std::vector<Binding> Unsupported;
    VTKRENDERINGSLANG_EXPORT bool operator==(const Plan& other) const;
    bool operator!=(const Plan& other) const { return !(*this == other); }

    struct Data;
    std::shared_ptr<const Data> Native;
  };

  struct HostValue
  {
    int ScalarType = VTK_FLOAT;
    unsigned int Width = 1;
    unsigned int Runs = 1;
    const void* Values = nullptr;
  };

  enum PackStatus
  {
    Packed,
    NoField,
    Unsupported,
    ScalarMismatch,
    ShapeMismatch
  };

  VTK_WRAPEXCLUDE Plan GetLastPlan(const std::function<bool(const Binding&)>& hostOwned) const;

  VTK_WRAPEXCLUDE static std::vector<std::uint8_t> BlockBytes(const Plan& plan, size_t block);

  VTK_WRAPEXCLUDE static PackStatus Pack(const Plan& plan, size_t block, const std::string& field,
    const HostValue& value, std::vector<std::uint8_t>& bytes);

  VTK_WRAPEXCLUDE static std::vector<std::string> Unmatched(
    const Plan& plan, const std::vector<std::string>& names);

  VTK_WRAPEXCLUDE static std::string GetParameterSchema(const Plan& plan);

  const std::string& GetLastDiagnostics() const;

protected:
  vtkSlangShaderManager();
  ~vtkSlangShaderManager() override;

private:
  vtkSlangShaderManager(const vtkSlangShaderManager&) = delete;
  void operator=(const vtkSlangShaderManager&) = delete;

  struct Internal;
  std::unique_ptr<Internal> Pimpl;
};

VTK_ABI_NAMESPACE_END
#endif
