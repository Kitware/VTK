/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCgShader.cxx

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

#include "vtkCgShader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLTexture.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLShader.h"

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <string>
#include <vector>
#include <map>

#define CG_UNIFORM_DOUBLE_AS_FLOAT 1

//-----------------------------------------------------------------------------
class CgStateMatrixMap
{
  public:

  CgStateMatrixMap()
    {
    // Define CGGLenums for mapping strings from xml file
    this->StateMap["CG_GL_MATRIX_IDENTITY"] = CG_GL_MATRIX_IDENTITY;
    this->StateMap["CG_GL_MATRIX_TRANSPOSE"] = CG_GL_MATRIX_TRANSPOSE;
    this->StateMap["CG_GL_MATRIX_INVERSE"] = CG_GL_MATRIX_INVERSE;
    this->StateMap["CG_GL_MATRIX_INVERSE_TRANSPOSE"] = CG_GL_MATRIX_INVERSE_TRANSPOSE;
    this->StateMap["CG_GL_MODELVIEW_MATRIX"] = CG_GL_MODELVIEW_MATRIX;
    this->StateMap["CG_GL_PROJECTION_MATRIX"] = CG_GL_PROJECTION_MATRIX;
    this->StateMap["CG_GL_TEXTURE_MATRIX"] = CG_GL_TEXTURE_MATRIX;
    this->StateMap["CG_GL_MODELVIEW_PROJECTION_MATRIX"] = CG_GL_MODELVIEW_PROJECTION_MATRIX;
    this->StateMap["CG_GL_VERTEX"] = CG_GL_VERTEX;
    this->StateMap["CG_GL_FRAGMENT"] = CG_GL_FRAGMENT;
    }
  ~CgStateMatrixMap()
    {
    // STL container free their memory before destruction
    // Their destructors are call automatically when this class is destroyed
    }

  bool HaveCGGLenum( std::string name )
    {
    if( this->StateMap.find(name) == this->StateMap.end() )
      {
      return 0;
      }
    return 1;
    }

  bool HaveCGGLenum( const char* name )
    {
    if(!name)
      {
      return 0;
      }
    std::string Name = name;
    return this->HaveCGGLenum(Name);
    }

  CGGLenum GetCGGLenum( std::string name )
    {
    return this->StateMap[ name ];
    }
  CGGLenum GetCGGLenum( const char* name )
    {
    std::string Name = name;
    return this->GetCGGLenum( Name );
    }

  protected:
  private:
    std::map< std::string, CGGLenum > StateMap;
};

//-----------------------------------------------------------------------------
static vtkCgShader *CurrentShader;
extern "C" {
  static void ErrorCallback(void)
    {
    CurrentShader->ReportError();
    }
}

//-----------------------------------------------------------------------------
class vtkCgShaderInternals
{
public:
  CGprofile Profile;
  CGcontext Context;
  CGprogram Program;
  CGerror LastError;
  CgStateMatrixMap StateMatrixMap;
  
  vtkCgShaderInternals()
    {
    this->LastError = CG_NO_ERROR;
    this->Context = 0;
    }
  
  CGparameter GetUniformParameter(const char* name)
    {
    if(!name)
      {
      vtkGenericWarningMacro( "NULL uniform shader parameter name.");
      return NULL;
      }

    if( cgIsProgram(this->Program) != GL_TRUE )
      {
      vtkGenericWarningMacro( "NULL shader program.");
      return NULL;
      }

    CGparameter p = cgGetNamedParameter( this->Program, name );
    if( (cgIsParameter(p)!=CG_TRUE) || (p==NULL) )
      {
      vtkGenericWarningMacro( "No parameter named: " << name << endl );
      return NULL;
      }
    return p;
    }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCgShader);

//-----------------------------------------------------------------------------
vtkCgShader::vtkCgShader()
{
  this->Internals = new vtkCgShaderInternals;
}

