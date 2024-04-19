// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGLObject.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebGLObject);
VTK_ABI_NAMESPACE_END
#include <map>
#include <vector>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkWebGLObject::vtkWebGLObject()
{
  this->iswireframeMode = false;
  this->hasChanged = false;
  this->webGlType = wTRIANGLES;
  this->hasTransparency = false;
  this->iswidget = false;
  this->interactAtServer = false;
}

//------------------------------------------------------------------------------
vtkWebGLObject::~vtkWebGLObject() = default;

//------------------------------------------------------------------------------
std::string vtkWebGLObject::GetId()
{
  return this->id;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetId(const std::string& i)
{
  this->id = i;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetType(WebGLObjectTypes t)
{
  this->webGlType = t;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetTransformationMatrix(vtkMatrix4x4* m)
{
  for (int i = 0; i < 16; i++)
    this->Matrix[i] = m->GetElement(i / 4, i % 4);
}

//------------------------------------------------------------------------------
std::string vtkWebGLObject::GetMD5()
{
  return this->MD5;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkWebGLObject::HasChanged()
{
  return this->hasChanged;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetWireframeMode(bool wireframe)
{
  this->iswireframeMode = wireframe;
}

//------------------------------------------------------------------------------
bool vtkWebGLObject::isWireframeMode()
{
  return this->iswireframeMode;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetVisibility(bool vis)
{
  this->isvisible = vis;
}

//------------------------------------------------------------------------------
bool vtkWebGLObject::isVisible()
{
  return this->isvisible;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetHasTransparency(bool t)
{
  this->hasTransparency = t;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetIsWidget(bool w)
{
  this->iswidget = w;
}

//------------------------------------------------------------------------------
bool vtkWebGLObject::isWidget()
{
  return this->iswidget;
}

//------------------------------------------------------------------------------
bool vtkWebGLObject::HasTransparency()
{
  return this->hasTransparency;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetRendererId(size_t i)
{
  this->rendererId = i;
}

//------------------------------------------------------------------------------
size_t vtkWebGLObject::GetRendererId()
{
  return this->rendererId;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetLayer(int l)
{
  this->layer = l;
}

//------------------------------------------------------------------------------
int vtkWebGLObject::GetLayer()
{
  return this->layer;
}

//------------------------------------------------------------------------------
bool vtkWebGLObject::InteractAtServer()
{
  return this->interactAtServer;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::SetInteractAtServer(bool i)
{
  this->interactAtServer = i;
}

//------------------------------------------------------------------------------
void vtkWebGLObject::GetBinaryData(int part, vtkUnsignedCharArray* buffer)
{
  if (!buffer)
  {
    vtkErrorMacro("Buffer must not be nullptr.");
    return;
  }

  const int binarySize = this->GetBinarySize(part);
  const unsigned char* binaryData = this->GetBinaryData(part);

  buffer->SetNumberOfComponents(1);
  buffer->SetNumberOfTuples(binarySize);

  if (binarySize)
  {
    std::copy(binaryData, binaryData + binarySize, buffer->GetPointer(0));
  }
}

//------------------------------------------------------------------------------
void vtkWebGLObject::GenerateBinaryData()
{
  this->hasChanged = false;
}
//------------------------------------------------------------------------------
unsigned char* vtkWebGLObject::GetBinaryData(int vtkNotUsed(part))
{
  return nullptr;
}
//------------------------------------------------------------------------------
int vtkWebGLObject::GetBinarySize(int vtkNotUsed(part))
{
  return 0;
}
//------------------------------------------------------------------------------
int vtkWebGLObject::GetNumberOfParts()
{
  return 0;
}
VTK_ABI_NAMESPACE_END
