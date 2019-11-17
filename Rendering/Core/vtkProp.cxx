/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProp.h"
#include "vtkAssemblyPaths.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKey.h"
#include "vtkObjectFactory.h"
#include "vtkShaderProperty.h"
#include <cassert>

vtkCxxSetObjectMacro(vtkProp, PropertyKeys, vtkInformation);

vtkInformationKeyMacro(vtkProp, GeneralTextureUnit, Integer);
vtkInformationKeyMacro(vtkProp, GeneralTextureTransform, DoubleVector);

//----------------------------------------------------------------------------
// Creates an Prop with the following defaults: visibility on.
vtkProp::vtkProp()
{
  this->Visibility = 1; // ON

  this->Pickable = 1;
  this->Dragable = 1;

  this->UseBounds = true;

  this->AllocatedRenderTime = 10.0;
  this->EstimatedRenderTime = 0.0;
  this->RenderTimeMultiplier = 1.0;

  this->Paths = nullptr;

  this->NumberOfConsumers = 0;
  this->Consumers = nullptr;

  this->PropertyKeys = nullptr;

  this->ShaderProperty = nullptr;
}

//----------------------------------------------------------------------------
vtkProp::~vtkProp()
{
  if (this->Paths)
  {
    this->Paths->Delete();
  }

  delete[] this->Consumers;

  if (this->PropertyKeys != nullptr)
  {
    this->PropertyKeys->Delete();
  }

  if (this->ShaderProperty)
  {
    this->ShaderProperty->UnRegister(this);
  }
}

//----------------------------------------------------------------------------
// This method is invoked if the prop is picked.
void vtkProp::Pick()
{
  this->InvokeEvent(vtkCommand::PickEvent, nullptr);
}

//----------------------------------------------------------------------------
// Shallow copy of vtkProp.
void vtkProp::ShallowCopy(vtkProp* prop)
{
  this->Visibility = prop->GetVisibility();
  this->Pickable = prop->GetPickable();
  this->Dragable = prop->GetDragable();
  this->SetShaderProperty(prop->GetShaderProperty());
}

//----------------------------------------------------------------------------
void vtkProp::InitPathTraversal()
{
  if (this->Paths == nullptr)
  {
    this->Paths = vtkAssemblyPaths::New();
    vtkAssemblyPath* path = vtkAssemblyPath::New();
    path->AddNode(this, nullptr);
    this->BuildPaths(this->Paths, path);
    path->Delete();
  }
  this->Paths->InitTraversal();
}

//----------------------------------------------------------------------------
vtkAssemblyPath* vtkProp::GetNextPath()
{
  if (!this->Paths)
  {
    return nullptr;
  }
  return this->Paths->GetNextItem();
}

//----------------------------------------------------------------------------
// This method is used in conjunction with the assembly object to build a copy
// of the assembly hierarchy. This hierarchy can then be traversed for
// rendering, picking or other operations.
void vtkProp::BuildPaths(vtkAssemblyPaths* paths, vtkAssemblyPath* path)
{
  // This is a leaf node in the assembly hierarchy so we
  // copy the path in preparation to assingning it to paths.
  vtkAssemblyPath* childPath = vtkAssemblyPath::New();
  childPath->ShallowCopy(path);

  // We can add this path to the list of paths
  paths->AddItem(childPath);
  childPath->Delete(); // okay, reference counting
}

//----------------------------------------------------------------------------
void vtkProp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
  os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");

  os << indent << "AllocatedRenderTime: " << this->AllocatedRenderTime << endl;
  os << indent << "EstimatedRenderTime: " << this->EstimatedRenderTime << endl;
  os << indent << "NumberOfConsumers: " << this->NumberOfConsumers << endl;
  os << indent << "RenderTimeMultiplier: " << this->RenderTimeMultiplier << endl;
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");

  os << indent << "PropertyKeys: ";
  if (this->PropertyKeys != nullptr)
  {
    this->PropertyKeys->PrintSelf(os, indent);
    os << endl;
  }
  else
  {
    os << "none." << endl;
  }

  os << indent << "useBounds: " << this->UseBounds << endl;
}

//----------------------------------------------------------------------------
void vtkProp::AddConsumer(vtkObject* c)
{
  // make sure it isn't already there
  if (this->IsConsumer(c))
  {
    return;
  }
  // add it to the list, reallocate memory
  vtkObject** tmp = this->Consumers;
  this->NumberOfConsumers++;
  this->Consumers = new vtkObject*[this->NumberOfConsumers];
  for (int i = 0; i < (this->NumberOfConsumers - 1); i++)
  {
    this->Consumers[i] = tmp[i];
  }
  this->Consumers[this->NumberOfConsumers - 1] = c;
  // free old memory
  delete[] tmp;
}

