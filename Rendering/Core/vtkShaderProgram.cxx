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
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkShader.h"
#include "vtkToolkits.h" // for VTK_USE_*_SHADERS
#include "vtkShaderDeviceAdapter.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLMaterial.h"
#include "vtkXMLShader.h"

#ifdef VTK_USE_CG_SHADERS 
#include "vtkCgShaderProgram.h"
#endif

#ifdef VTK_USE_GLSL_SHADERS 
#include "vtkGLSLShaderProgram.h"
#endif

vtkCxxSetObjectMacro(vtkShaderProgram, Material, vtkXMLMaterial);
//-----------------------------------------------------------------------------
vtkShaderProgram::vtkShaderProgram()
{
  this->Material= 0;
  this->ShaderCollection = vtkCollection::New();
  this->ShaderCollectionIterator = this->ShaderCollection->NewIterator();
  
  this->GLExtensionsLoaded = 0;
  this->ShaderDeviceAdapter = NULL;
}

//-----------------------------------------------------------------------------
vtkShaderProgram::~vtkShaderProgram()
{
  this->SetShaderDeviceAdapter(0);
  this->SetMaterial(0);
  this->ShaderCollection->Delete();
  this->ShaderCollectionIterator->Delete();
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::ReleaseGraphicsResources(vtkWindow *w)
{
  for (this->ShaderCollectionIterator->InitTraversal();
    !this->ShaderCollectionIterator->IsDoneWithTraversal();
    this->ShaderCollectionIterator->GoToNextItem())
    {
    vtkShader* shader = vtkShader::SafeDownCast(
      this->ShaderCollectionIterator->GetCurrentObject());
    shader->ReleaseGraphicsResources(w);
    }
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::SetShaderDeviceAdapter(vtkShaderDeviceAdapter* adapter)
{
  if (this->ShaderDeviceAdapter)
    {
    this->ShaderDeviceAdapter->SetShaderProgram(0);
    }
  vtkSetObjectBodyMacro(ShaderDeviceAdapter, vtkShaderDeviceAdapter, adapter);
  if (this->ShaderDeviceAdapter)
    {
    this->ShaderDeviceAdapter->SetShaderProgram(this);
    }
}

//-----------------------------------------------------------------------------
vtkCollectionIterator* vtkShaderProgram::NewShaderIterator()
{
  return this->ShaderCollection->NewIterator();
}

//-----------------------------------------------------------------------------
// conditional build - only include shaders that exist, based on the
// user-selected build options for shader types.
vtkShaderProgram* vtkShaderProgram::CreateShaderProgram(int shaderType)
{
  if( shaderType == vtkXMLShader::LANGUAGE_CG )
    {
#ifdef VTK_USE_CG_SHADERS
    return vtkCgShaderProgram::New();
#else
    vtkGenericWarningMacro("Cg shaders not supported.");
#endif
    }

  if( shaderType == vtkXMLShader::LANGUAGE_GLSL )
    {
#ifdef VTK_USE_GLSL_SHADERS 
    return vtkGLSLShaderProgram::New();
#else
    vtkGenericWarningMacro("GLSL shaders not supported.");
#endif
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::ReadMaterial()
{
  if (this->ShaderCollection->GetNumberOfItems() != 0)
    {
    vtkErrorMacro("ReadMaterial() can only be called on a clean ShaderProgram."
      "This shader program already has some shaders loaded.");
    return;
    }

  if (!this->Material)
    {
    vtkErrorMacro("No Material set to read.");
    return;
    }
 
  int cc;
  int max = this->Material->GetNumberOfVertexShaders();
  for (cc=0; cc < max; cc++)
    {
    vtkShader* shader = this->NewShader();
    shader->SetXMLShader(this->Material->GetVertexShader(cc));
    this->AddShader(shader);
    shader->Delete();
    }
  vtkDebugMacro(<< max << " Vertex shaders added.");
 
  max = this->Material->GetNumberOfFragmentShaders();
  for (cc=0; cc < max; cc++)
    {
    vtkShader* shader = this->NewShader();
    shader->SetXMLShader(this->Material->GetFragmentShader(cc));
    this->AddShader(shader);
    shader->Delete();
    }
  vtkDebugMacro(<< max << " Fragment shaders added.");
}

//-----------------------------------------------------------------------------
int vtkShaderProgram::AddShader(vtkShader* shader)
{
  int index = this->GetNumberOfShaders();
  this->ShaderCollection->AddItem(shader);
  return index;
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::RemoveShader(vtkShader* shader)
{
  this->ShaderCollection->RemoveItem(shader);
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::RemoveShader(int index)
{
  this->ShaderCollection->RemoveItem(index);
}

//-----------------------------------------------------------------------------
int vtkShaderProgram::GetNumberOfShaders()
{
  return this->ShaderCollection->GetNumberOfItems();
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::AddShaderVariable(const char* name, int numVars, int* x)
{
  vtkCollectionIterator* iter = this->ShaderCollectionIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkShader* shader = vtkShader::SafeDownCast(iter->GetCurrentObject());
    shader->AddShaderVariable(name, numVars, x);
    }
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::AddShaderVariable(const char* name, int numVars, float* x)
{
  vtkCollectionIterator* iter = this->ShaderCollectionIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkShader* shader = vtkShader::SafeDownCast(iter->GetCurrentObject());
    shader->AddShaderVariable(name, numVars, x);
    }
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::AddShaderVariable(const char* name, int numVars, double* x)
{
  vtkCollectionIterator* iter = this->ShaderCollectionIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkShader* shader = vtkShader::SafeDownCast(iter->GetCurrentObject());
    shader->AddShaderVariable(name, numVars, x);
    }
}

//-----------------------------------------------------------------------------
void vtkShaderProgram::PostRender(vtkActor*, vtkRenderer*)
{
  vtkCollectionIterator* iter = this->ShaderCollectionIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkShader* shader = vtkShader::SafeDownCast(iter->GetCurrentObject());
    shader->Unbind(); 
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
  
  vtkCollectionIterator* iter = this->ShaderCollectionIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkShader* shader = vtkShader::SafeDownCast(iter->GetCurrentObject());
    os << indent << "Shader: " << endl;
    shader->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "ShaderDeviceAdapter: " << this->ShaderDeviceAdapter << endl;
}
