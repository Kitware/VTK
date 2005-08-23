/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShader.h
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
// .NAME vtkGLSLShader
// .SECTION Description
// vtkGLSLShader is a concrete class that creates and compiles hardware
// shaders written in the OpenGL Shadering Language (GLSL, OpenGL2.0).
// While step linking a vertex and a fragment shader is performed by
// vtkGLSLShaderProgram, all shader parameters are initialized in this
// class.
//
// .Section vtkOpenGLExtensionManager
// All OpenGL calls are made through vtkOpenGLExtensionManager.
//
// .Section Supported Basic Shader Types:
//
// Scalar Types
// uniform float
// uniform int
// uniform int -- boolean scalar not yet tested
//
// Vector Types:
// uniform vec{2|3|4}
// uniform ivec{2|3|4}
// uniform bvec{2|3|4} -- boolean vector not yet tested
//
// Matrix Types:
// uniform mat{2|3|4}
//
// Texture Samplers:
// sample1D -- Not yet implemented in this cless.
// sample2D -- Not yet implemented in this class.
// sample3D -- Not yet implemented in this class.
// sampler1DShadow -- Not yet implemented in this class.
// sampler1DShadow -- Not yet implemented in this class.
// 
// User-Defined structures:
// uniform struct
//  NOTE: these must be defined and declared  outside of the 'main' shader
//  function.
//
//

#ifndef __vtkGLSLShader_h
#define __vtkGLSLShader_h

#include "vtkShader.h"
#include <vtkgl.h>

class vtkActor;
class vtkRenderer;
class vtkProperty;
class vtkLight;
class vtkCamera;
class vtkRenderWindow;

// manages all shaders defined in the XML file
// especially the part about sending things to the card
class VTK_RENDERING_EXPORT vtkGLSLShader : public vtkShader
{
public:
  static vtkGLSLShader *New();
  vtkTypeRevisionMacro(vtkGLSLShader, vtkShader);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  // Description:
  // Called to compile the shader code.
  // The subclasses must only compile the code in this method.
  // Returns if the compile was successful.
  // Subclasses should compile the code only if it was not
  // already compiled.
  virtual int Compile();

  //BTX
  virtual GLuint GetHandle();
  //ETX

  vtkSetMacro( Program, GLuint );
  vtkGetMacro( Program, GLuint );

  vtkGetMacro( Shader, GLuint );

  int IsShader();
  int IsCompiled();

protected:
  vtkGLSLShader();
  virtual ~vtkGLSLShader();

  // Desctiption:
  // This not sympatico with VTK's style since it's a GL type. One option might be
  // to store these as ints and cast the to GLint when interfacing with OpenGL. Otherwise
  // the parsing guards should suffice.
  //BTX
  GLuint Program;
  GLuint Shader;
  //ETX

  // Description:
  // Create an empty Shader context.
  void LoadShader();

  // Description:
  // Equivalent to cgGLSetParameter and glUniform.
  virtual void SetUniformParameter(const char* name, int numValues, const int* value);
  virtual void SetUniformParameter(const char* name, int numValues, const float* value);
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
  virtual void SetSamplerParameter(const char* name, vtkTexture* texture);
private:
  vtkGLSLShader(const vtkGLSLShader&); // Not Implemented
  void operator=(const vtkGLSLShader&); // Not Implemented

  //BTX
  GLint GetUniformLocation( const char* name );
  //ETX
};
#endif //__vtkGLSLShader_h
