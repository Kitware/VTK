/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShader.cxx
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

#include "vtkGLSLShader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLTexture.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLShader.h"

#include <vtkgl.h>
//#include <GL/glu.h>

#include <sys/types.h>
#include <vtkstd/string>
#include <vtkstd/vector>


#if 1
#define GLSLprintOpenGLError() GLSLprintOglError(__FILE__, __LINE__)
static int GLSLprintOglError(char *vtkNotUsed(file), int vtkNotUsed(line))
{
  //Returns 1 if an OpenGL error occurred, 0 otherwise.
  GLenum glErr;
  int    retCode = 0;

  glErr = glGetError();
  while (glErr != GL_NO_ERROR)
    {
    //printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
    cout << "Error!" << endl;
    retCode = 1;
    glErr = glGetError();
    }
  return retCode;
}
#endif

static void printLogInfo( GLuint shader, const char* filename)
{
#if 1
  GLint type = 0;
  vtkgl::GetShaderiv( shader, vtkgl::OBJECT_TYPE_ARB, &type);
  if( type == vtkgl::SHADER_OBJECT_ARB )
    {
    cout << "GLSL Shader." << endl;
    }
  else
    {
    cout << "Not a GLSL Program!!!." << endl;
    }

  vtkgl::GetShaderiv( shader, vtkgl::OBJECT_SUBTYPE_ARB, &type);
  if( type == vtkgl::VERTEX_SHADER_ARB )
    {
    cout << "GLSL Vertex Shader." << endl;
    }
  else if( type == vtkgl::FRAGMENT_SHADER_ARB )
    {
    cout << "GLSL Fragment Shader." << endl;
    }
  else
    {
    cout << "Not a GLSL Shader!!!." << endl;
    }

  GLint compiled = 0;
  vtkgl::GetShaderiv( shader, vtkgl::OBJECT_COMPILE_STATUS_ARB, &compiled );
  GLsizei maxLength = 0;
  vtkgl::GetShaderiv( shader, vtkgl::OBJECT_INFO_LOG_LENGTH_ARB, &maxLength );

  vtkgl::GLchar* info = new vtkgl::GLchar[maxLength];
  GLsizei charsWritten = 0;

  vtkgl::GetShaderInfoLog( shader, maxLength, &charsWritten, info );

  cout << "Compiled Status: " << compiled << endl;
  if( info )
    {
    cout << "Log message: " << filename << endl << (char *)info << endl;
    }


  GLSLprintOpenGLError();
#endif
}


#if 0
static void printAttributeInfo(GLuint program, const char* vtkNotUsed(filename))
{
  // print all uniform attributes
  GLint numAttrs;
  vtkgl::GetProgramiv( program, vtkgl::ACTIVE_UNIFORMS, &numAttrs);
  if( numAttrs == GL_INVALID_VALUE )
    {
    cout << "GL_INVALID_VALUE for number of attributes." << endl;
    }
  else if( numAttrs == GL_INVALID_OPERATION )
    {
    cout << "GL_INVALID_OPERATION for number of attributes." << endl;
    }
  else if( numAttrs == GL_INVALID_ENUM )
    {
    cout << "GL_INVALID_ENUM for number of attributes." << endl;
    }
  else if( numAttrs == GL_INVALID_OPERATION )
    {
    cout << "GL_INVALID_OPERATION for number of attributes." << endl;
    }
  else
    {
    cout << numAttrs << " Uniform parameters:" << endl;
    }

  GLint maxLength;
  vtkgl::GetProgramiv( program, vtkgl::ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
  GLint id;
  for( id=0; id<numAttrs; id++ )
    {
    vtkgl::GLchar *name = new vtkgl::GLchar[maxLength];
    GLint length;
    GLint size;
    GLenum type;
    vtkgl::GetActiveUniform( program, id, maxLength, &length, &size, &type, name);
    if( name )
      {
      cout << "\t" << (char *)name << endl;
      }
    delete[] name;
    }
  cout << endl;
}
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGLSLShader);
vtkCxxRevisionMacro(vtkGLSLShader, "1.2");

//-----------------------------------------------------------------------------
vtkGLSLShader::vtkGLSLShader()
{
  this->Shader = 0;
}