//-----------------------------------------------------------------------------
vtkCgShader::~vtkCgShader()
{
  this->ReleaseGraphicsResources(NULL);
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkCgShader::ReleaseGraphicsResources(vtkWindow* window)
{
  if (window &&
    window->GetMapped() &&
    cgIsContext(this->Internals->Context))
    {
    // This will also destroy any programs contained in the context.
    cgDestroyContext(this->Internals->Context);
    }
  this->Internals->Context = 0;
}

//-----------------------------------------------------------------------------
int vtkCgShader::Compile()
{
  if (!this->XMLShader || this->Internals->LastError != CG_NO_ERROR)
    {
    return 0;
    }

  if (!this->XMLShader->GetCode())
    {
    vtkErrorMacro("Shader doesn't have any code!");
    this->Internals->LastError = CG_INVALID_PROGRAM_HANDLE_ERROR;
    return 0;
    }

  // If we already have a compiled program, grab the
  // correct context and profilæ and return control.
  if( cgIsProgram(this->Internals->Program) == GL_TRUE )
    {
    if( cgGLIsProgramLoaded(this->Internals->Program) == GL_TRUE )
      {
      this->Internals->Profile = cgGetProgramProfile( this->Internals->Program );
      this->Internals->Context = cgGetProgramContext( this->Internals->Program );
      return 1;
      }
    }

  // Get a valid profile
  if( cgGLIsProfileSupported(this->Internals->Profile) == CG_FALSE )
    {
    switch(this->XMLShader->GetScope())
      {
    case vtkXMLShader::SCOPE_VERTEX:
      this->Internals->Profile = cgGLGetLatestProfile(CG_GL_VERTEX);
      break;

    case vtkXMLShader::SCOPE_FRAGMENT:
      this->Internals->Profile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
      break;

    default:
      vtkErrorMacro("Unsupported scope!");
      this->Internals->LastError = CG_UNKNOWN_PROFILE_ERROR;
      return 0;
      }
    }

  // Get a valid context
  if( cgIsContext(this->Internals->Context) == CG_FALSE )
    {
    this->Internals->Context = cgCreateContext();
    }

  ::CurrentShader = this;
  cgSetErrorCallback(ErrorCallback);
  this->Internals->LastError = CG_NO_ERROR;

  const char* source_string = this->XMLShader->GetCode();

  // Although Cg can create a shader form the file directly,
  // vtkXMLShader does not provide an interface to obtain the
  // filename (to keep the interface simple and clear).
  // So we always provide the contents of the file .

  if( cgIsContext(this->Internals->Context) == CG_TRUE
    && cgGLIsProfileSupported( this->Internals->Profile ) == CG_TRUE 
    && source_string)
    {
    this->Internals->Program = cgCreateProgram( this->Internals->Context,
      CG_SOURCE,
      source_string,
      this->Internals->Profile,
      this->XMLShader->GetEntry(),
      this->XMLShader->GetArgs());
    }

  if( cgIsProgram( this->Internals->Program ) == CG_TRUE )
    {
    cgGLLoadProgram(this->Internals->Program);
    }
  else
    {
    vtkErrorMacro("Failed to create Cg program.");
    return 0;
    }
  if (this->Internals->LastError != CG_NO_ERROR)
    {
    vtkErrorMacro("Error occured during Shader compile.");
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkCgShader::Bind()
{
  // Bind shader to hardware
  if(cgIsProgram( this->Internals->Program ) == CG_TRUE)
    {
    cgGLEnableProfile(this->Internals->Profile);
    cgGLBindProgram(this->Internals->Program);
    }
}


//-----------------------------------------------------------------------------
void vtkCgShader::Unbind()
{
  if(cgIsProgram( this->Internals->Program ) == CG_TRUE)
    {
    cgGLUnbindProgram(this->Internals->Profile);
    cgGLDisableProfile(this->Internals->Profile);
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::ReportError()
{
  CGerror error = cgGetError();
  this->Internals->LastError = error;
  vtkErrorMacro( << cgGetErrorString(error) );
  if( error == CG_COMPILER_ERROR )
    {
    vtkErrorMacro( << cgGetLastListing(this->Internals->Context) );
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::SetUniformParameter(const char* name, int numValues,
  const int* values)
{
  double* dvalues = new double[numValues];
  for (int i=0; i < numValues; i++)
    {
    dvalues[i] = static_cast<double>(values[i]);
    }
  
  this->SetUniformParameter(name, numValues, dvalues);
  delete[] dvalues;
}

//-----------------------------------------------------------------------------
void vtkCgShader::SetUniformParameter(const char* name, int numValues, const float* value)
{
  CGparameter param = this->Internals->GetUniformParameter(name);
  if (!param)
    {
    return;
    }
  switch (numValues)
    {
  case 1:
    cgGLSetParameter1fv(param, value);
    break;
  case 2:
    cgGLSetParameter2fv(param, value);
    break;
  case 3:
    cgGLSetParameter3fv(param, value);
    break;
  case 4:
    cgGLSetParameter4fv(param, value);
    break;
  default:
    vtkErrorMacro("Number of values not supported : " << numValues);
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::SetUniformParameter(const char* name, int numValues, const double* value)
{
  CGparameter param = this->Internals->GetUniformParameter(name);
  if (!param)
    {
    return;
    }
  switch (numValues)
    {
  case 1:
    cgGLSetParameter1dv(param, value);
    break;
  case 2:
    cgGLSetParameter2dv(param, value);
    break;
  case 3:
    cgGLSetParameter3dv(param, value);
    break;
  case 4:
    cgGLSetParameter4dv(param, value);
    break;
  default:
    vtkErrorMacro("Number of values not supported : " << numValues);
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::SetMatrixParameter(const char* name, int , int order, 
  const float* value)
{
  CGparameter param = this->Internals->GetUniformParameter(name);
  if (!param)
    {
    return;
    }
  if (order == vtkShader::RowMajor)
    {
    cgGLSetMatrixParameterfr(param, value);
    }
  else
    {
    cgGLSetMatrixParameterfc(param, value);
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::SetMatrixParameter(const char* name, int , int order, 
  const double* value)
{
  CGparameter param = this->Internals->GetUniformParameter(name);
  if (!param)
    {
    return;
    }
  if (order == vtkShader::RowMajor)
    {
    cgGLSetMatrixParameterdr(param, value);
    }
  else
    {
    cgGLSetMatrixParameterdc(param, value);
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::SetMatrixParameter(const char* name, const char* state_matrix_type,
  const char* transform_type)
{
  if (!state_matrix_type )
    {
    vtkErrorMacro( "state_matrix Type not specified!" );
    return;
    }
  if (!transform_type )
    {
    transform_type = "CG_GL_MATRIX_IDENTITY";
    }
  CGparameter param = this->Internals->GetUniformParameter(name);
  if (!param)
    {
    return;
    }

  if( this->Internals->StateMatrixMap.HaveCGGLenum(state_matrix_type) &&
      this->Internals->StateMatrixMap.HaveCGGLenum(transform_type) )
    {
    cgGLSetStateMatrixParameter(param,
      this->Internals->StateMatrixMap.GetCGGLenum(state_matrix_type),
      this->Internals->StateMatrixMap.GetCGGLenum(transform_type));
    }
  else
    {
    vtkErrorMacro( "Can't find state matrix valuse or xforms for : " <<
                   name << ", " << state_matrix_type << ", " << transform_type << endl );
    exit(0);
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::SetSamplerParameter(const char* name, vtkTexture* texture,int)
{
  CGparameter param = this->Internals->GetUniformParameter(name);
  if (!param)
    {
    return;
    }
  vtkOpenGLTexture* glTexture = vtkOpenGLTexture::SafeDownCast(texture);
  if (glTexture)
    {
    cgGLSetTextureParameter(param, glTexture->GetIndex());
    cgGLEnableTextureParameter(param);
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::PassShaderVariables(vtkActor* actor, vtkRenderer* renderer)
{
  bool push_transform = (actor->GetIsIdentity() == 0);
  if (push_transform)
    {
    double *mat = actor->GetMatrix()->Element[0];
    double mat2[16];
    mat2[0] = mat[0];
    mat2[1] = mat[4];
    mat2[2] = mat[8];
    mat2[3] = mat[12];
    mat2[4] = mat[1];
    mat2[5] = mat[5];
    mat2[6] = mat[9];
    mat2[7] = mat[13];
    mat2[8] = mat[2];
    mat2[9] = mat[6];
    mat2[10] = mat[10];
    mat2[11] = mat[14];
    mat2[12] = mat[3];
    mat2[13] = mat[7];
    mat2[14] = mat[11];
    mat2[15] = mat[15];

    // insert model transformation 
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glMultMatrixd(mat2);
    }
  this->Superclass::PassShaderVariables(actor, renderer);
  if (push_transform)
    {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