//----------------------------------------------------------------------------
void vtkProp::RemoveConsumer(vtkObject* c)
{
  // make sure it is already there
  if (!this->IsConsumer(c))
  {
    return;
  }
  // remove it from the list, reallocate memory
  vtkObject** tmp = this->Consumers;
  this->NumberOfConsumers--;
  this->Consumers = new vtkObject*[this->NumberOfConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfConsumers; i++)
  {
    if (tmp[i] != c)
    {
      this->Consumers[cnt] = tmp[i];
      cnt++;
    }
  }
  // free old memory
  delete[] tmp;
}

//----------------------------------------------------------------------------
int vtkProp::IsConsumer(vtkObject* c)
{
  int i;
  for (i = 0; i < this->NumberOfConsumers; i++)
  {
    if (this->Consumers[i] == c)
    {
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkObject* vtkProp::GetConsumer(int i)
{
  if (i >= this->NumberOfConsumers)
  {
    return nullptr;
  }
  return this->Consumers[i];
}

// ----------------------------------------------------------------------------
// Description:
// Tells if the prop has all the required keys.
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool vtkProp::HasKeys(vtkInformation* requiredKeys)
{
  bool result = requiredKeys == nullptr;
  if (!result)
  {
    vtkInformationIterator* it = vtkInformationIterator::New();
    it->SetInformation(requiredKeys);
    it->GoToFirstItem();
    result = true;
    while (result && !it->IsDoneWithTraversal())
    {
      vtkInformationKey* k = it->GetCurrentKey();
      result = this->PropertyKeys != nullptr && this->PropertyKeys->Has(k);
      it->GoToNextItem();
    }
    it->Delete();
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Render the opaque geometry only if the prop has all the requiredKeys.
// This is recursive for composite props like vtkAssembly.
// An implementation is provided in vtkProp but each composite prop
// must override it.
// It returns if the rendering was performed.
// \pre v_exists: v!=0
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool vtkProp::RenderFilteredOpaqueGeometry(vtkViewport* v, vtkInformation* requiredKeys)
{
  assert("pre: v_exists" && v != nullptr);
  bool result;
  if (this->HasKeys(requiredKeys))
  {
    result = this->RenderOpaqueGeometry(v) == 1;
  }
  else
  {
    result = false;
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Render the translucent polygonal geometry only if the prop has all the
// requiredKeys.
// This is recursive for composite props like vtkAssembly.
// An implementation is provided in vtkProp but each composite prop
// must override it.
// It returns if the rendering was performed.
// \pre v_exists: v!=0
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool vtkProp::RenderFilteredTranslucentPolygonalGeometry(
  vtkViewport* v, vtkInformation* requiredKeys)
{
  assert("pre: v_exists" && v != nullptr);
  bool result;
  if (this->HasKeys(requiredKeys))
  {
    result = this->RenderTranslucentPolygonalGeometry(v) == 1;
  }
  else
  {
    result = false;
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Render the volumetric geometry only if the prop has all the
// requiredKeys.
// This is recursive for composite props like vtkAssembly.
// An implementation is provided in vtkProp but each composite prop
// must override it.
// It returns if the rendering was performed.
// \pre v_exists: v!=0
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool vtkProp::RenderFilteredVolumetricGeometry(vtkViewport* v, vtkInformation* requiredKeys)
{
  assert("pre: v_exists" && v != nullptr);
  bool result;
  if (this->HasKeys(requiredKeys))
  {
    result = this->RenderVolumetricGeometry(v) == 1;
  }
  else
  {
    result = false;
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Render in the overlay of the viewport only if the prop has all the
// requiredKeys.
// This is recursive for composite props like vtkAssembly.
// An implementation is provided in vtkProp but each composite prop
// must override it.
// It returns if the rendering was performed.
// \pre v_exists: v!=0
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool vtkProp::RenderFilteredOverlay(vtkViewport* v, vtkInformation* requiredKeys)
{
  assert("pre: v_exists" && v != nullptr);
  bool result;
  if (this->HasKeys(requiredKeys))
  {
    result = this->RenderOverlay(v) == 1;
  }
  else
  {
    result = false;
  }
  return result;
}

void vtkProp::SetShaderProperty(vtkShaderProperty* property)
{
  if (this->ShaderProperty != property)
  {
    if (this->ShaderProperty != nullptr)
    {
      this->ShaderProperty->UnRegister(this);
    }
    this->ShaderProperty = property;
    if (this->ShaderProperty != nullptr)
    {
      this->ShaderProperty->Register(this);
    }
    this->Modified();
  }
}

vtkShaderProperty* vtkProp::GetShaderProperty()
{
  if (this->ShaderProperty == nullptr)
  {
    this->ShaderProperty = vtkShaderProperty::New();
    this->ShaderProperty->Register(this);
    this->ShaderProperty->Delete();
  }
  return this->ShaderProperty;
}
