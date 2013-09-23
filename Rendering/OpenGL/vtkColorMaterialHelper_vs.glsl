//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkColorMaterialHelper_vs.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

#version 110

#define VTK_GL_AMBIENT 1
#define VTK_GL_DIFFUSE 2
#define VTK_GL_SPECULAR 3
#define VTK_GL_AMBIENT_AND_DIFFUSE 4
#define VTK_GL_EMISSION 5

uniform int vtkColorMaterialHelper_Mode;

gl_MaterialParameters getMaterialParameters()
{
  gl_MaterialParameters materialParams = gl_FrontMaterial;
  if (vtkColorMaterialHelper_Mode == VTK_GL_AMBIENT)
    {
    materialParams.ambient = gl_Color;
    }
  else if (vtkColorMaterialHelper_Mode == VTK_GL_DIFFUSE)
    {
    materialParams.diffuse = gl_Color;
    }
  else if (vtkColorMaterialHelper_Mode == VTK_GL_SPECULAR)
    {
    materialParams.specular = gl_Color;
    }
  else if (vtkColorMaterialHelper_Mode == VTK_GL_AMBIENT_AND_DIFFUSE)
    {
    materialParams.ambient = gl_Color;
    materialParams.diffuse = gl_Color;
    }
  else if (vtkColorMaterialHelper_Mode == VTK_GL_EMISSION)
    {
    materialParams.emission = gl_Color;
    }
  return materialParams;
}
