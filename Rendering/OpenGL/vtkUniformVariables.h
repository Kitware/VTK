/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformVariables.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUniformVariables
 * @brief   GLSL uniform variables
 *
 * vtkUniformVariables is a list of uniform variables attached to either a
 * vtkShader2 object or to a vtkShaderProgram2. Uniform variables on
 * a vtkShaderProgram2 override values of uniform variables on a vtkShader2.
 *
 * @sa
 * vtkShader2 vtkShaderProgram2
*/

#ifndef vtkUniformVariables_h
#define vtkUniformVariables_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"

class vtkUniformVariablesMap; // internal

class VTKRENDERINGOPENGL_EXPORT vtkUniformVariables : public vtkObject
{
public:
  static vtkUniformVariables *New();
  vtkTypeMacro(vtkUniformVariables,vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Set an integer uniform variable.
   * \pre name_exists: name!=0
   * \pre value_exists: value!=0
   * \pre valid_numberOfComponents: numberOfComponents>=1 && numberOfComponents<=4
   */
  void SetUniformi(const char *name,
                   int numberOfComponents,
                   int *value);

  template<typename T>
  void SetUniformit(const char *name,
                   int numberOfComponents,
                   T *value);

  template<typename T>
  void SetUniformit(const char *name, T value)
  { this->SetUniformit(name, 1, &value); }

  /**
   * Set an float uniform variable.
   * \pre name_exists: name!=0
   * \pre value_exists: value!=0
   * \pre valid_numberOfComponents: numberOfComponents>=1 && numberOfComponents<=4
   */
  void SetUniformf(const char *name,
                   int numberOfComponents,
                   float *value);

  template<typename T>
  void SetUniformft(const char *name,
                   int numberOfComponents,
                   T *value);

  template<typename T>
  void SetUniformft(const char *name, T value)
  { this->SetUniformft(name, 1, &value); }

  /**
   * Set an array of integer uniform variables.
   * The array `value' is of size `numberOfElements'*`numberOfComponents.'.
   * \pre name_exists: name!=0
   * \pre value_exists: value!=0
   * \pre valid_numberOfComponents: numberOfComponents>=1 && numberOfComponents<=4
   * \pre valid_numberOfElements: numberOfElements>=1
   */
  void SetUniformiv(const char *name,
                    int numberOfComponents,
                    int numberOfElements,
                    int *value);

  /**
   * Set an array of float uniform variables.
   * The array `value' is of size `numberOfElements'*`numberOfComponents.'.
   * \pre name_exists: name!=0
   * \pre value_exists: value!=0
   * \pre valid_numberOfComponents: numberOfComponents>=1 && numberOfComponents<=4
   * \pre valid_numberOfElements: numberOfElements>=1
   */
  void SetUniformfv(const char *name,
                    int numberOfComponents,
                    int numberOfElements,
                    float *value);

  /**
   * Set a matrix uniform variable.
   * \pre name_exists: name!=0
   * \pre value_exists: value!=0
   * \pre valid_rows:  rows>=2 && rows<=4
   * \pre valid_columns: columns>=2 && columns<=4
   */
  void SetUniformMatrix(const char *name,
                        int rows,
                        int columns,
                        float *value);

  /**
   * Remove uniform `name' from the list.
   */
  void RemoveUniform(const char *name);

  /**
   * Remove all uniforms from the list.
   */
  void RemoveAllUniforms();

  /**
   * \pre need a valid OpenGL context and a shader program in use.
   */
  void Send(const char *name,
            int uniformIndex);

  /**
   * Place the internal cursor on the first uniform.
   */
  void Start();

  /**
   * Is the iteration done?
   */
  bool IsAtEnd();

  /**
   * Name of the uniform at the current cursor position.
   * \pre not_done: !this->IsAtEnd()
   */
  const char *GetCurrentName();

  /**
   * \pre need a valid OpenGL context and a shader program in use.
   * \pre not_done: !this->IsAtEnd()
   */
  void SendCurrentUniform(int uniformIndex);

  /**
   * Move the cursor to the next uniform.
   * \pre not_done: !this->IsAtEnd()
   */
  void Next();

  /**
   * Copy all the variables from `other'. Any existing variable will be
   * deleted first.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   */
  void DeepCopy(vtkUniformVariables *other);

  /**
   * Copy all the variables from `other'. Any existing variable will be
   * overwritten.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   */
  void Merge(vtkUniformVariables *other);

protected:
  vtkUniformVariables();
  virtual ~vtkUniformVariables();

private:
  vtkUniformVariables(const vtkUniformVariables&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUniformVariables&) VTK_DELETE_FUNCTION;

  vtkUniformVariablesMap *Map;
};

// ----------------------------------------------------------------------------
template<typename T>
void vtkUniformVariables::SetUniformit(const char *name,
                   int numberOfComponents,
                   T *value)
{
  int ivalues[4];
  for (int i=0; i<numberOfComponents; ++i)
  {
    ivalues[i] = static_cast<int>(value[i]);
  }
  this->SetUniformi(name, numberOfComponents, ivalues);
}

// ----------------------------------------------------------------------------
template<typename T>
void vtkUniformVariables::SetUniformft(const char *name,
                   int numberOfComponents,
                   T *value)
{
  float fvalues[4];
  for (int i=0; i<numberOfComponents; ++i)
  {
    fvalues[i] = static_cast<float>(value[i]);
  }
  this->SetUniformf(name, numberOfComponents, fvalues);
}

#endif
