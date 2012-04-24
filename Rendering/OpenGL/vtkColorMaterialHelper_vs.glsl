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
// Id: Id

#version 110

#define GL_AMBIENT 1
#define GL_DIFFUSE 2
#define GL_SPECULAR 3
#define GL_AMBIENT_AND_DIFFUSE 4
#define GL_EMISSION 5

uniform int vtkColorMaterialHelper_Mode;

gl_MaterialParameters getMaterialParameters()
{
  if (vtkColorMaterialHelper_Mode == 0)
    {
    return gl_FrontMaterial;
    }

  gl_MaterialParameters materialParams = gl_FrontMaterial;
  if (vtkColorMaterialHelper_Mode == GL_AMBIENT)
    {
    materialParams.ambient = gl_Color;
    }
  else if (vtkColorMaterialHelper_Mode == GL_DIFFUSE)
    {
    materialParams.diffuse = gl_Color;
    }
  else if (vtkColorMaterialHelper_Mode == GL_SPECULAR)
    {
    materialParams.specular = gl_Color;
    }
  else if (vtkColorMaterialHelper_Mode == GL_AMBIENT_AND_DIFFUSE)
    {
    materialParams.ambient = gl_Color;
    materialParams.diffuse = gl_Color;
    }
  else if (vtkColorMaterialHelper_Mode == GL_EMISSION)
    {
    materialParams.emission = gl_Color;
    }
  return materialParams;
}
