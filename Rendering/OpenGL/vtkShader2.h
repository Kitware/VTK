/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShader2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkShader2
 * @brief   GLSL Shader
 *
 * A vtkShader2 object is made of a source code encoded into a string and
 * a type, identifying the hardware programmable stage it is attached to.
 * Hardware programmable stages are (or will be)
 * 1. vertex shader
 * 2. tessellation control shader
 * 3. tessellation evaluation shader
 * 4. geometry shader
 * 5. fragment shader
 *
 * Note: tessellation control shader is called hull shader in DirectX11
 * Note: tessellation evaluation shader is called domain shader in DirectX11
 * Ref: OpenGL spec 4.0
 * http://www.opengl.org/registry/doc/glspec40.core.20100311.withchanges.pdf
 * Ref: "Introducing DirectX 11"
 * http://www.gamasutra.com/view/feature/3759/sponsored_feature_introducing_.php
 *
 * @sa
 * vtkShaderProgram2
*/

#ifndef vtkShader2_h
#define vtkShader2_h

#include "vtkWeakPointer.h" // for ren context
#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"

// Values for GetType()/SetType()
enum vtkShader2Type
{
  VTK_SHADER_TYPE_VERTEX = 0,
  VTK_SHADER_TYPE_TESSELLATION_CONTROL = 3, // new, not supported yet
  VTK_SHADER_TYPE_TESSELLATION_EVALUATION = 4, // new, not supported yet
  VTK_SHADER_TYPE_GEOMETRY = 1,
  VTK_SHADER_TYPE_FRAGMENT = 2
};

class vtkRenderWindow;
class vtkUniformVariables;

class VTKRENDERINGOPENGL_EXPORT vtkShader2 : public vtkObject
{
public:
  static vtkShader2 *New();
  vtkTypeMacro(vtkShader2,vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns if the context supports the required extensions.
   * Extensions are load when the context is set.
   */
  static bool IsSupported(vtkRenderWindow *context);

  //@{
  /**
   * String containing the shader source code. Reminder SetString makes a copy
   * of its argument.
   */
  vtkGetStringMacro(SourceCode);
  vtkSetStringMacro(SourceCode);
  //@}

  //@{
  /**
   * Return the shader type, .
   * \post valid_result: result==VTK_SHADER_TYPE_VERTEX ||
   * result==VTK_SHADER_TYPE_TESSELLATION_CONTROL ||
   * result==VTK_SHADER_TYPE_TESSELLATION_EVALUATION ||
   * result==VTK_SHADER_TYPE_GEOMETRY ||
   * result==VTK_SHADER_TYPE_FRAGMENT.
   */
  vtkGetMacro(Type,int);
  //@}

  //@{
  /**
   * Set the shader type, .
   * \pre valid_type: type==VTK_SHADER_TYPE_VERTEX ||
   * type==VTK_SHADER_TYPE_TESSELLATION_CONTROL ||
   * type==VTK_SHADER_TYPE_TESSELLATION_EVALUATION ||
   * type==VTK_SHADER_TYPE_GEOMETRY ||
   * type==VTK_SHADER_TYPE_FRAGMENT.
   * \post is_set: GetType()==type.
   */
  vtkSetMacro(Type,int);
  //@}

  /**
   * Return the shader type as a string.
   */
  const char *GetTypeAsString();

  /**
   * Compile the shader code.
   * The result of compilation can be query with GetLastCompileStatus()
   * The log of compilation can be query with GetLastCompileLog()
   * \pre SourceCode_exists: this->GetSourceCode()!=0
   */
  void Compile();

  /**
   * Tells if the last call to compile succeeded (true) or not (false).
   * Initial value is false.
   */
  bool GetLastCompileStatus();

  /**
   * Return the log of the last call to compile as a string.
   * Initial value is the empty string ""='\0'.
   * \post result_exists: result!=0
   */
  const char *GetLastCompileLog();

  //@{
  /**
   * Get/Set the context. This does not increase the reference count of the
   * context to avoid reference loops.
   * SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions.
   */
  void SetContext(vtkRenderWindow *context);
  vtkRenderWindow *GetContext();
  //@}

  /**
   * Release OpenGL resource (shader id).
   */
  virtual void ReleaseGraphicsResources();

  //@{
  /**
   * Return the OpenGL shader object id.
   */
  vtkGetMacro(Id,unsigned int);
  //@}

  //@{
  /**
   * Get/Set the list of uniform variables values.
   * Initial value is an empty list (not null pointer).
   */
  vtkGetObjectMacro(UniformVariables,vtkUniformVariables);
  virtual void SetUniformVariables(vtkUniformVariables *variables);
  //@}

protected:
  /**
   * Default constructor. SourceCode is NULL. Type is vertex.
   */
  vtkShader2();

  /**
   * Destructor. Delete SourceCode and LastCompileLog if any.
   */
  ~vtkShader2() VTK_OVERRIDE;

  /**
   * Load the required OpenGL extensions.
   */
  bool LoadRequiredExtensions(vtkRenderWindow *context);

  char *SourceCode;
  int Type;

  unsigned int Id; // actually GLuint. Initial value is 0.

  bool LastCompileStatus; // Initial value is false.
  char *LastCompileLog; // Initial value is the empty string ""='\0'.
  size_t LastCompileLogCapacity; // Initial value is 8.

  vtkTimeStamp LastCompileTime;
  vtkUniformVariables *UniformVariables; // Initial values is an empty list

  vtkWeakPointer<vtkRenderWindow> Context;

  bool ExtensionsLoaded;
  bool SupportGeometryShader;

private:
  vtkShader2(const vtkShader2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkShader2&) VTK_DELETE_FUNCTION;
};

#endif
