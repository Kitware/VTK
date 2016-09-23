/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGLObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWebGLObject.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>

vtkStandardNewMacro(vtkWebGLObject);
#include <vector>
#include <map>

//-----------------------------------------------------------------------------
vtkWebGLObject::vtkWebGLObject()
{
  this->iswireframeMode = false;
  this->hasChanged = false;
  this->webGlType = wTRIANGLES;
  this->hasTransparency = false;
  this->iswidget = false;
  this->interactAtServer = false;
}

//-----------------------------------------------------------------------------
vtkWebGLObject::~vtkWebGLObject()
{
}

//-----------------------------------------------------------------------------
std::string vtkWebGLObject::GetId()
{
  return this->id;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetId(std::string i)
{
  this->id = i;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetType(WebGLObjectTypes t)
{
  this->webGlType = t;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetTransformationMatrix(vtkMatrix4x4* m)
{
  for (int i=0; i<16; i++) this->Matrix[i] = m->GetElement(i/4, i%4);
}

//-----------------------------------------------------------------------------
std::string vtkWebGLObject::GetMD5()
{
  return this->MD5;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkWebGLObject::HasChanged()
{
  return this->hasChanged;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetWireframeMode(bool wireframe)
{
  this->iswireframeMode = wireframe;
}

//-----------------------------------------------------------------------------
bool vtkWebGLObject::isWireframeMode()
{
  return this->iswireframeMode;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetVisibility(bool vis)
{
  this->isvisible = vis;
}

//-----------------------------------------------------------------------------
bool vtkWebGLObject::isVisible()
{
  return this->isvisible;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetHasTransparency(bool t)
{
  this->hasTransparency = t;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetIsWidget(bool w)
{
  this->iswidget = w;
}

//-----------------------------------------------------------------------------
bool vtkWebGLObject::isWidget()
{
  return this->iswidget;
}

//-----------------------------------------------------------------------------
bool vtkWebGLObject::HasTransparency()
{
  return this->hasTransparency;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetRendererId(size_t i)
{
  this->rendererId = i;
}

//-----------------------------------------------------------------------------
size_t vtkWebGLObject::GetRendererId()
{
  return this->rendererId;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetLayer(int l)
{
  this->layer = l;
}

//-----------------------------------------------------------------------------
int vtkWebGLObject::GetLayer()
{
  return this->layer;
}

//-----------------------------------------------------------------------------
bool vtkWebGLObject::InteractAtServer()
{
  return this->interactAtServer;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::SetInteractAtServer(bool i)
{
  this->interactAtServer = i;
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::GetBinaryData(int part, vtkUnsignedCharArray* buffer)
{
  if (!buffer)
  {
    vtkErrorMacro("Buffer must not be NULL.");
    return;
  }

  const int binarySize = this->GetBinarySize(part);
  const unsigned char* binaryData = this->GetBinaryData(part);

  buffer->SetNumberOfComponents(1);
  buffer->SetNumberOfTuples(binarySize);

  if (binarySize)
  {
    std::copy(binaryData, binaryData+binarySize, buffer->GetPointer(0));
  }
}

//-----------------------------------------------------------------------------
void vtkWebGLObject::GenerateBinaryData(){this->hasChanged = false;}
//-----------------------------------------------------------------------------
unsigned char* vtkWebGLObject::GetBinaryData(int vtkNotUsed(part)){return NULL;}
//-----------------------------------------------------------------------------
int vtkWebGLObject::GetBinarySize(int vtkNotUsed(part)){return 0;}
//-----------------------------------------------------------------------------
int vtkWebGLObject::GetNumberOfParts(){return 0;}
