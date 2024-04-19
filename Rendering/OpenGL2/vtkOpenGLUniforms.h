// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLUniforms
 * @brief   helper class to set custom uniform variables in GLSL shaders.
 *
 * This class implements all SetUniform* functions supported by vtkShaderProgram but instead of
 * directly calling the underlying OpenGL functions, it caches the name and value of the variable
 * and provides a mechanism for client mappers to set all cached variables at once in a generic way.
 *
 * The basic types of GLSL uniform variables supported by the class are the following: int, float,
 * vec2i, vec3, vec4, mat3, mat4, int[], float[], vec2i[], vec3[], vec4[], mat4[]. All other
 * types supported by Set* functions undergo the same type conversions implemented in
 * vtkShaderProgram.
 *
 * @par Thanks:
 * Developed by Simon Drouin (sdrouin2@bwh.harvard.edu) at Brigham and Women's Hospital.
 */

#ifndef vtkOpenGLUniforms_h
#define vtkOpenGLUniforms_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkUniforms.h"
#include <string> // For member functions

VTK_ABI_NAMESPACE_BEGIN
class vtkUniformInternals;
class vtkShaderProgram;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLUniforms : public vtkUniforms
{
public:
  static vtkOpenGLUniforms* New();
  vtkTypeMacro(vtkOpenGLUniforms, vtkUniforms);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /** Build a string containing the GLSL declaration of
      all stored uniform variables. This is typically called
      by mappers to compose the complete shader program. */
  std::string GetDeclarations();

  /** Set all the stored uniform variables values
   *  in the shader program @p p. This is called
   *  when the shader is bound. */
  bool SetUniforms(vtkShaderProgram* p);

  /** Get modified time of the list of uniform variables,
   *  which is typically used to check whether the shader
   *  needs to be recompiled. The time stamp is not affected
   *  by the modification of the value of an existing variable
   *  as then no shader recompilation is needed. */
  vtkMTimeType GetUniformListMTime() override;

  /** Remove uniform variable named @p name */
  void RemoveUniform(const char* name) override;

  /** Remove all uniform variables */
  void RemoveAllUniforms() override;

  ///@{
  /** Generic setters and getter. Set and Get the value of
   *  uniform variable @p name, with TupleType @p tt, number
   *  of components @p nbComponents and values stored in
   *  @p value. These functions simplify io of uniforms */
  void SetUniform(const char* name, vtkUniforms::TupleType tt, int nbComponents,
    const std::vector<int>& value) override;
  void SetUniform(const char* name, vtkUniforms::TupleType tt, int nbComponents,
    const std::vector<float>& value) override;
  bool GetUniform(const char* name, std::vector<int>& value) override;
  bool GetUniform(const char* name, std::vector<float>& value) override;
  ///@}

  ///@{
  /** Set the @p name uniform value to @p v. */
  void SetUniformi(const char* name, int v) override;
  void SetUniformf(const char* name, float v) override;
  void SetUniform2i(const char* name, const int v[2]) override;
  void SetUniform2f(const char* name, const float v[2]) override;
  void SetUniform3f(const char* name, const float v[3]) override;
  void SetUniform4f(const char* name, const float v[4]) override;
  void SetUniformMatrix3x3(const char* name, float* v) override;
  void SetUniformMatrix4x4(const char* name, float* v) override;
  ///@}

  ///@{
  /** Set the @p name uniform array to @p f with @p count elements */
  void SetUniform1iv(const char* name, int count, const int* f) override;
  void SetUniform1fv(const char* name, int count, const float* f) override;
  void SetUniform2fv(const char* name, int count, const float (*f)[2]) override;
  void SetUniform3fv(const char* name, int count, const float (*f)[3]) override;
  void SetUniform4fv(const char* name, int count, const float (*f)[4]) override;
  void SetUniformMatrix4x4v(const char* name, int count, float* v) override;
  ///@}

  ///@{
  /** Set the @p name uniform to @p v.
   *  The following are convenience functions and do not reflect
   *  the way the data is stored and sent to OpenGL. Data is
   *  converted to match one of the basic supported types */
  void SetUniform3f(const char* name, const double v[3]) override;
  void SetUniform3uc(const char* name, const unsigned char v[3]) override; // maybe remove
  void SetUniform4uc(const char* name, const unsigned char v[4]) override; // maybe remove
  void SetUniformMatrix(const char* name, vtkMatrix3x3* v) override;
  void SetUniformMatrix(const char* name, vtkMatrix4x4* v) override;
  ///@}

  ///@{
  /** Get the @p name uniform value. Returns true on success. */
  bool GetUniformi(const char* name, int& v) override;
  bool GetUniformf(const char* name, float& v) override;
  bool GetUniform2i(const char* name, int v[2]) override;
  bool GetUniform2f(const char* name, float v[2]) override;
  bool GetUniform3f(const char* name, float v[3]) override;
  bool GetUniform4f(const char* name, float v[4]) override;
  bool GetUniformMatrix3x3(const char* name, float* v) override;
  bool GetUniformMatrix4x4(const char* name, float* v) override;
  ///@}

  ///@{
  /** Get the @p name uniform to @p v.
   *  The following are convenience functions and do not reflect
   *  the way the data is stored and sent to OpenGL. Data is
   *  converted from one of the basic supported types */
  bool GetUniform3f(const char* name, double v[3]) override;
  bool GetUniform3uc(const char* name, unsigned char v[3]) override;
  bool GetUniform4uc(const char* name, unsigned char v[4]) override;
  bool GetUniformMatrix(const char* name, vtkMatrix3x3* v) override;
  bool GetUniformMatrix(const char* name, vtkMatrix4x4* v) override;
  ///@}

  ///@{
  /** Get the @p name uniform vector to @p f with. */
  bool GetUniform1iv(const char* name, std::vector<int>& f) override;
  bool GetUniform1fv(const char* name, std::vector<float>& f) override;
  bool GetUniform2fv(const char* name, std::vector<float>& f) override;
  bool GetUniform3fv(const char* name, std::vector<float>& f) override;
  bool GetUniform4fv(const char* name, std::vector<float>& f) override;
  bool GetUniformMatrix4x4v(const char* name, std::vector<float>& f) override;
  ///@}

  /** Get number of all uniforms stored in this class */
  int GetNumberOfUniforms() override;

  /** Get number of all uniforms stored in this class.
    Valid range is between 0 and GetNumberOfUniforms() - 1.*/
  const char* GetNthUniformName(vtkIdType uniformIndex) override;

  /** Get type of scalars stored in uniform @p name */
  int GetUniformScalarType(const char* name) override;

  /** Get the tuple type stored in uniform @p name. This can be a scalar,
   * a vector of a matrix. */
  TupleType GetUniformTupleType(const char* name) override;

  /** Get the number of components stored in each tuple of uniform @p name.
   * for example, a uniform with tuples of matrix type and 9 components
   * contains 3x3 matrices */
  int GetUniformNumberOfComponents(const char* name) override;

  /** Get length of a uniform @p name that contains a variable-size vector.
    Size includes number of tuples. For example, 3fv returns 3 x number of triplets. */
  int GetUniformNumberOfTuples(const char* name) override;

protected:
  vtkOpenGLUniforms();
  ~vtkOpenGLUniforms() override;

private:
  vtkOpenGLUniforms(const vtkOpenGLUniforms&) = delete;
  void operator=(const vtkOpenGLUniforms&) = delete;

  vtkUniformInternals* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
