/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCgShaderProgram.cxx

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

#include "vtkCgShaderProgram.h"

#include "vtkActor.h"
#include "vtkCgShaderDeviceAdapter.h"
#include "vtkCgShader.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkXMLMaterialReader.h"


vtkStandardNewMacro(vtkCgShaderProgram);

//----------------------------------------------------------------------------
vtkCgShaderProgram::vtkCgShaderProgram()
{
  vtkCgShaderDeviceAdapter* da = vtkCgShaderDeviceAdapter::New();
  this->SetShaderDeviceAdapter(da);
  da->Delete();
}

//----------------------------------------------------------------------------
vtkCgShaderProgram::~vtkCgShaderProgram()
{
  this->SetShaderDeviceAdapter(0);
}

//----------------------------------------------------------------------------
vtkShader* vtkCgShaderProgram::NewShader()
{
  return vtkCgShader::New();
}

//----------------------------------------------------------------------------
void vtkCgShaderProgram::Render(vtkActor *actor, vtkRenderer *renderer )
{
  // Cg requires no communication between vertex and fragment shaders;
  // each can be installed in hardware independently. There's really
  // nothing for vtkCgShaderProgram to do but delegate all shader mechanics
  // tasks to it's vertex and fragment shaders.
  vtkCollectionIterator* iter = this->ShaderCollectionIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkShader* shader = vtkShader::SafeDownCast(iter->GetCurrentObject());
    if (shader->Compile())
      {
      shader->PassShaderVariables(actor, renderer);
      shader->Bind();
      }
    }
}

//----------------------------------------------------------------------------
void vtkCgShaderProgram::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
