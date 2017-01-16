/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightingHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLightingHelper
 * @brief   helper to assist in simulating lighting similar
 *  to default OpenGL pipeline.
 *
 *
 *  vtkLightingHelper is an helper to assist in simulating lighting similar
 *  to default OpenGL pipeline. Look at vtkLightingHelper_s for available
 *  GLSL functions.
 *
 * @sa
 *  vtkShaderProgram2
*/

#ifndef vtkLightingHelper_h
#define vtkLightingHelper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"
#include "vtkShader2.h" // for vtkShader2Type

class vtkShaderProgram2;

class VTKRENDERINGOPENGL_EXPORT vtkLightingHelper : public vtkObject
{
public:
  static vtkLightingHelper* New();
  vtkTypeMacro(vtkLightingHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  enum {
    VTK_MAX_LIGHTS=8
  };

  //@{
  /**
   * Get/Set the shader program to which we want to add the lighting kernels.
   * mode = VTK_SHADER_TYPE_VERTEX or VTK_SHADER_TYPE_FRAGMENT
   * depending on whether the vertex lighting or fragment lighting is to be
   * used.
   */
  void Initialize(vtkShaderProgram2 *shader, vtkShader2Type mode);
  vtkGetObjectMacro(Shader, vtkShaderProgram2);
  //@}

  /**
   * Encodes light state in diffuse component 3, where the shader looks for it.
   */
  void EncodeLightState(){ this->PrepareForRendering(); }
  void PrepareForRendering();

protected:
  vtkLightingHelper();
  ~vtkLightingHelper() VTK_OVERRIDE;

  void SetShader(vtkShaderProgram2 *shader);
  vtkShaderProgram2 *Shader;

private:
  vtkLightingHelper(const vtkLightingHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLightingHelper&) VTK_DELETE_FUNCTION;

};

#endif
