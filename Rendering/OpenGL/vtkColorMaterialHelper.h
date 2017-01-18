/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorMaterialHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkColorMaterialHelper
 * @brief   a helper to assist in simulating the
 *  ColorMaterial behaviour of the default OpenGL pipeline.
 *
 *
 *  vtkColorMaterialHelper is a helper to assist in simulating the
 *  ColorMaterial behaviour of the default OpenGL pipeline. Look at
 *  vtkColorMaterialHelper_s for available GLSL functions.
 *
 * @sa
 *  vtkShaderProgram2
*/

#ifndef vtkColorMaterialHelper_h
#define vtkColorMaterialHelper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"

class vtkShaderProgram2;

class VTKRENDERINGOPENGL_EXPORT vtkColorMaterialHelper : public vtkObject
{
public:
  static vtkColorMaterialHelper* New();
  vtkTypeMacro(vtkColorMaterialHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void Initialize(vtkShaderProgram2*);
  vtkGetObjectMacro(Shader, vtkShaderProgram2);

  /**
   * Capture current OpenGL state and initialize uniform variables
   * used by the helper shader.
   */
  void SetUniformVariables();

  /**
   * Captures current OpenGL state.
   * DEPRECATED (Use PrepareForRendering2 instead)
   */
  void PrepareForRendering();

  /**
   * Initializes uniform variables with the last captured
   * OpenGL state.
   * NOTHING IS RENDERED THIS SETS UNIFORMS.
   * DEPRECATED: Use SetUnformVariables instead.
   */
  void Render();

protected:
  vtkColorMaterialHelper();
  ~vtkColorMaterialHelper() VTK_OVERRIDE;

  void SetShader(vtkShaderProgram2*);
  vtkShaderProgram2 * Shader;

  enum eMaterialParamater
  {
    DISABLED = 0,
    AMBIENT = 1,
    DIFFUSE = 2,
    SPECULAR = 3,
    AMBIENT_AND_DIFFUSE = 4,
    EMISSION = 5
  };
  eMaterialParamater Mode;

private:
  vtkColorMaterialHelper(const vtkColorMaterialHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkColorMaterialHelper&) VTK_DELETE_FUNCTION;

};

#endif
