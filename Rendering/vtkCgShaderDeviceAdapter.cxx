/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCgShaderDeviceAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCgShaderDeviceAdapter.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkShaderProgram.h"
#include "vtkCgShader.h"
#include "vtkCollectionIterator.h"
#include "vtkXMLShader.h"

class vtkCgShaderDeviceAdapter::vtkInternal
{
public:
  vtkSmartPointer<vtkCgShader> VertexShader;
};

vtkStandardNewMacro(vtkCgShaderDeviceAdapter);
//----------------------------------------------------------------------------
vtkCgShaderDeviceAdapter::vtkCgShaderDeviceAdapter()
{
  this->Internal = new vtkInternal();
}

//----------------------------------------------------------------------------
vtkCgShaderDeviceAdapter::~vtkCgShaderDeviceAdapter()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkCgShaderDeviceAdapter::PrepareForRender()
{
  // locate the vertex CgShader which can accept varying parameters.
  vtkCollectionIterator* shaderIter = this->ShaderProgram->NewShaderIterator(); 

  for (shaderIter->InitTraversal(); !shaderIter->IsDoneWithTraversal();
    shaderIter->GoToNextItem())
    {
    vtkCgShader* shader = vtkCgShader::SafeDownCast(
      shaderIter->GetCurrentObject());
    if (shader && shader->GetScope() == vtkXMLShader::SCOPE_VERTEX)
      {
      this->Internal->VertexShader = shader;
      break;
      }
    }
  shaderIter->Delete();
}

template <class T>
void vtkCgShaderDeviceAdapterSendAttributeInternal(vtkCgShaderDeviceAdapter* self,
  const char* attrname, int components, const T* attribute, unsigned long offset)
{
  double converted_value[4];
  for (int cc=0; cc < 4 && cc < components; cc++)
    {
    converted_value[cc] = static_cast<double>((attribute+offset)[cc]);
    }
  self->SendAttributeInternal(attrname, components, converted_value);
}

VTK_TEMPLATE_SPECIALIZE
void vtkCgShaderDeviceAdapterSendAttributeInternal(vtkCgShaderDeviceAdapter* self,
  const char* attrname, int components, const float* attribute, unsigned long offset)
{
  self->SendAttributeInternal(attrname, components, (attribute+offset));
}

//----------------------------------------------------------------------------
void vtkCgShaderDeviceAdapter::SendAttributeInternal(
  const char* attrname, int components, const double* data)
{
  this->Internal->VertexShader->SetUniformParameter(attrname, components, data);
}

//----------------------------------------------------------------------------
void vtkCgShaderDeviceAdapter::SendAttributeInternal(
  const char* attrname, int components, const float* data)
{
  this->Internal->VertexShader->SetUniformParameter(attrname, components, data);
}

//----------------------------------------------------------------------------
void vtkCgShaderDeviceAdapter::SendAttribute(const char* attrname,
  int components, int type, 
  const void* attribute, unsigned long offset/*=0*/)
{
  switch (type)
    {
    vtkTemplateMacro(
      vtkCgShaderDeviceAdapterSendAttributeInternal(this,
        attrname, components, static_cast<const VTK_TT*>(attribute), offset));
    }
}

//----------------------------------------------------------------------------
void vtkCgShaderDeviceAdapter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

