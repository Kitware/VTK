/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include <map> // used for ivar

class vtkOpenGLUniforms;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLShaderProperty : public vtkShaderProperty
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

  virtual void AddVertexShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;
  virtual void AddFragmentShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;
  virtual void AddGeometryShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;

  virtual int GetNumberOfShaderReplacements() override;
  virtual std::string GetNthShaderReplacementTypeAsString(vtkIdType index) override;
  virtual void GetNthShaderReplacement(vtkIdType index, std::string& name, bool& replaceFirst,
    std::string& replacementValue, bool& replaceAll) override;

  virtual void ClearVertexShaderReplacement(
    const std::string& originalValue, bool replaceFirst) override;
  virtual void ClearFragmentShaderReplacement(
    const std::string& originalValue, bool replaceFirst) override;
  virtual void ClearGeometryShaderReplacement(
    const std::string& originalValue, bool replaceFirst) override;
  virtual void ClearAllVertexShaderReplacements() override;
  virtual void ClearAllFragmentShaderReplacements() override;
  virtual void ClearAllGeometryShaderReplacements() override;
  virtual void ClearAllShaderReplacements() override;

  //@{
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
  //@}

  /**
   * @brief GetAllShaderReplacements returns all user-specified shader
   * replacements. It is provided for iteration purpuses only (const)
   * and is mainly used by mappers when building the shaders.
   * @return const reference to internal map holding all replacements
   */
  typedef std::map<vtkShader::ReplacementSpec, vtkShader::ReplacementValue> ReplacementMap;
  const ReplacementMap& GetAllShaderReplacements() { return this->UserShaderReplacements; }

protected:
  vtkOpenGLShaderProperty();
  ~vtkOpenGLShaderProperty() override;

  ReplacementMap UserShaderReplacements;

private:
  vtkOpenGLShaderProperty(const vtkOpenGLShaderProperty&) = delete;
  void operator=(const vtkOpenGLShaderProperty&) = delete;
};

#endif
