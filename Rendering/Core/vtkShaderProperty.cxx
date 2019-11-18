/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShaderProperty.h"

#include "vtkObjectFactory.h"
#include "vtkUniforms.h"
#include <algorithm>

vtkAbstractObjectFactoryNewMacro(vtkShaderProperty);

vtkShaderProperty::vtkShaderProperty()
{
  this->VertexShaderCode = nullptr;
  this->FragmentShaderCode = nullptr;
  this->GeometryShaderCode = nullptr;
}

vtkShaderProperty::~vtkShaderProperty()
{
  this->SetVertexShaderCode(nullptr);
  this->SetFragmentShaderCode(nullptr);
  this->SetGeometryShaderCode(nullptr);
}

void vtkShaderProperty::DeepCopy(vtkShaderProperty* p)
{
  this->SetVertexShaderCode(p->GetVertexShaderCode());
  this->SetFragmentShaderCode(p->GetFragmentShaderCode());
  this->SetGeometryShaderCode(p->GetGeometryShaderCode());
}

vtkMTimeType vtkShaderProperty::GetShaderMTime()
{
  vtkMTimeType fragUniformMTime = this->FragmentCustomUniforms->GetUniformListMTime();
  vtkMTimeType vertUniformMTime = this->VertexCustomUniforms->GetUniformListMTime();
  vtkMTimeType geomUniformMTime = this->GeometryCustomUniforms->GetUniformListMTime();
  return std::max({ this->GetMTime(), fragUniformMTime, vertUniformMTime, geomUniformMTime });
}

bool vtkShaderProperty::HasVertexShaderCode()
{
  return this->VertexShaderCode && *this->VertexShaderCode;
}

bool vtkShaderProperty::HasFragmentShaderCode()
{
  return this->FragmentShaderCode && *this->FragmentShaderCode;
}

bool vtkShaderProperty::HasGeometryShaderCode()
{
  return this->GeometryShaderCode && *this->GeometryShaderCode;
}

//-----------------------------------------------------------------------------
void vtkShaderProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
