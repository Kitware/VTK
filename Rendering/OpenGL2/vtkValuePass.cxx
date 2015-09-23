/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkValuePass.h"

#include "vtkClearRGBPass.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkValuePass);

vtkInformationKeyMacro(vtkValuePass, RENDER_VALUES, Integer);

vtkInformationKeyMacro(vtkValuePass, SCALAR_MODE, Integer);
vtkInformationKeyMacro(vtkValuePass, ARRAY_MODE, Integer);
vtkInformationKeyMacro(vtkValuePass, ARRAY_ID, Integer);
vtkInformationKeyMacro(vtkValuePass, ARRAY_NAME, String);
vtkInformationKeyMacro(vtkValuePass, ARRAY_COMPONENT, Integer);

class vtkValuePass::vtkInternals
{
public:
  int FieldAssociation;
  int FieldAttributeType;
  std::string FieldName;
  bool FieldNameSet;
  int Component;
  double ScalarRange[2];

  vtkInternals()
    {
    this->FieldAssociation = 0;
    this->FieldAttributeType = 0;
    this->FieldName = "";
    this->FieldNameSet = false;
    this->Component = 0;
    }
};

// ----------------------------------------------------------------------------
vtkValuePass::vtkValuePass()
{
  this->Internals = new vtkInternals();
}

// ----------------------------------------------------------------------------
vtkValuePass::~vtkValuePass()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------------
void vtkValuePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputArrayToProcess(int fieldAssociation,
  const char *name)
{
  if (!this->Internals->FieldNameSet ||
      this->Internals->FieldAssociation != fieldAssociation ||
      this->Internals->FieldName != name)
    {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldName = name;
    this->Internals->FieldNameSet = true;
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputArrayToProcess(int fieldAssociation,
  int fieldAttributeType)
{
  if (this->Internals->FieldAssociation != fieldAssociation ||
      this->Internals->FieldAttributeType != fieldAttributeType ||
      this->Internals->FieldNameSet)
    {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldAttributeType = fieldAttributeType;
    this->Internals->FieldNameSet = false;
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputComponentToProcess(int component)
{
  if (this->Internals->Component != component)
    {
    this->Internals->Component = component;
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkValuePass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  vtkSmartPointer<vtkClearRGBPass> clear =
    vtkSmartPointer<vtkClearRGBPass>::New();
  clear->Render(s);

  this->NumberOfRenderedProps=0;
  this->RenderOpaqueGeometry(s);
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass with key checking.
// \pre s_exists: s!=0
void vtkValuePass::RenderOpaqueGeometry(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  std::vector<int> scalarVisibilities;

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
    {
    vtkProp *p = s->GetPropArray()[i];

    // Cache scalar visibility state and turn it on
    vtkActor *a = vtkActor::SafeDownCast(p);
    if (a)
      {
      scalarVisibilities.push_back(a->GetMapper()->GetScalarVisibility());
      a->GetMapper()->ScalarVisibilityOn();
      }

    vtkSmartPointer<vtkInformation> keys = p->GetPropertyKeys();
    if (!keys)
      {
      keys.TakeReference(vtkInformation::New());
      }
    keys->Set(vtkValuePass::RENDER_VALUES(), 1);
    keys->Set(vtkValuePass::SCALAR_MODE(), this->Internals->FieldAssociation);
    keys->Set(vtkValuePass::ARRAY_MODE(), this->Internals->FieldNameSet);
    keys->Set(vtkValuePass::ARRAY_ID(), this->Internals->FieldAttributeType);
    keys->Set(vtkValuePass::ARRAY_NAME(), this->Internals->FieldName.c_str());
    keys->Set(vtkValuePass::ARRAY_COMPONENT(), this->Internals->Component);
    p->SetPropertyKeys(keys);

    int rendered =
      p->RenderOpaqueGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
    }

  // Remove set keys
  i = 0;
  while (i < c)
    {
    vtkProp *p = s->GetPropArray()[i];

    // Restore scalar visibility state
    vtkActor *a = vtkActor::SafeDownCast(p);
    if (a)
      {
      a->GetMapper()->SetScalarVisibility(scalarVisibilities[i]);
      }

    vtkInformation *keys = p->GetPropertyKeys();
    keys->Remove(vtkValuePass::RENDER_VALUES());
    keys->Remove(vtkValuePass::SCALAR_MODE());
    keys->Remove(vtkValuePass::ARRAY_MODE());
    keys->Remove(vtkValuePass::ARRAY_ID());
    keys->Remove(vtkValuePass::ARRAY_NAME());
    keys->Remove(vtkValuePass::ARRAY_COMPONENT());
    p->SetPropertyKeys(keys);
    ++i;
    }
}
