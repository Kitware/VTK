// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUShaderProperty
 * @brief   represent GPU shader properties
 *
 * vtkWebGPUPolyDataMapper's shaders are Slang
 * (Rendering/WebGPU/slang/vtkPolyDataSurfaceSlang.slang) and are customized
 * with Slang types implementing SlangKit interfaces, selected by name. A
 * specialization of an interface that the program does not take warns and is
 * ignored.
 *
 * String shader replacements (AddVertexShaderReplacement and friends) and
 * shader code (SetVertexShaderCode and friends) are not supported: they warn
 * and are ignored. Their typed equivalent is IShaderHooks (module
 * slangkit_hooks): one method per insertion point, each a no-op unless
 * overridden.
 *
 * | Method     | Runs                                   | OpenGL tags it replaces            |
 * |------------|----------------------------------------|------------------------------------|
 * | vertex()   | vertex stage, before projection        | CustomBegin, Normal, TCoord, Color |
 * | position() | vertex stage, after projection         | PositionVC, CustomEnd              |
 * | clip()     | fragment stage, first; false discards  | Clip                               |
 * | color()    | after VTK's colors (scalars, textures) | Color, TCoord, TMap                |
 * | normal()   | after VTK's normal and edges           | Normal, Edges                      |
 * | lit()      | after lighting                         | Light, PBR                         |
 * | output()   | last: color, depth, picking ids        | Depth, Coincident, Picking, PrimID |
 *
 * \code{.cpp}
 * prop->SetSlangModule(R"(
 *   module my_hooks;
 *   import slangkit_hooks;
 *   public struct Stripes : IShaderHooks {
 *     public override void color(inout FragmentContext f) {
 *       if (frac(f.positionVC.y * 4.f) > 0.5f)
 *         f.diffuseColor = float3(1.f, 0.f, 0.f);
 *     }
 *   };)");
 * prop->SetSlangSpecialization("IShaderHooks", "Stripes");
 * \endcode
 *
 * The module can also be read from a .slang file (LoadSlangModule),
 * where Slang's language server (slangd) gives completion and diagnostics:
 * point its search paths at SlangKit's shaders directory.
 *
 * Higher-level interfaces (module slangkit_shading) replace whole parts of
 * the shading and are shared with anari-webgpu: IMaterial, ILightingModel,
 * INormalModifier, ISurfaceOutput (discard, final color, depth) and
 * IVertexModifier.
 *
 * A module or type that fails to compile reports an error, and the mapper
 * renders its program's default types instead. Types with fields are not
 * supported yet and are reported the same way. Selecting any type, or a
 * module, turns the Slang shaders on for all of the mapper's pipelines
 * (points, lines, triangles). UseSlang turns them on with the default types.
 */

#ifndef vtkWebGPUShaderProperty_h
#define vtkWebGPUShaderProperty_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkShaderProperty.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <string>  // For std::string
#include <utility> // For std::pair
#include <vector>  // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUShaderProperty : public vtkShaderProperty
{
public:
  static vtkWebGPUShaderProperty* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUShaderProperty, vtkShaderProperty);

  ///@{
  /**
   * Not supported: the Slang shaders are customized with typed hooks
   * (IShaderHooks, see the class documentation) instead. These warn and do
   * nothing.
   */
  void AddVertexShaderReplacement(const std::string&, bool, const std::string&, bool) override;
  void AddFragmentShaderReplacement(const std::string&, bool, const std::string&, bool) override;
  void AddGeometryShaderReplacement(const std::string&, bool, const std::string&, bool) override;
  void AddTessControlShaderReplacement(const std::string&, bool, const std::string&, bool) override;
  void AddTessEvaluationShaderReplacement(
    const std::string&, bool, const std::string&, bool) override;
  int GetNumberOfShaderReplacements() override { return 0; };
  std::string GetNthShaderReplacementTypeAsString(vtkIdType) override { return ""; };
  void GetNthShaderReplacement(vtkIdType, std::string&, bool&, std::string&, bool&) override {};
  void ClearVertexShaderReplacement(const std::string&, bool) override {};
  void ClearFragmentShaderReplacement(const std::string&, bool) override {};
  void ClearGeometryShaderReplacement(const std::string&, bool) override {};
  void ClearTessControlShaderReplacement(const std::string&, bool) override {};
  void ClearTessEvaluationShaderReplacement(const std::string&, bool) override {};
  void ClearAllVertexShaderReplacements() override {};
  void ClearAllFragmentShaderReplacements() override {};
  void ClearAllGeometryShaderReplacements() override {};
  void ClearAllTessControlShaderReplacements() override {};
  void ClearAllTessEvalShaderReplacements() override {};
  void ClearAllShaderReplacements() override {};
  ///@}

  vtkSetMacro(UseSlang, bool);
  vtkGetMacro(UseSlang, bool);
  vtkBooleanMacro(UseSlang, bool);

  vtkSetStdStringFromCharMacro(SlangModule);
  vtkGetCharFromStdStringMacro(SlangModule);

  bool LoadSlangModule(const char* fileName);

  void AddSlangSearchPath(const char* directory);
  void RemoveAllSlangSearchPaths();
  int GetNumberOfSlangSearchPaths();
  const char* GetNthSlangSearchPath(int n);

  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  void SetSlangSpecialization(const char* interfaceName, const char* typeName);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  const char* GetSlangSpecialization(const char* interfaceName);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  void RemoveAllSlangSpecializations();
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  int GetNumberOfSlangSpecializations();
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  const char* GetNthSlangSpecializationInterface(int n);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  const char* GetNthSlangSpecializationType(int n);

  bool HasSlang();

protected:
  vtkWebGPUShaderProperty();
  ~vtkWebGPUShaderProperty() override;

  bool UseSlang = false;
  std::string SlangModule;
  std::vector<std::pair<std::string, std::string>> SlangSpecializations;
  std::vector<std::string> SlangSearchPaths;

private:
  vtkWebGPUShaderProperty(const vtkWebGPUShaderProperty&) = delete;
  void operator=(const vtkWebGPUShaderProperty&) = delete;
};

#define vtkWebGPUShaderProperty_OVERRIDE_ATTRIBUTES                                                \
  vtkWebGPUShaderProperty::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
