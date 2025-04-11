// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkShaderProperty
 * @brief   represent GPU shader properties
 *
 * vtkShaderProperty is used to hold user-defined modifications of a
 * GPU shader program used in a mapper.
 *
 * @sa
 * vtkVolume vtkOpenGLUniform
 *
 * @par Thanks:
 * Developed by Simon Drouin (sdrouin2@bwh.harvard.edu) at Brigham and Women's Hospital.
 *
 */

#ifndef vtkShaderProperty_h
#define vtkShaderProperty_h

#include "vtkNew.h" // For iVars
#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class vtkUniforms;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALMANUAL vtkShaderProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkShaderProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with no shader replacements
   */
  static vtkShaderProperty* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(vtkShaderProperty* p);

  /**
   * @brief GetShaderMTime returns the last time a modification
   * was made that affected the code of the shader (either code
   * replacement was changed or one or more uniform variables were
   * added or removed. This timestamp can be used by mappers to
   * determine if the shader must be recompiled. Simply changing
   * the value of an existing uniform variable doesn't affect this
   * timestamp as it doesn't change the shader code.
   * @return timestamp of the last modification
   */
  vtkMTimeType GetShaderMTime();

  ///@{
  /**
   * Allow the program to set the shader codes used directly
   * instead of using the built in templates. Be aware, if
   * set, this template will be used for all cases,
   * primitive types, picking etc.
   */
  bool HasVertexShaderCode();
  bool HasFragmentShaderCode();
  bool HasGeometryShaderCode();
  bool HasTessControlShaderCode();
  bool HasTessEvalShaderCode();
  vtkSetStringMacro(VertexShaderCode);
  vtkGetStringMacro(VertexShaderCode);
  vtkSetStringMacro(FragmentShaderCode);
  vtkGetStringMacro(FragmentShaderCode);
  vtkSetStringMacro(GeometryShaderCode);
  vtkGetStringMacro(GeometryShaderCode);
  vtkSetStringMacro(TessControlShaderCode);
  vtkGetStringMacro(TessControlShaderCode);
  vtkSetStringMacro(TessEvaluationShaderCode);
  vtkGetStringMacro(TessEvaluationShaderCode);
  ///@}

  ///@{
  /**
   * The Uniforms object allows to set custom uniform variables
   * that are used in replacement shader code.
   */
  vtkGetObjectMacro(FragmentCustomUniforms, vtkUniforms);
  vtkGetObjectMacro(VertexCustomUniforms, vtkUniforms);
  vtkGetObjectMacro(GeometryCustomUniforms, vtkUniforms);
  vtkGetObjectMacro(TessControlCustomUniforms, vtkUniforms);
  vtkGetObjectMacro(TessEvaluationCustomUniforms, vtkUniforms);
  ///@}

  ///@{
  /**
   * This function enables you to apply your own substitutions
   * to the shader creation process. The shader code in this class
   * is created by applying a bunch of string replacements to a
   * shader template. Using this function you can apply your
   * own string replacements to add features you desire.
   */
  virtual void AddVertexShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) = 0;
  virtual void AddFragmentShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) = 0;
  virtual void AddGeometryShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) = 0;
  virtual void AddTessControlShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) = 0;
  virtual void AddTessEvaluationShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) = 0;
  virtual int GetNumberOfShaderReplacements() = 0;
  virtual std::string GetNthShaderReplacementTypeAsString(vtkIdType index) = 0;
  virtual void GetNthShaderReplacement(vtkIdType index, std::string& name, bool& replaceFirst,
    std::string& replacementValue, bool& replaceAll) = 0;
  virtual void ClearVertexShaderReplacement(
    const std::string& originalValue, bool replaceFirst) = 0;
  virtual void ClearFragmentShaderReplacement(
    const std::string& originalValue, bool replaceFirst) = 0;
  virtual void ClearGeometryShaderReplacement(
    const std::string& originalValue, bool replaceFirst) = 0;
  virtual void ClearTessControlShaderReplacement(
    const std::string& originalValue, bool replaceFirst) = 0;
  virtual void ClearTessEvaluationShaderReplacement(
    const std::string& originalValue, bool replaceFirst) = 0;
  virtual void ClearAllVertexShaderReplacements() = 0;
  virtual void ClearAllFragmentShaderReplacements() = 0;
  virtual void ClearAllGeometryShaderReplacements() = 0;
  virtual void ClearAllTessControlShaderReplacements() = 0;
  virtual void ClearAllTessEvalShaderReplacements() = 0;
  virtual void ClearAllShaderReplacements() = 0;
  ///@}

protected:
  vtkShaderProperty();
  ~vtkShaderProperty() override;

  char* VertexShaderCode;
  char* FragmentShaderCode;
  char* GeometryShaderCode;
  char* TessControlShaderCode;
  char* TessEvaluationShaderCode;

  vtkNew<vtkUniforms> FragmentCustomUniforms;
  vtkNew<vtkUniforms> VertexCustomUniforms;
  vtkNew<vtkUniforms> GeometryCustomUniforms;
  vtkNew<vtkUniforms> TessControlCustomUniforms;
  vtkNew<vtkUniforms> TessEvaluationCustomUniforms;

private:
  vtkShaderProperty(const vtkShaderProperty&) = delete;
  void operator=(const vtkShaderProperty&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
