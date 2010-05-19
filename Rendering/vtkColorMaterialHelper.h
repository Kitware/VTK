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

#include "vtkObject.h"

class vtkShaderProgram2;

class VTK_RENDERING_EXPORT vtkColorMaterialHelper : public vtkObject
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
  // Prepares the shader i.e. reads color material paramters state from OpenGL. 
  // This must be called before the shader is bound. 
  void PrepareForRendering();

  // Description:
  // Uploads any uniforms needed. This must be called only
  // after the shader has been bound, but before rendering the geometry.
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
