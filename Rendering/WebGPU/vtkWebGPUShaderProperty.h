// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUShaderProperty
 * @brief   represent GPU shader properties
 *
 * vtkWebGPUShaderProperty is a placeholder empty class. It only exists so that vtkProp::ShallowCopy
 * succeeds. This class is not implemented right now because shader replacements do not make
 * complete sense when working with vtkWebGPUPolyDataMapper.
 */

#ifndef vtkWebGPUShaderProperty_h
#define vtkWebGPUShaderProperty_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkShaderProperty.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUShaderProperty : public vtkShaderProperty
{
public:
  static vtkWebGPUShaderProperty* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUShaderProperty, vtkShaderProperty);

  ///@{
  /**
   * This function enables you to apply your own substitutions
   * to the shader creation process. The shader code in this class
   * is created by applying a bunch of string replacements to a
   * shader template. Using this function you can apply your
   * own string replacements to add features you desire.
   */
  void AddVertexShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool) override{};
  void AddFragmentShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool) override{};
  void AddGeometryShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool) override{};
  void AddTessControlShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool) override{};
  void AddTessEvaluationShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool) override{};
  int GetNumberOfShaderReplacements() override { return 0; };
  std::string GetNthShaderReplacementTypeAsString(vtkIdType) override { return ""; };
  void GetNthShaderReplacement(vtkIdType, std::string&, bool&, std::string&, bool&) override{};
  void ClearVertexShaderReplacement(const std::string&, bool) override{};
  void ClearFragmentShaderReplacement(const std::string&, bool) override{};
  void ClearGeometryShaderReplacement(const std::string&, bool) override{};
  void ClearTessControlShaderReplacement(const std::string&, bool) override{};
  void ClearTessEvaluationShaderReplacement(const std::string&, bool) override{};
  void ClearAllVertexShaderReplacements() override{};
  void ClearAllFragmentShaderReplacements() override{};
  void ClearAllGeometryShaderReplacements() override{};
  void ClearAllTessControlShaderReplacements() override{};
  void ClearAllTessEvalShaderReplacements() override{};
  void ClearAllShaderReplacements() override{};
  ///@}

protected:
  vtkWebGPUShaderProperty();
  ~vtkWebGPUShaderProperty() override;

private:
  vtkWebGPUShaderProperty(const vtkWebGPUShaderProperty&) = delete;
  void operator=(const vtkWebGPUShaderProperty&) = delete;
};

#define vtkWebGPUShaderProperty_OVERRIDE_ATTRIBUTES                                                \
  vtkWebGPUShaderProperty::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
