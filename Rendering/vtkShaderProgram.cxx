/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderProgram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkShaderProgram.h"

#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkShader.h"
#include "vtkToolkits.h" // for VTK_USE_*_SHADERS
#include "vtkXMLDataElement.h"
#include "vtkXMLMaterial.h"
#include "vtkXMLShader.h"

#ifdef VTK_USE_CG_SHADERS 
#include "vtkCgShaderProgram.h"
#endif

#ifdef VTK_USE_GLSL_SHADERS 
#include "vtkGLSLShaderProgram.h"
#endif

vtkCxxRevisionMacro(vtkShaderProgram, "1.1.2.3");

vtkCxxSetObjectMacro(vtkShaderProgram, Material, vtkXMLMaterial);
vtkCxxSetObjectMacro(vtkShaderProgram, VertexShader, vtkShader);
vtkCxxSetObjectMacro(vtkShaderProgram, FragmentShader, vtkShader);

//-----------------------------------------------------------------------------
vtkShaderProgram::vtkShaderProgram()
{
  this->Material= 0;
  this->VertexShader = 0;;
  this->FragmentShader = 0;

  this->GLExtensionsLoaded = 0;
}

//-----------------------------------------------------------------------------
vtkShaderProgram::~vtkShaderProgram()
{
  this->SetMaterial(0);
  this->SetVertexShader(0);
  this->SetFragmentShader(0);
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::ReleaseGraphicsResources(vtkWindow *w)
{
  if (this->VertexShader)
    {
    this->VertexShader->ReleaseGraphicsResources(w);
    }
  if (this->FragmentShader)
    {
    this->FragmentShader->ReleaseGraphicsResources(w);
    }
}

//-----------------------------------------------------------------------------
// conditional build - only include shaders that exist, based on the
// user-selected build options for shader types.
vtkShaderProgram* vtkShaderProgram::CreateShaderProgram(int shaderType)
{
#ifdef VTK_USE_CG_SHADERS
  if( shaderType == vtkXMLShader::LANGUAGE_CG )
    {
    return vtkCgShaderProgram::New();
    }
#endif

#ifdef VTK_USE_GLSL_SHADERS 
  if( shaderType == vtkXMLShader::LANGUAGE_GLSL )
    {
    return vtkGLSLShaderProgram::New();
    }
#endif
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::ReadMaterial()
{
  if( this->GetVertexShader())
    {
    this->VertexShader->SetXMLShader(this->GetMaterial()->GetVertexShader());
    }
  if( this->GetFragmentShader())
    {
    this->FragmentShader->SetXMLShader(
      this->GetMaterial()->GetFragmentShader());
    }
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::LoadExtensions( vtkRenderWindow* renWin )
{
  if( this->GetGLExtensionsLoaded() == 1 )
    {
    return;
    }
  // Load extensions using vtkOpenGLExtensionManager
  vtkOpenGLExtensionManager *extensions = vtkOpenGLExtensionManager::New();
  // How can I get access to the vtkRenderWindow from here?
  extensions->SetRenderWindow( renWin );
  if( extensions->ExtensionSupported("GL_VERSION_2_0" ) )
    {
    extensions->LoadExtension("GL_VERSION_2_0");
    this->SetGLExtensionsLoaded(1);
    }
  else
    {
    vtkErrorMacro( "Required extension (GL_VERSION_2_0) is not supported." )
    this->SetGLExtensionsLoaded(0);
    }
  extensions->Delete();
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::AddShaderVariable(const char* name, int numVars, int* x)
{
  this->VertexShader->AddShaderVariable( name, numVars, x );
  this->FragmentShader->AddShaderVariable( name, numVars, x );
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::AddShaderVariable(const char* name, int numVars, float* x)
{
  this->VertexShader->AddShaderVariable( name, numVars, x );
  this->FragmentShader->AddShaderVariable( name, numVars, x );
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::AddShaderVariable(const char* name, int numVars, double* x)
{
  this->VertexShader->AddShaderVariable( name, numVars, x );
  this->FragmentShader->AddShaderVariable( name, numVars, x );
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::PostRender(vtkActor*, vtkRenderer*)
{
  if (this->VertexShader)
    {
    this->VertexShader->Unbind();
    }
  if (this->FragmentShader)
    {
    this->FragmentShader->Unbind();
    }
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Material: ";
  if (this->Material)
    {
    os << endl;
    this->Material->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  
  os << indent << "VertexShader: ";
  if (this->VertexShader)
    {
    os << endl;
    this->VertexShader->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  
  os << indent << "FragmentShader: ";
  if (this->FragmentShader)
    {
    os << endl;
    this->FragmentShader->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
