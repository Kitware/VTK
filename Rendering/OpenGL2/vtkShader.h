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
 * @class   vtkShader
 * @brief   encapsulate a glsl shader
 *
 * vtkShader represents a shader, vertex, fragment, geometry etc
*/

#ifndef vtkShader_h
#define vtkShader_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkObject.h"

#include <string> // For member variables.
#include <vector> // For member variables.

/**
 * @brief Vertex or Fragment shader, combined into a ShaderProgram.
 *
 * This class creates a Vertex, Fragment or Geometry shader, that can be attached to a
 * ShaderProgram in order to render geometry etc.
 */

class VTKRENDERINGOPENGL2_EXPORT vtkShader : public vtkObject
{
public:
  static vtkShader *New();
  vtkTypeMacro(vtkShader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;


  /** Available shader types. */
  enum Type {
    Vertex,    /**< Vertex shader */
    Fragment,  /**< Fragment shader */
    Geometry,  /**< Geometry shader */
    Unknown    /**< Unknown (default) */
  };

  /** Set the shader type. */
  void SetType(Type type);

  /** Get the shader type, typically Vertex or Fragment. */
  Type GetType() const { return this->ShaderType; }

  /** Set the shader source to the supplied string. */
  void SetSource(const std::string &source);

  /** Get the source for the shader. */
  std::string GetSource() const { return this->Source; }

  /** Get the error message (empty if none) for the shader. */
  std::string GetError() const { return this->Error; }

  /** Get the handle of the shader. */
  int GetHandle() const { return this->Handle; }

  /** Compile the shader.
   * @note A valid context must to current in order to compile the shader.
   */
  bool Compile();

  /** Delete the shader.
   * @note This should only be done once the ShaderProgram is done with the
   * Shader.
   */
  void Cleanup();

protected:
  vtkShader();
  ~vtkShader() VTK_OVERRIDE;

  Type ShaderType;
  int  Handle;
  bool Dirty;

  std::string Source;
  std::string Error;

private:
  vtkShader(const vtkShader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkShader&) VTK_DELETE_FUNCTION;
};


#endif
