/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLUniform
 * @brief   helper class to set custom uniform variables in GLSL shaders.
 *
 * This class implements all SetUniform* functions supported by vtkShaderProgram but instead of
 * directly calling the underlying OpenGL functions, it caches the name and value of the variable
 * and provides a mechanism for client mappers to set all cached variables at once in a generic way.
*/

#ifndef vtkOpenGLUniforms_h
#define vtkOpenGLUniforms_h

#include "vtkObject.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

#include <string> // For member functions

class vtkUniformInternals;
class vtkMatrix3x3;
class vtkMatrix4x4;
class vtkShaderProgram;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT  vtkOpenGLUniforms : public vtkObject
{
public:
    static vtkOpenGLUniforms *New();
    vtkTypeMacro(vtkOpenGLUniforms, vtkObject);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    std::string GetDeclarations();
    bool SetUniforms( vtkShaderProgram * p );
    vtkMTimeType GetUniformListMTime();

    /** Add the @p name uniform variable with value @p defaultValue. */
    void AddUniformi (const char *name, int defaultValue);
    void AddUniformf(const char *name, float defaultValue);
    void AddUniform2i(const char *name, const int defaultValue[2]);
    void AddUniform2f(const char *name, const float defaultValue[2]);
    void AddUniform3f(const char *name, const float defaultValue[3]);
    void AddUniform3f(const char *name, const double defaultValue[3]);
    void AddUniform4f(const char *name, const float defaultValue[4]);
    void AddUniform3uc(const char *name, const unsigned char defaultValue[3]); // maybe remove
    void AddUniform4uc(const char *name, const unsigned char defaultValue[4]); // maybe remove
    void AddUniformMatrix(const char *name, vtkMatrix3x3 *defaultValue);
    void AddUniformMatrix(const char *name, vtkMatrix4x4 *defaultValue);
    void AddUniformMatrix3x3(const char *name, float *defaultValue);
    void AddUniformMatrix4x4(const char *name, float *defaultValue);

    /** Add the @p name uniform array to @p f with @p count elements */
    void AddUniform1iv(const char *name, const int count, const int *f);
    void AddUniform1fv (const char *name, const int count, const float *f);
    void AddUniform2fv (const char *name, const int count, const float(*f)[2]);
    void AddUniform3fv (const char *name, const int count, const float(*f)[3]);
    void AddUniform4fv (const char *name, const int count, const float(*f)[4]);
    void AddUniformMatrix4x4v (const char *name, const int count, float *v);

    /** Remove uniform variable named @p name */
    void RemoveUniform(const char *name);

    /** Remove all uniform variables */
    void RemoveAllUniforms();

    /** Set the @p name uniform value to @p v. */
    void SetUniformi(const char *name, int v);
    void SetUniformf(const char *name, float v);
    void SetUniform2i(const char *name, const int v[2]);
    void SetUniform2f(const char *name, const float v[2]);
    void SetUniform3f(const char *name, const float v[3]);
    void SetUniform3f(const char *name, const double v[3]);
    void SetUniform4f(const char *name, const float v[4]);
    void SetUniform3uc(const char *name, const unsigned char v[3]); // maybe remove
    void SetUniform4uc(const char *name, const unsigned char v[4]); // maybe remove
    void SetUniformMatrix(const char *name, vtkMatrix3x3 *v);
    void SetUniformMatrix(const char *name, vtkMatrix4x4 *v);
    void SetUniformMatrix3x3(const char *name, float *v);
    void SetUniformMatrix4x4(const char *name, float *v);

    /** Set the @p name uniform array to @p f with @p count elements */
    void SetUniform1iv(const char *name, const int count, const int *f);
    void SetUniform1fv(const char *name, const int count, const float *f);
    void SetUniform2fv(const char *name, const int count, const float (*f)[2]);
    void SetUniform3fv(const char *name, const int count, const float (*f)[3]);
    void SetUniform4fv(const char *name, const int count, const float (*f)[4]);
    void SetUniformMatrix4x4v(const char *name, const int count, float *v);

protected:
  vtkOpenGLUniforms();
  ~vtkOpenGLUniforms() override;

private:
  vtkOpenGLUniforms(const vtkOpenGLUniforms&) = delete;
  void operator=(const vtkOpenGLUniforms&) = delete;

  vtkUniformInternals * Internals;
};

#endif
