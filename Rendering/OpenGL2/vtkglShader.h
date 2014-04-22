/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkglShader_h
#define __vtkglShader_h

#include "vtkRenderingOpenGL2Module.h"

#include <string> // For member variables.

namespace vtkgl {

/**
 * @brief Vertex or Fragment shader, combined into a ShaderProgram.
 *
 * This class creates a Vertex or Fragment shader, that can be attached to a
 * ShaderProgram in order to render geometry etc.
 */

class VTKRENDERINGOPENGL2_EXPORT Shader
{
public:
  /** Available shader types. */
  enum Type {
    Vertex,    /**< Vertex shader */
    Fragment,  /**< Fragment shader */
    Unknown    /**< Unknown (default) */
  };

  explicit Shader(Type type = Unknown, const std::string &source = "");
  ~Shader();

  /** Set the shader type. */
  void setType(Type type);

  /** Get the shader type, typically Vertex or Fragment. */
  Type type() const { return m_type; }

  /** Set the shader source to the supplied string. */
  void setSource(const std::string &source);

  /** Get the source for the shader. */
  std::string source() const { return m_source; }

  /** Get the error message (empty if none) for the shader. */
  std::string error() const { return m_error; }

  /** Get the handle of the shader. */
  int handle() const { return m_handle; }

  /** Compile the shader.
   * @note A valid context must to current in order to compile the shader.
   */
  bool compile();

  /** Delete the shader.
   * @note This should only be done once the ShaderProgram is done with the
   * Shader.
   */
  void cleanup();

protected:
  Type m_type;
  int  m_handle;
  bool m_dirty;

  std::string m_source;
  std::string m_error;
};

}

#endif
