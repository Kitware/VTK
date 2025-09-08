// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLShaderProperty
 * @brief   represent GPU shader properties
 *
 * vtkOpenGLShaderProperty is used to hold user-defined modifications of a
 * GPU shader program used in a mapper.
 *
 * @sa
 * vtkShaderProperty vtkUniforms vtkOpenGLUniform
 *
 * @par Thanks:
 * Developed by Simon Drouin (sdrouin2@bwh.harvard.edu) at Brigham and Women's Hospital.
 *
 */

#ifndef vtkOpenGLShaderProperty_h
#define vtkOpenGLShaderProperty_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkShader.h"                 // For methods (shader types)
#include "vtkShaderProperty.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO
#include <map>                // used for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLUniforms;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLShaderProperty : public vtkShaderProperty
{
public:
  vtkTypeMacro(vtkOpenGLShaderProperty, vtkShaderProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with no shader replacements
   */
  static vtkOpenGLShaderProperty* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(vtkOpenGLShaderProperty* p);

  void AddVertexShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;
  void AddFragmentShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;
  void AddGeometryShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;
  void AddTessControlShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;
  void AddTessEvaluationShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;

  int GetNumberOfShaderReplacements() override;
  std::string GetNthShaderReplacementTypeAsString(vtkIdType index) override;
  void GetNthShaderReplacement(vtkIdType index, std::string& name, bool& replaceFirst,
    std::string& replacementValue, bool& replaceAll) override;

  void ClearVertexShaderReplacement(const std::string& originalValue, bool replaceFirst) override;
  void ClearFragmentShaderReplacement(const std::string& originalValue, bool replaceFirst) override;
  void ClearGeometryShaderReplacement(const std::string& originalValue, bool replaceFirst) override;
  void ClearTessControlShaderReplacement(
    const std::string& originalValue, bool replaceFirst) override;
  void ClearTessEvaluationShaderReplacement(
    const std::string& originalValue, bool replaceFirst) override;
  void ClearAllVertexShaderReplacements() override;
  void ClearAllFragmentShaderReplacements() override;
  void ClearAllGeometryShaderReplacements() override;
  void ClearAllTessControlShaderReplacements() override;
  void ClearAllTessEvalShaderReplacements() override;
  void ClearAllShaderReplacements() override;

  ///@{
  /**
   * This function enables you to apply your own substitutions
   * to the shader creation process. The shader code in this class
   * is created by applying a bunch of string replacements to a
   * shader template. Using this function you can apply your
   * own string replacements to add features you desire.
   */
  void AddShaderReplacement(vtkShader::Type shaderType, // vertex, fragment, etc
    const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll);
  void ClearShaderReplacement(vtkShader::Type shaderType, // vertex, fragment, etc
    const std::string& originalValue, bool replaceFirst);
  void ClearAllShaderReplacements(vtkShader::Type shaderType);
  ///@}

  typedef std::map<vtkShader::ReplacementSpec, vtkShader::ReplacementValue> ReplacementMap;
  /**
   * @brief GetAllShaderReplacements returns all user-specified shader
   * replacements. It is provided for iteration purposes only (const)
   * and is mainly used by mappers when building the shaders.
   * @return const reference to internal map holding all replacements
   */
  const ReplacementMap& GetAllShaderReplacements() { return this->UserShaderReplacements; }

protected:
  vtkOpenGLShaderProperty();
  ~vtkOpenGLShaderProperty() override;

  ReplacementMap UserShaderReplacements;

private:
  vtkOpenGLShaderProperty(const vtkOpenGLShaderProperty&) = delete;
  void operator=(const vtkOpenGLShaderProperty&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
