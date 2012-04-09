/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCgShader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkCgShader - Cg Shader
// .SECTION Description
// vtkCgShader is the only class that interfaces directly with the Cg
// libraries. Once is has a valid shader described by a vtkXMLDataElement
// it will create, compile, install, and initialize the parameters of a Cg
// hardware shader.
//
// .Section Supported Shader Types:
//
// Basic Types:
// uniform float
// uniform double
//
// Vector Types:
// uniform float{1|2|3|4}
// uniform double{1|2|3|4}
//
// Matrix Types:
// uniform float{1|2|3|4}x{1|2|3|4}
// uniform double{1|2|3|4}x{1|2|3|4}
//
// NOTE: In the above, 'double' and 'float' refer to the application's
// representation, the hardware shader must define all of the above types
// as 'uniform float'.
//
// State Matrix Parameters:
// uniform float4x4
// with the following Cg-defined settings:
//  CG_GL_MATRIX_IDENTITY
//  CG_GL_MATRIX_TRANSPOSE
//  CG_GL_MATRIX_INVERSE
//  CG_GL_MATRIX_INVERSE_TRANSPOSE
//  CG_GL_MODELVIEW_MATRIX
//  CG_GL_PROJECTION_MATRIX
//  CG_GL_TEXTURE_MATRIX
//  CG_GL_MODELVIEW_PROJECTION_MATRIX
//
// Texture Samplers:
// sample1D - Not tested
// sample2D
// sample3D - Not supported by VTK
// sampleRECT - Not supported by VTK
// sampleCUBE - Not supported by VTK
//
// User-Defined:
// uniform struct
//
// VTK-Specific Hardware Data Structures:
// vtkProperty
// vtkLight
// vtkCamera
// See vtkSNL/Rendering/Shaders/vtkProperty.cg
// See vtkSNL/Rendering/Shaders/vtkLight.cg
// See vtkSNL/Rendering/Shaders/vtkCamera.cg
//
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at
// Sandia National Labs.

#ifndef __vtkCgShader_h
#define __vtkCgShader_h

#include "vtkShader.h"

class vtkActor;
class vtkCamera;
class vtkCgShaderInternals;
class vtkLight;
class vtkProperty;
class vtkRenderer;

// manages all shaders defined in the XML file
// especially the part about sending things to the card
class VTK_RENDERING_EXPORT vtkCgShader : public vtkShader
{
public:
  static vtkCgShader *New();
  vtkTypeMacro(vtkCgShader, vtkShader);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Called to compile the shader code.
  // The vtkShaderProgram calls this method only when
  // vtkShader::IsCompiled() returns false.
  // The subclasses must only compile the code in this method.
  // Returns if the compile was successful.
  virtual int Compile();

  // Description:
  // In this method the shader can enable/bind itself. This is applicable
  // only to Cg, since in GLSL, individual shaders in a program can't be
  // enabled/bound.
  virtual void  Bind();

  // Description:
  // Called to unbind the shader. As with Bind(), this is only applicable
  // to Cg.
  virtual void Unbind();

  // Description:
  // Internal method don't call directly. Called by Cg erro callback
  // to report Cg errors.
  void ReportError();

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);


  // Description:
  // Called to pass VTK actor/property/light values and other
  // Shader variables over to the shader. This is called by the ShaderProgram
  // during each render. We override this method for Cg shaders, since for Cg shaders,
  // we need to ensure that the actor transformations are pushed before
  // state matrix uniform variables are bound.
  virtual void PassShaderVariables(vtkActor* actor, vtkRenderer* ren);
//BTX
protected:
  vtkCgShader();
  ~vtkCgShader();

  // Description:
  // Equivalent to cgGLSetParameter and glUniform.
  virtual void SetUniformParameter(const char* name, int numValues, const int* value) ;
  virtual void SetUniformParameter(const char* name, int numValues, const float* value) ;
  virtual void SetUniformParameter(const char* name, int numValues, const double* value);

  // Description:
  // Equivalent to cgGLSetMatrixParameterfc and glUniformMatrix.
  virtual void SetMatrixParameter(const char* name, int numValues,
    int order, const float* value);
  virtual void SetMatrixParameter(const char* name, int numValues,
    int order, const double* value);
  virtual void SetMatrixParameter(const char* name, const char* state_matix_type,
    const char* transform_type);

  virtual void SetSamplerParameter(const char* name, vtkTexture* texture,
                                   int);

  friend class vtkCgShaderDeviceAdapter;
private:
  vtkCgShader(const vtkCgShader&); // Not Implemented
  void operator=(const vtkCgShader&); // Not Implemented

  vtkCgShaderInternals* Internals;
//ETX
};
#endif //_vtkCgShader_h
