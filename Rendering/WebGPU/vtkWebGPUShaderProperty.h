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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkShaderProperty.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkWebGPUShaderProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkWebGPUShaderProperty, vtkShaderProperty);

  /**
   * Construct object with no shader replacements
   */
  static vtkWebGPUShaderProperty* New();

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
    const std::string&, bool){};
  void AddFragmentShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool){};
  void AddGeometryShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool){};
  void AddTessControlShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool){};
  void AddTessEvaluationShaderReplacement(const std::string&,
    bool, // do this replacement before the default
    const std::string&, bool){};
  int GetNumberOfShaderReplacements() { return 0; };
  std::string GetNthShaderReplacementTypeAsString(vtkIdType) { return ""; };
  void GetNthShaderReplacement(vtkIdType, std::string&, bool&, std::string&, bool&){};
  void ClearVertexShaderReplacement(const std::string&, bool){};
  void ClearFragmentShaderReplacement(const std::string&, bool){};
  void ClearGeometryShaderReplacement(const std::string&, bool){};
  void ClearTessControlShaderReplacement(const std::string&, bool){};
  void ClearTessEvaluationShaderReplacement(const std::string&, bool){};
  void ClearAllVertexShaderReplacements(){};
  void ClearAllFragmentShaderReplacements(){};
  void ClearAllGeometryShaderReplacements(){};
  void ClearAllTessControlShaderReplacements(){};
  void ClearAllTessEvalShaderReplacements(){};
  void ClearAllShaderReplacements(){};
  ///@}

protected:
  vtkWebGPUShaderProperty();
  ~vtkWebGPUShaderProperty() override;

private:
  vtkWebGPUShaderProperty(const vtkWebGPUShaderProperty&) = delete;
  void operator=(const vtkWebGPUShaderProperty&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
