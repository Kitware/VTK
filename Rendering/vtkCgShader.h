//-*- c++ -*- *******************************************************

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCgShader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
// .NAME vtkCgShader
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

#ifndef __vtkCgShader_h
#define __vtkCgShader_h

#include "vtkShader.h"

#include <Cg/cg.h>
#include <Cg/cgGL.h>

class vtkActor;
class vtkRenderer;
//BTX
class CgStateMatrixMap;
//ETX

class vtkProperty;
class vtkLight;
class vtkCamera;

// manages all shaders defined in the XML file
// especially the part about sending things to the card
class VTK_RENDERING_EXPORT vtkCgShader : public vtkShader
{
public:
  static vtkCgShader *New();
  vtkTypeRevisionMacro(vtkCgShader, vtkShader);
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

protected:
  vtkCgShader();
  ~vtkCgShader();

  CGprofile Profile;
  CGcontext Context;
  CGprogram Program;
  CGerror   LastError;
  
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

  // Description:
  // Equivalen to cgGLSetTexture(), GLSL merely does a glUniform1v().
  // Subclass may have to cast the texture to vtkOpenGLTexture to obtain
  // the GLunint for texture this texture.
  virtual void SetTexture(const char* name, vtkTexture* texture) { };
private:
  vtkCgShader(const vtkCgShader&); // Not Implemented
  void operator=(const vtkCgShader&); // Not Implemented

  //BTX
  // Initialize specific types of uniform variables
  CgStateMatrixMap *StateMatrixMap;
  CGparameter GetUniformParameter(const char* name);
  //ETX
};
#endif //_vtkCgShader_h