//-----------------------------------------------------------------------------
vtkGLSLShader::~vtkGLSLShader()
{
  this->ReleaseGraphicsResources(0);
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::ReleaseGraphicsResources(vtkWindow*)
{
  if (this->IsShader())
    {
    vtkgl::DeleteShader(this->Shader);
    this->Shader = 0;
    }
}

//-----------------------------------------------------------------------------
int vtkGLSLShader::IsCompiled()
{
  GLint value = 0;
  if( this->IsShader() )
    {
    vtkgl::GetShaderiv( static_cast<GLuint>(this->Shader),
                   vtkgl::COMPILE_STATUS,
                   &value );
    }
  if( value == 1 )
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
int vtkGLSLShader::IsShader()
{
  if( this->Shader )
    {
    if( vtkgl::IsShader( static_cast<GLuint>(this->Shader) ) == GL_TRUE )
      {
      return 1;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::LoadShader()
{
  // if we have a shader, don't create a new one
  if( !this->IsShader() )
    {
    // create an empty shader object
    switch (this->XMLShader->GetScope())
      {
    case vtkXMLShader::SCOPE_VERTEX:
      this->Shader = vtkgl::CreateShader( vtkgl::VERTEX_SHADER_ARB );
      break;
      
    case vtkXMLShader::SCOPE_FRAGMENT:
      this->Shader = vtkgl::CreateShader( vtkgl::FRAGMENT_SHADER_ARB );
      break;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkGLSLShader::Compile()
{
  if (!this->XMLShader)
    {
    return 0;
    }

  if (!this->XMLShader->GetCode())
    {
    vtkErrorMacro("Shader doesn't have any code!");
    return 0;
      
    }

  if (this->IsCompiled())
    {
    return 1;
    }

  // create a shader context if needed.
  this->LoadShader();

  if( !this->IsShader() )
    {
    vtkErrorMacro( "Shader not loaded!!!" << endl );
    if( this->Shader && this->XMLShader->GetName() )
      {
      printLogInfo(static_cast<GLuint>(this->Shader), this->XMLShader->GetName());
      }
    return 0;
    }

  // if we have the source available, try to load it
  // Load the shader as a single string seems to work best
  const vtkgl::GLchar* source = 
    static_cast<const vtkgl::GLchar*>(this->XMLShader->GetCode());
  
  // Since the entire shader is sent to GL as a single string, the number of
  // lines (second argument) is '1'.
  vtkgl::ShaderSource( static_cast<GLuint>(this->Shader), 1, &source, NULL );

  // make sure the source has been loaded
  // print an error log if the shader is not compiled
  vtkgl::CompileShader(static_cast<GLuint>(this->Shader));

  if( !this->IsCompiled() )
    {
    vtkErrorMacro( "Shader not compiled!!!" << endl );
    if( this->Shader && this->XMLShader->GetName() )
      {
      printLogInfo( static_cast<GLuint>(this->Shader), this->XMLShader->GetName() );
      }
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::SetUniformParameter(const char* name, int numValues, 
  const int* values)
{
  GLint loc = static_cast<GLint>(this->GetUniformLocation(name));
  if (loc == -1)
    {
    return;
    }
  switch(numValues)
    {
  case 1:
    vtkgl::Uniform1iv(loc, 1, reinterpret_cast<const GLint*>(values));
    break;
  case 2:
    vtkgl::Uniform2iv(loc, 1, reinterpret_cast<const GLint*>(values));
    break;
  case 3:
    vtkgl::Uniform3iv(loc, 1, reinterpret_cast<const GLint*>(values));
    break;
  case 4:
    vtkgl::Uniform4iv(loc, 1, reinterpret_cast<const GLint*>(values));
    break;
  default:
    vtkErrorMacro("Number of values not supported: " << numValues);
    }
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::SetUniformParameter(const char* name, int numValues, 
  const float* values)
{
  GLint loc = static_cast<GLint>(this->GetUniformLocation(name));
  if (loc == -1)
    {
    return;
    }
  switch(numValues)
    {
  case 1:
    vtkgl::Uniform1fv(loc, 1, values);
    break;
  case 2:
    vtkgl::Uniform2fv(loc, 1, values);
    break;
  case 3:
    vtkgl::Uniform3fv(loc, 1, values);
    break;
  case 4:
    vtkgl::Uniform4fv(loc, 1, values);
    break;
  default:
    vtkErrorMacro("Number of values not supported: " << numValues);
    }
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::SetUniformParameter(const char* name, int numValues, 
  const double* values)
{
  float* fvalues = new float [numValues];

  for (int i=0; i<numValues; i++)
    {
    fvalues[i] = static_cast<float>(values[i]);
    }
  this->SetUniformParameter(name, numValues, fvalues);
  delete []fvalues;
}

//-----------------------------------------------------------------------------
void vtkGLSLShader:: SetMatrixParameter(const char* name, int numValues, 
  int order, const float* value)
{
  int transpose = (order == vtkShader::RowMajor)? 1: 0;

  GLint loc = static_cast<GLint>(this->GetUniformLocation(name));
  if (loc == -1)
    {
    return;
    }
  switch (numValues)
    {
  case 2*2:
    vtkgl::UniformMatrix2fv(loc, 1, transpose, value);
    break;
  case 3*3:
    vtkgl::UniformMatrix3fv(loc, 1, transpose, value);
    break;
  case 4*4:
    vtkgl::UniformMatrix4fv(loc, 1, transpose, value);
    break;
  default:
    vtkErrorMacro("Number of values not supported: " << numValues);
    }
}

//-----------------------------------------------------------------------------
void vtkGLSLShader:: SetMatrixParameter(const char* name, int numValues, 
  int order, const double* value)
{
  float *v = new float[numValues];
  for (int i=0; i < numValues; i++)
    {
    v[i] = static_cast<float>(value[i]);
    }
  this->SetMatrixParameter(name, numValues, order, v);
  delete []v;
}


//-----------------------------------------------------------------------------
void vtkGLSLShader::SetMatrixParameter(const char*, const char*, const char*)
{
  vtkErrorMacro("GLSL does not support any system matrices!");
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::SetSamplerParameter(const char* name, vtkTexture* texture)
{
  vtkOpenGLTexture* glTexture = vtkOpenGLTexture::SafeDownCast(texture);
  if (glTexture)
    {
    int id = static_cast<int>(glTexture->GetIndex());
    this->SetUniformParameter(name, 1, &id);
    }
}

//-----------------------------------------------------------------------------
int vtkGLSLShader::GetUniformLocation( const char* name )
{
  if( !name )
    {
    vtkErrorMacro( "NULL uniform shader parameter name.");
    return -1;
    }

  if( vtkgl::IsProgram(this->GetProgram())!=GL_TRUE)
    {
    vtkErrorMacro( "NULL shader program.");
    return -1;
    }

  int location = vtkgl::GetUniformLocation( this->GetProgram(), name );
  if( location == -1 )
    {
    vtkErrorMacro( "No such shader parameter. " << name );
    }

  return location;
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Program: " << this->Program << endl;
}
