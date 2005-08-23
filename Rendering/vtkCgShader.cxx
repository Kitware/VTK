// -*- c++ -*- *******************************************************

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCgShader.cxx
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
#include "vtkCgShader.h"

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

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtkstd/map>

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

  CGGLenum GetCGGLenum( vtkstd::string name )
    {
    return this->StateMap[ name ];
    }
  CGGLenum GetCGGLenum( const char* name )
    {
    vtkstd::string Name = name;
    return this->GetCGGLenum( Name );
    }

  protected:
  private:
    vtkstd::map< vtkstd::string, CGGLenum > StateMap;
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
vtkCxxRevisionMacro(vtkCgShader, "1.1.2.2");
vtkStandardNewMacro(vtkCgShader);

//-----------------------------------------------------------------------------
vtkCgShader::vtkCgShader()
{
  this->StateMatrixMap = new CgStateMatrixMap;
  this->LastError = CG_NO_ERROR;
}

//-----------------------------------------------------------------------------
vtkCgShader::~vtkCgShader()
{
  delete this->StateMatrixMap;
}

//-----------------------------------------------------------------------------
int vtkCgShader::Compile()
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

  // If we already have a compiled program, grab the
  // correct context and profilæ and return control.
  if( cgIsProgram(this->Program) == GL_TRUE )
    {
    if( cgGLIsProgramLoaded(this->Program) == GL_TRUE )
      {
      this->Profile = cgGetProgramProfile( this->Program );
      this->Context = cgGetProgramContext( this->Program );
      return 1;
      }
    }

  // Get a valid profile
  if( cgGLIsProfileSupported(this->Profile) == CG_FALSE )
    {
    switch(this->XMLShader->GetScope())
      {
    case vtkXMLShader::SCOPE_VERTEX:
      this->Profile = cgGLGetLatestProfile(CG_GL_VERTEX);
      break;

    case vtkXMLShader::SCOPE_FRAGMENT:
      this->Profile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
      break;

    default:
      vtkErrorMacro("Unsupported scope!");
      return 0;
      }
    }

  // Get a valid context
  if( cgIsContext(this->Context) == CG_FALSE )
    {
    this->Context = cgCreateContext();
    }

  ::CurrentShader = this;
  cgSetErrorCallback(ErrorCallback);
  this->LastError = CG_NO_ERROR;

  const char* source_string = this->XMLShader->GetCode();

  // Although Cg can create a shader form the file directly,
  // vtkXMLShader does not provide an interface to obtain the
  // filename (to keep the interface simple and clear).
  // So we always provide the contents of the file .

  if( cgIsContext(this->Context) == CG_TRUE
    && cgGLIsProfileSupported( this->Profile ) == CG_TRUE 
    && source_string)
    {
    this->Program = cgCreateProgram( this->Context,
      CG_SOURCE,
      source_string,
      this->Profile,
      this->XMLShader->GetEntry(),
      this->XMLShader->GetArgs());
    }

  if( cgIsProgram( this->Program ) == CG_TRUE )
    {
    cgGLLoadProgram(this->Program);
    }
  else
    {
    vtkErrorMacro("Failed to create Cg program.");
    return 0;
    }
  if (this->LastError != CG_NO_ERROR)
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
  if(cgIsProgram( this->Program ) == CG_TRUE)
    {
    cgGLEnableProfile(this->Profile);
    cgGLBindProgram(this->Program);
    }
}


//-----------------------------------------------------------------------------
void vtkCgShader::Unbind()
{
  if(cgIsProgram( this->Program ) == CG_TRUE)
    {
    cgGLUnbindProgram(this->Profile);
    cgGLDisableProfile(this->Profile);
    }
}

//-----------------------------------------------------------------------------
void vtkCgShader::ReportError()
{
  CGerror error = cgGetError();
  this->LastError = error;
  vtkErrorMacro( << cgGetErrorString(error) );
  if( error == CG_COMPILER_ERROR )
    {
    vtkErrorMacro( << cgGetLastListing(this->Context) );
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
  CGparameter param = this->GetUniformParameter(name);
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
  CGparameter param = this->GetUniformParameter(name);
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
  CGparameter param = this->GetUniformParameter(name);
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
  CGparameter param = this->GetUniformParameter(name);
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
void vtkCgShader::SetMatrixParameter(const char* name, const char* state_matix_type,
  const char* transform_type)
{
  if (transform_type==0)
    {
    transform_type = "CG_GL_MATRIX_IDENTITY";
    }
  CGparameter param = this->GetUniformParameter(name);
  if (!param)
    {
    return;
    }
  cgGLSetStateMatrixParameter(param,
    this->StateMatrixMap->GetCGGLenum(state_matix_type),
    this->StateMatrixMap->GetCGGLenum(transform_type));
}

//-----------------------------------------------------------------------------
void vtkCgShader::SetSamplerParameter(const char* name, vtkTexture* texture)
{
  CGparameter param = this->GetUniformParameter(name);
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
CGparameter vtkCgShader::GetUniformParameter( const char* name )
{
  if(!name)
    {
    vtkErrorMacro( "NULL uniform shader parameter name.");
    return NULL;
    }

  if( cgIsProgram(this->Program) != GL_TRUE )
    {
    vtkErrorMacro( "NULL shader program.");
    return NULL;
    }

  CGparameter p = cgGetNamedParameter( this->Program, name );
  if( (cgIsParameter(p)!=CG_TRUE) || (p==NULL) )
    {
    vtkErrorMacro( "No parameter named: " << name << endl );
    return NULL;
    }
  return p;
}


//-----------------------------------------------------------------------------
void vtkCgShader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
