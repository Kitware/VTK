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
// .NAME vtkColorMaterialHelper - a helper to assist in simulating the
//  ColorMaterial behaviour of the default OpenGL pipeline.
//
// .SECTION Description
//  vtkColorMaterialHelper is a helper to assist in simulating the
//  ColorMaterial behaviour of the default OpenGL pipeline. Look at
//  vtkColorMaterialHelper_s for available GLSL functions.
//
// .SECTION see also
//  vtkShaderProgram2

#ifndef __vtkColorMaterialHelper_h
#define __vtkColorMaterialHelper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"

class vtkShaderProgram2;

class VTKRENDERINGOPENGL_EXPORT vtkColorMaterialHelper : public vtkObject
{
public:
  static vtkColorMaterialHelper* New();
  vtkTypeMacro(vtkColorMaterialHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  void Initialize(vtkShaderProgram2*);
  vtkGetObjectMacro(Shader, vtkShaderProgram2);
  //ETX

  // Description:
  // Capture current OpenGL state and initialize uniform variables
  // used by the helper shader.
  void SetUniformVariables();

  // Description:
  // Captures current OpenGL state.
  // DEPRECATED (Use PrepareForRendering2 instead)
  void PrepareForRendering();

  // Description:
  // Initializes uniform variables with the last captured
  // OpenGL state.
  // NOTHING IS RENDERED THIS SETS UNIFORMS.
  // DEPRECATED: Use SetUnformVariables instead.
  void Render();

//BTX
protected:
  vtkColorMaterialHelper();
  ~vtkColorMaterialHelper();

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
  vtkColorMaterialHelper(const vtkColorMaterialHelper&); // Not implemented.
  void operator=(const vtkColorMaterialHelper&); // Not implemented.
//ETX
};

#endif
