// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniforms
 * @brief   helper class to set custom uniform variables in GPU shaders.
 *
 * This class implements a generic mechanism to declare and set the value of custom uniform
 * variables to be used in GPU shader programs used by mappers. It allows users who specify
 * custom shader code for mappers to change the value of the variable they define without
 * triggering a costly rebuild of the shader. This class is used mostly as an interface and
 * the implementation is found in graphics api specific derived classes (e.g.: vtkOpenGLUniforms).
 *
 * @sa
 * vtkOpenGLUniforms vtkShaderProperty
 *
 * @par Thanks:
 * Developed by Simon Drouin (sdrouin2@bwh.harvard.edu) at Brigham and Women's Hospital.
 */

#ifndef vtkUniforms_h
#define vtkUniforms_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include <string>                   // member function parameters
#include <vector>                   // member function parameters

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix3x3;
class vtkMatrix4x4;

class VTKRENDERINGCORE_EXPORT vtkUniforms : public vtkObject
{
public:
  static vtkUniforms* New();
  vtkTypeMacro(vtkUniforms, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual vtkMTimeType GetUniformListMTime() = 0;

  ///@{
  /**
   * Types of tuples that can be stored : scalar, vector, matrix
   */
  ///@}
  enum TupleType
  {
    TupleTypeInvalid = 0,
    TupleTypeScalar,
    TupleTypeVector,
    TupleTypeMatrix,
    NumberOfTupleTypes
  };

  /** Convert between TupleType and string */
  static std::string TupleTypeToString(TupleType tt);
  static TupleType StringToTupleType(const std::string& s);

  /** Convert between scalar types an string */
  static std::string ScalarTypeToString(int scalarType);
  static int StringToScalarType(const std::string& s);

  /** Remove uniform variable named @p name */
  virtual void RemoveUniform(const char* name) = 0;

  /** Remove all uniform variables */
  virtual void RemoveAllUniforms() = 0;

  ///@{
  /** Generic setters and getter. Set and Get the value of
   *  uniform variable @p name, with TupleType @p tt, number
   *  of components @p nbComponents and values stored in
   *  @p value. These functions simplify io of uniforms */
  virtual void SetUniform(const char* name, vtkUniforms::TupleType tt, int nbComponents,
    const std::vector<int>& value) = 0;
  virtual void SetUniform(const char* name, vtkUniforms::TupleType tt, int nbComponents,
    const std::vector<float>& value) = 0;
  virtual bool GetUniform(const char* name, std::vector<int>& value) = 0;
  virtual bool GetUniform(const char* name, std::vector<float>& value) = 0;
  ///@}

  ///@{
  /** Set the @p name uniform value to @p v. */
  virtual void SetUniformi(const char* name, int v) = 0;
  virtual void SetUniformf(const char* name, float v) = 0;
  virtual void SetUniform2i(const char* name, const int v[2]) = 0;
  virtual void SetUniform2f(const char* name, const float v[2]) = 0;
  virtual void SetUniform3f(const char* name, const float v[3]) = 0;
  virtual void SetUniform4f(const char* name, const float v[4]) = 0;
  virtual void SetUniformMatrix3x3(const char* name, float* v) = 0;
  virtual void SetUniformMatrix4x4(const char* name, float* v) = 0;
  ///@}

  ///@{
  /** Set the @p name uniform array to @p f with @p count elements */
  virtual void SetUniform1iv(const char* name, int count, const int* f) = 0;
  virtual void SetUniform1fv(const char* name, int count, const float* f) = 0;
  virtual void SetUniform2fv(const char* name, int count, const float (*f)[2]) = 0;
  virtual void SetUniform3fv(const char* name, int count, const float (*f)[3]) = 0;
  virtual void SetUniform4fv(const char* name, int count, const float (*f)[4]) = 0;
  virtual void SetUniformMatrix4x4v(const char* name, int count, float* v) = 0;
  ///@}

  ///@{
  /** Set the @p name uniform to @p v.
   *  The following are convenience functions and do not reflect
   *  the way the data is stored and sent to OpenGL. Data is
   *  converted to match one of the basic supported types */
  virtual void SetUniform3f(const char* name, const double v[3]) = 0;
  virtual void SetUniform3uc(const char* name, const unsigned char v[3]) = 0; // maybe remove
  virtual void SetUniform4uc(const char* name, const unsigned char v[4]) = 0; // maybe remove
  virtual void SetUniformMatrix(const char* name, vtkMatrix3x3* v) = 0;
  virtual void SetUniformMatrix(const char* name, vtkMatrix4x4* v) = 0;
  ///@}

  ///@{
  /** Get the @p name uniform value. Returns true on success. */
  virtual bool GetUniformi(const char* name, int& v) = 0;
  virtual bool GetUniformf(const char* name, float& v) = 0;
  virtual bool GetUniform2i(const char* name, int v[2]) = 0;
  virtual bool GetUniform2f(const char* name, float v[2]) = 0;
  virtual bool GetUniform3f(const char* name, float v[3]) = 0;
  virtual bool GetUniform4f(const char* name, float v[4]) = 0;
  virtual bool GetUniformMatrix3x3(const char* name, float* v) = 0;
  virtual bool GetUniformMatrix4x4(const char* name, float* v) = 0;
  ///@}

  ///@{
  /** Get the @p name uniform to @p v.
   *  The following are convenience functions and do not reflect
   *  the way the data is stored and sent to OpenGL. Data is
   *  converted from one of the basic supported types */
  virtual bool GetUniform3f(const char* name, double v[3]) = 0;
  virtual bool GetUniform3uc(const char* name, unsigned char v[3]) = 0;
  virtual bool GetUniform4uc(const char* name, unsigned char v[4]) = 0;
  virtual bool GetUniformMatrix(const char* name, vtkMatrix3x3* v) = 0;
  virtual bool GetUniformMatrix(const char* name, vtkMatrix4x4* v) = 0;
  ///@}

  ///@{
  /** Get the @p name uniform vector to @p f with. */
  virtual bool GetUniform1iv(const char* name, std::vector<int>& f) = 0;
  virtual bool GetUniform1fv(const char* name, std::vector<float>& f) = 0;
  virtual bool GetUniform2fv(const char* name, std::vector<float>& f) = 0;
  virtual bool GetUniform3fv(const char* name, std::vector<float>& f) = 0;
  virtual bool GetUniform4fv(const char* name, std::vector<float>& f) = 0;
  virtual bool GetUniformMatrix4x4v(const char* name, std::vector<float>& f) = 0;
  ///@}

  /** Get number of all uniforms stored in this class */
  virtual int GetNumberOfUniforms() = 0;

  /** Get number of all uniforms stored in this class.
    Valid range is between 0 and GetNumberOfUniforms() - 1.*/
  virtual const char* GetNthUniformName(vtkIdType uniformIndex) = 0;

  /** Get type of scalars stored in uniform @p name */
  virtual int GetUniformScalarType(const char* name) = 0;

  /** Get the tuple type stored in uniform @p name. This can be a scalar,
   *  a vector of a matrix. */
  virtual TupleType GetUniformTupleType(const char* name) = 0;

  /** Get the number of components stored in each tuple of uniform @p name.
   *  for example, a uniform with tuples of matrix type and 9 components
   *  contains 3x3 matrices */
  virtual int GetUniformNumberOfComponents(const char* name) = 0;

  /** Number of tuples of uniform @p name that contains a variable-size vector.
   *  For example, for 3 components uniforms of type vector, this is the number
   *  of triplets. */
  virtual int GetUniformNumberOfTuples(const char* name) = 0;

protected:
  vtkUniforms() = default;
  ~vtkUniforms() override = default;

private:
  vtkUniforms(const vtkUniforms&) = delete;
  void operator=(const vtkUniforms&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
