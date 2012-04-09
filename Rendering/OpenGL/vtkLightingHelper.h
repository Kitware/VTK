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
// .NAME vtkLightingHelper - helper to assist in simulating lighting similar
//  to default OpenGL pipeline.
//
// .SECTION Description
//  vtkLightingHelper is an helper to assist in simulating lighting similar
//  to default OpenGL pipeline. Look at vtkLightingHelper_s for available
//  GLSL functions.
//
// .SECTION see also
//  vtkShaderProgram2

#ifndef __vtkLightingHelper_h
#define __vtkLightingHelper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"
#include "vtkShader2.h" // for vtkShader2Type

class vtkShaderProgram2;

class VTKRENDERINGOPENGL_EXPORT vtkLightingHelper : public vtkObject
{
public:
  static vtkLightingHelper* New();
  vtkTypeMacro(vtkLightingHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the shader program to which we want to add the lighting kernels.
  // mode = VTK_SHADER_TYPE_VERTEX or VTK_SHADER_TYPE_FRAGMENT
  // depending on whether the vertex lighting or fragment lighting is to be
  // used.
  void Initialize(vtkShaderProgram2 *shader,
                  vtkShader2Type mode);
  vtkGetObjectMacro(Shader, vtkShaderProgram2);

  // Description:
  // Updates any lighting specific information needed.
  // This must be called before the shader program is bound.
  void PrepareForRendering();

//BTX
protected:
  vtkLightingHelper();
  ~vtkLightingHelper();

  void SetShader(vtkShaderProgram2 *shader);
  vtkShaderProgram2 *Shader;

private:
  vtkLightingHelper(const vtkLightingHelper&); // Not implemented.
  void operator=(const vtkLightingHelper&); // Not implemented.
//ETX
};

#endif
